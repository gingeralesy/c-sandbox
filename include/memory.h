#ifndef __MEM_H__
#define __MEM_H__

#include "common.h"

typedef enum gc_error_e
{
  GC_NO_ERROR = 0x0,

  GC_OUT_OF_MEMORY_ERROR = 0x10,
  GC_INVALID_INPUT_ERROR = 0x11,
  GC_UNINITIALIZED_ERROR = 0x12
} gc_error;

bool     gc_init(uint32_t, uint32_t);
uint64_t gc_alloc(size_t);
Pointer  gc_data(uint64_t);
bool     gc_free(uint64_t);
bool     gc_destroy(void);

bool     gc_init_err(uint32_t, uint32_t, gc_error *);
uint64_t gc_alloc_err(size_t, gc_error *);
bool     gc_free_err(uint64_t, gc_error *);
bool     gc_destroy_err(gc_error *);

const char * gc_error_string(gc_error);

#endif // __MEM_H__
