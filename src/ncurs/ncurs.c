#include "ncurs.h"

#include "ticket.h"
#include "memory.h"

#define MAX_MAIN_PROCESSES (8)
#define MAX_PROCESSES (64)
#define MAX_CHAR (1024)

#define NCURS_PROC_INITIALIZER                                          \
  { NULL, false, 0U, 0U, 0U, {0}, 0U, {0}, NULL, NULL, NULL, NULL, TICKET_MUTEX_INITIALIZER, TICKET_MUTEX_INITIALIZER, NULL }

typedef struct ncurs_process_t {
  WINDOW *main;
  bool running;
  uint32_t id;
  uint32_t pid;
  uint32_t current_processes;
  pthread_t processes[MAX_PROCESSES];
  uint32_t queue_count;
  chtype ch_queue[MAX_CHAR];
  void (*update_f)(struct timespec *, Pointer);
  void (*handle_key_f)(chtype, Pointer);
  Pointer update_data;
  Pointer keyhandler_data;
  ticket_mutex work_lock;
  ticket_mutex write_lock;
  FILE *debuglog;
} NCursProc;

static NCursProc s_ncurs_main_processes[MAX_MAIN_PROCESSES] = {0};
static uint32_t s_ncurs_main_process_count = 0U;

static void (*ncurs_default_sigint_handler)(int);
static void (*ncurs_default_sigwinch_handler)(int);

static Pointer queue_input(Pointer);
static Pointer update(Pointer);
static Pointer raise_signal(Pointer);

static uint32_t genid(void);
static NCursProc * new_process(void);
static NCursProc * get_process(uint32_t id);
static void default_handler(int32_t);
static void resize(int32_t);
static void quit(int32_t);

static void handle_input(NCursProc *, chtype);
static bool start_process(NCursProc *, Pointer (*)(Pointer), Pointer);
static void init(NCursProc *);
static void clean(NCursProc *);
static void writelog(NCursProc *,const char *);

// --- Private ---
// -- Processes --

Pointer queue_input(Pointer params)
{
  NCursProc *proc = (NCursProc *)params;
  chtype input = 0;
  while (proc->running)
  {
    if (proc->queue_count < MAX_CHAR &&
        (input = getch()) != ERR && proc->running)
    {
      ticket_lock(&proc->work_lock);
      if (proc->running)
        proc->ch_queue[proc->queue_count++] = input;
      ticket_unlock(&proc->work_lock);
    }
  }
  return NULL;
}

Pointer update(Pointer params)
{
  static chtype s_chars[MAX_CHAR] = {0};
  NCursProc *proc = (NCursProc *)params;
  uint32_t i = 0U;
  uint64_t fps = 16666667UL; // 1/60 sec
  struct timespec prev = {0};
  struct timespec cur = {0};
  struct timespec delta = {0};
  struct timespec rem = {0};
  while (proc->running && rem.tv_sec == 0 && rem.tv_nsec == 0)
  {
    uint32_t count = 0U;
    ticket_lock(&proc->work_lock);
    if (proc->running)
    {
      memcpy(s_chars, proc->ch_queue, proc->queue_count);
      count = proc->queue_count;
    }
    proc->queue_count = 0;
    ticket_unlock(&proc->work_lock);

    for (i = 0U; i < count; i++)
    {
      if (!proc->running)
        break;
      handle_input(proc, s_chars[i]);
    }

    timespec_get(&cur, TIME_UTC);
    delta.tv_sec = (cur.tv_sec - prev.tv_sec);
    delta.tv_nsec = (cur.tv_nsec - prev.tv_nsec);
    memcpy(&prev, &cur, sizeof(struct timespec));

    if (proc->running && proc->update_f != NULL)
      proc->update_f(&delta, proc->update_data);
    refresh();

    if (proc->running && delta.tv_sec == 0 && delta.tv_nsec < fps)
      nanosleep(&delta, &rem);
  }
  return NULL;
}

