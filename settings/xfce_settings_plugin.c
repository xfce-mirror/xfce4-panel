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

#include <gtk/gtk.h>
#include <libxml/tree.h>
#include <X11/Xlib.h>

#include <libxfce4mcs/mcs-common.h>
#include <libxfce4mcs/mcs-manager.h>
#include <libxfce4util/i18n.h>
#include <libxfce4util/util.h>
#include <xfce-mcs-manager/manager-plugin.h>

#include "xfce_settings.h"
#include "xfce_settings_plugin.h"
#include "xfce_settings_dialog.h"

#include "icons/xfce_icon.xpm"

#define DEFAULT_THEME "Curve"

#define strequal(s1,s2) !strcmp(s1, s2)

McsSetting xfce_options[XFCE_OPTIONS];

static McsManager *mcs_manager = NULL;

static void xfce_init_options(void);
static void xfce_create_channel(McsManager *sm);

McsPluginInitResult mcs_plugin_init(McsPlugin *mp)
{
#ifdef ENABLE_NLS
    /* This is required for UTF-8 at least - Please don't remove it */
    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);
#endif


    mcs_manager = mp->manager;

    xfce_create_channel(mp->manager);
    xfce_set_options(mp->manager);
    
    mp->plugin_name = g_strdup(CHANNEL);
    mp->caption = g_strdup(_("XFce Panel"));
    mp->run_dialog = run_xfce_settings_dialog;
    mp->icon = gdk_pixbuf_new_from_xpm_data((const char **)xfce_icon_xpm);

    return MCS_PLUGIN_INIT_OK;
}

static void xfce_create_channel(McsManager *sm)
{
    mcs_manager_add_channel(sm, CHANNEL);
}

static void xfce_init_options(void)
{
    int i;
    McsSetting *opt;

    for (i = 0; i < XFCE_OPTIONS; i++)
    {
	opt = &xfce_options[i];

	opt->channel_name = CHANNEL;
	opt->last_change_serial = 0;
		        
	switch (i)
	{
	case XFCE_ORIENTATION:
	    opt->name = "orientation";
	    opt->type = MCS_TYPE_INT;
	    opt->data.v_int =  0;
	    break;
	case XFCE_LAYER:
	    opt->name = "layer";
	    opt->type = MCS_TYPE_INT;
	    opt->data.v_int = 0;
	    break;
	case XFCE_SIZE:
	    opt->name = "size";
	    opt->type = MCS_TYPE_INT;
	    opt->data.v_int = 1;
	    break;
	case XFCE_POPUPPOSITION:
	    opt->name = "popupposition";
	    opt->type = MCS_TYPE_INT;
	    opt->data.v_int = 1;
	    break;
	case XFCE_THEME:
	    opt->name = "theme";
	    opt->type = MCS_TYPE_STRING;
	    opt->data.v_string = g_strdup(DEFAULT_THEME);
	    break;
	case XFCE_POSITION:
	    opt->name = "position";
	    opt->type = MCS_TYPE_INT;
	    opt->data.v_int = XFCE_POSITION_NONE;
	    mcs_manager_set_setting(mcs_manager, opt, CHANNEL);
	    break;
	default:
	    g_warning("xfce4 settings: unknown option id: %d\n", i);
	}
    }
}

#define XMLDATA(doc,node) xmlNodeListGetString(doc, node->children, 1)

static void xfce_parse_xml(xmlNodePtr node)
{
    xmlChar *value;
    int i;
    
    for (i = 0; i < XFCE_OPTIONS - 1; i++)
    {
	McsSetting *setting = &xfce_options[i];

	value = xmlGetProp(node, xfce_settings_names[i]);

	if (!value)
	    continue;
	
	if (setting->type == MCS_TYPE_INT)
	{
	    setting->data.v_int = atoi(value);
	}
	else if (setting->type == MCS_TYPE_STRING)
	{
	    g_free(setting->data.v_string);
	    setting->data.v_string = g_strdup(value);
	}

	g_free(value);

	mcs_manager_set_setting(mcs_manager, setting, CHANNEL);
    }
}

static char *get_read_config_file(void)
{
    char *path;

    path = xfce_get_userfile("settings", "xfce-settings.xml", NULL);

    if (g_file_test(path, G_FILE_TEST_EXISTS))
	return path;
    else
	g_free(path);

    path = g_build_filename(SYSCONFDIR, "xfce4", "settings",
		    "xfce-settings.xml", NULL);
    
    if (g_file_test(path, G_FILE_TEST_EXISTS))
	return(path);
    else
	g_free(path);
    
    return(NULL);
}

static void xfce_read_options(void)
{
    xmlDocPtr doc;
    xmlNodePtr node;
    char *path;

    xfce_init_options();
    
    path = get_read_config_file();

    if (!path)
	return;
    
    doc = xmlParseFile(path);
    g_free(path);
    
    node = xmlDocGetRootElement(doc);

    for(node = node->children; node; node = node->next)
    {
        if(xmlStrEqual(node->name, (const xmlChar *)"Settings"))
	{
	    xfce_parse_xml(node);

            break;
        }
    }

    xmlFreeDoc(doc);
}

void xfce_write_options(McsManager *sm)
{
    xmlDocPtr doc;
    xmlNodePtr node;
    char value[3];
    char *rcfile, *dir;
    int i;

    doc = xmlNewDoc("1.0");
    doc->children = xmlNewDocRawNode(doc, NULL, "XFce", NULL);

    node = (xmlNodePtr) doc->children;
    xmlDocSetRootElement(doc, node);

    node = xmlNewTextChild(node, NULL, "Settings", NULL);
    
    for (i = 0; i < XFCE_OPTIONS - 1; i++)
    {
	McsSetting *setting = &xfce_options[i];

	if (setting->type == MCS_TYPE_INT)
	{
	    snprintf(value, 3, "%d", setting->data.v_int);
	    xmlSetProp(node, xfce_settings_names[i], value);
	}
	else if (setting->type == MCS_TYPE_STRING)
	{
	    xmlSetProp(node, xfce_settings_names[i], setting->data.v_string);
	}
    }

    rcfile = g_build_filename(g_get_home_dir(), ".xfce4", "settings", 
	                      "xfce-settings.xml", NULL);

    dir = g_path_get_dirname(rcfile);

    if(!g_file_test(dir, G_FILE_TEST_IS_DIR))
    {
	char *cmd;

	/* FIXME: is this portable enough */
	cmd = g_strconcat("mkdir -p ", dir, NULL);
	system(cmd);
	g_free(cmd);
    }

    g_free(dir);

    xmlSaveFormatFile(rcfile, doc, 1);
    xmlFreeDoc(doc);
    g_free(rcfile);
}

void xfce_set_options(McsManager *sm)
{
    int i;
    static gboolean first = TRUE;
    
    if (first)
    {
	xfce_read_options();
	first = FALSE;
    }
            
    for (i = 0; i < XFCE_OPTIONS-1; i++)
    {
	mcs_manager_set_setting(sm, &xfce_options[i], CHANNEL);
    }

    mcs_manager_notify(sm, CHANNEL);
    xfce_write_options(sm);
}

/* macro defined in manager-plugin.h */
MCS_PLUGIN_CHECK_INIT
