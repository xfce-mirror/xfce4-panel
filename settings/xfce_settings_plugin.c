/*  xfce4
 *
 *  Copyright (C) 2002 Jasper Huijsmans <huysmans@users.sourceforge.net>
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
#include <libxfcegui4/icons.h>

#include "xfce_settings.h"
#include "xfce_settings_plugin.h"
#include "xfce_settings_dialog.h"

#define DEFAULT_THEME "Curve"

#define STREQUAL(s1,s2) !strcmp(s1, s2)

static McsManager *mcs_manager = NULL;

static void xfce_set_options (McsManager * sm);

McsPluginInitResult
mcs_plugin_init (McsPlugin * mp)
{
    xfce_textdomain(GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");

    mcs_manager = mp->manager;

    xfce_set_options (mp->manager);

    mp->plugin_name = g_strdup (CHANNEL);
    mp->caption = g_strdup (_("Xfce Panel"));
    mp->run_dialog = run_xfce_settings_dialog;
    mp->icon = xfce_themed_icon_load ("xfce4-panel", 48);

    return MCS_PLUGIN_INIT_OK;
}

/* GMarkup parser for old style config file */
static void
old_xml_start_element (GMarkupParseContext  *context,
                       const gchar          *element_name,
                       const gchar         **attribute_names,
                       const gchar         **attribute_values,
                       gpointer              user_data,
                       GError              **error)
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
	    opt.data.v_string = (char*)attribute_values[j];
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
    int len;
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
    char *file, *dir;
    
    /* this initializes base directory, but not settings dir */
    dir = xfce_get_userfile ("settings", NULL);
    file = g_build_filename (dir, "panel.xml", NULL);
    
    if (!g_file_test (dir, G_FILE_TEST_IS_DIR) 
	&& mkdir (dir, S_IRUSR | S_IWUSR | S_IXUSR) < 0)
    {
	g_critical ("Couldn't create directory %s", dir);
    }
    else
    {
	mcs_manager_save_channel_to_file (sm, CHANNEL, file);
    }

    g_free (dir);
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
    char *file;
    gboolean found = FALSE;

    file = xfce_get_userfile ("settings", "panel.xml", NULL);
    
    if (g_file_test (file, G_FILE_TEST_EXISTS))
    {
	mcs_manager_add_channel_from_file(sm, CHANNEL, file);
	
	found = TRUE;
    }
    else
    {
	g_free (file);

	file = xfce_get_userfile ("settings", "xfce-settings.xml", NULL);
    }

    if (!found && g_file_test (file, G_FILE_TEST_EXISTS))
    {
	mcs_manager_add_channel (sm, CHANNEL);

	DBG ("reading old style options");
	
	old_xml_read_options (file);

	found = TRUE;
    }
    else
    {
	g_free (file);

	file = g_build_filename (SYSCONFDIR, "xfce4", "panel.xml", NULL);
    }

    if (!found && g_file_test (file, G_FILE_TEST_EXISTS))
    {
	mcs_manager_add_channel_from_file(sm, CHANNEL, file);
	
	found = TRUE;
    }
    
    g_free (file);

    /* set values if not already set */
    xfce_init_options ();
    
    mcs_manager_notify (sm, CHANNEL);
    xfce_write_options (sm);
}

/* macro defined in manager-plugin.h */
MCS_PLUGIN_CHECK_INIT
