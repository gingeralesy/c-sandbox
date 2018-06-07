#ifndef OBJECT_H
#define OBJECT_H

#include "common.h"

typedef struct object_t
{
  uint64_t hash;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
} Object;

void     object_fill  (Object *obj, uint64_t hash);
bool     object_equals(Object *obj, Object *oth);
uint64_t object_hash  (Object *obj);


#endif // OBJECT_H
