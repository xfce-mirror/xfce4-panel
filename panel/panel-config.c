/* $Id$
 *
 * Copyright (c) 2005 Jasper Huijsmans <jasper@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <gdk/gdkx.h>
#include <gtk/gtkenums.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/xfce-panel-macros.h>
#include <libxfce4panel/xfce-panel-convenience.h>
#include <libxfce4panel/xfce-panel-item-iface.h>

#include "panel-config.h"
#include "panel-private.h"
#include "panel.h"
#include "panel-properties.h"
#include "panel-app.h"

#ifndef _
#define _(x) x
#endif

#define PANEL_LAUNCHER  "launcher"

static GPtrArray *config_parse_file (const gchar *filename);

static gboolean config_save_to_file (GPtrArray *panels, const gchar *filename);


/* fallback panel */

static GPtrArray *
create_fallback_panel_array (void)
{
    GPtrArray *array;
    Panel     *panel;

    DBG ("No suitable panel configuration was found.");

    panel = panel_new ();
    g_object_ref (G_OBJECT (panel));
    gtk_object_sink (GTK_OBJECT (panel));

    panel_add_item (panel, PANEL_LAUNCHER);

    array = g_ptr_array_new ();

    g_ptr_array_add (array, panel);

    return array;
}

/* public API */

GPtrArray *
panel_config_create_panels (void)
{
    gchar       *file;
    GPtrArray   *array = NULL;
    const gchar *path  = "xfce4" G_DIR_SEPARATOR_S
                         "panel" G_DIR_SEPARATOR_S
                         "panels.xml";

    if (G_UNLIKELY (!xfce_allow_panel_customization ()))
    {
        file = g_build_filename (SYSCONFDIR, "xdg", path, NULL);

        if (!g_file_test (file, G_FILE_TEST_IS_REGULAR))
        {
            g_free (file);

            file = NULL;
        }
    }
    else
    {
        file = xfce_resource_lookup (XFCE_RESOURCE_CONFIG, path);

        if (G_UNLIKELY (!file))
        {
            file = g_build_filename (SYSCONFDIR, path, NULL);

            if (!g_file_test (file, G_FILE_TEST_IS_REGULAR))
            {
                g_free (file);

                file = NULL;
            }
        }
    }

    if (G_LIKELY (file))
    {
        array = config_parse_file (file);
        g_free (file);
    }

    if (!array)
        array = create_fallback_panel_array ();

    DBG ("Successfully configured %d panel(s).", array->len);

    return array;
}

gboolean
panel_config_save_panels (GPtrArray * panels)
{
    gchar       *file;
    gboolean     failed = FALSE;
    gint         i;
    const gchar *path = "xfce4" G_DIR_SEPARATOR_S
                        "panel" G_DIR_SEPARATOR_S
                        "panels.xml";

    if (xfce_allow_panel_customization ())
    {
        file = xfce_resource_save_location (XFCE_RESOURCE_CONFIG, path, TRUE);

        failed = !config_save_to_file (panels, file);

        g_free (file);

        for (i = 0; i < panels->len; ++i)
        {
            Panel *panel = g_ptr_array_index (panels, i);

            panel_save_items (panel);
        }
    }

    return !failed;
}

/* GMarkup parsing */

typedef enum
{
    START,
    PANELS,
    PANEL,
    PROPERTIES,
    ITEMS,
    UNKNOWN
}
ParserState;

typedef struct _ConfigParser ConfigParser;
struct _ConfigParser
{
    GPtrArray   *panels;
    Panel       *current_panel;
    ParserState  state;

    guint        properties_set : 1;
    guint        monitor_set : 1;

    /* properties */
    gint         size;
    gint         monitor;
    gint         screen_position;
    gint         full_width;
    gint         xoffset;
    gint         yoffset;
    gint         handle_style;
    guint        autohide : 1;
    gint         transparency;
    guint        activetrans : 1;
};

static void
init_properties (ConfigParser *parser)
{
    parser->properties_set  = FALSE;
    parser->monitor_set     = FALSE;

    parser->size            = DEFAULT_SIZE;
    parser->monitor         = 0;
    parser->screen_position = DEFAULT_SCREEN_POSITION;
    parser->full_width      = XFCE_PANEL_NORMAL_WIDTH;
    parser->xoffset         = 0;
    parser->yoffset         = 0;
    parser->handle_style    = XFCE_HANDLE_STYLE_NONE;
    parser->autohide        = DEFAULT_AUTOHIDE;
    parser->transparency    = DEFAULT_TRANSPARENCY;
    parser->activetrans     = DEFAULT_ACTIVE_TRANS;
}

