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


static void
launcher_dialog_builder_died (gpointer user_data, GObject *where_object_was)
{
  g_message ("builder destroyed");
}



void
launcher_dialog_show (LauncherPlugin *plugin)
{
  GtkBuilder *builder;
  GObject    *dialog;
  GObject    *object, *menu;
  
  panel_return_if_fail (XFCE_IS_LAUNCHER_PLUGIN (plugin));
  
  builder = gtk_builder_new ();
  g_object_weak_ref (G_OBJECT (builder), launcher_dialog_builder_died, NULL);
  if (gtk_builder_add_from_string (builder, launcher_dialog_glade, launcher_dialog_glade_length, NULL))
    {
      dialog = gtk_builder_get_object (builder, "dialog");
      g_object_weak_ref (G_OBJECT (dialog), (GWeakNotify) g_object_unref, builder);
      gtk_widget_show (GTK_WIDGET (dialog));
      
      object = gtk_builder_get_object (builder, "close-button");
      g_signal_connect_swapped (G_OBJECT (object), "clicked", G_CALLBACK (gtk_widget_destroy), dialog);
      
      object = gtk_builder_get_object (builder, "entry-add");
      menu = gtk_builder_get_object (builder, "add-menu");
      g_signal_connect (G_OBJECT (object), "clicked", G_CALLBACK (launcher_dialog_add_button_clicked), GTK_MENU (menu));
    }
}
