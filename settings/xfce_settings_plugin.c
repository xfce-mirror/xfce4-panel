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

#include <stdlib.h>

#include <gtk/gtk.h>
#include <X11/Xlib.h>

#include <libxfce4mcs/mcs-common.h>
#include <libxfce4mcs/mcs-manager.h>
#include <libxfce4util/i18n.h>
#include <libxfce4util/util.h>
#include <libxfce4util/debug.h>
#include <xfce-mcs-manager/manager-plugin.h>
#include <libxfcegui4/icons.h>

#include "xfce_settings.h"
#include "xfce_settings_plugin.h"
#include "xfce_settings_dialog.h"

#include "icons/xfce4-panel-icon.h"

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
    mp->caption = g_strdup (_("XFce Panel"));
    mp->run_dialog = run_xfce_settings_dialog;
    mp->icon = xfce_inline_icon_at_size (panel_icon_data, 48, 48);

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

#if 0
#define XMLDATA(doc,node) xmlNodeListGetString(doc, node->children, 1)

static void
old_xfce_parse_xml (xmlNodePtr node)
{
    McsSetting opt;
    xmlChar *value;
    int i;

    opt.channel_name = CHANNEL;
    opt.last_change_serial = 0;

    for (i = 0; i < XFCE_OPTIONS; i++)
    {
	opt.name = xfce_settings_names[i];

	value = xmlGetProp (node, opt.name);

	if (!value)
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
	    opt.data.v_int = (int) strtol (value, NULL, 0);
	}
	else
	{
	    opt.data.v_string = (char *)value;
	}

/*	setting->type == MCS_TYPE_INT ? 
	    g_print ("parse xml: %s = %d\n", 
		     setting->name, setting->data.v_int) :
	    g_print ("parse xml: %s = %s\n", 
		     setting->name, setting->data.v_string);
*/
	mcs_manager_set_setting (mcs_manager, &opt, CHANNEL);

	g_free (value);
    }
}

static void
old_xfce_read_options (const char *path)
{
    xmlDocPtr doc;
    xmlNodePtr node;

    if (!path)
	return;

    doc = xmlParseFile (path);

    node = xmlDocGetRootElement (doc);

    for (node = node->children; node; node = node->next)
    {
	if (xmlStrEqual (node->name, (const xmlChar *) "Settings"))
	{
	    old_xfce_parse_xml (node);

	    break;
	}
    }

    xmlFreeDoc (doc);
}
#endif

void
xfce_write_options (McsManager * sm)
{
    char *file;
    
    file = xfce_get_userfile ("settings", "panel.xml", NULL);
	
    mcs_manager_save_channel_to_file (sm, CHANNEL, file);

    g_free (file);

#if 0
    xmlDocPtr doc;
    xmlNodePtr node;
    char value[3];
    char *rcfile, *dir;
    int i;

    doc = xmlNewDoc ("1.0");
    doc->children = xmlNewDocRawNode (doc, NULL, "XFce", NULL);

    node = (xmlNodePtr) doc->children;
    xmlDocSetRootElement (doc, node);

    node = xmlNewTextChild (node, NULL, "Settings", NULL);

    for (i = 0; i < XFCE_OPTIONS; i++)
    {
	McsSetting *setting = &xfce_options[i];

	if (setting->type == MCS_TYPE_INT)
	{
	    snprintf (value, 3, "%d", setting->data.v_int);
	    xmlSetProp (node, xfce_settings_names[i], value);
	}
	else if (setting->type == MCS_TYPE_STRING)
	{
	    xmlSetProp (node, xfce_settings_names[i], setting->data.v_string);
	}
    }

    rcfile = g_build_filename (g_get_home_dir (), ".xfce4", "settings",
			       "xfce-settings.xml", NULL);

    dir = g_path_get_dirname (rcfile);

    if (!g_file_test (dir, G_FILE_TEST_IS_DIR))
    {
	char *cmd;

	/* FIXME: is this portable enough */
	cmd = g_strconcat ("mkdir -p ", dir, NULL);
	system (cmd);
	g_free (cmd);
    }

    g_free (dir);

    xmlSaveFormatFile (rcfile, doc, 1);
    xmlFreeDoc (doc);
    g_free (rcfile);
#endif
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
