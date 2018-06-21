#ifndef __TICKET_H__
#define __TICKET_H__

#include "common.h"

#define TICKET_MUTEX_INITIALIZER                                \
  { PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, 0UL, 0UL, 0UL }

typedef struct ticket_mutex_t
{
  pthread_cond_t cond;
  pthread_mutex_t mutex;
  uint64_t queue_head;
  uint64_t queue_tail;
  uint64_t queue_length;
} ticket_mutex;

uint64_t ticket_lock(ticket_mutex *);
uint64_t ticket_unlock(ticket_mutex *);
uint64_t ticket_locked(ticket_mutex *);

#endif // __TICKET_H__
