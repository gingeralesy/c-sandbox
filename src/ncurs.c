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
#ifdef _WIN32
#include <windows.h>
#endif // _WIN32

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
  chtype ch_queue[MAX_CHAR];
  ticket_mutex sb_lock;
  FILE *debuglog;
} NCursProc;

static NCursProc *ncurs_main_processes[MAX_MAIN_PROCESSES] = {0};
static unsigned int ncurs_main_process_count = 0;

static void (*ncurs_default_sigint_handler)(int);
#ifdef __linux
static void (*ncurs_default_sigwinch_handler)(int);
#endif // __linux

static void * queue_input(void *);
static void * update(void *);
static void * raise_signal(void *);

static unsigned int to_curse_string(chtype *, char *, unsigned int);
static void default_handler(int);
#ifdef __linux
static void resize(int);
#endif // __linux
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
#ifdef _WIN32
  INPUT_RECORD input = {0};
  DWORD numread = 0;
  HANDLE inhand = GetStdHandle(STD_INPUT_HANDLE);
  while (proc->running)
  {
    if (proc->queue_count < MAX_CHAR &&
        WaitForSingleObjectEx(inhand, 500, SB_true) == SB_success &&
        proc->running && ReadConsoleInput(inhand, &input, 1, &numread) &&
        proc->running && numread == 1 && input.EventType == KEY_EVENT &&
        ((KEY_EVENT_RECORD*)&input.Event)->bKeyDown)
    {
      ticket_lock(&proc->sb_lock);
      if (proc->running)
        proc->ch_queue[proc->queue_count++] =
          (chtype)((KEY_EVENT_RECORD*)&input.Event)->uChar.UnicodeChar;
      ticket_unlock(&proc->sb_lock);
    }
  }
#else // __linux
  chtype input = 0;
  while (proc->running)
  {
    if (proc->queue_count < MAX_CHAR &&
        (input = getch()) != ERR && proc->running)
    {
      ticket_lock(&proc->sb_lock);
      if (proc->running)
        proc->ch_queue[proc->queue_count++] = input;
      ticket_unlock(&proc->sb_lock);
    }
  }
#endif // _WIN32
  return NULL;
}

void * update(void *params)
{
  NCursProc *proc = (NCursProc *)params;
  unsigned int i;
#ifdef _WIN32
  DWORD prev = 0;
  DWORD cur = 0;
  DWORD delta = 0;
  DWORD fps = 17; // 1/60 sec
  while (proc->running)
#else // __linux
  unsigned long int fps = 16666667; // 1/60 sec
  struct timespec prev = {0};
  struct timespec cur = {0};
  struct timespec delta = {0};
  struct timespec rem = {0};
  while (proc->running && rem.tv_sec == 0 && rem.tv_nsec == 0)
#endif // _WIN32
  {
    ticket_lock(&proc->sb_lock);
    if (proc->running)
    {
      for (i = 0; i < proc->queue_count; i++)
        handle_input(proc, proc->ch_queue[i]);
    }
    proc->queue_count = 0;
    ticket_unlock(&proc->sb_lock);
    refresh();
#ifdef _WIN32
    cur = GetTickCount();
    delta = cur - prev;
    prev = cur;

    if (proc->running && 0 <= delta && delta < fps)
      Sleep(delta);
#else // __linux
    timespec_get(&cur, TIME_UTC);
    delta.tv_sec = (cur.tv_sec - prev.tv_sec);
    delta.tv_nsec = (cur.tv_nsec - prev.tv_nsec);
    memcpy(&prev, &cur, sizeof(struct timespec));

    if (proc->running && delta.tv_sec == 0 && delta.tv_nsec < fps)
      nanosleep(&delta, &rem);
#endif // _WIN32
  }
  return NULL;
}

void * raise_signal(void *params)
{
  if (params)
  {
    int sig = (int)(*((int *)params));
    raise(sig);
    gc_free(params);
  }
  return NULL;
}

// -- Event handlers --

#ifdef __linux
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
#endif // __linux

void quit(int sig)
{
  unsigned int i = 0;
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

  default_handler(sig);
}

void default_handler(int sig)
{
  switch (sig)
  {
  case SIGINT:
    if (ncurs_default_sigint_handler)
      ncurs_default_sigint_handler(sig);
    break;
#ifdef __linux
  case SIGWINCH:
    if (ncurs_default_sigwinch_handler)
      ncurs_default_sigwinch_handler(sig);
    break;
#endif // __linux__
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
  if (proc->running)
  {
    char chbuffer[32] = {0};
    chtype buffer[32] = {0};
    int *sigparam = NULL;
    switch (input)
    {
    case 'q':
    case EOF:
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
#ifdef _WIN32
    ncurs_default_sigint_handler = signal(SIGINT, &quit);
#else // __linux
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
#endif // _WIN32
  }

  proc->main = initscr();
  keypad(proc->main, SB_true);
  nonl();
  noecho();
  cbreak();
#ifdef __linux
  halfdelay(5);
#endif // __linux
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
    fflush(proc->debuglog);
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

  if (gc_init(0, 0))
  {
    pid = add_main_process(&proc);
    if (0 < pid)
    {
      proc.pid = pid;
      init(&proc);
      writelog(&proc, "Starting threads");
      if (start_process(&proc, &queue_input, &proc) &&
          start_process(&proc, &update, &proc))
      {
        retval = SB_success;
        writelog(&proc, "Waiting for threads");
        for (i = 0; i < proc.current_processes; i++)
        {
          writelog(&proc, "Thread finished");
          pthread_join(proc.processes[i], NULL);
        }
        writelog(&proc, "Done");
      }
      clean(&proc);
      raise(SIGINT);
    }
    gc_destroy();
  }
  return retval;
}
