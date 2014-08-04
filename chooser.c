#include "opts.h"
#include "util.h"
#include "curses.h"

#include <unistd.h>
#include <locale.h>
#include <glib/gprintf.h>
#include <stdlib.h>
#include <string.h>

#define CHAR_VT 0x000bu

inline static void reopen_std_streams(void)
{
    conf.original_stdout = stdout;
    FILE* new_stdout = fopen("/dev/tty", "w");
    if (!new_stdout)
        fatal("can't open /dev/tty");
    stdout = new_stdout;

    conf.original_stdin = stdin;
    FILE* new_stdin = fopen("/dev/tty", "r");
    if (!new_stdin)
        fatal("can't open /dev/tty");
    stdin = new_stdin;
}

inline static void revert_std_streams(void)
{
    fclose(stdout);
    stdout = conf.original_stdout;
    fclose(stdin);
    stdin  = conf.original_stdin;
}

static void adjust_channel_encoding(GIOChannel* channel, const gchar* name, gboolean binary)
{
    GIOStatus   status;
    const char* charset;
    if (!binary)
        g_get_charset(&charset);
    else
        charset = NULL;
        status = g_io_channel_set_encoding(channel, charset, NULL);
        if (status != G_IO_STATUS_NORMAL)
            fatal("can't set encoding `%s' for `%s'", charset == NULL? "binary" : charset, name);
}

static void read_channel(GIOChannel* reader, const gchar* name)
{
    if (!reader)
        fatal("can't read `%s'", name);
    adjust_channel_encoding(reader, name, FALSE);

    GIOStatus   status;
    g_io_channel_set_line_term(reader, NULL, -1);

    GString* gstr   = g_string_new("");
    while ( (status = g_io_channel_read_line_string(reader, gstr, NULL, NULL)) != G_IO_STATUS_EOF )
    {
        if (status == G_IO_STATUS_ERROR)
            fatal("error while reading `%s' occured", name);

        gboolean is_white = TRUE;
        gunichar c;
        gchar*   p = gstr->str;
        while (*p)
        {
            c = g_utf8_get_char(p);
            p = g_utf8_next_char(p);
            if (!g_unichar_isspace(c) && !(c == CHAR_VT) ) /*vertical tabulation is «space» symbol too*/
            {
                is_white = FALSE;
                break;
            }
        }
        if (is_white && !conf.whitelines)
            continue;

        line_t* line = g_new(line_t, 1);

        gchar*  norm = g_utf8_normalize(gstr->str, gstr->len, G_NORMALIZE_ALL_COMPOSE);
        GString* ns;
        if (g_strcmp0(gstr->str, norm) == 0)
            ns = gstr;
        else
            ns = g_string_new(norm);
        g_free(norm);

        line->checked = conf.initial;
        line->realstr = gstr;
        line->string  = ns;
        line->size    = ns->len;
        line->length  = g_utf8_strlen(ns->str, ns->len);
        line->width   = g_utf8_strwidth(ns->str);
        line->white   = is_white;
        if (line->width > max_string_width)
            max_string_width = line->width;

        g_ptr_array_add(strings, line);
        gstr = g_string_new("");
    }

    g_string_free(gstr, TRUE);
    g_io_channel_shutdown(reader, FALSE, NULL);
    g_io_channel_unref(reader);
}

inline static void read_data(void)
{
    if ( !isatty(STDIN_FILENO) )
        read_channel(g_io_channel_unix_new(STDIN_FILENO), "stdin");
    register gint   i;
    gchar**         p = conf.infiles;
    for (i = 0; p[i] && p ; i++)
        read_channel(g_io_channel_new_file(p[i], "r", NULL), p[i]);
}

static void write_data(void)
{
    GIOChannel* writer = g_io_channel_unix_new(STDOUT_FILENO);
    if (!writer)
        fatal("%s", "can't write to stdout");
    adjust_channel_encoding(writer, "stdout", TRUE);

    for (guint i = 0; i < SL; i++)
    {
        line_t* lt = (line_t*) g_ptr_array_index(strings, i);
        if (lt->checked)
        {
            glong  written = 0;
            glong  size    = lt->realstr->len;
            gchar* str     = lt->realstr->str;
            while (written < size)
            {
                gsize written_once;
                g_io_channel_write_chars(writer, str + written, size - written, &written_once, NULL);
                written += written_once;
            }
            g_io_channel_flush(writer, NULL);
        }
    }
    g_io_channel_shutdown(writer, TRUE, NULL);
    g_io_channel_unref(writer);
}

gint main(gint argc, gchar **argv)
{
    setlocale(LC_ALL, "");
    parseopts(argc, argv);
    max_string_width = 0;
    strings = g_ptr_array_new();
    read_data();

    /*nothing to check, exit*/
    if (SL == 0)
        exit(EXIT_SUCCESS);
    /*one element only, already checked. pass it quietly.*/
    else if (SL == 1 && conf.initial)
    {
        write_data();
        exit(EXIT_SUCCESS);
    }

    reopen_std_streams();
    curses_init();

    key_loop();

    curses_deinit();
    revert_std_streams();
    write_data();
    exit(EXIT_SUCCESS);
}
