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


typedef struct
{
  XfconfChannel *channel;
  gchar         *channel_prop;

  GObject       *object;
  gchar         *object_prop;
} PropertyBinding;



static void panel_properties_object_notify     (GObject       *object,
                                                GParamSpec    *pspec,
                                                gpointer       user_data);
static void panel_properties_object_disconnect (gpointer       user_data,
                                                GClosure      *closure);
static void panel_properties_channel_notify    (XfconfChannel *channel,
                                                const gchar   *property,
                                                const GValue  *value,
                                                gpointer       user_data);
static void panel_properties_channel_destroyed (gpointer       user_data,
                                                GObject       *where_the_channel_was);



/* shared table which is used to speed up the panel startup */
static GHashTable *shared_hash_table = NULL;



static void
panel_properties_object_notify (GObject    *object,
                                GParamSpec *pspec,
                                gpointer    user_data)
{
  GValue           value = { 0, };
  PropertyBinding *binding = user_data;

  panel_return_if_fail (G_IS_OBJECT (object));
  panel_return_if_fail (binding->object == object);
  panel_return_if_fail (XFCONF_IS_CHANNEL (binding->channel));

  /* get the property from the object */
  g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
  g_object_get_property (object, g_param_spec_get_name (pspec), &value);

  /* set the xfconf property, unfortunately function blocking is
   * pointless there because dbus is async. so we will receive
   * a property-changed from the channel. */
  xfconf_channel_set_property (binding->channel, binding->channel_prop,
                               &value);

  /* cleanup */
  g_value_unset (&value);
}



static void
panel_properties_object_disconnect (gpointer  user_data,
                                    GClosure *closure)
{
  PropertyBinding *binding = user_data;

  panel_return_if_fail (XFCONF_IS_CHANNEL (binding->channel));

  /* disconnect from the channel */
  if (g_signal_handlers_disconnect_by_func (G_OBJECT (binding->channel),
          panel_properties_channel_notify, binding) > 0)
    {
      g_object_weak_unref (G_OBJECT (binding->channel),
          panel_properties_channel_destroyed, binding);
    }

  /* cleanup */
  g_free (binding->channel_prop);
  g_free (binding->object_prop);
  g_slice_free (PropertyBinding, binding);
}



static void
panel_properties_channel_notify (XfconfChannel *channel,
                                 const gchar   *property,
                                 const GValue  *value,
                                 gpointer       user_data)
{
  PropertyBinding *binding = user_data;
  GParamSpec      *pspec;
  GValue           def_value = { 0, };

  panel_return_if_fail (XFCONF_IS_CHANNEL (channel));
  panel_return_if_fail (binding->channel == channel);
  panel_return_if_fail (G_IS_OBJECT (binding->object));

  /* for a property reset, set the default value */
  if (G_VALUE_TYPE (value) == G_TYPE_INVALID)
    {
      pspec = g_object_class_find_property(G_OBJECT_GET_CLASS (binding->object),
                                           binding->object_prop);
      panel_return_if_fail (pspec == NULL);
      if (G_UNLIKELY (pspec == NULL))
        return;

      g_param_value_set_default (pspec, &def_value);
      value = &def_value;
    }

  /* block property notify */
  g_signal_handlers_block_by_func (G_OBJECT (binding->object),
      G_CALLBACK (panel_properties_object_notify), binding);

  /* set the property */
  g_object_set_property (binding->object, binding->object_prop, value);

  /* unblock property notify */
  g_signal_handlers_unblock_by_func (G_OBJECT (binding->object),
      G_CALLBACK (panel_properties_object_notify), binding);

  /* cleanup */
  if (G_UNLIKELY (value == &def_value))
    g_value_unset (&def_value);
}



static void
panel_properties_channel_destroyed (gpointer  user_data,
                                    GObject  *where_the_channel_was)
{
  PropertyBinding *binding = user_data;

  panel_return_if_fail (binding->channel == (XfconfChannel *) where_the_channel_was);
  panel_return_if_fail (G_IS_OBJECT (binding->object));

  /* disconnect from the object, which will take
   * care of freeing everything else */
  g_signal_handlers_disconnect_by_func (G_OBJECT (binding->object),
      panel_properties_object_notify, binding);
}



