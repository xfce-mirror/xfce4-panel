#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gmodule.h>

#include "xfce.h"
#include "module.h"
#include "icons.h"
#include "builtins.h"

/*****************************************************************************/

void
set_module_names (void)
{
  builtin_module_names [CLOCK_MODULE] = _("Clock");
  builtin_module_names [TRASH_MODULE] = _("Trashcan");
};

PanelModule *
panel_module_new (void)
{
  PanelModule *pm = g_new (PanelModule, 1);

  pm->id = -1;
  pm->name = NULL;
  pm->dir = NULL;
  pm->eventbox = NULL;
  pm->gmodule = NULL;
  pm->data = NULL;
  pm->caption = NULL;
  pm->main = NULL;
  pm->run = NULL;
  pm->stop = NULL;
  pm->set_size = NULL;
  pm->set_style = NULL;
  pm->configure = NULL;
  pm->xmlnode = NULL;

  return pm;
}

static char *
find_module (const char *name)
{
  const char *home = g_getenv ("HOME");
  char dir [MAXSTRLEN + 1];
  char *path;
  
  snprintf (dir, MAXSTRLEN, "%s/.xfce4/panel/plugins", home);
  path = g_build_filename (dir, name, NULL);
  
  if (g_file_test (path, G_FILE_TEST_EXISTS))
    return path;
  else
    g_free (path);
  
  strcpy (dir, "/usr/share/xfce4/panel/plugins");
  path = g_build_filename (dir, name, NULL);
  
  if (g_file_test (path, G_FILE_TEST_EXISTS))
    return path;
  else
    g_free (path);
  
  strcpy (dir, "/usr/local/share/xfce4/panel/plugins");
  path = g_build_filename (dir, name, NULL);
  
  if (g_file_test (path, G_FILE_TEST_EXISTS))
    return path;
  else
    g_free (path);
  
  return NULL;
}

gboolean
create_extern_module (PanelModule * pm)
{
  gpointer tmp;
  char *path;
  
  g_return_val_if_fail (pm->name, FALSE);
  g_return_val_if_fail (g_module_supported (), FALSE);
  
  if (pm->dir)
    path = g_build_filename (pm->dir, pm->name, NULL);
  else
    path = find_module (pm->name);
  
  g_return_val_if_fail (path, FALSE);
  
  pm->gmodule = g_module_open (path, 0);
  
  if (!pm->gmodule || 
      !g_module_symbol (pm->gmodule, "is_xfce_panel_module", &tmp))
    {
      g_printerr ("Not a panel module: %s\n", pm->name);
      return FALSE;
    }
    
  if (g_module_symbol (pm->gmodule, "module_init", &tmp))
    pm->init = tmp;

/* this should be done by the init function 
  if (g_module_symbol (pm->gmodule, "module_run", &tmp))
    pm->run = tmp;
    
  if (g_module_symbol (pm->gmodule, "module_stop", &tmp))
    pm->stop = tmp;
    
  if (g_module_symbol (pm->gmodule, "module_set_size", &tmp))
    pm->set_size = tmp;
    
  if (g_module_symbol (pm->gmodule, "module_set_style", &tmp))
    pm->set_style = tmp;
    
  if (g_module_symbol (pm->gmodule, "module_configure", &tmp))
    pm->configure = tmp;

  if (g_module_symbol (pm->gmodule, "module_caption", &tmp))
    {
      char * (*get_caption) (void) = tmp;
      pm->caption = get_caption ();
    }
*/
  /* the init function fills all other pointers */
  if (pm->init)
    pm->init (pm);

  if (!pm->init || !pm->run || !pm->stop)
    {
      g_module_close (pm->gmodule);
      pm->gmodule = NULL;
      
      return FALSE;
    }
  
  return TRUE;
}

void
create_panel_module (PanelModule * pm, GtkWidget *eventbox)
{
  pm->eventbox = eventbox;

  switch (pm->id)
    {
    case CLOCK_MODULE:
      create_clock_module (pm);
      break;
    case TRASH_MODULE:
      create_trash_module (pm);
      break;
    case EXTERN_MODULE:
      create_extern_module (pm);
      break;
    }
}

void
panel_module_set_size (PanelModule * pm, int size)
{
  if (pm->set_size)
    pm->set_size (pm->data, size);
}

void
panel_module_set_style (PanelModule * pm, int style)
{
  if (pm->set_style)
    pm->set_style (pm->data, style);
}

void
panel_module_free (PanelModule * pm)
{
  if (pm->stop)
    pm->stop (pm->data);
  
  if (pm->gmodule)
    {
      g_module_close (pm->gmodule);
      g_free (pm->name);
      g_free (pm->dir);
    }
    
  g_free (pm);
}
