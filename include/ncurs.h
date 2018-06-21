#ifndef __NCURS_H__
#define __NCURS_H__

#include "common.h"

#include <ncurses.h>

uint32_t ncurs_convert_string(chtype *output, char *input, uint32_t length);
void     ncurs_quit(uint32_t id);
uint32_t ncurs_start(void (*update_f)(struct timespec *, Pointer),
                     Pointer update_data,
                     void (*handle_key_f)(chtype, Pointer),
                     Pointer keyhandler_data);
void     ncurs_wait(uint32_t id);
WINDOW * ncurs_window(uint32_t id);

#endif // __NCURS_H__
