#ifndef __TICKET_H__
#define __TICKET_H__

#include <stdint.h>
#include <pthread.h>

#define TICKET_MUTEX_INITIALIZER                                \
  { PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, 0, 0 }

typedef struct ticket_mutex_t
{
  pthread_cond_t cond;
  pthread_mutex_t mutex;
  pthread_mutex_t size_mutex;
  uint64_t queue_head;
  uint64_t queue_tail;
} ticket_mutex;

uint64_t ticket_lock(ticket_mutex *);
uint64_t ticket_unlock(ticket_mutex *);
uint64_t ticket_locked(ticket_mutex *);

#endif // __TICKET_H__
