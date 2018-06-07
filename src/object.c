#include "object.h"

void object_fill(Object *obj, uint64_t hash)
{
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
  obj->hash = hash;
  memcpy(&(obj->mutex), &mutex, sizeof(pthread_mutex_t));
  memcpy(&(obj->cond), &cond, sizeof(pthread_cond_t));
}

bool object_equals(Object *obj, Object *oth)
{
  return (obj != NULL && (obj == oth || object_hash(obj) == object_hash(oth)));
}

uint64_t object_hash(Object *obj)
{
  return (obj != NULL ? obj->hash : 0UL);
}
