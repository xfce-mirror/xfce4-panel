/*  settings.c
 *  
 *  Copyright (C) 2002 Jasper Huijsmans (huysmans@users.sourceforge.net)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <config.h>
#include <my_gettext.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "xfce.h"
#include "settings.h"
#include "groups.h"
#include "popup.h"
#include "item.h"

#define ROOT	"Xfce"
#define NS	"http://www.xfce.org/xfce4/panel/1.0"

gboolean disable_user_config = FALSE;

xmlDocPtr xmlconfig = NULL;

/*  Configuration
 *  -------------
*/
static gboolean check_disable_user_config(void)
{
    const char *var = g_getenv("XFCE_DISABLE_USER_CONFIG");

    return (var && !strequal(var, "0"));
}

/*  Reading xml
 *  -----------
*/
static xmlDocPtr read_xml_file(void)
{
    char *rcfile;
    xmlDocPtr doc = NULL;

    xmlKeepBlanksDefault(0);

    rcfile = get_read_file(XFCERC);

    if(rcfile)
    {
        doc = xmlParseFile(rcfile);
	g_free(rcfile);
    }
    
    return doc;
}

static xmlNodePtr get_xml_root(void)
{
    xmlNodePtr node = NULL;
    
    /* global xmlDocPtr */
    if(!xmlconfig)
        xmlconfig = read_xml_file();

    if (!xmlconfig)
    {
	g_message("%s: No config file found", PACKAGE);
	return NULL;
    }

    node = xmlDocGetRootElement(xmlconfig);

    if(!node)
    {
        g_warning("%s: empty document: %s\n", PACKAGE, xmlconfig->name);

        xmlFreeDoc(xmlconfig);
	xmlconfig = NULL;
	return NULL;
    }

    if(!xmlStrEqual(node->name, (const xmlChar *)ROOT))
    {
        g_warning("%s: wrong document type: %s\n", PACKAGE, xmlconfig->name);

        xmlFreeDoc(xmlconfig);
	xmlconfig = NULL;
	return NULL;
    }

    return node;
}

void get_global_prefs(void)
{
    xmlNodePtr node;

    node = get_xml_root();

    if (!node)
	return;
    
    /* Now parse the xml tree */
    for(node = node->children; node; node = node->next)
    {
        if(xmlStrEqual(node->name, (const xmlChar *)"Panel"))
        {
            panel_parse_xml(node);

            break;
        }
    }

    /* leave the xmldoc open for get_panel_config() */
}

void get_panel_config(void)
{
    xmlNodePtr node;

    disable_user_config = check_disable_user_config();

    node = get_xml_root();

    if (!node)
    {
	/* create empty items */
	groups_set_from_xml(NULL);
	return;
    }
    
    /* Now parse the xml tree */
    for(node = node->children; node; node = node->next)
    {
	if (xmlStrEqual(node->name, (const xmlChar *)"Groups"))
	{
	    groups_set_from_xml(node);
	    break;
	}

	/* old format */
        if (xmlStrEqual(node->name, (const xmlChar *)"Left"))
            old_groups_set_from_xml(LEFT, node);
        else if (xmlStrEqual(node->name, (const xmlChar *)"Right"))
            old_groups_set_from_xml(RIGHT, node);
    }

    xmlFreeDoc(xmlconfig);
    xmlconfig = NULL;
}

void write_panel_config(void)
{
    char *dir;
    char *rcfile;
    xmlNodePtr root;
    static gboolean backup = TRUE;

    disable_user_config = check_disable_user_config();

    if(disable_user_config)
        return;

    rcfile = get_save_file(XFCERC);

    if(g_file_test(rcfile, G_FILE_TEST_EXISTS))
    {
	if (backup)
	{
	    write_backup_file(rcfile);
	    backup = FALSE;
	}
	
    }
    else
    {
        dir = g_path_get_dirname(rcfile);

        if(!g_file_test(dir, G_FILE_TEST_IS_DIR))
            mkdir(dir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

        g_free(dir);
    }

    xmlconfig = xmlNewDoc("1.0");
    xmlconfig->children = xmlNewDocRawNode(xmlconfig, NULL, ROOT, NULL);

    root = (xmlNodePtr) xmlconfig->children;
    xmlDocSetRootElement(xmlconfig, root);

    panel_write_xml(root);
    
    groups_write_xml(root);

    xmlSaveFormatFile(rcfile, xmlconfig, 1);

    xmlFreeDoc(xmlconfig);
    xmlconfig = NULL;
}
