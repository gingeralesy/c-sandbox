#include "ncurs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <ncurses.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#include "common.h"
#include "ticket.h"
#include "memory.h"

#define MAX_MAIN_PROCESSES (8)
#define MAX_PROCESSES (64)
#define MAX_CHAR (1024)

#define NCURS_PROC_INITIALIZER                          \
  { 0, 0, 0, 0, {0}, 0, {0}, TICKET_MUTEX_INITIALIZER, 0 }

typedef struct sandbox_ncurs_process {
  WINDOW *main;
  SB_bool running;
  unsigned int pid;
  unsigned int current_processes;
  pthread_t processes[MAX_PROCESSES];
  unsigned int queue_count;
  int ch_queue[MAX_CHAR];
  ticket_mutex sb_lock;
  FILE *debuglog;
} NCursProc;

static NCursProc *ncurs_main_processes[MAX_MAIN_PROCESSES] = {0};
static unsigned int ncurs_main_process_count = 0;

static void (*ncurs_default_sigint_handler)(int);
static void (*ncurs_default_sigwinch_handler)(int);

static void * queue_input(void *);
static void * update(void *);
static void * raise_signal(void *);

static unsigned int to_curse_string(chtype *, char *, unsigned int);
static void default_handler(int);
static void resize(int);
static void quit(int);

static void handle_input(NCursProc *, chtype);
static SB_bool start_process(NCursProc *, void * (*)(void *), void *);
static void init(NCursProc *);
static void clean(NCursProc *);
static SB_bool add_main_process(NCursProc *);
static void writelog(NCursProc *,const char *);

// --- Private ---
// -- Processes --

void * queue_input(void *params)
{
  NCursProc *proc = (NCursProc *)params;
  chtype input = 0;
  while (proc->running)
  {
    if (proc->queue_count < MAX_CHAR && (input = getch()) != ERR &&
        proc->running)
    {
      ticket_lock(&proc->sb_lock);
      if (proc->running)
        proc->ch_queue[proc->queue_count++] = input;
      ticket_unlock(&proc->sb_lock);
    }
  }
  return NULL;
}

void * update(void *params)
{
  NCursProc *proc = (NCursProc *)params;
  unsigned int i;
  unsigned long int fps = 16666667; // 1/60 sec
  struct timespec prev = {0};
  struct timespec cur = {0};
  struct timespec delta = {0};
  struct timespec rem = {0};
  while (proc->running && rem.tv_sec == 0 && rem.tv_nsec == 0)
  {
    ticket_lock(&proc->sb_lock);
    if (proc->running)
    {
      for (i = 0; i < proc->queue_count; i++)
        handle_input(proc, proc->ch_queue[i]);
      proc->queue_count = 0;
      refresh();
      ticket_unlock(&proc->sb_lock);

      timespec_get(&cur, TIME_UTC);
      delta.tv_sec = (cur.tv_sec - prev.tv_sec);
      delta.tv_nsec = (cur.tv_nsec - prev.tv_nsec);
      memcpy(&prev, &cur, sizeof(struct timespec));

      ticket_unlock(&proc->sb_lock);
      if (delta.tv_sec == 0 && delta.tv_nsec < fps)
        nanosleep(&delta, &rem);
    }
    else
    {
      ticket_unlock(&proc->sb_lock);
    }
  }
  return NULL;
}

void * raise_signal(void *params)
{
  if (params)
  {
    raise((int)(*((int *)params)));
    gc_free(params);
  }
  return NULL;
}

// -- Event handlers --

void resize(int sig)
{
  unsigned int i = 0;
  for (i = 0; i < ncurs_main_process_count; i++)
  {
    NCursProc *proc = ncurs_main_processes[i];
    if (proc && proc->running)
    {
      ticket_lock(&proc->sb_lock);
      if (proc->running)
      {
        endwin();
        refresh();
      }
      ticket_unlock(&proc->sb_lock);
    }
  }
  default_handler(sig);
}

void quit(int sig)
{
  unsigned int i = 0;
  default_handler(sig);

  for (i = 0; i < ncurs_main_process_count; i++)
  {
    NCursProc *proc = ncurs_main_processes[i];
    if (proc && proc->running)
    {
      ticket_lock(&proc->sb_lock);
      if (proc->running)
        proc->running = SB_false;
      ticket_unlock(&proc->sb_lock);
    }
  }
}

void default_handler(int sig)
{
  switch (sig)
  {
  case SIGINT:
    if (ncurs_default_sigint_handler)
      ncurs_default_sigint_handler(sig);
    break;
  case SIGWINCH:
    if (ncurs_default_sigwinch_handler)
      ncurs_default_sigwinch_handler(sig);
    break;
  default:
    break;
  }
}

// -- Private interface --

