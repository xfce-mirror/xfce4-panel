#ifndef __XFCE_CALLBACKS_H__
#define __XFCE_CALLBACKS_H__

#include <gtk/gtk.h>
#include "xfce.h"

/*****************************************************************************/

void exec_cmd (const char *cmd);
GList *gnome_uri_list_extract_filenames (const gchar * uri_list);
void gnome_uri_list_free_strings (GList * list);

/*****************************************************************************/

gboolean panel_delete_cb (GtkWidget * window, GdkEvent * ev, gpointer data);

/*****************************************************************************/

gboolean screen_button_pressed_cb (GtkButton * b, GdkEventButton * ev,
				   ScreenButton * sb);

void mini_lock_cb (char *cmd);
void mini_info_cb (void);
void mini_palet_cb (void);

void mini_power_cb (GtkButton * b, GdkEventButton * ev, gpointer data);

/*****************************************************************************/

void iconify_cb (void);

gboolean group_button_press_cb (GtkButton * b, GdkEventButton * ev,
				PanelGroup * pg);

/*****************************************************************************/

void toggle_popup (GtkWidget * button, PanelPopup * pp);

void tearoff_popup (GtkWidget * button, PanelPopup * pp);

gboolean delete_popup (GtkWidget * window, GdkEvent * ev, PanelPopup * pp);

/*****************************************************************************/

void addtomenu_item_drop_cb (GtkWidget * widget, GdkDragContext * context,
			     gint x, gint y, GtkSelectionData * data,
			     guint info, guint time, PanelPopup * pp);

void addtomenu_item_click_cb (GtkButton * b, PanelPopup * pp);

void item_drop_cb (GtkWidget * widget, GdkDragContext * context,
		   gint x, gint y, GtkSelectionData * data,
		   guint info, guint time, char *command);

void item_click_cb (GtkButton * b, char *command);

gboolean item_in_menu_press_cb (GtkButton * b, GdkEventButton * ev, 
				PanelItem * pi);

gboolean item_on_panel_press_cb (GtkButton * b, GdkEventButton * ev, 
				 PanelGroup * pg);

#endif /* __XFCE_CALLBACKS_H__ */
