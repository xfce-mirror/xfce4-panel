/*
 * Copyright (C) 2009-2010 Nick Schermer <nick@xfce.org>
 *
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

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#include <libxfce4ui/libxfce4ui.h>
#include <exo/exo.h>

#include <common/panel-private.h>
#include <common/panel-utils.h>



static void
panel_utils_help_button_clicked (GtkWidget       *button,
                                 XfcePanelPlugin *panel_plugin)
{
  GtkWidget *toplevel;

  panel_return_if_fail (XFCE_IS_PANEL_PLUGIN (panel_plugin));
  panel_return_if_fail (GTK_IS_WIDGET (button));

  toplevel = gtk_widget_get_toplevel (button);
  panel_utils_show_help (GTK_WINDOW (toplevel),
      xfce_panel_plugin_get_name (panel_plugin),
      "properties");
}



GtkBuilder *
panel_utils_builder_new (XfcePanelPlugin  *panel_plugin,
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

          button = gtk_builder_get_object (builder, "help-button");
          if (G_LIKELY (button != NULL))
            g_signal_connect (G_OBJECT (button), "clicked",
                G_CALLBACK (panel_utils_help_button_clicked), panel_plugin);

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



void
panel_utils_show_help (GtkWindow   *parent,
                       const gchar *page,
                       const gchar *offset)
{
  GdkScreen *screen;
  gchar     *filename;
  gchar     *uri = NULL;
  GError    *error = NULL;
  gchar     *locale;
  gboolean   exists;

  panel_return_if_fail (parent == NULL || GTK_IS_WINDOW (parent));

  if (G_LIKELY (parent != NULL))
    screen = gtk_window_get_screen (parent);
  else
    screen = gdk_screen_get_default ();

  if (page == NULL)
    page = "index";

  /* get the locale of the user */
  locale = g_strdup (setlocale (LC_MESSAGES, NULL));
  if (G_LIKELY (locale != NULL))
    locale = g_strdelimit (locale, "._", '\0');
  else
    locale = g_strdup ("C");

  /* check if the help page exists on the system */
  filename = g_strconcat (HELPDIR, G_DIR_SEPARATOR_S, locale,
                          G_DIR_SEPARATOR_S, page, ".html", NULL);
  exists = g_file_test (filename, G_FILE_TEST_EXISTS);
  if (!exists)
    {
      g_free (filename);
      filename = g_strconcat (HELPDIR, G_DIR_SEPARATOR_S "C"
                              G_DIR_SEPARATOR_S, page, ".html", NULL);
      exists = g_file_test (filename, G_FILE_TEST_EXISTS);
    }

  if (G_LIKELY (exists))
    uri = g_strconcat ("file://", filename, offset != NULL ? "#" : NULL, offset, NULL);
  else if (xfce_dialog_confirm (parent, "web-browser", _("_Read Online"),
               _("You can read the user manual online. This manual may however "
                 "not exactly match your panel version."),
               _("The user manual is not installed on your computer")))
    uri = g_strconcat ("http://foo-projects.org/~nick/docs/xfce4-panel/?lang=",
                       locale, "&page=", page, "&offset=", offset, NULL);

  g_free (filename);
  g_free (locale);

  /* try to run the documentation browser */
  if (uri != NULL
      && !exo_execute_preferred_application_on_screen ("WebBrowser", uri, NULL,
                                                       NULL, screen, &error))
    {
      /* display an error message to the user */
      xfce_dialog_show_error (parent, error, _("Failed to open the documentation browser"));
      g_error_free (error);
    }

  g_free (uri);
}



gboolean
panel_utils_grab_available (void)
{
  GdkScreen     *screen;
  GdkWindow     *root;
  GdkGrabStatus  grab_pointer = GDK_GRAB_FROZEN;
  GdkGrabStatus  grab_keyboard = GDK_GRAB_FROZEN;
  gboolean       grab_succeed = FALSE;
  guint          i;
  GdkEventMask   pointer_mask = GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
                                | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK
                                | GDK_POINTER_MOTION_MASK;

  screen = xfce_gdk_screen_get_active (NULL);
  root = gdk_screen_get_root_window (screen);

  /* don't try to get the grab for longer then 1/4 second */
  for (i = 0; i < (G_USEC_PER_SEC / 100 / 4); i++)
    {
      grab_keyboard = gdk_keyboard_grab (root, TRUE, GDK_CURRENT_TIME);
      if (grab_keyboard == GDK_GRAB_SUCCESS)
        {
          grab_pointer = gdk_pointer_grab (root, TRUE, pointer_mask,
                                           NULL, NULL, GDK_CURRENT_TIME);
          if (grab_pointer == GDK_GRAB_SUCCESS)
            {
              grab_succeed = TRUE;
              break;
            }
        }

      g_usleep (100);
    }

  /* release the grab so the gtk_menu_popup() can take it */
  if (grab_pointer == GDK_GRAB_SUCCESS)
    gdk_pointer_ungrab (GDK_CURRENT_TIME);
  if (grab_keyboard == GDK_GRAB_SUCCESS)
    gdk_keyboard_ungrab (GDK_CURRENT_TIME);

  if (!grab_succeed)
    {
      g_printerr (PACKAGE_NAME ": Unable to get keyboard and mouse "
                  "grab. Menu popup failed.\n");
    }

  return grab_succeed;
}



void
panel_utils_set_atk_info (GtkWidget   *widget,
                          const gchar *name,
                          const gchar *description)
{
  static gboolean  initialized = FALSE;
  static gboolean  atk_enabled = TRUE;
  AtkObject       *object;

  panel_return_if_fail (GTK_IS_WIDGET (widget));

  if (atk_enabled)
    {
      object = gtk_widget_get_accessible (widget);

      if (!initialized)
        {
          initialized = TRUE;
          atk_enabled = GTK_IS_ACCESSIBLE (object);

          if (!atk_enabled)
            return;
        }

      if (name != NULL)
        atk_object_set_name (object, name);

      if (description != NULL)
        atk_object_set_description (object, description);
    }
}
