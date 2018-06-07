#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <ctype.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#ifdef _WIN32
#include <windows.h>
#endif // _WIN32


#ifndef min
#define min(x,y) (x < y ? x : y)
#endif // min
#ifndef max
#define max(x,y) (x < y ? y : x)
#endif // max

#ifndef NULL
#define NULL ((void *)0)
#endif // NULL

typedef enum SB_bool_e {
  SB_false = 0,
  SB_true = 1
} SB_bool;

typedef enum exit_value_e {
  SB_success = 0,
  SB_failure = 1
} exit_value;

typedef void * Pointer;

#endif // __COMMON_H__
