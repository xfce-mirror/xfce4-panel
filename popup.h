#ifndef __XFCE_POPUP_H__
#define __XFCE_POPUP_H__

struct _PanelPopup
{
  /* button */
  GtkWidget *button;
  GdkPixbuf *up;
  GdkPixbuf *down;
  GtkWidget *arrow_image;

  /* menu */
  GtkWidget *window;
  GtkWidget *frame;
  GtkWidget *vbox;

  GtkSizeGroup *hgroup;

  PanelItem *addtomenu_item;
  GtkWidget *separator;
  GList *items;			/* type PanelItem */
  GtkWidget *tearoff_button;
  gboolean detached;
  
  gpointer xmlnode;
};

struct _PopupItem
{
  char *command;
  char *icon_path;
  char *caption;
  char *tooltip;

  GtkWidget *button;
  GtkWidget *hbox;
  GdkPixbuf *pb;
  GtkWidget *image;
  GtkWidget *label;
  
  gpointer xmlnode;
};

/*****************************************************************************/

PanelPopup *panel_popup_new (void);
void create_panel_popup (PanelPopup * pp);
void panel_popup_set_size (PanelPopup * pp, int size);
void panel_popup_set_style (PanelPopup * pp, int style);
void panel_popup_set_popup_size (PanelPopup * pp, int size);
void panel_popup_free (PanelPopup * pp);

void hide_current_popup_menu (void);

#endif	/* __XFCE_POPUP_H__ */
