#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gmodule.h>

#include "xfce.h"
#include "dialogs.h"
#include "callbacks.h"
#include "settings.h"
#include "central.h"
#include "side.h"
#include "item.h"
#include "popup.h"
#include "module.h"
#include "icons.h"

/*****************************************************************************/

enum
{
  TARGET_STRING,
  TARGET_ROOTWIN,
  TARGET_URL
};

static GtkTargetEntry target_table[] = {
  {"text/uri-list", 0, TARGET_URL},
  {"STRING", 0, TARGET_STRING}
};

static guint n_targets = sizeof (target_table) / sizeof (target_table[0]);

/*****************************************************************************/
/* item */

enum
{ RESPONSE_REMOVE, RESPONSE_CHANGE, RESPONSE_CANCEL };

typedef struct
{
  GtkWidget *frame;
  GtkWidget *pos_spinbutton;
  GtkWidget *command_entry;
  GtkWidget *command_browse_button;
  GtkWidget *icon_option_menu;
  GtkWidget *icon_browse_button;
  GtkWidget *icon_entry;
  int icon_id;
  char *icon_path;
  GdkPixbuf *pb;
  GtkWidget *preview;
  GtkWidget *caption_entry;
  GtkWidget *tooltip_entry;
}
ItemDialogFrame;

ItemDialogFrame idf;

static void
icon_id_changed (void)
{
  int new_id =
    gtk_option_menu_get_history (GTK_OPTION_MENU (idf.icon_option_menu));

  if (idf.icon_id == new_id)
    return;

  g_object_unref (idf.pb);
  idf.pb = NULL;

  if (new_id == 0)
    {
      idf.icon_id = -1;

      if (idf.icon_path)
	{
	  gtk_entry_set_text (GTK_ENTRY (idf.icon_entry), idf.icon_path);

	  idf.pb = gdk_pixbuf_new_from_file (idf.icon_path, NULL);
	}

      if (!idf.pb)
	idf.pb = get_pixbuf_from_id (UNKNOWN_ICON);
    }
  else
    {
      if (idf.icon_id == -1)
	{
	  g_free (idf.icon_path);
	  idf.icon_path =
	    g_strdup (gtk_entry_get_text (GTK_ENTRY (idf.icon_entry)));
	}

      idf.icon_id = new_id;

      gtk_entry_set_text (GTK_ENTRY (idf.icon_entry), "");
      idf.pb = get_pixbuf_from_id (new_id);
    }


  gtk_image_set_from_pixbuf (GTK_IMAGE (idf.preview), idf.pb);
}

static void
create_icon_option_menu (void)
{
  GtkWidget *menu = gtk_menu_new ();
  GtkWidget *mi;
  int i;

  mi = gtk_menu_item_new_with_label (_("External Icon"));
  gtk_widget_show (mi);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

  for (i = 1; i < NUM_ICONS; i++)
    {
      mi = gtk_menu_item_new_with_label (icon_names[i]);
      gtk_widget_show (mi);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    }

  idf.icon_option_menu = gtk_option_menu_new ();
  gtk_option_menu_set_menu (GTK_OPTION_MENU (idf.icon_option_menu), menu);

  g_signal_connect_swapped (idf.icon_option_menu, "changed",
			    G_CALLBACK (icon_id_changed), NULL);
}

static void
icon_drop_cb (GtkWidget * widget, GdkDragContext * context,
	      gint x, gint y, GtkSelectionData * data,
	      guint info, guint time, gpointer user_data)
{
  GList *fnames;
  guint count;

  fnames = gnome_uri_list_extract_filenames ((char *) data->data);
  count = g_list_length (fnames);

  if (count > 0)
    {
      char *icon;

      icon = (char *) fnames->data;

      idf.icon_path = g_strdup (icon);
      /* this takes care of setting the preview */
      gtk_option_menu_set_history (GTK_OPTION_MENU (idf.icon_option_menu), 0);
      icon_id_changed ();
    }

  gnome_uri_list_free_strings (fnames);
  gtk_drag_finish (context, (count > 0), (context->action == GDK_ACTION_MOVE),
		   time);
}

