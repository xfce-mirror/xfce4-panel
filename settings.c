#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "xfce.h"
#include "settings.h"
#include "central.h"
#include "side.h"
#include "popup.h"
#include "item.h"
#include "module.h"
#include "icons.h"

#define strcaseequal(s1,s2) !g_strcasecmp(s1, s2)
#define strncaseequal(s1,s2) !g_strncasecmp(s1, s2)

static char *default_screen_names[] = {
  N_("One"),
  N_("Two"),
  N_("Three"),
  N_("Four"),
  N_("Five"),
  N_("Six"),
  N_("Seven"),
  N_("Eight"),
  N_("Nine"),
  N_("Ten"),
  N_("Eleven"),
  N_("Twelve")
};

static gboolean disable_user_config = FALSE;

/*****************************************************************************/
/* file handling */
static void
write_backup_file (void)
{
  FILE *fp;
  FILE *bakfp;
  const char *home;
  char rcfile [MAXSTRLEN + 1];
  char bakfile [MAXSTRLEN + 1];
  
  home = g_getenv ("HOME");
  snprintf (rcfile, MAXSTRLEN, "%s/%s", home, RCFILE);
  snprintf (bakfile, MAXSTRLEN, "%s.bak", rcfile);

  if ((bakfp = fopen (bakfile, "w")) && (fp = fopen (rcfile, "r")))
    {
      char c;

      while ((c = fgetc (fp)) != EOF)
	putc (c, bakfp);

      fclose (fp);
      fclose (bakfp);
    }
}

static char *
localized_rcfile (void)
{
  const char *locale;
  const char *home;
  char base_locale [3];
  char rcfile [MAXSTRLEN + 1];
  char sysrcfile [MAXSTRLEN + 1];
  char file [MAXSTRLEN + 1];
  
  home = g_getenv ("HOME");
  
  if (!(locale = g_getenv ("LC_MESSAGES")))
    locale = g_getenv ("LANG");
  
  if (locale)
    {
      base_locale[0] = locale[0];
      base_locale[1] = locale[1];
      base_locale[3] = '\0';
    }
  
  snprintf (rcfile, MAXSTRLEN, "%s/%s/%s", home, RCDIR, RCFILE);
  snprintf (sysrcfile, MAXSTRLEN, "%s/%s", SYSCONFDIR, RCFILE);
  
  if (!disable_user_config && g_file_test (rcfile, G_FILE_TEST_EXISTS))
    return g_strdup (rcfile);
  else
    {
      if (!locale)
	{
	  if (g_file_test (sysrcfile, G_FILE_TEST_EXISTS))
	    return g_strdup (sysrcfile);
	  else
	    return NULL;
	}
      else
	{
	  snprintf (file, MAXSTRLEN, "%s.%s", sysrcfile, locale);
	  
	  if (g_file_test (file, G_FILE_TEST_EXISTS))
	    return g_strdup (file);
	  
	  snprintf (file, MAXSTRLEN, "%s.%s", sysrcfile, base_locale);
	  
	  if (g_file_test (file, G_FILE_TEST_EXISTS))
	    return g_strdup (file);
	  else
	    return NULL;
	}
    }
}

xmlDocPtr 
read_xml_file (void)
{
  char *rcfile;
  xmlDocPtr doc;
  
  xmlKeepBlanksDefault (0);
  
  rcfile = localized_rcfile ();
  doc = xmlParseFile (rcfile);
  
  g_free (rcfile);
  return doc;
}

/*****************************************************************************/

static void
set_default_groups (SidePanel * sp)
{
  int i;
  
  for (i = 0; i < NBGROUPS; i++)
    {
      PanelGroup *pg = sp->groups [i];
     
      /* top */
      if (i != 0 && !pg->popup)
	{
	  pg->popup = panel_popup_new ();
	}
	
      if (pg->type != -1)
	continue;
      
      pg->type = ICON;
      
      /* bottom */
      pg->item = panel_item_new (NULL);
      pg->item->id = UNKNOWN_ICON;
      pg->item->tooltip = g_strdup (_("Click mouse button 3 to change item"));
      pg->item->command = g_strdup ("xfterm");
    }
}

