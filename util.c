#include "util.h"
#include "conf.h"
#include "opts.h"

#include <string.h>
#include <glib/gprintf.h>
#include <unistd.h>
#include <stdlib.h>

/* Print error and exit with EXIT_FAILURE code. */
void _fatal(gint line, const gchar *fct, const gchar *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    gint atty = isatty(STDERR_FILENO);
    if (atty) g_fprintf(stderr, ANSI_COLOR_BG_RED);
    g_fprintf(stderr, "E: %s: %s:%d: ", APPNAME, fct, line);
    g_vfprintf(stderr, fmt, ap);
    va_end(ap);
    if (atty) g_fprintf(stderr, ANSI_COLOR_RESET);
    g_fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

/* Print error message on stderr. */
void _warn(gint line, const gchar *fct, const gchar *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    gint atty = isatty(STDERR_FILENO);
    if (atty) g_fprintf(stderr, ANSI_COLOR_YELLOW);
    g_fprintf(stderr, "E: %s: %s:%d: ", APPNAME, fct, line);
    g_vfprintf(stderr, fmt, ap);
    va_end(ap);
    if (atty) g_fprintf(stderr, ANSI_COLOR_RESET);
    g_fprintf(stderr, "\n");
}

glong g_utf8_strwidth(char* p)
{
    gunichar c;
    glong width = 0;
    while (*p)
    {
        c = get_unichar(p);
        if (is_separator(c))
            break;
        width += gunichar_width(c);
        p = g_utf8_next_char(p);
    }
    return width;
}

gchar* g_utf8_substring_by_width(gchar* str, glong start_width, glong finish_width)
{
    gunichar c;
    glong length = 0;
    glong start  = 0;
    glong finish = 0;
    glong w = 0;
    gchar* p = str;
    while (*p)
    {
        c = get_unichar(p);
        glong width = gunichar_width(c);
        w += width;
        p = g_utf8_next_char(p);
        length++;

        if (w == start_width)
            start = length;
        else if (w == start_width + 1 && width == 2)
            start = length - 1;
        if (w == finish_width)
        {
            finish = length;
            break;
        }
        else if (w == finish_width + 1 && width == 2)
        {
            finish = length;
            break;
        }
    }
    if (finish < start)
        finish = start = 0;
    return g_utf8_substring(str, start, finish);
}

gchar* get_utf8_substring_by_width(guint index, glong start_width, glong finish_width)
{
    gchar* str = get_str(index);
    if (width_is_length(index))
        return g_utf8_substring(str, start_width, finish_width);
    else
        return g_utf8_substring_by_width(str, start_width, finish_width);
}
