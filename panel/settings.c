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
#include "central.h"
#include "side.h"
#include "popup.h"
#include "item.h"
#include "module.h"

#define ROOT	"Xfce"
#define NS	"http://www.xfce.org/xfce4/panel/1.0"

gboolean disable_user_config = FALSE;

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

   File handling

-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

static void write_backup_file(const char *path)
{
    FILE *fp;
    FILE *bakfp;
    char bakfile[MAXSTRLEN + 1];

    snprintf(bakfile, MAXSTRLEN, "%s.bak", path);

    if((bakfp = fopen(bakfile, "w")) && (fp = fopen(path, "r")))
    {
        char c;

        while((c = fgetc(fp)) != EOF)
            putc(c, bakfp);

        fclose(fp);
        fclose(bakfp);
    }
}

static char *localized_rcfile(void)
{
    const char *locale;
    const char *home;
    char base_locale[3];
    char rcfile[MAXSTRLEN + 1];
    char sysrcfile[MAXSTRLEN + 1];
    char file[MAXSTRLEN + 1];

    home = g_getenv("HOME");

    if(!(locale = g_getenv("LC_MESSAGES")))
        locale = g_getenv("LANG");

    if(locale)
    {
        base_locale[0] = locale[0];
        base_locale[1] = locale[1];
        base_locale[3] = '\0';
    }

    snprintf(rcfile, MAXSTRLEN, "%s/%s/%s", home, RCDIR, RCFILE);
    snprintf(sysrcfile, MAXSTRLEN, "%s/xfce4/%s", SYSCONFDIR, RCFILE);

    if(!disable_user_config && g_file_test(rcfile, G_FILE_TEST_EXISTS))
        return g_strdup(rcfile);
    else
    {
        if(!locale)
        {
            if(g_file_test(sysrcfile, G_FILE_TEST_EXISTS))
                return g_strdup(sysrcfile);
            else
                return NULL;
        }
        else
        {
            snprintf(file, MAXSTRLEN, "%s.%s", sysrcfile, locale);

            if(g_file_test(file, G_FILE_TEST_EXISTS))
                return g_strdup(file);

            snprintf(file, MAXSTRLEN, "%s.%s", sysrcfile, base_locale);

            if(g_file_test(file, G_FILE_TEST_EXISTS))
                return g_strdup(file);
            else if(g_file_test(sysrcfile, G_FILE_TEST_EXISTS))
                return g_strdup(sysrcfile);
            else
                return NULL;
        }
    }
}

static char *home_rcfile(void)
{
    const char *home;
    char rcfile[MAXSTRLEN + 1];

    home = g_getenv("HOME");
    snprintf(rcfile, MAXSTRLEN, "%s/%s/%s", home, RCDIR, RCFILE);

    return g_strdup(rcfile);
}

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*

   Reading xml

-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/


xmlDocPtr xmlconfig = NULL;

static xmlDocPtr read_xml_file(void)
{
    char *rcfile;
    xmlDocPtr doc;

    xmlKeepBlanksDefault(0);

    rcfile = localized_rcfile();
    
    if (rcfile)
	doc = xmlParseFile(rcfile);
    else
    {
	xmlNodePtr root;
	
	doc = xmlNewDoc("1.0");
	doc->children = xmlNewDocRawNode(xmlconfig, NULL, ROOT, NULL);

	root = (xmlNodePtr) doc->children;
	xmlDocSetRootElement(doc, root);
	
	xmlNewTextChild(root, NULL, "Panel", NULL);
	xmlNewTextChild(root, NULL, "Central", NULL);
	xmlNewTextChild(root, NULL, "Left", NULL);
	xmlNewTextChild(root, NULL, "Right", NULL);
    }

    g_free(rcfile);
    return doc;
}

static void settings_from_file(void)
{
    xmlNodePtr node;

    /* global xmlDocPtr */
    xmlconfig = read_xml_file();

    if(!xmlconfig)
        return;

    node = xmlDocGetRootElement(xmlconfig);

    if(!node)
    {
        g_printerr(_("xfce: %s (line %d): empty document\n"), __FILE__, __LINE__);
        return;
    }

    if(!xmlStrEqual(node->name, (const xmlChar *)ROOT))
    {
        g_printerr(_("xfce: %s (line %d): wrong document type\n"),
                   __FILE__, __LINE__);
        return;
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

    rcfile = home_rcfile();

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
    
    return (var && !strequal(var,"0"));
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