static void
config_set_property (ConfigParser *parser,
                     const gchar  *name,
                     const gchar  *value)
{
    g_return_if_fail (name != NULL && value != NULL);

    parser->properties_set = TRUE;

    if (strcmp (name, "size") == 0)
    {
        parser->size = (gint) strtol (value, NULL, 0);
    }
    else if (strcmp (name, "monitor") == 0)
    {
        parser->monitor = (gint) strtol (value, NULL, 0);
        parser->monitor_set = TRUE;
    }
    else if (strcmp (name, "screen-position") == 0)
    {
        parser->screen_position = (gint) strtol (value, NULL, 0);
    }
    else if (strcmp (name, "fullwidth") == 0)
    {
        parser->full_width = ((gint) strtol (value, NULL, 0));
    }
    else if (strcmp (name, "xoffset") == 0)
    {
        parser->xoffset = (gint) strtol (value, NULL, 0);
    }
    else if (strcmp (name, "yoffset") == 0)
    {
        parser->yoffset = (gint) strtol (value, NULL, 0);
    }
    else if (strcmp (name, "handlestyle") == 0)
    {
        parser->handle_style = (gint) strtol (value, NULL, 0);
    }
    else if (strcmp (name, "autohide") == 0)
    {
        parser->autohide = ((gint) strtol (value, NULL, 0) == 1);
    }
    else if (strcmp (name, "transparency") == 0)
    {
        parser->transparency = (gint) strtol (value, NULL, 0);
    }
    else if (strcmp (name, "activetrans") == 0)
    {
        parser->activetrans = ((gint) strtol (value, NULL, 0) == 1);
    }
}

static void
start_element_handler (GMarkupParseContext  *context,
                       const gchar          *element_name,
                       const gchar         **attribute_names,
                       const gchar         **attribute_values,
                       gpointer              user_data,
                       GError              **error)
{
    ConfigParser *parser = user_data;
    gchar        *name = NULL;
    gchar        *value = NULL;
    gint          i = 0;

    switch (parser->state)
    {
        case START:
            if (strcmp (element_name, "panels") == 0)
            {
                parser->state = PANELS;
            }
            break;

        case PANELS:
            if (strcmp (element_name, "panel") == 0)
            {
                parser->state = PANEL;
                parser->current_panel = panel_new ();
                g_ptr_array_add (parser->panels, parser->current_panel);
                init_properties (parser);
                TIMER_ELAPSED(" + start config panel");
            }
            break;

        case PANEL:
            if (strcmp (element_name, "properties") == 0)
            {
                TIMER_ELAPSED(" ++ start properties");
                parser->state = PROPERTIES;
            }
            else if (strcmp (element_name, "items") == 0)
            {
                TIMER_ELAPSED(" ++ start items");
                parser->state = ITEMS;
            }
            break;

        case PROPERTIES:
            if (strcmp (element_name, "property") == 0)
            {
                while (attribute_names[i] != NULL)
                {
                    if (strcmp (attribute_names[i], "name") == 0)
                    {
                        name = (char *) attribute_values[i];
                    }
                    else if (strcmp (attribute_names[i], "value") == 0)
                    {
                        value = (char *) attribute_values[i];
                    }
                    ++i;
                }

                if (name != NULL && value != NULL)
                {
                    config_set_property (parser, name, value);
                }
                else
                {
                    g_warning ("Property name or value not defined");
                }
            }
            break;

        case ITEMS:
            if (strcmp (element_name, "item") == 0)
            {
                while (attribute_names[i] != NULL)
                {
                    if (strcmp (attribute_names[i], "name") == 0)
                    {
                        name = (char *) attribute_values[i];
                    }
                    else if (strcmp (attribute_names[i], "id") == 0)
                    {
                        value = (char *) attribute_values[i];
                    }
                    ++i;
                }

                if (name != NULL && value != NULL)
                {
                    DBG ("Add item: name=\"%s\", id=\"%s\"", name, value);

                    TIMER_ELAPSED(" +++ add item: %s", name);
                    panel_add_item_with_id (parser->current_panel,
                                                name, value);
                }
                else
                {
                    g_warning ("No item name found");
                }
            }
            break;

        default:
            g_warning ("start unknown element \"%s\"", element_name);
            break;
    }
}

