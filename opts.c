#include "conf.h"
#include "opts.h"
#include "util.h"

static void assign_boolean(GKeyFile* key_file, gboolean* var, const gchar* key)
{
    GError* error  = NULL;
    *var = g_key_file_get_boolean(key_file, "options", key, &error);
    if (error != NULL)
    {
        warn("%s\n", error->message);
        g_error_free(error);
    }
}

static void assign_string(GKeyFile* key_file, gchar** var, const gchar* key)
{
    GError* error  = NULL;
    *var = g_key_file_get_string(key_file, "options", key, &error);
    if (error != NULL)
    {
        /* not critical at all, will be -1 */
        /* warn("%s\n", error->message); */
        g_error_free(error);
    }
}

static short get_curs_color(const char* name)
{
    if (name == NULL)
        return -1;
    short c = -1;
    gchar* n = g_ascii_strdown(name, -1);
    if     (is_name(n, "black"))
        c = COLOR_BLACK;
    else if(is_name(n, "red"))
        c = COLOR_RED;
    else if(is_name(n, "green"))
        c = COLOR_GREEN;
    else if(is_name(n, "yellow"))
        c = COLOR_YELLOW;
    else if(is_name(n, "blue"))
        c = COLOR_BLUE;
    else if(is_name(n, "magenta"))
        c = COLOR_MAGENTA;
    else if(is_name(n, "cyan"))
        c = COLOR_CYAN;
    else if(is_name(n, "white"))
        c = COLOR_WHITE;
    g_free(n);
    return c;
}

inline static void loadconf()
{
    const gchar* const *config_dirs = g_get_system_config_dirs();
    GPtrArray          *ptr_paths   = g_ptr_array_new();
    g_ptr_array_add(ptr_paths, g_build_filename(g_get_user_config_dir(), APPNAME, "/", NULL));
    for(; *config_dirs; config_dirs++)
        g_ptr_array_add(ptr_paths, g_build_filename(*config_dirs, APPNAME, "/", NULL));
    gchar** paths = (gchar**)g_ptr_array_free(ptr_paths, FALSE);

    GError* error = NULL;
    GKeyFile* key_file = g_key_file_new();
    gchar* fullpath = NULL;
    gboolean can_be_loaded = g_key_file_load_from_dirs(key_file, "config", (const gchar**)paths, &fullpath, G_KEY_FILE_NONE, &error);
    if (!can_be_loaded)
    {
        warn("%s\n", error->message);
        g_error_free(error);
    }
    else
    {

        assign_boolean(key_file, &conf.onecolumn,  "onecolumn" );
        assign_boolean(key_file, &conf.checkbox,   "checkbox"  );
        assign_boolean(key_file, &conf.numbers,    "numbers"   );
        assign_boolean(key_file, &conf.underline,  "underline" );
        assign_boolean(key_file, &conf.color,      "color"     );
        assign_boolean(key_file, &conf.radiobox,   "radiobox"  );
        assign_boolean(key_file, &conf.initial,    "initial"   );
        assign_boolean(key_file, &conf.whitelines, "whitelines");
        assign_boolean(key_file, &conf.fullattr,   "fullattr"  );
        assign_string (key_file, &conf.foreground, "foreground");
        assign_string (key_file, &conf.background, "background");
    }

    g_key_file_free(key_file);
    g_strfreev(paths);
    g_free(fullpath);
}

void parseopts(gint argc, gchar **argv)
{
    loadconf();

    GOptionContext      *context;
    const GOptionEntry  entries[] =
    {
        { "onecolumn",      'o', 0,                     G_OPTION_ARG_NONE,   &conf.onecolumn,  "output in one column",                 NULL },
        { "checkbox",       'x', 0,                     G_OPTION_ARG_NONE,   &conf.checkbox,   "show checkbox",                        NULL },
        { "numbers",        'n', 0,                     G_OPTION_ARG_NONE,   &conf.numbers,    "show numbers of lines",                NULL },
        { "underline",      'u', 0,                     G_OPTION_ARG_NONE,   &conf.underline,  "underline checked lines",              NULL },
        { "color",          'c', 0,                     G_OPTION_ARG_NONE,   &conf.color,      "highlight checked lines with color",   NULL },
        { "radiobox",       'r', 0,                     G_OPTION_ARG_NONE,   &conf.radiobox,   "only one line can be checked",         NULL },
        { "initial",        'i', 0,                     G_OPTION_ARG_NONE,   &conf.initial,    "check all lines initially",            NULL },
        { "whitelines",     'w', 0,                     G_OPTION_ARG_NONE,   &conf.whitelines, "do not skip white lines",              NULL },
        { "fullattr",       'l', 0,                     G_OPTION_ARG_NONE,   &conf.fullattr,   "draw attributes till the end of line", NULL },
        { "not-onecolumn",  'O', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE,   &conf.onecolumn,  "not --onecolumn",                      NULL },
        { "not-checkbox",   'X', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE,   &conf.checkbox,   "not --checkbox",                       NULL },
        { "not-numbers",    'N', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE,   &conf.numbers,    "not --numbers",                        NULL },
        { "not-underline",  'U', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE,   &conf.underline,  "not --underline",                      NULL },
        { "not-color",      'C', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE,   &conf.color,      "not --color",                          NULL },
        { "not-radiobox",   'R', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE,   &conf.radiobox,   "not --radiobox",                       NULL },
        { "not-initial",    'I', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE,   &conf.initial,    "not --initial",                        NULL },
        { "not-whitelines", 'W', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE,   &conf.whitelines, "not --whitelines",                     NULL },
        { "not-fullattr",   'L', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE,   &conf.fullattr,   "not --fullattr",                       NULL },
        { "foreground",     'f', 0,                     G_OPTION_ARG_STRING, &conf.foreground, "foreground color to use to highlight", NULL },
        { "background",     'b', 0,                     G_OPTION_ARG_STRING, &conf.background, "background color to use to highlight", NULL },
        { NULL,              0,  0,                     0,                   NULL,             NULL,                                   NULL },
    };

    context = g_option_context_new("[FILES]");
    g_option_context_add_main_entries(context, entries, NULL);
    GError* err = NULL;
    if (!g_option_context_parse(context, &argc, &argv, &err))
        fatal("%s\n", err->message);
    g_option_context_free(context);
    conf.fg = get_curs_color(conf.foreground);
    conf.bg = get_curs_color(conf.background);

    conf.execpath   = g_strdup(argv[0]);
    conf.infiles    = argv + 1;
}