static void
set_default_settings (XfcePanel * p)
{
  int i;
  
  if (p->style == -1)
    p->style = NEW_STYLE;
  if (p->size == -1)
    p->size = MEDIUM;
  if (p->popup_size == -1)
    p->popup_size = MEDIUM;
  if (p->num_screens == -1)
    p->num_screens = 4;
  if (p->num_groups_left == -1)
    p->num_groups_left = 5;
  if (p->num_groups_right == -1)
    p->num_groups_right = 5;
  
  for (i = 0; i < NBSCREENS; i++)
    {
      if (!p->central_panel->screen_names[i])
	p->central_panel->screen_names[i] = g_strdup (default_screen_names [i]);
    }
  
  /* central panel has no settings */
  set_default_groups (p->left_panel);
  set_default_groups (p->right_panel);
}

/*****************************************************************************/

static xmlDocPtr xmlconfig;

static void
xml_get_panel_properties (xmlNodePtr node, XfcePanel * p)
{
  xmlChar *value;

  value = xmlGetProp (node, (const xmlChar *) "size");
  if (value)
    p->size = atoi (value);
  
  if (!value || p->size < SMALL || p->size > LARGE)
    p->size = MEDIUM;

  g_free (value);
  
  value = xmlGetProp (node, (const xmlChar *) "popupsize");
  if (value)
    p->popup_size = atoi (value);
  
  if (!value || p->popup_size < SMALL || p->popup_size > LARGE)
    p->popup_size = MEDIUM;

  g_free (value);
  
  value = xmlGetProp (node, (const xmlChar *) "style");
  if (value)
    p->size = atoi (value);
  
  if (!value || p->size < NEW_STYLE || p->size > OLD_STYLE)
    p->size = NEW_STYLE;

  g_free (value);
  
  value = xmlGetProp (node, (const xmlChar *) "icontheme");
  if (value)
    p->icon_theme = g_strdup (value);
  else
    p->icon_theme = NULL;

  g_free (value);
}

static void
xml_get_position (xmlNodePtr node, XfcePanel * p)
{
  xmlChar *value;
  
  value = xmlGetProp (node, (const xmlChar *) "x");
  if (value)
    p->x = atoi (value);
  
  if (!value || p->x < 0 || p->x > gdk_screen_width())
    p->x = -1;

  g_free (value);

  value = xmlGetProp (node, (const xmlChar *) "y");
  if (value)
    p->y = atoi (value);
  
  if (!value || p->y < 0 || p->y > gdk_screen_height())
    p->y = -1;

  g_free (value);
}

static void
xml_parse_screens (xmlNodePtr node, XfcePanel * p)
{
  CentralPanel *cp = p->central_panel;
  xmlNodePtr child;
  xmlChar *value;
  int n;

  value = xmlGetProp (node, (const xmlChar *) "visible");
  
  if (value)
    p->num_screens = atoi (value);
  
  if (!value || p->num_screens < 1 || p->num_screens > NBSCREENS)
    p->num_screens = 4;

  g_free (value);
  
  n = 0;
  
  for (child = node->children; child; child = child->next)
    {
      if (!xmlStrEqual (child->name, (const xmlChar *) "Screen"))
	continue;
      
      value = xmlNodeListGetString (xmlconfig, child->children, 0);
      
      if (value)
	cp->screen_names [n] = (char *) value;
      
      n++;
    }
}

static void
xml_parse_central (xmlNodePtr node, XfcePanel * p)
{
  xmlNodePtr child;
  xmlChar *value;

  p->central_panel->xmlnode = (gpointer) node;
  
  for (child = node->children; child; child = child->next)
    {
      if (xmlStrEqual (child->name, (const xmlChar *) "Screens"))
	xml_parse_screens (child, p);
      else if (xmlStrEqual (child->name, (const xmlChar *) "Lock"))
	{
	  value = xmlNodeListGetString (xmlconfig, child->children, 0);
	  
	  if (value)
	    p->central_panel->lock_command = (char *) value;
	}
      else if (xmlStrEqual (child->name, (const xmlChar *) "Power"))
	{
	  /* something to end the session */
	}
    }
}

