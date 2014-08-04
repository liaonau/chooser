#include <string.h>
#include <unistd.h>

#include "curses.h"
#include "util.h"

#include <readline/readline.h>
#include <readline/history.h>
#include <glib/gstdio.h>

GArray*    grid;
prompt_t   prompt;
WINDOW*    ws;
FILE*      null;
#define    COLORPAIR 1

inline static void repaint();
static void regrid();

#define bounds(x, min, max) x = MIN((max), MAX((min), (x)))

static gchar* get_prefix(glong index)
{
    gchar* prefix;
    gchar space = ' ';
    if (view.current == index && (view.top_x > 0 || is_white(index)) )
        space = '>';
    prefix = g_strnfill(1, space);

    if (conf.numbers)
    {
        gchar* tmp = prefix;
        glong max_n = num_digits(SL);
        glong n = num_digits(index);
        gchar* str_index = g_strdup_printf("%ld", index);
        gchar* num_prefix = g_strnfill(max_n - n, ' ');
        prefix = g_strconcat(num_prefix, str_index, tmp, NULL);
        g_free(tmp);
        g_free(num_prefix);
        g_free(str_index);
    }

    if (conf.checkbox)
    {
        gchar* symbol = " ";
        if (is_checked(index))
        {
            symbol = "x";
            if (conf.radiobox) symbol = "o";
        }
        gchar* tmp = prefix;
        prefix = g_strconcat(tmp, "[", symbol, "]", NULL);
        g_free(tmp);
    }
    return prefix;
}

static inline glong get_prefix_width()
{
    gchar* prefix = get_prefix(0);
    glong width = g_utf8_strwidth(prefix);
    g_free(prefix);
    return width;
}

void curses_init()
{
    null = g_fopen("/dev/null", "w");
    unsetenv("COLUMNS");
    unsetenv("LINES");
    initscr();
    noecho();
    nonl();
    cbreak();
    keypad(stdscr, TRUE);
    meta(stdscr, TRUE);
    intrflush(stdscr, FALSE);
    leaveok(stdscr, TRUE);

    if (has_colors())
    {
        start_color();
        use_default_colors();
        init_pair(COLORPAIR, conf.fg, conf.bg);
    }
    else
        conf.color = FALSE;

    curs_set(false);

    ws = newwin(1, COLS, EL, 0);

    view.current = 0;
    view.top_x   = 0;
    view.top_y   = 0;

    prompt.buf   = g_string_new("");
    prompt.on    = false;
    prompt.regex = NULL;

    grid = g_array_sized_new(FALSE, TRUE, sizeof(grid_line_t*), SL);
    for (glong i = 0; i < SL; i++)
    {
        grid_line_t* grid_line = g_malloc(sizeof(grid_line_t));
        g_array_append_val(grid, grid_line);
    }

    regrid();
}

void curses_deinit()
{
    delwin(ws);
    endwin();
}

static inline void correct_top_y()
{
    bounds(view.current, 0, SL - 1);
    glong max = view.rows - EL;
    bounds(view.top_y, 0, max);
    glong y   = view.current / view.cols;
    bounds(view.top_y, MAX(0, (y - EL + 1)), y);
}

static inline void correct_top_x()
{
    if (view.top_x < 0)
        view.top_x = 0;
    else
    {
        glong max_top_x = MAX(0, (get_width(view.current) - COLS + view.prefix_width));
        view.top_x = MIN(max_top_x, view.top_x);
    }
}

static void regrid()
{
    glong pw = get_prefix_width();
    view.prefix_width   = pw;
    glong mtw = MIN((COLS - pw), max_string_width);
    view.max_text_width = mtw;
    correct_top_x();

    view.cols = conf.onecolumn ? 1 : (COLS / (mtw + pw));
    view.rows = SL % view.cols == 0 ? SL / view.cols : (SL / view.cols) + 1;
    correct_top_y();

    for (glong i = 0; i < SL; i++)
    {
        glong yn = i / view.cols;
        glong xn = i % view.cols;
        grid_line_t* grid_line = g_array_index(grid, grid_line_t*, i);
        grid_line->y = yn;
        grid_line->x = (mtw + pw) * xn;
    }
    mvwin(ws, LINES - 1, 0);
}

