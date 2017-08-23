#include "memory.h"

#include "rbtree.h"
#include "ticket.h"
#include "common.h"

#define DEFAULT_INITIAL_SIZE (524288)    // 512 kB
#define DEFAULT_MAX_SIZE     (536870912) // 512 MB

typedef struct gc_memory_block_t
{
  gc_bool marked;
  uint64_t start;
  uint64_t size;
} gc_memory_block;

static char * gc_memory = NULL;
static rbt_node * gc_blocks = NULL;

static uint64_t gc_current_size = 0;
static uint64_t gc_max_size = 0;

static ticket_mutex lock = TICKET_MUTEX_INITIALIZER;


static void block_free(void *);


void block_free(void *block)
{
  gc_memory_block *_block = (gc_memory_block *)block;
  free(_block);
}


gc_bool gc_init(uint64_t initial_size, uint64_t max_size)
{
  return gc_init_err(initial_size, max_size, NULL);
}

void * gc_alloc(size_t size)
{
  return gc_alloc_err(size, NULL);
}

gc_bool gc_free(void *pointer)
{
  return gc_free_err(pointer, NULL);
}

gc_bool gc_destroy()
{
  return gc_destroy_err(NULL);
}

gc_bool gc_init_err(uint64_t initial_size, uint64_t max_size, gc_error *error)
{
  gc_bool retval = (gc_memory ? GC_TRUE : GC_FALSE);
  if (!gc_memory)
  {
    uint64_t _initial_size = initial_size;
    uint64_t _max_size = max_size;
    if (_initial_size == 0)
      _initial_size = DEFAULT_INITIAL_SIZE;
    if (_max_size == 0)
      _max_size = DEFAULT_MAX_SIZE;

    ticket_lock(&lock);
    if (!gc_memory)
    {
      gc_memory = (char *)calloc(_initial_size,sizeof(char));
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
    ticket_unlock(&lock);
  }
  return retval;
}

void * gc_alloc_err(size_t size, gc_error *error)
{
  void * retptr = NULL;
  size_t _size = size / sizeof(char);
  if (gc_init_err(DEFAULT_INITIAL_SIZE, DEFAULT_MAX_SIZE, error))
  {
    rbt_node *node = NULL;
    gc_memory_block *block = NULL;
    gc_memory_block *next_block = NULL;
    
    ticket_lock(&lock);
    node = rbt_get_first(gc_blocks, NULL, (void **)&block);
    if (node)
    {
      do
      {
        node = rbt_get_next(node, NULL, (void **)&next_block);
        if (block)
        {
          uint64_t new_start = block->start + block->size;
          if ((!next_block && _size + new_start <= gc_max_size) ||
              (next_block && _size <= next_block->start - new_start))
          {
            void *old_block = NULL;
            gc_memory_block *new_block = NULL;
            if (gc_current_size < new_start + _size)
            {
              char *new_memory = NULL;
              char *tmp_memory = NULL;
              uint64_t new_size = gc_current_size;
              do
              {
                new_size = 2 * new_size;
              } while (new_size < new_start + _size);

              new_size = min(gc_max_size, new_size);
              if (gc_current_size == new_size || new_size < new_start + _size)
                break;

              new_memory = (char *)calloc(new_size,sizeof(char));
              if (!new_memory)
                break;

              memcpy(new_memory, gc_memory, gc_current_size);

              tmp_memory = gc_memory;
              gc_memory = new_memory;
              free(tmp_memory);
            }

            new_block = (gc_memory_block *)calloc(1,sizeof(gc_memory_block));
            if (!new_block)
              break;

            new_block->start = new_start;
            new_block->size = _size;
            gc_blocks =
              rbt_put(gc_blocks, new_start, new_block, sizeof(gc_memory_block),
                      &old_block);
            retptr = (void *)(gc_memory + (new_start * sizeof(char)));

            if (old_block)
              block_free(old_block);
            old_block = NULL;
            break;
          }
        }
        block = next_block;
      } while (node);
    }
    else if (_size <= gc_max_size)
    {
      void *old_block = NULL;
      gc_memory_block *new_block = NULL;
      if (gc_current_size < _size)
      {
        char *new_memory = NULL;
        char *tmp_memory = NULL;
        do
        {
          gc_current_size = 2 * gc_current_size;
        } while (gc_current_size < _size);
        gc_current_size = min(gc_max_size, gc_current_size);

        new_memory = (char *)calloc(gc_current_size,sizeof(char));
        if (!new_memory)
        {
          if (error)
            (*error) = GC_OUT_OF_MEMORY_ERROR;
          return NULL;
        }

        memcpy(new_memory, gc_memory, gc_current_size);

        tmp_memory = gc_memory;
        gc_memory = new_memory;
        free(tmp_memory);
      }

      new_block = (gc_memory_block *)calloc(1,sizeof(gc_memory_block));
      if (!new_block)
      {
        if (error)
          (*error) = GC_OUT_OF_MEMORY_ERROR;
        return NULL;
      }

      new_block->start = 0;
      new_block->size = _size;
      gc_blocks =
        rbt_put(gc_blocks, 0, new_block, sizeof(gc_memory_block), &old_block);
      retptr = (void *)(gc_memory);

      if (old_block)
        block_free(old_block);
      old_block = NULL;
    }
    ticket_unlock(&lock);
    
    if (error)
      (*error) = (retptr ? GC_NO_ERROR : GC_OUT_OF_MEMORY_ERROR);
  }
  else if (error)
  {
    (*error) = GC_UNINITIALIZED_ERROR;
  }
  return retptr;
}

gc_bool gc_free_err(void *pointer, gc_error *error)
{
  gc_bool retval = GC_FALSE;
  if (gc_memory)
  {
    ticket_lock(&lock);
    int64_t pint = (int64_t)pointer - (int64_t)gc_memory;
    if (0 <= pint && gc_blocks)
    {
      gc_memory_block *block = (gc_memory_block *)rbt_get(gc_blocks, pint);
      if (block)
      {
        block->marked = GC_TRUE;
        if (error)
          (*error) = GC_NO_ERROR;
        retval = GC_TRUE;
      }
      else
      {
        (*error) = GC_INVALID_INPUT_ERROR;
      }
    }
    else if (error)
    {
      (*error) = GC_INVALID_INPUT_ERROR;
    }
    ticket_unlock(&lock);
  }
  else if (error)
  {
    (*error) = GC_UNINITIALIZED_ERROR;
  }
  return retval;
}

gc_bool gc_destroy_err(gc_error *error)
{
  ticket_lock(&lock);
  if (gc_memory)
  {
    char *tmp_mem = gc_memory;
    gc_memory = NULL;
    free(tmp_mem);
  }

  if (gc_blocks)
  {
    rbt_node *tmp_rbt = gc_blocks;
    gc_blocks = NULL;
    rbt_free(tmp_rbt, block_free);
  }

  gc_current_size = 0;
  gc_max_size = 0;
  ticket_unlock(&lock);

  return GC_TRUE;
}
