#ifndef __XFCE_ITEM_H__
#define __XFCE_ITEM_H__

struct _PanelItem
{
  char *caption;
  char *command;
  char *tooltip;
  int id;
  char *path;			/* if id==EXTERN_ICON */

  GtkWidget *button;
  GdkPixbuf *pb;
  GtkWidget *image;
  /* for popup items */
  gboolean in_menu;
  PanelPopup *pp;		/* parent popup structure */
  int pos;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *label;

  gpointer xmlnode;
};

/*****************************************************************************/

PanelItem *panel_item_new (PanelPopup * pp);
void create_addtomenu_item (PanelItem * pi);
void create_panel_item (PanelItem * pi);
void panel_item_set_size (PanelItem * pi, int size);
void panel_item_set_popup_size (PanelItem * pi, int size);
void panel_item_set_style (PanelItem * pi, int style);
void panel_item_free (PanelItem * pi);

#endif	/* __XFCE_ITEM_H__ */
