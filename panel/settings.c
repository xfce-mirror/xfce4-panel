/*  settings.c
 *  
 *  Copyright (C) 2002 Jasper Huijsmans (j.b.huijsmans@hetnet.nl)
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "settings.h"

#include "xfce.h"
#include "xfce_support.h"
#include "central.h"
#include "side.h"
#include "popup.h"
#include "item.h"
#include "module.h"

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
    xmlNewTextChild(root, NULL, "Central", NULL);
    xmlNewTextChild(root, NULL, "Left", NULL);
    xmlNewTextChild(root, NULL, "Right", NULL);

    return doc;
}


static xmlDocPtr read_xml_file(void)
{
    char *rcfile;
    xmlDocPtr doc = NULL;

    xmlKeepBlanksDefault(0);

    rcfile = get_read_file(RCFILE);

    if(rcfile)
        doc = xmlParseFile(rcfile);

    if(!doc)
        doc = make_empty_doc();

    g_free(rcfile);
    return doc;
}

static void settings_from_file(void)
{
    xmlNodePtr node;

    /* global xmlDocPtr */
    xmlconfig = read_xml_file();

    node = xmlDocGetRootElement(xmlconfig);

    if(!node)
    {
        g_printerr(_("xfce: %s (line %d): empty document\n"), __FILE__, __LINE__);

        xmlFreeDoc(xmlconfig);
        xmlconfig = make_empty_doc();
        node = xmlDocGetRootElement(xmlconfig);
    }

    if(!xmlStrEqual(node->name, (const xmlChar *)ROOT))
    {
        g_printerr(_("xfce: %s (line %d): wrong document type\n"),
                   __FILE__, __LINE__);

        xmlFreeDoc(xmlconfig);
        xmlconfig = make_empty_doc();
        node = xmlDocGetRootElement(xmlconfig);
    }

    /* Now parse the xml tree */
    for(node = node->children; node; node = node->next)
    {
        if(xmlStrEqual(node->name, (const xmlChar *)"Panel"))
            panel_parse_xml(node);
        else if(xmlStrEqual(node->name, (const xmlChar *)"Central"))
            central_panel_parse_xml(node);
        else if(xmlStrEqual(node->name, (const xmlChar *)"Left"))
            side_panel_parse_xml(node, LEFT);
        else if(xmlStrEqual(node->name, (const xmlChar *)"Right"))
            side_panel_parse_xml(node, RIGHT);
    }

    xmlFreeDoc(xmlconfig);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
   Writing xml
-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

void settings_to_file(void)
{
    char *dir;
    char *rcfile;
    xmlNodePtr root;

    rcfile = get_save_file(RCFILE);

    if(g_file_test(rcfile, G_FILE_TEST_EXISTS))
        write_backup_file(rcfile);
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
    central_panel_write_xml(root);
    side_panel_write_xml(root, LEFT);
    side_panel_write_xml(root, RIGHT);

    xmlSaveFormatFile(rcfile, xmlconfig, 1);

    xmlFreeDoc(xmlconfig);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

   Configuration

-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/


static gboolean check_disable_user_config(void)
{
/*    char userconf[MAXSTRLEN + 1];

    snprintf(userconf, MAXSTRLEN, "%s/disable_user_config", SYSCONFDIR);
    
    return g_file_test(userconf, G_FILE_TEST_EXISTS);
*/
    const char *var = g_getenv("XFCE_DISABLE_USER_CONFIG");

    return (var && !strequal(var, "0"));
}

void get_panel_config(void)
{
    disable_user_config = check_disable_user_config();

    settings_from_file();
}

void write_panel_config(void)
{
    disable_user_config = check_disable_user_config();

    if(!disable_user_config)
        settings_to_file();
}
