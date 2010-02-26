/* $Id$ */
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

#include <common/panel-xfconf.h>
#include <libxfce4panel/xfce-panel-macros.h>



XfconfChannel *
panel_properties_get_channel (GObject *object_for_weak_ref)
{
  GError        *error = NULL;
  XfconfChannel *channel;

  panel_return_val_if_fail (G_IS_OBJECT (object_for_weak_ref), NULL);

  if (!xfconf_init (&error))
    {
      g_critical ("Failed to initialize Xfconf: %s", error->message);
      g_error_free (error);
      return NULL;
    }

  channel = xfconf_channel_get (XFCE_PANEL_PLUGIN_CHANNEL_NAME);
  g_object_weak_ref (object_for_weak_ref, (GWeakNotify) xfconf_shutdown, NULL);

  return channel;
}



void
panel_properties_bind (XfconfChannel       *channel,
                       GObject             *object,
                       const gchar         *property_base,
                       const PanelProperty *properties,
                       gboolean             save_properties)
{
  const PanelProperty *prop;
  gchar               *property;

  panel_return_if_fail (channel == NULL || XFCONF_IS_CHANNEL (channel));
  panel_return_if_fail (G_IS_OBJECT (object));
  panel_return_if_fail (property_base != NULL && *property_base == '/');
  panel_return_if_fail (properties != NULL);

  if (G_LIKELY (channel == NULL))
    channel = panel_properties_get_channel (object);
  panel_return_if_fail (XFCONF_IS_CHANNEL (channel));

  /* walk the properties array */
  for (prop = properties; prop->property != NULL; prop++)
    {
      property = g_strconcat (property_base, "/", prop->property, NULL);
      xfconf_g_property_bind (channel, property, prop->type, object, prop->property);
      g_free (property);
    }
}



void
panel_properties_unbind (GObject *object)
{
  xfconf_g_property_unbind_all (object);
}
