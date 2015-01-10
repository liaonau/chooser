#pragma once
#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED

#include <glib.h>
#include <stdio.h>
#include <ncursesw/ncurses.h>
#include <stdbool.h>

#define APPNAME "chooser"

typedef struct
{
    gboolean onecolumn;
    gboolean checkbox;
    gboolean numbers;
    gboolean underline;
    gboolean color;
    gboolean radiobox;
    gboolean initial;
    gboolean fullattr;
    gboolean whitelines;
    gchar*   foreground;
    gchar*   background;
    short    fg;
    short    bg;

    gchar*   execpath;

    gchar**  infiles;

    FILE*    original_stdout;
    FILE*    original_stdin;
} conf_t;

typedef struct
{
    gboolean checked;
    GString* string;
    GString* realstr;

    glong    width;
    glong    length;
    glong    size;

    gboolean white;
} line_t;

typedef struct
{
    glong current;
    glong top_x;
    glong top_y;

    glong prefix_width;
    glong max_text_width;

    glong cols;
    glong rows;
} view_t;


GPtrArray* strings;
#define SL strings->len

guint max_string_width;

conf_t conf;
view_t view;