static void draw_element(glong index)
{
    grid_line_t* gl = g_array_index(grid, grid_line_t*, index);
    gchar* prefix = get_prefix(index);
    mvaddstr(gl->y - view.top_y, gl->x, prefix);
    g_free(prefix);

    if (index == view.current)
        standout();
    if (is_checked(index))
    {
        if (conf.color)
            attron(COLOR_PAIR(COLORPAIR));
        if (conf.underline)
            attron(A_UNDERLINE);
    }

    glong start  = MIN(view.top_x, get_width(index));
    glong finish = MIN(view.max_text_width, get_width(index) - start) + start;
    glong fill_width = view.max_text_width - (finish - start);
    gchar* p = get_utf8_substring_by_width(index, start, finish);
    gchar* s = p;
    gunichar c;
    while(*p)
    {
        c = get_unichar(p);
        gchar* ns = g_ucs4_to_utf8(&c, 1, NULL, NULL, NULL);
        addstr(ns);
        g_free(ns);
        p = g_utf8_next_char(p);
    }
    g_free(s);
    if (!conf.fullattr)
        standend();
    gchar* fs = g_strnfill(fill_width, ' ');
    addstr(fs);
    g_free(fs);
    standend();
}

static void redraw_search_line(gchar* search_method)
{
    werase(ws);
    gchar* pr = "Search:";
    if (rl_editing_mode == 0)
    {
        gchar* name = rl_get_keymap_name(rl_get_keymap());
        pr = "Search(-- NORMAL --):";
        if (g_strcmp0(name, "vi-insert") == 0)
            pr = "Search(-- INSERT --):";
    }
    gchar* prompt_separator = " ";
    glong prompt_width = g_utf8_strwidth(search_method) + g_utf8_strwidth(pr) + 1;
    glong rl_width     = g_utf8_strwidth(rl_line_buffer);

    gchar* str_to_point = g_utf8_substring(rl_line_buffer, 0, g_utf8_strlen(rl_line_buffer, rl_point));
    glong point_width   = g_utf8_strwidth(str_to_point);
    g_free(str_to_point);

    glong width  = MIN(rl_width, COLS - prompt_width);
    glong start  = 0;
    glong finish = width;
    if (rl_width > width)
    {
        start  = (point_width / width) * width;
        finish = MIN(width + start, rl_width);
        prompt_separator = "…";
    }
    gchar* to_print = g_utf8_substring_by_width(rl_line_buffer, start, finish);
    gchar* search   = g_strconcat(search_method, pr, prompt_separator, to_print, NULL);
    g_free(to_print);
    mvwaddstr(ws, 0, 0, search);
    g_free(search);
    while(point_width > width)
        point_width -= width;
    wmove(ws, 0, prompt_width + point_width);
    wrefresh(ws);
}

static inline gchar* get_position_status()
{
    glong yn = view.current / view.cols;
    glong xn = view.current % view.cols;
    glong percent = (100 * (view.top_y + EL / 2)) / view.rows;
    if (view.top_y == (view.rows - EL) || view.rows <= EL)
        percent = 100;
    else if (view.top_y == 0)
        percent = 0;
    glong vcm, ynm, xnm;
    ynm  = num_digits(view.rows);
    xnm  = num_digits(view.cols);
    gchar* position_format;
    gchar* position;
    if (view.cols > 1)
    {
        vcm  = num_digits(SL);
        position_format = g_strdup_printf("[№%%-%ldld:%%%ldld,%%-%ldld] %%3ld%%%% ", vcm, ynm, xnm);
        position = g_strdup_printf(position_format, view.current, yn, xn, percent);
    }
    else
    {
        position_format = g_strdup_printf("[№%%%ldld,%%-%ldld] %%3ld%%%% ", ynm, xnm);
        position = g_strdup_printf(position_format, yn, view.top_x, percent);
    }
    g_free(position_format);
    return position;
}

static void draw_status_line()
{
    glong width = 0;

    gchar* position = get_position_status();
    mvwaddstr(ws, 0, width, position);
    width += strlen(position);
    g_free(position);
    if (!prompt.on && prompt.buf->len > 0)
    {
        mvwaddstr(ws, 0, width, "[");
        waddstr(ws, prompt.buf->str);
        waddstr(ws, "]");
    }
}

static inline void draw()
{
    correct_top_x();
    for (glong i = view.top_y * view.cols; i < MIN(SL, view.cols *(EL + view.top_y)); i++)
        draw_element(i);
}

static inline void repaint()
{
    erase();
    werase(ws);
    draw();
    draw_status_line();
    refresh();
    wrefresh(ws);
}

static inline void center_view()
{
    view.top_y = view.current / view.cols - EL / 2;
    correct_top_y();
}

