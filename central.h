#ifndef __XFCE_CENTRAL_H__
#define __XFCE_CENTRAL_H__

#include <gtk/gtk.h>
#include "xfce.h"

struct _CentralPanel
{
  char *screen_names[NBSCREENS];
  char *lock_command;
  char *lock_tip;

  GtkWidget *frame;
  GtkWidget *hbox;

  GtkWidget *left_table;
  GtkWidget *left_buttons[2];

  GtkWidget *right_table;
  GtkWidget *right_buttons[2];

  GtkWidget *desktop_table;
  ScreenButton *screen_buttons[NBSCREENS];
  
  gpointer xmlnode;
};

struct _ScreenButton
{
  GtkWidget *frame;
  GtkWidget *button;
  GtkWidget *label;
  int callback_id;
};

/*****************************************************************************/

CentralPanel *central_panel_new (void);
void add_central_panel (XfcePanel * p);
void create_central_panel (CentralPanel * cp);
void central_panel_set_size (CentralPanel * cp, int size);
void central_panel_set_style (CentralPanel * cp, int style);
void central_panel_set_screens (CentralPanel * cp, int n);
void central_panel_free (CentralPanel * cp);
void central_panel_set_current (CentralPanel * cp, int n);

/*****************************************************************************/

ScreenButton *screen_button_new (void);
void screen_button_free (ScreenButton * sb);

#endif /* __XFCE_CENTRAL_H__ */