Pointer raise_signal(Pointer params)
{
  if (params)
  {
    const char *msg_format = "Raising signal %s.";
    char buffer[32] = {0};
    NCursProc *proc = NULL;
    uint32_t id = 0U;
    int32_t sig = 0;
    size_t i = 0;

    memcpy(&id, params, sizeof(uint32_t));
    memcpy(&sig, params + sizeof(uint32_t), sizeof(int32_t));
    proc = get_process(id);

    switch(sig)
    {
    case SIGINT:
      i = sprintf(buffer, msg_format, "SIGINT");
      break;
    default:
      i = sprintf(buffer, msg_format, "UNKNOWN_SIGNAL");
      break;
    }
    buffer[i] = '\0';
    writelog(proc, buffer);
    
    raise(sig);
  }
  return NULL;
}

// -- Event handlers --

void resize(int32_t sig)
{
  NCursProc *proc = NULL;
  uint32_t i = 0U;
  for (i = 0U, proc = s_ncurs_main_processes; i < s_ncurs_main_process_count; i++, proc++)
  {
    if (proc && proc->running)
    {
      ticket_lock(&proc->work_lock);
      if (proc->running)
      {
        endwin();
        refresh();
      }
      ticket_unlock(&proc->work_lock);
    }
  }
  default_handler(sig);
}

void quit(int32_t sig)
{
  NCursProc *proc = NULL;
  uint32_t i = 0U;
  for (i = 0U, proc = s_ncurs_main_processes; i < s_ncurs_main_process_count; i++, proc++)
  {
    if (proc != NULL && proc->running)
    {
      ticket_lock(&proc->work_lock);
      if (proc->running)
        proc->running = false;
      clean(proc);
      ticket_unlock(&proc->work_lock);
    }
  }
  default_handler(sig);
}

void default_handler(int32_t sig)
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

uint32_t genid()
{
  static ticket_mutex ticket = TICKET_MUTEX_INITIALIZER;
  static uint32_t counter = 0U;
  uint32_t retval = 0U;
  
  ticket_lock(&ticket);
  counter += 1;
  retval = counter;
  ticket_unlock(&ticket);
  return retval;
}

NCursProc * new_process()
{
  NCursProc proc = NCURS_PROC_INITIALIZER;
  NCursProc *pproc = NULL;
  uint32_t i = 0U, id = genid();

  pproc = s_ncurs_main_processes;
  while (pproc->id != 0U && i < MAX_PROCESSES)
  {
    i++;
    pproc++;
  }
  if (i == MAX_PROCESSES)
    return NULL;

  memcpy(pproc, &proc, sizeof(NCursProc));
  s_ncurs_main_process_count += 1U;
  pproc->id = id;

  return pproc;
}

NCursProc * get_process(uint32_t id)
{
  if (0U < id)
  {
    uint32_t i;
    NCursProc *proc = NULL, *tmp = NULL;
    for (i = 0U, tmp = s_ncurs_main_processes; i < s_ncurs_main_process_count; i++, tmp++)
    {
      if (tmp->id == id)
      {
        proc = tmp;
        break;
      }
    }
    if (proc != NULL && 0U < proc->id)
      return proc;
  }
  return NULL;
}

void handle_input(NCursProc *proc, chtype input)
{
  if (proc->running && proc->handle_key_f != NULL)
    proc->handle_key_f(input, proc->keyhandler_data);
}

bool start_process(NCursProc *proc, Pointer (*routine)(Pointer), Pointer arg)
{
  pthread_t thread = {0};
  if (proc->running && proc->current_processes < MAX_PROCESSES &&
      pthread_create(&thread, NULL, routine, arg) == EXIT_SUCCESS)
  {
    proc->processes[proc->current_processes] = thread;
    proc->current_processes += 1;
    return true;
  }
  return false;
}

void init(NCursProc *proc)
{
  writelog(proc, "Initialising...");

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
    writelog(proc, "Signal handling is set.");
  }

  proc->main = initscr();
  keypad(proc->main, true);
  nonl();
  noecho();
  cbreak();
  halfdelay(5);
  curs_set(0);
  proc->running = true;
  writelog(proc, "Finished initialising.");
}

