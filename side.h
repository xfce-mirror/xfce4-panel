#ifndef __XFCE_SIDE_H__
#define __XFCE_SIDE_H__

#include <gtk/gtk.h>
#include "xfce.h"

struct _SidePanel
{
  int side;			/* LEFT or RIGHT */
  GtkWidget *hbox;
  
  int num_groups;
  PanelGroup *groups[NBGROUPS];
  
  gpointer xmlnode;
};

struct _PanelGroup
{
  int index;
  GtkWidget *vbox;

  /* top */
  MoveHandle *handle;
  PanelPopup *popup;

  /* bottom */
  int type;			/* ICON, BUILTIN or PLUGIN */
  GtkWidget *eventbox;		/* catch mouse 3 click here */
  PanelItem *item;
  PanelModule *module;
  
  gpointer xmlnode;
};

struct _MoveHandle
{
  GtkWidget *hbox;		/* to be added to panel group vbox */

  GtkWidget *button;

  GtkWidget *handle_frame;
  GtkWidget *eventbox;
};

/*****************************************************************************/

SidePanel *side_panel_new (int side);
void add_side_panel (XfcePanel * p, int side);
void create_side_panel (SidePanel * sp);
void side_panel_set_size (SidePanel * sp, int size);
void side_panel_set_style (SidePanel * sp, int style);
void side_panel_set_popup_size (SidePanel * sp, int size);
void side_panel_set_groups (SidePanel * sp, int n);
void side_panel_free (SidePanel * sp);

/*****************************************************************************/

PanelGroup *panel_group_new (int index);
void create_panel_group (PanelGroup * pg, int side);
void panel_group_set_size (PanelGroup * pg, int size);
void panel_group_set_style (PanelGroup * pg, int style);
void panel_group_set_popup_size (PanelGroup * pg, int size);
void panel_group_free (PanelGroup * pg);

/*****************************************************************************/

MoveHandle *move_handle_new (int side);
void move_handle_set_size (MoveHandle * mh, int size);
void move_handle_set_style (MoveHandle * mh, int style);
void move_handle_free (MoveHandle * mh);

#endif /* __XFCE_SIDE_H__ */