static void
xml_parse_item (xmlNodePtr node, PanelItem * pi)
{
  xmlNodePtr child;
  xmlChar *value;
  
  pi->xmlnode = (gpointer) node;
  
  for (child = node->children; child; child = child->next)
    {
      if (xmlStrEqual (child->name, (const xmlChar *) "Caption"))
	{
	  value = xmlNodeListGetString (xmlconfig, child->children, 0);
	  
	  if (value)
	    pi->caption = (char *) value;
	}
      else if (xmlStrEqual (child->name, (const xmlChar *) "Command"))
	{
	  value = xmlNodeListGetString (xmlconfig, child->children, 0);
	  
	  if (value)
	    pi->command = (char *) value;
	}
      else if (xmlStrEqual (child->name, (const xmlChar *) "Tooltip"))
	{
	  value = xmlNodeListGetString (xmlconfig, child->children, 0);
	  
	  if (value)
	    pi->tooltip = (char *) value;
	}
      else if (xmlStrEqual (child->name, (const xmlChar *) "Icon"))
	{
	  value = xmlGetProp (child, (const xmlChar *) "id");
	  
	  if (value)
	    pi->id = atoi (value);
	  
	  if (!value || pi->id < EXTERN_ICON || pi->id >= NUM_ICONS)
	    pi->id = UNKNOWN_ICON;
	  
	  g_free (value);
	  
	  if (pi->id == EXTERN_ICON)
	    {
	      value = xmlNodeListGetString (xmlconfig, child->children, 0);
	      
	      if (value)
		pi->path = (char *) value;
	      else
		pi->id = UNKNOWN_ICON;
	    }
	}
    }
}

static void
xml_parse_popup (xmlNodePtr node, PanelPopup * pp)
{
  xmlNodePtr child;
  
  pp->xmlnode = node;
  
  for (child = node->children; child; child = child->next)
    {
      PanelItem *pi;
      
      if (!xmlStrEqual (child->name, (const xmlChar *) "Item"))
	continue;

      pi = panel_item_new (pp);
      pi->in_menu = TRUE;
      xml_parse_item (child, pi);
      pp->items = g_list_append (pp->items, pi);
    }
}

static void
xml_parse_group (xmlNodePtr node, PanelGroup * pg)
{
  xmlNodePtr child;
  xmlChar *value;

  pg->xmlnode = (gpointer) node;
  
  for (child = node->children; child; child = child->next)
    {
      if (xmlStrEqual (child->name, (const xmlChar *) "Popup"))
	{
	  pg->popup = panel_popup_new ();
	  xml_parse_popup (child, pg->popup);
	}
      else if (xmlStrEqual (child->name, (const xmlChar *) "Item"))
	{
	  pg->type = ICON;
	  pg->item = panel_item_new (NULL);
	  xml_parse_item (child, pg->item);
	}
      else if (xmlStrEqual (child->name, (const xmlChar *) "Module"))
	{
	  PanelModule *pm;
	  
	  value = xmlGetProp (child, (const xmlChar *) "id");
	  
	  if (!value)
	    continue;
	  
	  pg->type = MODULE;
	  pm = pg->module = panel_module_new ();
	  pm->id = atoi (value);
	  
	  g_free (value);
	  
	  if (pm->id == EXTERN_MODULE)
	    {
	      value = xmlNodeListGetString (xmlconfig, child->children, 0);
	      
	      if (value)
		pm->name = (char *) value;
	      else
		pm->id = EXTERN_MODULE - 1;
	    }
	  
	  if (pm->id < EXTERN_MODULE || pm->id >= BUILTIN_MODULES)
	    {
	      panel_module_free (pm);
	      pg->type = -1;
	    }
	}
    }
}

static void
xml_parse_left (xmlNodePtr node, XfcePanel * p)
{
  xmlNodePtr child;
  xmlChar *value;
  int n;

  p->left_panel->xmlnode = (gpointer) node;
  
  value = xmlGetProp (node, (const xmlChar *) "visible");
  
  if (value)
    p->num_groups_left = atoi (value);
  
  if (!value || p->num_groups_left < 1 || p->num_groups_left > NBGROUPS)
    p->num_groups_left = 5;

  g_free (value);
  
  n = 0;
  
  for (child = node->children; child; child = child->next)
    {
      if (!xmlStrEqual (child->name, (const xmlChar *) "Group"))
	continue;

      xml_parse_group (child, p->left_panel->groups [n]);
      n++;
    }
}