void clean(NCursProc *proc)
{
  writelog(proc, "Cleaning");
  endwin();
  writelog(proc, NULL);
}

void writelog(NCursProc *proc, const char *message)
{
#ifdef _SANDBOX_DEBUG
  ticket_lock(&proc->write_lock);
  if (message)
  {
    if (proc->debuglog == NULL)
    {
      char filename[128] = {0};
      sprintf(filename, "debug.log.%u", proc->pid);
      remove(filename);
      proc->debuglog = fopen(filename,"w");
    }

    if (proc->debuglog != NULL)
    {
      char buffer[2048] = {0};
      sprintf(buffer, "LOG: %s\n", message);
      fputs(buffer, proc->debuglog);
      fflush(proc->debuglog);
    }
  }
  else if (proc->debuglog != NULL)
  {
    fflush(proc->debuglog);
    fclose(proc->debuglog);
    proc->debuglog = NULL;
  }
  ticket_unlock(&proc->write_lock);
#endif // _SANDBOX_DEBUG
}

// --- Public ---

uint32_t ncurs_convert_string(chtype *output, char *input, uint32_t length)
{
  uint32_t i = 0;
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

void ncurs_quit(uint32_t id)
{
  NCursProc *proc = NULL;

  proc = get_process(id);
  if (proc == NULL)
    proc = get_process(1U);

  if (proc != NULL)
  {
    Pointer params = NULL;
    int32_t sig = SIGINT;

    params = calloc(1, sizeof(uint32_t) + sizeof(int32_t));
    memcpy(params, &id, sizeof(uint32_t));
    memcpy(params + sizeof(uint32_t), &sig, sizeof(int32_t));

    if (!start_process(proc, &raise_signal, params))
    {
      writelog(proc,
               " !!! Failed to raise interrupt signal through a thread !!!");
      clean(proc);
      raise(SIGINT);
    }
  }
  else
  {
    NCursProc *tmp = NULL;
    uint32_t i;

    writelog(proc,
             " !!! Failed to get a process so using the first running process found !!!");
    for (i = 0U, tmp = s_ncurs_main_processes; i < s_ncurs_main_process_count; i++, tmp++)
    {
      if (tmp->running)
      {
        ncurs_quit(tmp->id);
        return;
      }
    }

    writelog(proc, " !!! No running processes, raising a global SIGINT !!!");
    endwin();
    raise(SIGINT);
  }
}

uint32_t ncurs_start(void (*update_f)(struct timespec *, Pointer),
                     Pointer update_data,
                     void (*handle_key_f)(chtype, Pointer),
                     Pointer keyhandler_data)
{
  uint32_t id = 0U;
  NCursProc *proc = new_process();

  if (0U < proc->id)
  {
    init(proc);
    id = proc->id;
    proc->update_f = update_f;
    proc->handle_key_f = handle_key_f;
    proc->update_data = update_data;
    proc->keyhandler_data = keyhandler_data;

    writelog(proc, "Starting threads");
    if (start_process(proc, &queue_input, proc) &&
        start_process(proc, &update, proc))
    {
      writelog(proc, "Start success");
    }
    else
    {
      writelog(proc, "Start failed");
      clean(proc);
      raise(SIGINT);
    }
  }  
  return id;
}

void ncurs_wait(uint32_t id)
{
  if (0U < id)
  {
    uint32_t i = 0U;
    NCursProc *proc = get_process(id);
    writelog(proc, "Waiting for threads");
    while (i < proc->current_processes)
    {
      writelog(proc, "Thread finished");
      pthread_join(proc->processes[i], NULL);
      i += 1;
    }
    writelog(proc, "Done");
  }
  else
  {
    NCursProc *proc = get_process(1U);
    writelog(proc, " !!! Could not get process !!!");
  }
}

WINDOW *ncurs_window(uint32_t id)
{
  if (0U < id)
  {
    NCursProc *proc = get_process(id);
    if (proc != NULL && proc->running)
      return proc->main;
  }
  return NULL;
}