unsigned int to_curse_string(chtype *output, char *input,
                             unsigned int length)
{
  unsigned int i = 0;
  if (sizeof(char) == sizeof(chtype))
  {
    while (i < length && output[i] != '\0')
      i += 1;
    memcpy(output, input, i);
  }
  else
  {
    while (i < length && input[i] != '\0')
    {
      output[i] = (chtype)input[i];
      i += 1;
    }
  }
  if (i < length)
    output[i] = '\0';
  return i;
}

void handle_input(NCursProc *proc, chtype input)
{
  char chbuffer[32] = {0};
  chtype buffer[32] = {0};
  int *sigparam = NULL;
  switch (input)
  {
  case 'q':
  case EOF:
    writelog(proc, "Handling quit...");
    sigparam = (int *)gc_alloc(sizeof(int));
    sigparam = (int *)gc_alloc(sizeof(int));
    sigparam = (int *)gc_alloc(sizeof(int));
    sigparam = (int *)gc_alloc(sizeof(int));
    sigparam = (int *)gc_alloc(sizeof(int));
    sigparam = (int *)gc_alloc(sizeof(int));
    sigparam = (int *)gc_alloc(sizeof(int));
    (*sigparam) = SIGINT;
    start_process(proc, &raise_signal, sigparam);
    break;
  default:
    sprintf(chbuffer, "key %s : %d", keyname(input), input);
    erase();
    if (0 < to_curse_string(buffer, chbuffer, 32))
      mvaddchstr(0, 0, buffer);
    break;
  }
}

SB_bool start_process(NCursProc *proc, void * (*routine)(void *), void *arg)
{
  pthread_t thread = {0};
  if (proc->running && proc->current_processes < MAX_PROCESSES &&
      pthread_create(&thread, NULL, routine, arg) == SB_success)
  {
    proc->processes[proc->current_processes++] = thread;
    return SB_true;
  }
  return SB_false;
}

void init(NCursProc *proc)
{
  writelog(proc, "Starting");

  if (!proc->running)
  {
    struct sigaction sa_quit = {0};
    struct sigaction sa_resize = {0};
    struct sigaction old = {0};
    
    sigaction(SIGINT, NULL, &old);
    ncurs_default_sigint_handler = old.sa_handler;

    sa_quit.sa_handler = quit;
    if (sa_quit.sa_flags & SA_RESTART)
      sa_quit.sa_flags ^= SA_RESTART;
    sigemptyset(&sa_quit.sa_mask);
    sigaction(SIGINT, &sa_quit, NULL);
    
    sigaction(SIGWINCH, NULL, &old);
    ncurs_default_sigwinch_handler = old.sa_handler;

    sa_resize.sa_handler = resize;
    sigemptyset(&sa_resize.sa_mask);
    sigaction(SIGWINCH, &sa_resize, NULL);
  }

  proc->main = initscr();
  keypad(proc->main, SB_true);
  nonl();
  noecho();
  cbreak();
  halfdelay(5);
  curs_set(0);
  proc->running = SB_true;
}

void clean(NCursProc *proc)
{
  writelog(proc, "Cleaning");
  endwin();
  writelog(proc, NULL);
}

unsigned int add_main_process(NCursProc *proc)
{
  if (ncurs_main_process_count < MAX_MAIN_PROCESSES)
  {
    ncurs_main_processes[ncurs_main_process_count++] = proc;
    return ncurs_main_process_count;
  }
  return 0;
}

void writelog(NCursProc *proc, const char *message)
{
  if (message)
  {
    if (!proc->debuglog)
    {
      char filename[128];
      sprintf(filename, "debug.log.%u", proc->pid);
      remove(filename);
      proc->debuglog = fopen(filename,"w");
    }

    if (proc->debuglog)
    {
      char buffer[2048] = {0};
      sprintf(buffer, "LOG: %s\n", message);
      fputs(buffer, proc->debuglog);
      fflush(proc->debuglog);
    }
  }
  else if (proc->debuglog)
  {
    fclose(proc->debuglog);
    proc->debuglog = NULL;
  }
}

// --- Public ---

int ncurs_main(int argc, char *argv[])
{
  unsigned int i = 0;
  unsigned int pid = 0;
  exit_value retval = SB_failure;
  NCursProc proc = NCURS_PROC_INITIALIZER;

  if (gc_init(0,0))
  {
    pid = add_main_process(&proc);
    if (0 < pid)
    {
      proc.pid = pid;
      init(&proc);
      if (start_process(&proc, &queue_input, &proc) &&
          start_process(&proc, &update, &proc))
      {
        retval = SB_success;
        for (i = 0; i < proc.current_processes; i++)
          pthread_join(proc.processes[i], NULL);
      }
      raise(SIGINT);
      clean(&proc);
    }
  }
  return retval;
}
