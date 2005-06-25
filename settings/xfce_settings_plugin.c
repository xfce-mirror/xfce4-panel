/*  $Id$
 *
 *  Copyright 2002-2004 Jasper Huijsmans (jasper@xfce.org)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <sys/stat.h>

#include <stdlib.h>

#include <gtk/gtk.h>
#include <X11/Xlib.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfce4mcs/mcs-common.h>
#include <libxfce4mcs/mcs-manager.h>
#include <xfce-mcs-manager/manager-plugin.h>
#include <libxfcegui4/xfce-icontheme.h>

#include "xfce_settings.h"
#include "xfce_settings_plugin.h"
#include "xfce_settings_dialog.h"


#define DEFAULT_THEME   "Rodent"
#define STREQUAL(s1,s2) !strcmp(s1, s2)


static McsManager *mcs_manager = NULL;
static XfceIconTheme *icon_theme = NULL;
static int theme_cb = 0;

static void ensure_base_dir_spec (XfceResourceType type, 
                                  const char *old_subdir, 
                                  const char *old_file,
                                  const char *new_subdir, 
                                  const char *new_file);
static void xfce_set_options (McsManager * sm);
static void theme_changed (XfceIconTheme *theme, 
                           McsPlugin *mp);


McsPluginInitResult
mcs_plugin_init (McsPlugin * mp)
{
    xfce_textdomain (GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");

    ensure_base_dir_spec (XFCE_RESOURCE_CONFIG, 
                          "settings", "panel.xml",
                          "mcs_settings", "panel.xml");
    
    ensure_base_dir_spec (XFCE_RESOURCE_CONFIG, 
                          "settings", "xfce-settings.xml",
                          "mcs_settings", "xfce-settings.xml");
    
    mcs_manager = mp->manager;

    xfce_set_options (mp->manager);

    mp->plugin_name = g_strdup (CHANNEL);
    mp->caption = g_strdup (_("Xfce Panel"));
    mp->run_dialog = run_xfce_settings_dialog;

    icon_theme = xfce_icon_theme_get_for_screen (gdk_screen_get_default ());
    mp->icon = xfce_icon_theme_load (icon_theme, "xfce4-panel", 48);
    theme_cb = g_signal_connect (icon_theme, "changed", 
                                 G_CALLBACK (theme_changed), mp);

    return MCS_PLUGIN_INIT_OK;
}

/* Base Dir Spec compliance */
static void
ensure_base_dir_spec (XfceResourceType type, 
                      const char *old_subdir, const char *old_file,
                      const char *new_subdir, const char *new_file)
{
    char  *old, *new, *path;
    int c;
    FILE *r, *w;
    GError *error = NULL;

    new = g_build_filename ("xfce4", new_subdir, NULL);
    path = xfce_resource_save_location (type, new, FALSE);
    g_free (new);

    if (G_UNLIKELY (path == NULL))
      return;

    if (!xfce_mkdirhier (path, 0700, &error))
    {
        g_printerr ("%s\n", error->message);
        g_error_free (error);
        goto path_failed;
    }

    new = g_build_filename (path, new_file, NULL);
    
    if (g_file_test (new, G_FILE_TEST_EXISTS))
    {
        DBG ("New file exists: %s\n", new);
        goto new_exists;
    }
    
    old = g_build_filename (xfce_get_userdir (), old_subdir, old_file, NULL);
    
    if (!g_file_test (old, G_FILE_TEST_EXISTS))
    {
        DBG ("No old config file was found: %s\n", old);
        goto old_failed;
    }

    if (!(r = fopen (old, "r")))
    {
        g_printerr ("Could not open file for reading: %s\n", old);
        goto r_failed;
    }

    if (!(w = fopen (new, "w")))
    {
        g_printerr ("Could not open file for writing: %s\n", new);
        goto w_failed;
    }

    while ((c = getc (r)) != EOF)
        putc (c, w);

    fclose (w);
    
w_failed:
    fclose (r);

r_failed:

old_failed:
    g_free (old);

new_exists:
    g_free (new);
    
path_failed:
    g_free (path);
}

/* GMarkup parser for old style config file */
static void
old_xml_start_element (GMarkupParseContext * context,
		       const gchar * element_name,
		       const gchar ** attribute_names,
		       const gchar ** attribute_values,
		       gpointer user_data, GError ** error)
{
    int i, j;
    McsSetting opt;

    if (!STREQUAL (element_name, "Settings"))
	return;

    opt.channel_name = CHANNEL;
    opt.last_change_serial = 0;

    for (j = 0; attribute_names[j] != NULL; ++j)
    {
	opt.name = NULL;

	for (i = 0; i < XFCE_OPTIONS; ++i)
	{
	    if (STREQUAL (xfce_settings_names[i], attribute_names[j]))
	    {
		opt.name = xfce_settings_names[i];
		break;
	    }
	}

	if (!opt.name)
	    continue;

	switch (i)
	{
	    case XFCE_ORIENTATION:
		opt.type = MCS_TYPE_INT;
		break;
	    case XFCE_LAYER:
		opt.type = MCS_TYPE_INT;
		break;
	    case XFCE_SIZE:
		opt.type = MCS_TYPE_INT;
		break;
	    case XFCE_POPUPPOSITION:
		opt.type = MCS_TYPE_INT;
		break;
	    case XFCE_THEME:
		opt.type = MCS_TYPE_STRING;
		break;
	    case XFCE_AUTOHIDE:
		opt.type = MCS_TYPE_INT;
		break;
	}

	if (opt.type == MCS_TYPE_INT)
	{
	    opt.data.v_int = (int) strtol (attribute_values[j], NULL, 0);
	}
	else
	{
	    opt.data.v_string = (char *) attribute_values[j];
	}

	mcs_manager_set_setting (mcs_manager, &opt, CHANNEL);
    }
}

