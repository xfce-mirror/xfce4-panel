#ifndef __XFCE_MODULE_H__
#define __XFCE_MODULE_H__

#include <gtk/gtk.h>
#include <gmodule.h>
#include "xfce.h"

enum
{
  EXTERN_MODULE = -1,
  CLOCK_MODULE,
  TRASH_MODULE,
  BUILTIN_MODULES
};

char *builtin_module_names [BUILTIN_MODULES];

struct _PanelModule
{
  int id;
  char *name;			/* if id==EXTERN_MODULE */
  char *dir;

  GtkWidget *eventbox;

  GModule *gmodule;		/* for PLUGIN */

  gpointer data;		/* usually the module structure */
  char *caption;		/* name to show in the configuration dialog */  
  GtkWidget *main;		/* widget to connect callback to */
  
  /* Module functions
     The init function takes the PanelModule structure as argument
     and fills the other functions and pointers when appropriate
  */
  gpointer (*init) (PanelModule *);
  void (*run) (gpointer);
  void (*stop) (gpointer);
  void (*set_size) (gpointer, int);
  void (*set_style) (gpointer, int);
  void (*configure) (PanelModule *);
  
  gpointer xmlnode;
};

/*****************************************************************************/

void set_module_names (void);

PanelModule *panel_module_new (void);
gboolean create_extern_module (PanelModule * pm);
void create_panel_module (PanelModule * pm, GtkWidget *eventbox);
void panel_module_set_size (PanelModule * pm, int size);
void panel_module_set_style (PanelModule * pm, int style);
void panel_module_free (PanelModule * pm);

#endif /* __XFCE_MODULE_H__ */
