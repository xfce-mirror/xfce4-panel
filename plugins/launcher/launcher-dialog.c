/* $Id$ */
/*
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <exo/exo.h>
#include <libxfce4ui/libxfce4ui.h>

#include "launcher.h"
#include "launcher-dialog.h"
#include "launcher-dialog_glade.h"


static void
launcher_dialog_add_button_clicked (GtkWidget *button,
                                    GtkWidget *menu)
{
  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 1,
                  gtk_get_current_event_time());
}



void
launcher_dialog_show (LauncherPlugin *plugin)
{
  GtkBuilder *builder;
  GObject    *dialog;
  GObject    *object, *menu;

  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin));

  builder = gtk_builder_new ();
  if (gtk_builder_add_from_string (builder, launcher_dialog_glade, launcher_dialog_glade_length, NULL))
    {
      dialog = gtk_builder_get_object (builder, "dialog");
      g_object_weak_ref (G_OBJECT (dialog), (GWeakNotify) g_object_unref, builder);

      object = gtk_builder_get_object (builder, "close-button");
      g_signal_connect_swapped (G_OBJECT (object), "clicked", G_CALLBACK (gtk_widget_destroy), dialog);

      object = gtk_builder_get_object (builder, "entry-add");
      menu = gtk_builder_get_object (builder, "add-menu");
      g_signal_connect (G_OBJECT (object), "clicked", G_CALLBACK (launcher_dialog_add_button_clicked), GTK_MENU (menu));

      /* connect binding to the advanced properties */
      object = gtk_builder_get_object (builder, "disable-tooltips");
      xfconf_g_property_bind (plugin->channel, "/disable-tooltips", G_TYPE_BOOLEAN, object, "active");

      object = gtk_builder_get_object (builder, "show-labels");
      xfconf_g_property_bind (plugin->channel, "/show-labels", G_TYPE_BOOLEAN, object, "active");

      object = gtk_builder_get_object (builder, "move-first");
      xfconf_g_property_bind (plugin->channel, "/move-first", G_TYPE_BOOLEAN, object, "active");

      object = gtk_builder_get_object (builder, "arrow-position");
      xfconf_g_property_bind (plugin->channel, "/arrow-position", G_TYPE_UINT, object, "active");

      /* TODO remove when implemented by glade */
      GtkCellRenderer *cell1 = gtk_cell_renderer_text_new ();
      object = gtk_builder_get_object (builder, "arrow-position");
      gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (object), cell1, TRUE);
      gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (object), cell1, "text", 0, NULL);

      gtk_widget_show (GTK_WIDGET (dialog));
    }
  else
    {
      /* release the builder */
      g_object_unref (G_OBJECT (builder));
    }
}
