#ifndef __MEM_H__
#define __MEM_H__

#include "common.h"

typedef enum gc_bool_e
{
  GC_FALSE = 0x00,
  GC_TRUE = 0x01
} gc_bool;

typedef enum gc_error_e
{
  GC_NO_ERROR = 0x0,

  GC_OUT_OF_MEMORY_ERROR = 0x10,
  GC_INVALID_INPUT_ERROR = 0x11,
  GC_UNINITIALIZED_ERROR = 0x12
} gc_error;

gc_bool gc_init(uint32_t, uint32_t);
void *  gc_alloc(size_t);
gc_bool gc_free(void *);
gc_bool gc_destroy(void);

gc_bool gc_init_err(uint32_t, uint32_t, gc_error *);
void *  gc_alloc_err(size_t, gc_error *);
gc_bool gc_free_err(void *, gc_error *);
gc_bool gc_destroy_err(gc_error *);

const char * gc_error_string(gc_error);

#endif // __MEM_H__
