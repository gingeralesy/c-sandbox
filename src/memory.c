#include "memory.h"

#include "ticket.h"

#define BLOCK_HEADER "BLOCK"
#define BLOCK_HEADER_LENGTH (5)

#define DEFAULT_INITIAL_SIZE ((uint32_t)524288)    // 512 kB
#define DEFAULT_MAX_SIZE     ((uint32_t)536870912) // 512 MB

typedef struct gc_memory_block_t gc_block;

typedef struct gc_memory_block_t
{
  char head[BLOCK_HEADER_LENGTH];
  uint64_t id;
  gc_bool marked;
  size_t size;
  uint32_t ref_count;
  gc_block *references;
  uint8_t data;
} gc_block;

static uint8_t *gc_memory = NULL;

static uint32_t gc_current_size = 0;
static uint32_t gc_max_size = 0;

static ticket_mutex s_lock = TICKET_MUTEX_INITIALIZER;


static uint64_t   genid(void);
static gc_block * alloc_space(size_t);
static gc_block * get_block(uint64_t);


uint64_t genid()
{
  static ticket_mutex mutex = TICKET_MUTEX_INITIALIZER;
  static uint64_t counter = 1UL;
  uint64_t retid = 0UL;

  ticket_lock(&mutex);
  retid = counter;
  counter += 1;
  ticket_unlock(&mutex);
  return retid;
}

gc_block * alloc_space(size_t size)
{
  gc_block *block = NULL;
  gc_block *tmp = NULL;
  uint32_t i = 0;
  uint32_t start = 0;
  size_t block_size = sizeof(gc_block);
  size_t needed = size + block_size;

  for (i = 0; i < gc_current_size; i++)
  {
    if (needed <= i - start)
    {
      block = (gc_block *)(&(gc_memory[start]));
      break;
    }

    if (memcmp((&(gc_memory[i])), BLOCK_HEADER, BLOCK_HEADER_LENGTH) == 0)
    {
      tmp = (gc_block *)(&(gc_memory[i]));
      if (!tmp->marked)
      {
        i += tmp->size + block_size;
        start = i;
      }
    }
  }

  if (!block && gc_current_size < gc_max_size &&
      needed <= gc_max_size - gc_current_size)
  {
    uint8_t *tmp_memory = NULL;
    uint32_t new_size = 0;
    new_size =
      min((gc_current_size * max(needed / gc_current_size, 2)),
          gc_max_size);
    tmp_memory = gc_memory;
    gc_memory = (uint8_t *)calloc(new_size,sizeof(uint8_t));
    memcpy(gc_memory, tmp_memory, gc_current_size);
    free(tmp_memory);
    block = (gc_block *)(&(gc_memory[gc_current_size]));
    gc_current_size = new_size;
  }

  if (block)
  {
    memcpy(block->head, BLOCK_HEADER, BLOCK_HEADER_LENGTH);
    block->id = genid();
    block->size = size;
    memset(&(block->data), 0, size);
  }

  return block;
}

gc_block * get_block(uint64_t id)
{
  // TODO: Get the correct block based on the ID from a RBTree
  return NULL;
}


gc_bool gc_init(uint32_t initial_size, uint32_t max_size)
{
  return gc_init_err(initial_size, max_size, NULL);
}

uint64_t gc_alloc(size_t size)
{
  return gc_alloc_err(size, NULL);
}

Pointer gc_data(uin64_t id)
{
  gc_block block = NULL;
  if (id == 0UL)
    return NULL;
  block = get_block(id);
  if (block == NULL)
    return NULL;
  return (Pointer)(&(block->data));
}

gc_bool gc_free(uint64_t block_id)
{
  return gc_free_err(block_id, NULL);
}

gc_bool gc_destroy()
{
  return gc_destroy_err(NULL);
}

gc_bool gc_init_err(uint32_t initial_size, uint32_t max_size, gc_error *error)
{
  gc_bool retval = (gc_memory ? GC_TRUE : GC_FALSE);
  if (!gc_memory)
  {
    uint32_t _initial_size = initial_size;
    uint32_t _max_size = max_size;
    if (_initial_size == 0)
      _initial_size = DEFAULT_INITIAL_SIZE;
    if (_max_size == 0)
      _max_size = DEFAULT_MAX_SIZE;

    ticket_lock(&s_lock);
    if (!gc_memory)
    {
      gc_memory = (uint8_t *)calloc(_initial_size,sizeof(uint8_t));
      if (gc_memory)
      {
        gc_current_size = _initial_size;
        gc_max_size = _max_size;
        
        if (error)
          (*error) = GC_NO_ERROR;
        retval = GC_TRUE;
      }
      else if (error)
      {
        (*error) = GC_OUT_OF_MEMORY_ERROR;
      }
    }
    ticket_unlock(&s_lock);
  }
  return retval;
}

uint64_t gc_alloc_err(size_t size, gc_error *error)
{
  uint64_t id = 0UL;
  if (gc_init_err(DEFAULT_INITIAL_SIZE, DEFAULT_MAX_SIZE, error))
  {
    gc_block *block = NULL;
    
    ticket_lock(&s_lock);
    block = alloc_space(size);
    if (block)
      id = block->id;
    ticket_unlock(&s_lock);

    if (error)
      (*error) = (0UL < id ? GC_NO_ERROR : GC_OUT_OF_MEMORY_ERROR);
  }
  else if (error)
  {
    (*error) = GC_UNINITIALIZED_ERROR;
  }
  return id;
}

gc_bool gc_free_err(Pointer pointer, gc_error *error)
{
  gc_bool retval = GC_FALSE;
  if (gc_memory)
  {
    gc_block *block = NULL;
    ticket_lock(&s_lock);
    block = get_block(pointer);
    if (block)
    {
      block->marked = GC_TRUE;
      retval = GC_TRUE;
      memset(&(block->data), 0, block->size);
      if (error)
        (*error) = GC_NO_ERROR;
    }
    else if (error)
    {
      (*error) = GC_INVALID_INPUT_ERROR;
    }
    ticket_unlock(&s_lock);
  }
  else if (error)
  {
    (*error) = GC_UNINITIALIZED_ERROR;
  }
  return retval;
}

gc_bool gc_destroy_err(gc_error *error)
{
  ticket_lock(&s_lock);
  if (gc_memory)
  {
    uint8_t *tmp_mem = gc_memory;
    gc_memory = NULL;
    memset(tmp_mem, 0, gc_current_size);
    free(tmp_mem);
  }

  gc_current_size = 0;
  gc_max_size = 0;
  ticket_unlock(&s_lock);

  return GC_TRUE;
}

const char * gc_error_string(gc_error error)
{
  switch(error)
  {
  case GC_NO_ERROR:
    return "No error.";
  case GC_OUT_OF_MEMORY_ERROR:
    return "Out of memory error.";
  case GC_INVALID_INPUT_ERROR:
    return "Invalid input error.";
  case GC_UNINITIALIZED_ERROR:
    return "Uninitialised error.";
  default:
    return "Unknown error.";
  }
}