static void
xml_parse_right (xmlNodePtr node, XfcePanel * p)
{
  xmlNodePtr child;
  xmlChar *value;
  int n;

  p->right_panel->xmlnode = (gpointer) node;
  
  value = xmlGetProp (node, (const xmlChar *) "visible");
  
  if (value)
    p->num_groups_right = atoi (value);
  
  if (!value || p->num_groups_left < 1 || p->num_groups_left > NBGROUPS)
    p->num_groups_right = 5;

  g_free (value);
  
  n = 0;
  
  for (child = node->children; child; child = child->next)
    {
      if (!xmlStrEqual (child->name, (const xmlChar *) "Group"))
	continue;

      xml_parse_group (child, p->right_panel->groups [n]);
      n++;
    }
}

static void
settings_from_file (XfcePanel *p)
{
#if 1
  xmlNodePtr node;

  /* global xmlDocPtr */
  xmlconfig = read_xml_file ();
  
  if (!xmlconfig)
    return;
  
  node = xmlDocGetRootElement(xmlconfig);
  
  if (!node)
    {
      g_printerr (_("xfce: %s (line %d): empty document\n"), 
		  __FILE__, __LINE__);
      return;
    }

  if (!xmlStrEqual (node->name, (const xmlChar *) "Panel"))
    {
      g_printerr (_("xfce: %s (line %d): wrong document type\n"), 
		  __FILE__, __LINE__);
      return;
    }
  
  p->xmlnode = node;
    
  xml_get_panel_properties (node, p);
  
  for (node = node->children; node; node = node->next)
    {
      if (xmlStrEqual (node->name, (const xmlChar *) "Position"))
	xml_get_position (node, p);
      else if (xmlStrEqual (node->name, (const xmlChar *) "Central"))
	xml_parse_central (node, p);
      else if (xmlStrEqual (node->name, (const xmlChar *) "Left"))
	xml_parse_left (node, p);
      else if (xmlStrEqual (node->name, (const xmlChar *) "Right"))
	xml_parse_right (node, p);
    }

#else
  PanelGroup *pg;
  
  pg = p->left_panel->groups[0];
  pg->type = MODULE;
  pg->module = panel_module_new ();
  pg->module->id = CLOCK_MODULE;
  
  pg = p->left_panel->groups[1];
  pg->type = ICON;
  pg->item = panel_item_new ();
  pg->item->id = EXTERN_ICON;
  pg->item->path = g_strdup ("/usr/share/xfce/icons/FileManager.xpm");
  pg->item->command = g_strdup ("xftree");
  pg->item->tooltip = g_strdup ("Bestandsbeheer");
  
  pg = p->left_panel->groups[2];
  pg->type = MODULE;
  pg->module = panel_module_new ();
  pg->module->id = EXTERN_MODULE;
  pg->module->name = g_strdup ("mailcheck");
  pg->module->dir = g_strdup ("/home/huysmans/Documents/Projects/xfce-panel");
  
  pg = p->right_panel->groups[0];
  pg->type = MODULE;
  pg->module = panel_module_new ();
  pg->module->id = TRASH_MODULE;
  
  pg = p->right_panel->groups[1];
  pg->type = ICON;
  pg->item = panel_item_new ();
  pg->item->id = EXTERN_ICON;
  pg->item->path = g_strdup ("/usr/share/xfce/icons/Info.xpm");
  pg->item->command = g_strdup ("xfhelp");
  pg->item->tooltip = g_strdup ("XFce Handleiding");
#endif
}

gboolean
check_disable_user_config (void)
{
  char userconf [MAXSTRLEN + 1];
  
  snprintf (userconf, MAXSTRLEN, "%s/disable_user_config", SYSCONFDIR);
  
  return g_file_test (userconf, G_FILE_TEST_EXISTS);
}

void
get_settings (XfcePanel * p)
{
  disable_user_config = check_disable_user_config ();
    
  settings_from_file (p);
  
  /* fill empty items */
  set_default_settings (p);
}