static void
end_element_handler (GMarkupParseContext *context,
                     const gchar          *element_name,
                     gpointer              user_data,
                     GError              **error)
{
    ConfigParser *parser = user_data;

    switch (parser->state)
    {
        case START:
            g_warning ("end unexpected element: \"%s\"", element_name);
            break;

        case PANELS:
            if (strcmp ("panels", element_name) == 0)
                parser->state = START;
            break;

        case PANEL:
            if (strcmp ("panel", element_name) == 0)
            {
                parser->state = PANELS;
                parser->current_panel = NULL;
                TIMER_ELAPSED(" + end config panel");
            }
            break;

        case PROPERTIES:
            if (strcmp ("properties", element_name) == 0)
            {
                parser->state = PANEL;
                if (parser->properties_set)
                {
                    if (parser->screen_position == XFCE_SCREEN_POSITION_NONE)
                        parser->screen_position =
                            XFCE_SCREEN_POSITION_FLOATING_H;

                    if (!parser->monitor_set)
                    {
                        parser->monitor = DefaultScreen (GDK_DISPLAY());
                    }


                    g_object_set (G_OBJECT (parser->current_panel),
                                  "size",            parser->size,
                                  "monitor",         parser->monitor,
                                  "screen-position", parser->screen_position,
                                  "fullwidth",       parser->full_width,
                                  "xoffset",         parser->xoffset,
                                  "yoffset",         parser->yoffset,
                                  "handle-style",    parser->handle_style,
                                  "autohide",        parser->autohide,
                                  "transparency",    parser->transparency,
                                  "activetrans",     parser->activetrans,
                                  NULL);
                }
                TIMER_ELAPSED(" ++ end properties");
            }
            break;

        case ITEMS:
            if (strcmp ("items", element_name) == 0)
            {
                parser->state = PANEL;
                TIMER_ELAPSED(" ++ end items");
            }
            break;

        default:
            g_warning ("end unknown element \"%s\"", element_name);
            break;
    }
}

static GMarkupParser markup_parser = {
    start_element_handler,
    end_element_handler,
    NULL,
    NULL,
    NULL
};