static inline void move_element(glong i)
{
    view.current += i;
    correct_top_y();
}

static inline void move_page(glong i)
{
    view.top_y += i* EL;
    view.current += i * EL * view.cols;
    correct_top_y();
}

static inline void move_home()
{
    view.current = 0;
    view.top_y = 0;
}

static inline void move_end()
{
    view.current = SL - 1;
    view.top_y = MAX(view.rows - EL, 0);
}

static void move_horizontal(glong i)
{
    glong y = view.current / view.cols;
    glong x = view.current % view.cols + i;
    gint max_x = view.cols - 1;
    if (y == view.rows - 1)
        max_x = SL - (view.rows - 1) * view.cols - 1;
    bounds(x, 0, max_x);
    view.current = view.cols * y + x;
    correct_top_y();
}

static void move_vertical(glong i)
{
    glong y = view.current / view.cols + i;
    glong x = view.current % view.cols;
    gint max_y = view.rows - 1;
    gint max_x = SL - (view.rows - 1) * view.cols - 1;
    if (x > max_x)
        max_y = view.rows - 2;

    bounds(y, 0, max_y);
    view.current = view.cols * y + x;
    correct_top_y();
}

static inline void x_move_left(glong offset)
{
    if (offset > view.top_x)
        offset = view.top_x;
    gchar* p = get_utf8_substring_by_width(view.current, view.top_x - offset, view.top_x);
    view.top_x -= g_utf8_strwidth(p);
    g_free(p);
    correct_top_x();
}

static inline void x_move_right(glong offset)
{
    glong max_right_x = MAX(0, (glong)(get_width(view.current) - COLS + view.prefix_width));
    if (offset > max_right_x - view.top_x)
        offset = max_right_x - view.top_x;
    gchar* p = get_utf8_substring_by_width(view.current, view.top_x, view.top_x + offset);
    view.top_x += g_utf8_strwidth(p);
    g_free(p);
    correct_top_x();
}

static inline void x_move_home()
{
    view.top_x = 0;
    correct_top_x();
}

static inline void x_move_end()
{
    view.top_x = get_width(view.current) - COLS + view.prefix_width;
    correct_top_x();
}

static inline void clear_search()
{
    g_string_truncate(prompt.buf, 0);
}

static inline void toggle_option(gboolean* b, gboolean do_regrid)
{
    *b = !(*b);
    if (do_regrid)
        regrid();
}

static inline void toggle_color()
{
    if (has_colors())
        conf.color = !conf.color;
}

static void cb_search()
{
    gchar* rllb = g_utf8_normalize(rl_line_buffer, -1, G_NORMALIZE_ALL_COMPOSE);
    g_string_assign(prompt.buf, rllb);
    g_free(rllb);
    if (g_utf8_strlen(prompt.buf->str, -1) > 0)
        add_history(rl_line_buffer);
    rl_callback_handler_remove();
    prompt.on = false;
    curs_set(false);
    resetty();
}

static void find_next_line_mathching()
{
    if (prompt.regex == NULL)
        return;
    guint c = view.current;
    for (guint i = c + 1; i <= SL + c; i++)
    {
        if (g_regex_match(prompt.regex, get_str(i % SL), G_REGEX_MATCH_NOTEMPTY, NULL))
        {
            view.current = i % SL;
            center_view();
            break;
        }
    }
}

static void find_prev_line_mathching()
{
    if (prompt.regex == NULL)
        return;
    guint c = view.current;
    for (guint i = SL + c - 1; i >= c - 1; i--)
    {
        if (g_regex_match(prompt.regex, get_str(i % SL), G_REGEX_MATCH_NOTEMPTY, NULL))
        {
            view.current = i % SL;
            center_view();
            break;
        }
    }
}

static void check_founded(gint do_toggle)
{
    if (conf.radiobox)
        return;
    if (prompt.regex != NULL)
    {
        for (guint i = 0; i < SL; i++)
        {
            if (g_regex_match(prompt.regex, get_str(i), G_REGEX_MATCH_NOTEMPTY, NULL))
            {
                if (do_toggle == 0)
                    toggle_checked(i);
                else if (do_toggle > 0)
                    set_checked(i, TRUE);
                else
                    set_checked(i, FALSE);
            }
        }
    }
}