static void
old_xml_read_options (const char *path)
{
    GMarkupParser *parser;
    GMarkupParseContext *context;
    char *contents;
    gsize len;
    GError *error = NULL;

    if (!g_file_get_contents (path, &contents, &len, &error))
    {
	g_warning ("%s", error->message);
	g_error_free (error);
	return;
    }

    parser = g_new0 (GMarkupParser, 1);
    parser->start_element = old_xml_start_element;

    context = g_markup_parse_context_new (parser, 0, NULL, NULL);

    if (!g_markup_parse_context_parse (context, contents, len, &error))
    {
	g_warning ("%s", error->message);
	g_error_free (error);
    }

    g_markup_parse_context_free (context);
    g_free (parser);
}

void
xfce_write_options (McsManager * sm)
{
    char *file, *path;

    path = g_build_filename ("xfce4", "mcs_settings", "panel.xml", NULL);
    
    file = xfce_resource_save_location (XFCE_RESOURCE_CONFIG, path, TRUE);

    if (!file)
    {
	g_critical ("Couldn't create file %s", file);
    }
    else
    {
	mcs_manager_save_channel_to_file (sm, CHANNEL, file);
    }

    g_free (path);
    g_free (file);
}

/* option initialization */

static void
xfce_init_options (void)
{
    int i;
    McsSetting opt;
    McsSetting *setting;

    opt.channel_name = CHANNEL;
    opt.last_change_serial = 0;

    for (i = 0; i < XFCE_OPTIONS; i++)
    {
	opt.name = xfce_settings_names[i];

	setting = mcs_manager_setting_lookup (mcs_manager, opt.name, CHANNEL);

	if (setting)
	    continue;

	switch (i)
	{
	    case XFCE_ORIENTATION:
		opt.type = MCS_TYPE_INT;
		opt.data.v_int = 0;
		break;
	    case XFCE_LAYER:
		opt.type = MCS_TYPE_INT;
		opt.data.v_int = 0;
		break;
	    case XFCE_SIZE:
		opt.type = MCS_TYPE_INT;
		opt.data.v_int = 1;
		break;
	    case XFCE_POPUPPOSITION:
		opt.type = MCS_TYPE_INT;
		opt.data.v_int = 1;
		break;
	    case XFCE_THEME:
		opt.type = MCS_TYPE_STRING;
		opt.data.v_string = DEFAULT_THEME;
		break;
	    case XFCE_AUTOHIDE:
		opt.type = MCS_TYPE_INT;
		opt.data.v_int = 0;
		break;
	}

	mcs_manager_set_setting (mcs_manager, &opt, CHANNEL);
    }
}

static void
xfce_set_options (McsManager * sm)
{
    char *file, *newpath, *oldpath, **dirs, **d;
    gboolean channel_created = FALSE;

    dirs = xfce_resource_dirs (XFCE_RESOURCE_CONFIG);
    
    newpath = g_build_filename ("xfce4", "mcs_settings", "panel.xml", NULL);
    oldpath = g_build_filename ("xfce4", "mcs_settings", "xfce-settings.xml", 
                                NULL);
    
    for (d = dirs; *d != NULL; ++d)
    {
        file = g_build_filename (*d, newpath, NULL);
        
        if (g_file_test (file, G_FILE_TEST_EXISTS))
        {
            mcs_manager_add_channel_from_file (sm, CHANNEL, file);
            g_free (file);
            channel_created = TRUE;
            break;
        }
        
        g_free (file);
        
        file = g_build_filename (*d, oldpath, NULL);
        
        if (g_file_test (file, G_FILE_TEST_EXISTS))
        {
            DBG ("reading old style options");
            mcs_manager_add_channel (sm, CHANNEL);
            old_xml_read_options (file);

            channel_created = TRUE;
            g_free (file);
            break;
        }
        
        g_free (file);
    }
    
    g_strfreev (dirs);
    g_free (newpath);
    g_free (oldpath);

    if (!channel_created)
        mcs_manager_add_channel (sm, CHANNEL);
    
    /* set values if not already set */
    xfce_init_options ();

    mcs_manager_notify (sm, CHANNEL);
    xfce_write_options (sm);
}

static void
theme_changed (XfceIconTheme *icontheme, McsPlugin *mp)
{
    GdkPixbuf *pb;

    pb = xfce_icon_theme_load (icontheme, "xfce4-panel", 48);

    if (pb)
    {
        if (mp->icon)
            g_object_unref (mp->icon);

        mp->icon = pb;
    }
}

/* macro defined in manager-plugin.h */
MCS_PLUGIN_CHECK_INIT

G_MODULE_EXPORT void
g_module_unload (GModule *gm)
{
    g_signal_handler_disconnect (icon_theme, theme_cb);

    g_object_unref (icon_theme);
}