static GPtrArray *
config_parse_file (const gchar *filename)
{
    GPtrArray           *array = NULL;
    gchar               *contents;
    GError              *error;
    GMarkupParseContext *context;
    ConfigParser         parser;
    struct stat          sb;
    size_t               bytes;
    gint                 fd, rc;
#ifdef HAVE_MMAP
    void                *addr;
#endif

    g_return_val_if_fail (filename != NULL && strlen (filename) > 0, NULL);

    if (stat (filename, &sb) < 0)
        return NULL;

    if ((fd = open (filename, O_RDONLY, 0)) < 0)
    {
        g_critical ("Unable to open file %s to load configuration: %s",
                    filename, g_strerror (errno));
        return NULL;
    }

    contents = NULL;

#ifdef HAVE_MMAP
    /* Try to mmap(2) the config file, as this save us a lot of
     * kernelspace -> userspace copying
     */

#ifdef MAP_FILE
    addr = mmap (NULL, sb.st_size, PROT_READ, MAP_FILE | MAP_PRIVATE, fd, 0);
#else
    addr = mmap (NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
#endif

    if (addr != NULL)
    {
        /* nice, mmap did the job */
        contents = addr;
    }
    else
    {
        g_warning ("Failed to mmap file %s to load data: %s. "
                   "Using read fallback.", filename, g_strerror (errno));
    }
#endif /* HAVE_MMAP */

    if (contents == NULL)
    {
            contents = panel_slice_alloc ((size_t) sb.st_size);

        if (contents == NULL)
        {
            g_critical ("Unable to allocate %lu bytes of memory to load "
                        "contents of file %s: %s",
                        (gulong) sb.st_size, filename, g_strerror (errno));
            goto finished;
        }

        for (bytes = 0; bytes < (size_t) sb.st_size;)
        {
            errno = 0;
            rc = read (fd, contents + bytes, sb.st_size - bytes);

            if (rc < 0)
            {
                if (errno == EINTR || errno == EAGAIN)
                    continue;

                g_critical ("Unable to read contents from file %s: %s",
                            filename, g_strerror (errno));
                goto finished;
            }
            else if (rc == 0)
            {
                g_critical ("Unexpected end of file reading contents from "
                            "file %s: %s", filename, g_strerror (errno));
            }

            bytes += rc;
        }
    }

    /* parse the file */
    error = NULL;

    parser.state = START;
    parser.panels = array = g_ptr_array_new ();
    parser.current_panel = NULL;

    context = g_markup_parse_context_new (&markup_parser, 0, &parser, NULL);

    if (!g_markup_parse_context_parse (context, contents, sb.st_size, &error)
        || !g_markup_parse_context_end_parse (context, &error))
    {
        g_critical ("Unable to parse file %s: %s",
                    filename, error->message);
        g_error_free (error);
    }

    g_markup_parse_context_free (context);

finished:
#ifdef HAVE_MMAP
    if (addr != NULL)
    {
        if (munmap (addr, sb.st_size))
        {
            g_critical ("Unable to unmap file %s: %s. This should not happen!",
                        filename, g_strerror (errno));
        }

        contents = NULL;
    }
#endif

    if (contents != NULL)
        panel_slice_free1 ((size_t) sb.st_size, contents);

    if (close (fd) < 0)
    {
        g_critical ("Failed to close file %s: %s", filename,
                    g_strerror (errno));
    }

    if (array && array->len == 0)
    {
        g_ptr_array_free (array, TRUE);
        array = NULL;
    }

    return array;
}

gboolean
config_save_to_file (GPtrArray   *array,
                     const gchar *filename)
{
    FILE                *fp;
    gchar                tmp_path[PATH_MAX];
    gint                 i;

    g_return_val_if_fail (array != NULL, FALSE);
    g_return_val_if_fail (filename != NULL || (strlen (filename) > 0), FALSE);

    DBG ("Save configuration of %d panels.", array->len);

    g_snprintf (tmp_path, PATH_MAX, "%s.tmp", filename);

    fp = fopen (tmp_path, "w");
    if (fp == NULL)
    {
        g_critical ("Unable to open file %s: %s",
                    tmp_path, g_strerror (errno));
        return FALSE;
    }

    /* Write header */
    fprintf (fp, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                 "<!DOCTYPE config SYSTEM \"config.dtd\">\n"
                 "<panels>\n");

    for (i = 0; i < array->len; ++i)
    {
        gint            size            = 0;
        gint            monitor         = 0;
        gint            screen_position = 0;
        gint            fullwidth       = 0;
        gint            xoffset         = 0;
        gint            yoffset         = 0;
        gint            handle_style    = 0;
        gint            transparency    = 0;
        gboolean        autohide        = FALSE;
        gboolean        activetrans     = FALSE;
        Panel          *panel;
        GList          *configlist, *l;
        XfcePanelItem  *item;

        DBG ("Saving panel %d", i + 1);

        panel = g_ptr_array_index (array, i);

        g_object_get (G_OBJECT (panel),
                      "size",            &size,
                      "monitor",         &monitor,
                      "screen-position", &screen_position,
                      "fullwidth",       &fullwidth,
                      "xoffset",         &xoffset,
                      "yoffset",         &yoffset,
                      "handle-style",    &handle_style,
                      "autohide",        &autohide,
                      "transparency",    &transparency,
                      "activetrans",     &activetrans,
                      NULL);

        /* grouping */
        fprintf (fp, "\t<panel>\n"
                     "\t\t<properties>\n");

        /* properties */
        fprintf (fp, "\t\t\t<property name=\"size\" value=\"%d\"/>\n",
                     size);

        fprintf (fp, "\t\t\t<property name=\"monitor\" value=\"%d\"/>\n",
                     monitor);

        fprintf (fp, "\t\t\t<property name=\"screen-position\" "
                     "value=\"%d\"/>\n", screen_position);

        fprintf (fp, "\t\t\t<property name=\"fullwidth\" value=\"%d\"/>\n",
                     fullwidth);

        fprintf (fp, "\t\t\t<property name=\"xoffset\" value=\"%d\"/>\n",
                     xoffset);

        fprintf (fp, "\t\t\t<property name=\"yoffset\" value=\"%d\"/>\n",
                     yoffset);

        fprintf (fp, "\t\t\t<property name=\"handlestyle\" value=\"%d\"/>\n",
                     handle_style);

        fprintf (fp, "\t\t\t<property name=\"autohide\" value=\"%d\"/>\n",
                     autohide);

        fprintf (fp, "\t\t\t<property name=\"transparency\" value=\"%d\"/>\n",
                     transparency);

        fprintf (fp, "\t\t\t<property name=\"activetrans\" value=\"%d\"/>\n",
                     activetrans);

        /* grouping */
        fprintf (fp, "\t\t</properties>\n"
                     "\t\t<items>\n");

        /* panel items */
        configlist = panel_get_item_list (panel);

        for (l = configlist; l != NULL; l = l->next)
        {
            item = l->data;

            fprintf (fp, "\t\t\t<item name=\"%s\" id=\"%s\"/>\n",
                     xfce_panel_item_get_name (item),
                     xfce_panel_item_get_id (item));
        }

        g_list_free (configlist);

        /* grouping */
        fprintf (fp, "\t\t</items>\n"
                     "\t</panel>\n");

    }

    /* closing */
    fprintf (fp, "</panels>\n");

    if (fclose (fp) == EOF)
    {
        g_critical ("Unable to close file handle for %s: %s", tmp_path,
                    g_strerror (errno));
        unlink (tmp_path);
        return FALSE;
    }

    if (rename (tmp_path, filename) < 0)
    {
        g_critical ("Unable to rename file %s to %s: %s", tmp_path, filename,
                    g_strerror (errno));
        return FALSE;
    }

    return TRUE;
}