static void perform_search(GRegexCompileFlags compile_options)
{
    rl_outstream = null;
    Keymap map_vi    = rl_get_keymap_by_name("vi");
    Keymap map_emacs = rl_get_keymap_by_name("emacs");
    rl_unbind_key_in_map(CTRL('l'), map_vi);
    rl_unbind_key_in_map(CTRL('l'), map_emacs);
    savetty();
    //It's a kind of magic.
    reset_shell_mode();
    curs_set(true);
    prompt.on = true;
    rl_callback_handler_install("", cb_search);
    def_shell_mode();
    gchar* search_method = (compile_options & G_REGEX_CASELESS) ? "Ignore Case " : "";
    redraw_search_line(search_method);
    while(true)
    {
        rl_callback_read_char();
        if (!prompt.on)
            break;
        redraw_search_line(search_method);
    }
    if (prompt.regex != NULL)
        g_regex_unref(prompt.regex);
    prompt.regex = g_regex_new(prompt.buf->str, compile_options, G_REGEX_MATCH_NOTEMPTY, NULL);
}

/* static inline const gchar* get_key(wint_t* key) */
/* { */
/*     *key = (wint_t)getch(); */
/*     const char* name = keyname(*key); */
/*     if (name != NULL) */
/*         if (g_str_has_prefix(name, "M-")) */
/*         { */
/*             ungetch(*key); */
/*             get_wch(key); */
/*             name = key_name((wchar_t)*key); */
/*         } */
/*     return name; */
/* } */

void key_loop()
{
    repaint();
    wint_t key;
    /* const gchar* name; */

    while(TRUE)
    {
        get_wch(&key);
        /* name = get_key(&key); */
        gboolean do_repaint = TRUE;

        switch(key)
        {
        case 'm':
            check_founded(1);
            break;
        case 'M':
            check_founded(-1);
            break;
        case 'T':
            check_founded(0);
            break;
        case 's':
            perform_search(G_REGEX_CASELESS);
            break;
        case 'S':
            perform_search(0);
            break;
        case '/':
            perform_search(G_REGEX_CASELESS);
            find_next_line_mathching();
            break;
        case '?':
            perform_search(G_REGEX_CASELESS);
            find_prev_line_mathching();
            break;
        case 'n':
            find_next_line_mathching();
            break;
        case 'N':
            find_prev_line_mathching();
            break;
        case 'c':
            clear_search();
            break;

        case 'z':
            center_view();
            break;
        case CTRL('i'):
            move_element(1);
            break;
        case KEY_BTAB:
            move_element(-1);
            break;
        case KEY_DOWN:
        case 'j':
            move_vertical(1);
            break;
        case KEY_UP:
        case 'k':
            move_vertical(-1);
            break;
        case KEY_LEFT:
        case 'h':
            move_horizontal(-1);
            break;
        case KEY_RIGHT:
        case 'l':
            move_horizontal(1);
            break;
        case KEY_NPAGE:
            move_page(1);
            break;
        case KEY_PPAGE:
            move_page(-1);
            break;
        case 'G':
            move_end();
            break;
        case 'g':
            move_home();
            break;
        case ',':
            x_move_left(1);
            break;
        case 'H':
        case '<':
            x_move_left(COLS/2);
            break;
        case '.':
            x_move_right(1);
            break;
        case 'L':
        case '>':
            x_move_right(COLS/2);
            break;
        case KEY_HOME:
        case '^':
            x_move_home();
            break;
        case KEY_END:
        case '$':
            x_move_end();
            break;

        case ' ':
            toggle_checked(view.current);
            break;
        case '!':
            uncheck_all();
            toggle_checked(view.current);
            break;
        case 't':
            toggle_all();
            break;
        case 'a':
            check_all();
            break;
        case 'A':
            uncheck_all();
            break;

        case 'x':
            toggle_option(&conf.checkbox, true);
            break;
        case '#':
            toggle_option(&conf.numbers, true);
            break;
        case 'o':
        case '1':
            toggle_option(&conf.onecolumn, true);
            break;
        case 'u':
            toggle_option(&conf.underline, false);
            break;
        case 'f':
            toggle_option(&conf.fullattr, false);
            break;
        case 'r':
            toggle_radiobox();
            break;
        case 'C':
            toggle_color();
            break;

        case KEY_RESIZE:
        case KEY_REFRESH:
        case CTRL('l'):
            regrid();
            break;

        case KEY_ENTER:
        case '\n':
        case '\r':
            return;
        case 'q':
            uncheck_all();
            return;

        case 27 :
        default :
            do_repaint = FALSE;
        }

        if (do_repaint)
            repaint();
    }
}
