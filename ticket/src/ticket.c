#include "ticket.h"

uint64_t ticket_lock(ticket_mutex *ticket)
{
  uint64_t queue_cur;
  pthread_mutex_lock(&ticket->mutex);
  queue_cur = ticket->queue_tail;
  ticket->queue_tail += 1;
  ticket->queue_length += 1;
  while (queue_cur != ticket->queue_head)
    pthread_cond_wait(&ticket->cond, &ticket->mutex);
  pthread_mutex_unlock(&ticket->mutex);
  return ticket_locked(ticket);
}

uint64_t ticket_unlock(ticket_mutex *ticket)
{
  if (0 < ticket_locked(ticket))
  {
    pthread_mutex_lock(&ticket->mutex);
    if (0 < ticket_locked(ticket))
    {
      ticket->queue_head += 1;
      ticket->queue_length -= 1;
      pthread_cond_broadcast(&ticket->cond);
    }
    pthread_mutex_unlock(&ticket->mutex);
  }
  return ticket_locked(ticket);
}

uint64_t ticket_locked(ticket_mutex *ticket)
{
  return (ticket != NULL ? ticket->queue_length : 0UL);
}
