#pragma once

#include "conf.h"

typedef struct
{
    gboolean on;
    GString* buf;
    GRegex*  regex;
}
prompt_t;

typedef struct
{
    gint x;
    gint y;
}
grid_line_t;

//effective LINES, last line is for status bar
#define EL (LINES - 1)

void curses_init();
void curses_deinit();
void key_loop();
