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
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <gmodule.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include <libxfce4util/libxfce4util.h>

#include "xfce.h"
#include "settings.h"
#include "item-control.h"
#include "item.h"

#define ROOT	"Xfce"
#define NS	"http://www.xfce.org/xfce4/panel/1.0"

#define XML_FORMAT_VERSION  2

G_MODULE_EXPORT /* EXPORT:disable_user_config */
gboolean disable_user_config = FALSE;

G_MODULE_EXPORT /* EXPORT:xmlconfig */
xmlDocPtr xmlconfig = NULL;

/*  Configuration
 *  -------------
*/
static gboolean
check_disable_user_config (void)
{
    const char *var;
    XfceKiosk *kiosk;
    gboolean result;

    var = g_getenv ("XFCE_DISABLE_USER_CONFIG");
    
    kiosk = xfce_kiosk_new ("xfce4-panel");
    result = xfce_kiosk_query (kiosk, "CustomizePanel");
    xfce_kiosk_free (kiosk);

    if (var != NULL)
    {
	g_warning ("Deprecated XFCE_DISABLE_USER_CONFIG environment variable "
		   "found. Please use the new KIOSK mode instead.");
    }

    if (!result)
	return TRUE;

    return (var && !strequal (var, "0"));
}

/*  File locations
 *  --------------
*/
static char *
get_save_file (const char * name)
{
    int scr;
    char *path, *file = NULL;

    scr = DefaultScreen (gdk_display);

    if (scr == 0)
    {
        path = g_build_filename ("xfce4", "panel", name, NULL);
    }
    else
    {
	char *realname;

	realname = g_strdup_printf ("%s.%u", name, scr);

        path = g_build_filename ("xfce4", "panel", realname, NULL);
        
	g_free (realname);
    }

    file = xfce_resource_save_location (XFCE_RESOURCE_CONFIG, path, TRUE);
    g_free (path);
    
    return file;
}

static char *
get_read_file (const char * name)
{
    char *file = NULL;

    if (G_UNLIKELY (disable_user_config))
    {
	file = g_build_filename (SYSCONFDIR, "xdg", "xfce4", "panel", 
                                 name, NULL);

        if (!g_file_test (file, G_FILE_TEST_IS_REGULAR))
        {
            g_free (file);
            
            file = NULL;
        }
    }
    else
    {
        char *path = g_build_filename ("xfce4", "panel", name, NULL);
        
        file = xfce_resource_lookup (XFCE_RESOURCE_CONFIG, path);

        g_free (path);
    }
    
    return file;
}

static void
write_backup_file (const char * path)
{
    FILE *fp;
    FILE *bakfp;
    char bakfile[MAXSTRLEN + 1];
    int c;

    snprintf (bakfile, MAXSTRLEN, "%s.bak", path);

    if (!(bakfp = fopen (bakfile, "w")))
        return;
    
    if (!(fp = fopen (path, "r")))
    {
        fclose (bakfp);
        return;
    }
    
    while ((c = getc (fp)) != EOF)
        putc (c, bakfp);

    fclose (fp);
    fclose (bakfp);
}


/*  Reading xml
 *  -----------
*/
static xmlDocPtr
read_xml_file (void)
{
    char *rcfile;
    xmlDocPtr doc = NULL;

    disable_user_config = check_disable_user_config ();

    xmlKeepBlanksDefault (0);

    rcfile = get_read_file ("contents.xml");

    if (rcfile)
    {
	doc = xmlParseFile (rcfile);
	g_free (rcfile);
    }

    return doc;
}

static xmlNodePtr
get_xml_root (void)
{
    xmlNodePtr node = NULL;

    /* global xmlDocPtr */
    if (!xmlconfig)
	xmlconfig = read_xml_file ();

    if (!xmlconfig)
    {
	g_message ("%s: No config file found", PACKAGE);
	return NULL;
    }

    node = xmlDocGetRootElement (xmlconfig);

    if (!node)
    {
	g_warning ("%s: empty document: %s\n", PACKAGE, xmlconfig->name);

	xmlFreeDoc (xmlconfig);
	xmlconfig = NULL;
	return NULL;
    }

    if (!xmlStrEqual (node->name, (const xmlChar *) ROOT))
    {
	g_warning ("%s: wrong document type: %s\n", PACKAGE, xmlconfig->name);

	xmlFreeDoc (xmlconfig);
	xmlconfig = NULL;
	return NULL;
    }

    return node;
}