XfconfChannel *
panel_properties_get_channel (void)
{
  static XfconfChannel *channel = NULL;

  if (G_UNLIKELY (channel == NULL))
    {
      channel = xfconf_channel_new (XFCE_PANEL_PLUGIN_CHANNEL_NAME);
      g_object_add_weak_pointer (G_OBJECT (channel), (gpointer) &channel);
    }
  else
    {
      g_object_ref (G_OBJECT (channel));
    }

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
  const GValue        *value;
  gchar                buf[512];
  PropertyBinding     *binding;
  GHashTable          *hash_table;

  panel_return_if_fail (channel == NULL || XFCONF_IS_CHANNEL (channel));
  panel_return_if_fail (G_IS_OBJECT (object));
  panel_return_if_fail (property_base != NULL && *property_base == '/');
  panel_return_if_fail (properties != NULL);

  /* get or ref the hash table */
  if (save_properties)
    hash_table = NULL;
  else if (shared_hash_table != NULL)
    hash_table = g_hash_table_ref (shared_hash_table);
  else
    hash_table = xfconf_channel_get_properties (channel, property_base);

  if (G_LIKELY (channel == NULL))
    {
      /* use the default channel if none is set by the user and take
       * care of refcounting */
      channel = panel_properties_get_channel ();
      g_object_weak_ref (G_OBJECT (object), (GWeakNotify) g_object_unref, channel);
    }

  /* walk the properties array */
  for (prop = properties; prop->property != NULL; prop++)
    {
      /* create a new binding */
      binding = g_slice_new (PropertyBinding);
      binding->channel = channel;
      binding->channel_prop = g_strconcat (property_base, "/", prop->property, NULL);
      binding->object = object;
      binding->object_prop = g_strdup (prop->property);

      /* lookup the property value */
      if (hash_table != NULL)
        {
          value = g_hash_table_lookup (hash_table, binding->channel_prop);
          if (G_IS_VALUE (value))
            {
              if (G_LIKELY (G_VALUE_TYPE (value) == prop->type))
                g_object_set_property (object, prop->property, value);
              else
                g_message ("Value types of property \"%s\" do not "
                           "match: channel = %s, property = %s", buf,
                           G_VALUE_TYPE_NAME (value),
                           g_type_name (prop->type));
            }
        }

      /* monitor object property changes */
      g_snprintf (buf, sizeof (buf), "notify::%s", prop->property);
      g_signal_connect_data (G_OBJECT (object), buf,
          G_CALLBACK (panel_properties_object_notify), binding,
          panel_properties_object_disconnect, 0);

      /* emit the property so it gets saved */
      if (save_properties)
        g_object_notify (G_OBJECT (object), prop->property);

      /* monitor channel changes */
      g_snprintf (buf, sizeof (buf), "property-changed::%s", binding->channel_prop);
      g_object_weak_ref (G_OBJECT (channel), panel_properties_channel_destroyed, binding);
      g_signal_connect (G_OBJECT (channel), buf,
          G_CALLBACK (panel_properties_channel_notify), binding);
    }

  /* cleanup */
  if (G_LIKELY (hash_table != NULL))
    g_hash_table_unref (hash_table);
}



void
panel_properties_unbind (GObject *object)
{
  panel_return_if_fail (G_IS_OBJECT (object));
  panel_return_if_fail (!XFCONF_IS_CHANNEL (object));

  /* disconnect the notify functions */
  g_signal_handlers_disconnect_matched (object, G_SIGNAL_MATCH_FUNC,
      0, 0, NULL, G_CALLBACK (panel_properties_object_notify), NULL);
}



void
panel_properties_shared_hash_table (GHashTable *hash_table)
{
  /* release previous table */
  if (shared_hash_table != NULL)
    g_hash_table_unref (shared_hash_table);

  /* set new table */
  if (hash_table != NULL)
    shared_hash_table = g_hash_table_ref (hash_table);
  else
    shared_hash_table = NULL;
}
