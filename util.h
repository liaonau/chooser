#pragma once
#include "conf.h"

/* ANSI term color codes */
#define ANSI_COLOR_RESET   "\x1b[0m"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"

#define ANSI_COLOR_BG_RED  "\x1b[41m"

void _fatal(gint, const gchar *, const gchar *, ...);
#define fatal(string, ...) _fatal(__LINE__, __func__, string, ##__VA_ARGS__)

#define warn(string, ...) _warn(__LINE__, __func__, string, ##__VA_ARGS__)
void _warn(gint, const gchar *, const gchar *, ...);

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

glong g_utf8_strwidth(char*);
gchar* g_utf8_substring_by_width(gchar*, glong, glong);
gchar* get_utf8_substring_by_width(guint, glong, glong);


inline static guint num_digits(const gulong n)
{
    if (n < 10) return 1;
    return 1 + num_digits(n / 10);
}

static inline gboolean is_name(const gchar* name, const gchar* name_compare)
{
    return (g_strcmp0(name, name_compare) == 0);
}

inline static guint gunichar_width(gunichar x)
{
    if (g_unichar_iswide(x))
        return 2;
    else if (g_unichar_iszerowidth(x))
        return 0;
    else if (g_unichar_iscntrl(x))
        return 2;
    else
        return 1;
}

inline static gunichar get_unichar(gchar* s)
{
    gunichar c = g_utf8_get_char_validated(s, -1);
    if (c == 0x0009) //tab
        c = 0x2409;
    if (c == 0x000b) //vertical tab
        c = 0x240b;
    else if (c == (gunichar)(-2) ||c == (gunichar)(-1))
        c = ' ';
    return c;
}

inline static gboolean has_character(const gchar* str)
{
    return (str && g_utf8_validate(str, -1, NULL) && g_utf8_strlen(str, -1) > 0);
}

inline static gboolean is_separator(gunichar c)
{
    return (c == '\n' || c == '\r' || c == 0x0000 || c == 0x2029);
}

inline static line_t* get_line_t(guint index)
{
    return (line_t*) g_ptr_array_index(strings, index);
}

inline static gchar* get_str(guint index)
{
    return get_line_t(index)->string->str;
}

inline static glong get_size(guint index)
{
    return get_line_t(index)->size;
}

inline static glong get_length(guint index)
{
    return get_line_t(index)->length;
}

inline static glong get_width(guint index)
{
    return get_line_t(index)->width;
}

inline static gboolean is_white(guint index)
{
    return get_line_t(index)->white;
}

inline static gboolean width_is_length(guint index)
{
    return (get_width(index) == get_length(index));
}

inline static gboolean is_checked(guint index)
{
    return get_line_t(index)->checked;
}

inline static void set_checked(guint index, gboolean b)
{
    get_line_t(index)->checked = b;
}

inline static void uncheck_all()
{
    for (glong i = 0; i < SL; i++)
        set_checked(i, false);
}

inline static void check_all()
{
    if (!conf.radiobox)
    {
        for (glong i = 0; i < SL; i++)
            set_checked(i, true);
    }
    else
    {
        uncheck_all();
        set_checked(view.current, TRUE);
    }
}

inline static void toggle_all()
{
    if (!conf.radiobox)
    {
        for (glong i = 0; i < SL; i++)
            set_checked(i, !is_checked(i));
    }
}

inline static void toggle_radiobox()
{
    if (!conf.radiobox)
    {
        if (is_checked(view.current))
        {
            uncheck_all();
            set_checked(view.current, TRUE);
        }
        else
            uncheck_all();
    }
    conf.radiobox = !conf.radiobox;
}

inline static void toggle_checked(guint index)
{
    gboolean state = is_checked(index);
    if (!state && conf.radiobox)
        uncheck_all();
    set_checked(index, !state);
}