G_MODULE_EXPORT /* EXPORT:get_global_prefs */
void
get_global_prefs (void)
{
    xmlNodePtr node;

    node = get_xml_root ();

    if (!node)
	return;

    /* Now parse the xml tree */
    for (node = node->children; node; node = node->next)
    {
	if (xmlStrEqual (node->name, (const xmlChar *) "Panel"))
	{
	    panel_parse_xml (node);

	    break;
	}
    }

    /* leave the xmldoc open for get_panel_config() */
}

G_MODULE_EXPORT /* EXPORT:get_panel_config */
void
get_panel_config (void)
{
    xmlNodePtr node;

    node = get_xml_root ();

    if (!node)
    {
	xfce_err (_("No data was found. The panel will be empty."));
	return;
    }

    /* Now parse the xml tree */
    for (node = node->children; node; node = node->next)
    {
	if (xmlStrEqual (node->name, (const xmlChar *) "Groups"))
	{
	    groups_set_from_xml (node);
	    break;
	}

	/* old format */
	if (xmlStrEqual (node->name, (const xmlChar *) "Left"))
	    old_groups_set_from_xml (LEFT, node);
	else if (xmlStrEqual (node->name, (const xmlChar *) "Right"))
	    old_groups_set_from_xml (RIGHT, node);
    }

    xmlFreeDoc (xmlconfig);
    xmlconfig = NULL;
}

G_MODULE_EXPORT /* EXPORT:write_panel_config */
void
write_panel_config (void)
{
    char *xfcerc, *tmprc;
    xmlNodePtr root;
    char value[3];

    if (disable_user_config)
	return;

    xfcerc = get_save_file ("contents.xml.active");
    
    if (!xfcerc)
    {
        g_critical ("Could not write config file");
        return;
    }
    
    tmprc = g_strconcat (xfcerc, ".tmp", NULL);

    xmlconfig = xmlNewDoc ("1.0");
    xmlconfig->children = xmlNewDocRawNode (xmlconfig, NULL, ROOT, NULL);

    root = (xmlNodePtr) xmlconfig->children;
    xmlDocSetRootElement (xmlconfig, root);

    snprintf (value, 2, "%d", XML_FORMAT_VERSION);
    xmlSetProp (root, "xmlversion", value);

    panel_write_xml (root);

    groups_write_xml (root);

    if (-1 == xmlSaveFormatFile (tmprc, xmlconfig, 1))
    {
	g_critical ("Could not save xml file");
	goto out;
    }

    if (g_file_test (xfcerc, G_FILE_TEST_EXISTS) && unlink (xfcerc))
    {
	g_critical ("Could not remove old contents.xml");
	goto out;
    }

    if (link (tmprc, xfcerc))
    {
	g_critical ("Could not link new contents.xml");
	goto out;
    }

    if (unlink (tmprc))
    {
	g_warning ("Could not remove temporary file contents.xml.tmp");
    }

  out:
    g_free (tmprc);
    g_free (xfcerc);
    xmlFreeDoc (xmlconfig);
    xmlconfig = NULL;
}

G_MODULE_EXPORT /* EXPORT:write_final_panel_config */
void
write_final_panel_config (void)
{
    char *xfcerc, *tmprc;
    
    if (disable_user_config)
	return;

    xfcerc = get_save_file ("contents.xml");
    
    tmprc = g_strconcat (xfcerc, ".active", NULL);

    if (!g_file_test (tmprc, G_FILE_TEST_EXISTS))
    {
        goto out;
    }
    
    if (g_file_test (xfcerc, G_FILE_TEST_EXISTS))
    {
        write_backup_file (xfcerc);
    
        if (unlink (xfcerc))
        {
            g_critical ("Could not remove old contents.xml");
            goto out;
        }
    }

    if (link (tmprc, xfcerc))
    {
	g_critical ("Could not link new contents.xml");
	goto out;
    }

    if (unlink (tmprc))
    {
	g_warning ("Could not remove temporary file contents.xml.tmp");
    }

  out:
    g_free (tmprc);
    g_free (xfcerc);
}

