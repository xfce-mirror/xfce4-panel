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

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
  Reading xml
-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
xmlDocPtr xmlconfig = NULL;

static xmlDocPtr make_empty_doc(void)
{
    xmlDocPtr doc;
    xmlNodePtr root;

    doc = xmlNewDoc("1.0");
    doc->children = xmlNewDocRawNode(doc, NULL, ROOT, NULL);

    root = (xmlNodePtr) doc->children;
    xmlDocSetRootElement(doc, root);

    xmlNewTextChild(root, NULL, "Panel", NULL);
    xmlNewTextChild(root, NULL, "Groups", NULL);

    return doc;
}


static xmlDocPtr read_xml_file(void)
{
    char *rcfile;
    xmlDocPtr doc = NULL;

    xmlKeepBlanksDefault(0);

    rcfile = get_read_file(XFCERC);

    if(rcfile)
        doc = xmlParseFile(rcfile);

    if(!doc)
        doc = make_empty_doc();

    g_free(rcfile);
    return doc;
}

/*  Configuration
 *  -------------
*/
static gboolean check_disable_user_config(void)
{
    const char *var = g_getenv("XFCE_DISABLE_USER_CONFIG");

    return (var && !strequal(var, "0"));
}

/* for now we still read this from the xml file until we have a new 
 * preferences system */
void get_global_prefs(void)
{
    xmlNodePtr node;

    /* global xmlDocPtr */
    if(!xmlconfig)
        xmlconfig = read_xml_file();

    node = xmlDocGetRootElement(xmlconfig);

    if(!node)
    {
        g_printerr("%s: %s (line %d): empty document\n", PACKAGE, __FILE__,
                   __LINE__);

        xmlFreeDoc(xmlconfig);
        xmlconfig = make_empty_doc();
        node = xmlDocGetRootElement(xmlconfig);
    }

    if(!xmlStrEqual(node->name, (const xmlChar *)ROOT))
    {
        g_printerr("%s: %s (line %d): wrong document type\n", PACKAGE, 
		   __FILE__, __LINE__);

        xmlFreeDoc(xmlconfig);
        xmlconfig = make_empty_doc();
        node = xmlDocGetRootElement(xmlconfig);
    }

    /* Now parse the xml tree */
    for(node = node->children; node; node = node->next)
    {
        if(xmlStrEqual(node->name, (const xmlChar *)"Panel"))
        {
            panel_parse_xml(node);

            break;
        }
    }

}

void get_panel_config(void)
{
    xmlNodePtr node;

    disable_user_config = check_disable_user_config();

    /* global xmlDocPtr */
    if(!xmlconfig)
        xmlconfig = read_xml_file();

    node = xmlDocGetRootElement(xmlconfig);

    if(!node)
    {
        g_printerr("%s: %s (line %d): empty document\n", PACKAGE, __FILE__,
                   __LINE__);

        xmlFreeDoc(xmlconfig);
        xmlconfig = make_empty_doc();
        node = xmlDocGetRootElement(xmlconfig);
    }

    if(!xmlStrEqual(node->name, (const xmlChar *)ROOT))
    {
        g_printerr("%s: %s (line %d): wrong document type\n",
                   PACKAGE, __FILE__, __LINE__);

        xmlFreeDoc(xmlconfig);
        xmlconfig = make_empty_doc();
        node = xmlDocGetRootElement(xmlconfig);
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
