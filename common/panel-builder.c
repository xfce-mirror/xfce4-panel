/*
 * Copyright (C) 2009 Nick Schermer <nick@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <common/panel-private.h>
#include <common/panel-builder.h>



GtkBuilder *
panel_builder_new (XfcePanelPlugin  *panel_plugin,
                   const gchar      *buffer,
                   gsize             length,
                   GObject         **dialog_return)
{
  GError     *error = NULL;
  GtkBuilder *builder;
  GObject    *dialog, *button;

  panel_return_val_if_fail (XFCE_IS_PANEL_PLUGIN (panel_plugin), NULL);

  builder = gtk_builder_new ();
  if (gtk_builder_add_from_string (builder, buffer, length, &error))
    {
      dialog = gtk_builder_get_object (builder, "dialog");
      if (G_LIKELY (dialog != NULL))
        {
          g_object_weak_ref (G_OBJECT (dialog),
                             (GWeakNotify) g_object_unref, builder);
          xfce_panel_plugin_take_window (panel_plugin, GTK_WINDOW (dialog));

          xfce_panel_plugin_block_menu (panel_plugin);
          g_object_weak_ref (G_OBJECT (dialog),
                             (GWeakNotify) xfce_panel_plugin_unblock_menu,
                             panel_plugin);

          button = gtk_builder_get_object (builder, "close-button");
          if (G_LIKELY (button != NULL))
            g_signal_connect_swapped (G_OBJECT (button), "clicked",
                                      G_CALLBACK (gtk_widget_destroy), dialog);

          if (G_LIKELY (dialog_return != NULL))
            *dialog_return = dialog;

          return builder;
        }
      else
        {
          g_set_error_literal (&error, 0, 0, "No widget with the name \"dialog\" found");
        }
    }

  g_critical ("Faild to construct the builder for plugin %s-%d: %s.",
              xfce_panel_plugin_get_name (panel_plugin),
              xfce_panel_plugin_get_unique_id (panel_plugin),
              error->message);
  g_error_free (error);
  g_object_unref (G_OBJECT (builder));

  return NULL;
}

