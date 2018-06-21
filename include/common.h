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

#ifndef bool
#ifdef BOOL
typedef BOOL bool;
#else
typedef int bool;
#endif // BOOL
#endif // bool

#ifndef true
#ifdef TRUE
typedef TRUE true;
#else
#define true (!(0))
#endif // TRUE
#endif // true

#ifndef false
#ifdef FALSE
typedef FALSE false;
#else
#define false (!(true))
#endif // FALSE
#endif // true

#ifndef min
#define min(x,y) (x < y ? x : y)
#endif // min
#ifndef max
#define max(x,y) (x < y ? y : x)
#endif // max

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS (0)
#endif // EXIT_SUCCESS

#ifndef EXIT_FAILURE
#define EXIT_FAILURE (1)
#endif // EXIT_FAILURE

typedef void * Pointer;

#ifndef NULL
#define NULL ((Pointer)(0))
#endif // NULL

#endif // __COMMON_H__