static void
create_item_dialog_frame (PanelItem * pi, int num_items)
{
  GtkWidget *main_hbox;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *hbox;
  GtkSizeGroup *sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  GtkWidget *frame;
  GtkWidget *eventbox;

  idf.frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (idf.frame), GTK_SHADOW_NONE);

  main_hbox = gtk_hbox_new (FALSE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (main_hbox), 10);
  gtk_container_add (GTK_CONTAINER (idf.frame), main_hbox);

  vbox = gtk_vbox_new (TRUE, 8);
  gtk_box_pack_start (GTK_BOX (main_hbox), vbox, TRUE, TRUE, 0);

  hbox = gtk_hbox_new (FALSE, 4);

  label = gtk_label_new (_("Command:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_size_group_add_widget (sg, label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  idf.command_entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), idf.command_entry, TRUE, TRUE, 0);

  idf.command_browse_button = gtk_button_new_with_label (" ... ");
  gtk_box_pack_start (GTK_BOX (hbox), idf.command_browse_button,
		      FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

  hbox = gtk_hbox_new (FALSE, 4);

  label = gtk_label_new (_("Icon:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_size_group_add_widget (sg, label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  create_icon_option_menu ();
  gtk_box_pack_start (GTK_BOX (hbox), idf.icon_option_menu, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

  hbox = gtk_hbox_new (FALSE, 4);

  label = gtk_label_new ("");
  gtk_size_group_add_widget (sg, label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  idf.icon_entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), idf.icon_entry, TRUE, TRUE, 0);

  idf.icon_browse_button = gtk_button_new_with_label (" ... ");
  gtk_box_pack_start (GTK_BOX (hbox), idf.icon_browse_button,
		      FALSE, FALSE, 0);

  idf.icon_id = pi->id;
  idf.icon_path = NULL;

  if (pi->id == -1)
    {
      gtk_option_menu_set_history (GTK_OPTION_MENU (idf.icon_option_menu), 0);
      if (pi->path)
	gtk_entry_set_text (GTK_ENTRY (idf.icon_entry), pi->path);
    }
  else
    {
      gtk_option_menu_set_history (GTK_OPTION_MENU (idf.icon_option_menu),
				   pi->id);
      gtk_widget_set_sensitive (idf.icon_entry, FALSE);
      gtk_widget_set_sensitive (idf.icon_browse_button, FALSE);
    }

  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

  if (pi->in_menu)
    {
      hbox = gtk_hbox_new (FALSE, 4);

      label = gtk_label_new (_("Caption:"));
      gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
      gtk_size_group_add_widget (sg, label);
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

      idf.caption_entry = gtk_entry_new ();
      gtk_box_pack_start (GTK_BOX (hbox), idf.caption_entry, TRUE, TRUE, 0);

      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
    }

  hbox = gtk_hbox_new (FALSE, 4);

  label = gtk_label_new (_("Tooltip:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_size_group_add_widget (sg, label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  idf.tooltip_entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), idf.tooltip_entry, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

  if (pi->in_menu && num_items > 1)
    {
      hbox = gtk_hbox_new (FALSE, 4);

      label = gtk_label_new (_("Position:"));
      gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
      gtk_size_group_add_widget (sg, label);
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

      idf.pos_spinbutton = gtk_spin_button_new_with_range (1, num_items, 1);
      gtk_spin_button_set_value (GTK_SPIN_BUTTON (idf.pos_spinbutton),
				 pi->pos + 1);
      gtk_box_pack_start (GTK_BOX (hbox), idf.pos_spinbutton,
			  FALSE, FALSE, 0);

      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
    }

  if (pi->command)
    gtk_entry_set_text (GTK_ENTRY (idf.command_entry), pi->command);
  if (pi->caption && pi->in_menu)
    gtk_entry_set_text (GTK_ENTRY (idf.caption_entry), pi->caption);
  if (pi->tooltip)
    gtk_entry_set_text (GTK_ENTRY (idf.tooltip_entry), pi->tooltip);

  frame = gtk_frame_new (_("Icon Preview"));
  gtk_box_pack_start (GTK_BOX (main_hbox), frame, TRUE, TRUE, 0);

  eventbox = gtk_event_box_new ();
  panel_set_tooltip (eventbox,
		     _("Drag file onto this frame to change the icon"));
  gtk_container_add (GTK_CONTAINER (frame), eventbox);

  idf.pb = pi->pb;
  g_object_ref (idf.pb);

  idf.preview = gtk_image_new_from_pixbuf (idf.pb);
  gtk_container_add (GTK_CONTAINER (eventbox), idf.preview);

  /* signals */
  gtk_drag_dest_set (eventbox, GTK_DEST_DEFAULT_ALL,
		     target_table, n_targets, GDK_ACTION_COPY);

  g_signal_connect (eventbox, "drag_data_received",
		    G_CALLBACK (icon_drop_cb), NULL);

  gtk_widget_show_all (idf.frame);
}

/*****************************************************************************/
/* popup item */

void
edit_item_dlg (PanelItem * pi)
{
  GtkWidget *dialog;
  int response;
  int n;

  if (pi->in_menu)
    {
      dialog = gtk_dialog_new_with_buttons (_("Edit item"), NULL,
					    GTK_DIALOG_MODAL,
					    GTK_STOCK_CANCEL,
					    RESPONSE_CHANGE, 
					    GTK_STOCK_REMOVE,
					    RESPONSE_REMOVE,
					    GTK_STOCK_APPLY,
					    RESPONSE_CHANGE,
					    NULL);
    }
  else
    {
      dialog = gtk_dialog_new_with_buttons (_("Edit item"), NULL,
					    GTK_DIALOG_MODAL,
					    GTK_STOCK_CANCEL,
					    RESPONSE_CHANGE, 
					    GTK_STOCK_APPLY,
					    RESPONSE_CHANGE,
					    NULL);
    }

  n = pi->in_menu ? g_list_length (pi->pp->items) : 0;

  create_item_dialog_frame (pi, n);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
		      idf.frame, TRUE, TRUE, 0);

  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);

  response = gtk_dialog_run (GTK_DIALOG (dialog));

  switch (response)
    {
    case RESPONSE_REMOVE:
      break;
    case RESPONSE_CHANGE:
      break;
    }

  gtk_widget_destroy (dialog);
  g_object_unref (idf.pb);
}

void
add_item_dialog (PanelPopup * pp)
{
  PanelItem *pi;
  GtkWidget *dialog;
  int response, n;

  dialog = gtk_dialog_new_with_buttons (_("Add item"), NULL,
					GTK_DIALOG_MODAL,
					GTK_STOCK_CANCEL,
					RESPONSE_CHANGE, 
					GTK_STOCK_ADD,
					RESPONSE_CHANGE,
					NULL);

  pi = panel_item_new (pp);
  pi->pos = 0;
  pi->in_menu = TRUE;
  pi->id = -1;
  create_panel_item (pi);

  n = g_list_length (pp->items) + 1;

  create_item_dialog_frame (pi, n);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
		      idf.frame, TRUE, TRUE, 0);

  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);

  response = gtk_dialog_run (GTK_DIALOG (dialog));

  if (response == RESPONSE_CHANGE)
    {
    }
  else
    panel_item_free (pi);

  gtk_widget_destroy (dialog);
  g_object_unref (idf.pb);
}

/*****************************************************************************/
/* module */

typedef struct
{
  GtkWidget *frame;
  GtkWidget *module_option_menu;
  int module_id;
  GtkWidget *configure_button;
  GList *modules;
  PanelModule *current;
}
ModuleDialogFrame;

static ModuleDialogFrame mdf;

static void
module_id_changed (void)
{
  int new_id =
    gtk_option_menu_get_history (GTK_OPTION_MENU (mdf.module_option_menu));

  if (mdf.module_id == new_id)
    return;

  mdf.module_id = new_id;

  if (new_id == 0)
    {
      mdf.current = NULL;

      gtk_widget_set_sensitive (mdf.configure_button, FALSE);
    }
  else if (new_id <= BUILTIN_MODULES)
    {
      mdf.current = (PanelModule *) g_list_nth (mdf.modules, new_id - 1)->data;
    }
  else
    {
      mdf.current = (PanelModule *) g_list_nth (mdf.modules, new_id - 1)->data;
    }

  if (mdf.current && mdf.current->configure)
    {
      gtk_widget_set_sensitive (mdf.configure_button, TRUE);
    }
}

static char *
get_module_name_from_file (const char *file)
{
  char name[strlen (file)];
  char *s;

  strcpy (name, file);
  
  s = strchr (name, '.');
  
  while (s)
    {
      if (s + 1 && !strncmp (s + 1, G_MODULE_SUFFIX, strlen (G_MODULE_SUFFIX)))
	{
	  *(s + 1 + strlen (G_MODULE_SUFFIX)) = '\0';
	  break;
	}
	
      s = strchr (s + 1, '.');
    }

  return g_strdup (name);
}

static void
create_module_option_menu (void)
{
  GtkWidget *menu = gtk_menu_new ();
  GtkWidget *mi;
  int i;
  const char *home, *file;
  char dirname[MAXSTRLEN + 1];
  GList *module_names = NULL;
  GDir *gdir;

  mdf.modules = NULL;
  mdf.current = NULL;

  /* first empty item */
  mi = gtk_menu_item_new_with_label ("");
  gtk_widget_show (mi);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
  
  /* builtins */
  for (i = 0; i < BUILTIN_MODULES; i++)
    {
      PanelModule *pm = panel_module_new ();
      GtkWidget *eventbox = gtk_event_box_new ();

      pm->id = i;
      create_panel_module (pm, eventbox);

      mdf.modules = g_list_append (mdf.modules, pm);
      
      mi = gtk_menu_item_new_with_label (builtin_module_names[i]);
      gtk_widget_show (mi);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    }

  /* separator 
  mi = gtk_separator_menu_item_new ();
  gtk_widget_show (mi);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
*/
  /* plugins */
  home = g_getenv ("HOME");
  for (i = 0; i < 3; i++)
    {
      if (i == 0)
	snprintf (dirname, MAXSTRLEN, "%s/.xfce4/panel/plugins", home);
      else if (i == 1)
	strcpy (dirname, "/usr/local/share/xfce4/panel/plugins");
      else
	strcpy (dirname, "/usr/share/xfce4/panel/plugins");

      gdir = g_dir_open (dirname, 0, NULL);

      if (gdir)
	{
	  while ((file = g_dir_read_name (gdir)))
	    {
	      PanelModule *pm = panel_module_new ();
	      char *path;
	      GModule *tmp;

	      pm->id = EXTERN_MODULE;

	      path = g_build_filename (dirname, file, NULL);

	      tmp = g_module_open (path, 0);
	      g_free (path);

	      if (!tmp)
		{
		  panel_module_free (pm);
		  continue;
		}

	      pm->name = get_module_name_from_file (file);

	      g_module_close (tmp);

	      pm->dir = g_strdup (dirname);
	      pm->eventbox = gtk_event_box_new ();

	      if (g_list_find_custom (module_names, pm->name,
				      (GCompareFunc) strcmp) ||
		  !create_extern_module (pm))
		{
		  gtk_widget_destroy (pm->eventbox);
		  panel_module_free (pm);
		  continue;
		}

	      mdf.modules = g_list_append (mdf.modules, pm);
	      module_names = g_list_append (module_names, pm->name);

	      if (pm->caption)
		mi = gtk_menu_item_new_with_label (pm->caption);
	      else
		mi = gtk_menu_item_new_with_label (pm->name);

	      gtk_widget_show (mi);
	      gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
	    }

	  g_dir_close (gdir);
	}
    }

  g_list_free (module_names);

  mdf.module_option_menu = gtk_option_menu_new ();
  gtk_option_menu_set_menu (GTK_OPTION_MENU (mdf.module_option_menu), menu);
}

static void
configure_click_cb (void)
{
  if (mdf.current && mdf.current->configure)
    mdf.current->configure (mdf.current);
}

static void
create_module_dialog_frame (PanelModule * pm)
{
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *hbox;
  GtkSizeGroup *sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  GList *modules;
  int i;

  mdf.frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (mdf.frame), GTK_SHADOW_NONE);

  vbox = gtk_vbox_new (FALSE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);
  gtk_container_add (GTK_CONTAINER (mdf.frame), vbox);

  hbox = gtk_hbox_new (FALSE, 4);

  label = gtk_label_new (_("Module:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_size_group_add_widget (sg, label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  create_module_option_menu ();
  gtk_box_pack_start (GTK_BOX (hbox), mdf.module_option_menu, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

  hbox = gtk_hbox_new (FALSE, 4);

  label = gtk_label_new ("");
  gtk_size_group_add_widget (sg, label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  mdf.configure_button = gtk_button_new_from_stock (GTK_STOCK_PREFERENCES);
  gtk_widget_set_sensitive (mdf.configure_button, FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), mdf.configure_button, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /* option menu id 0 is empty */
  for (i = 1, modules = mdf.modules; modules; modules = modules->next, i++)
    {
      PanelModule * module = (PanelModule *) modules->data;
      
      if (!module || !pm || pm->id != module->id)
	continue;
      
      if (!module->id == EXTERN_MODULE || 
	  (module->name && pm->name && strequal (module->name, pm->name)))
	{
	  gtk_option_menu_set_history (GTK_OPTION_MENU (mdf.module_option_menu), 
				       i);
	  module_id_changed ();
	  break;
	}
    }
  
  /* signals */
  g_signal_connect_swapped (mdf.module_option_menu, "changed",
			    G_CALLBACK (module_id_changed), NULL);
    
  g_signal_connect_swapped (mdf.configure_button, "clicked",
			    G_CALLBACK (configure_click_cb), NULL);

  gtk_widget_show_all (mdf.frame);
}

typedef struct
{
  gboolean is_icon;
  GtkWidget *icon_radio_button;
  GtkWidget *module_radio_button;
  GtkWidget *notebook;
}
PanelGroupDialog;

static PanelGroupDialog pgd;

static void
item_type_changed (GtkToggleButton * b, gpointer data)
{
  if (gtk_toggle_button_get_active (b))
    gtk_notebook_set_current_page (GTK_NOTEBOOK (pgd.notebook), 0);
  else
    gtk_notebook_set_current_page (GTK_NOTEBOOK (pgd.notebook), 1);
}

void
edit_item_or_module_dlg (PanelGroup * pg)
{
  PanelItem *pi;
  PanelModule *pm;
  GtkWidget *dialog;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *separator;
  int response;
  gboolean was_item = TRUE;

  dialog = gtk_dialog_new_with_buttons (_("Change item"), NULL,
					GTK_DIALOG_MODAL,
					GTK_STOCK_CANCEL,
					RESPONSE_CHANGE, 
					GTK_STOCK_APPLY,
					RESPONSE_CHANGE,
					NULL);


  if (!pg->item)
    {
      was_item = FALSE;
      pi = panel_item_new (NULL);
      pi->id = -1;
      create_panel_item (pi);
    }
  else
    pi = pg->item;

  pm = pg->module;

  pgd.is_icon = (pg->type == ICON);

  /* radio buttons */
  hbox = gtk_hbox_new (FALSE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
		      hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("Item type:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  pgd.icon_radio_button =
    gtk_radio_button_new_with_mnemonic (NULL, _("_Icon"));
  gtk_box_pack_start (GTK_BOX (hbox), pgd.icon_radio_button, FALSE, FALSE, 0);

  pgd.module_radio_button =
    gtk_radio_button_new_with_mnemonic_from_widget (GTK_RADIO_BUTTON
						    (pgd.icon_radio_button),
						    _("_Module"));
  gtk_box_pack_start (GTK_BOX (hbox), pgd.module_radio_button, FALSE, FALSE,
		      0);

  gtk_widget_show_all (hbox);

  separator = gtk_hseparator_new ();
  gtk_widget_show (separator);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
		      separator, FALSE, FALSE, 0);

  /* notebook */
  pgd.notebook = gtk_notebook_new ();
  gtk_widget_show (pgd.notebook);
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (pgd.notebook), FALSE);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (pgd.notebook), FALSE);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox),
		      pgd.notebook, FALSE, FALSE, 0);

  /* item page */
  create_item_dialog_frame (pi, 0);
  gtk_notebook_append_page (GTK_NOTEBOOK (pgd.notebook), idf.frame, NULL);

  /* module page */
  create_module_dialog_frame (pm);
  gtk_notebook_append_page (GTK_NOTEBOOK (pgd.notebook), mdf.frame, NULL);

  /* signals */
  g_signal_connect (pgd.icon_radio_button, "toggled",
		    G_CALLBACK (item_type_changed), NULL);

  if (pgd.is_icon)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pgd.icon_radio_button),
				  TRUE);
  else
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pgd.module_radio_button),
				    TRUE);
      gtk_notebook_set_current_page (GTK_NOTEBOOK (pgd.notebook), 1);
    }
  
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);

  response = gtk_dialog_run (GTK_DIALOG (dialog));

  if (response == RESPONSE_CHANGE)
    {
      if (!was_item)
	panel_item_free (pi);
    }
  else if (!was_item)
    panel_item_free (pi);

  gtk_widget_destroy (dialog);
  g_object_unref (idf.pb);
}
