/*
 *  Copyright (c) 2013 Andrzej Radecki <andrzejr@xfce.org>
 *  Copyright (c) 2017 Viktor Odintsev <ninetls@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */



#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "sn-config.h"
#include "sn-plugin.h"

#include "common/panel-debug.h"
#include "common/panel-xfconf.h"

#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4util/libxfce4util.h>
#include <xfconf/xfconf.h>



static void
sn_config_finalize (GObject *object);

static void
sn_config_get_property (GObject *object,
                        guint prop_id,
                        GValue *value,
                        GParamSpec *pspec);

static void
sn_config_set_property (GObject *object,
                        guint prop_id,
                        const GValue *value,
                        GParamSpec *pspec);



struct _SnConfig
{
  GObject __parent__;

  gint icon_size;
  gboolean single_row;
  gboolean square_icons;
  gboolean symbolic_icons;
  gboolean menu_is_primary;
  gboolean hide_new_items;
  GList *known_items[N_SN_ITEM_TYPES];
  GHashTable *hidden_items[N_SN_ITEM_TYPES];

  /* not xfconf properties but it is still convenient to have them here */
  GtkOrientation orientation;
  GtkOrientation panel_orientation;
  gint nrows;
  gint panel_size;
  gint panel_icon_size;
};

G_DEFINE_FINAL_TYPE (SnConfig, sn_config, G_TYPE_OBJECT)



enum
{
  PROP_0,
  PROP_ICON_SIZE,
  PROP_SINGLE_ROW,
  PROP_SQUARE_ICONS,
  PROP_SYMBOLIC_ICONS,
  PROP_MENU_IS_PRIMARY,
  PROP_HIDE_NEW_ITEMS,
  PROP_KNOWN_ITEMS,
  PROP_HIDDEN_ITEMS,
  PROP_KNOWN_LEGACY_ITEMS,
  PROP_HIDDEN_LEGACY_ITEMS
};

enum
{
  CONFIGURATION_CHANGED,
  ITEM_LIST_CHANGED,
  COLLECT_KNOWN_ITEMS,
  LEGACY_ITEM_LIST_CHANGED,
  ICONS_CHANGED,
  LAST_SIGNAL
};

static guint sn_config_signals[LAST_SIGNAL] = { 0 };



static void
sn_config_class_init (SnConfigClass *klass)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = sn_config_finalize;
  object_class->get_property = sn_config_get_property;
  object_class->set_property = sn_config_set_property;

  g_object_class_install_property (object_class,
                                   PROP_ICON_SIZE,
                                   g_param_spec_int ("icon-size", NULL, NULL,
                                                     0, 64, DEFAULT_ICON_SIZE,
                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
                                   PROP_SINGLE_ROW,
                                   g_param_spec_boolean ("single-row", NULL, NULL,
                                                         DEFAULT_SINGLE_ROW,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
                                   PROP_SQUARE_ICONS,
                                   g_param_spec_boolean ("square-icons", NULL, NULL,
                                                         DEFAULT_SQUARE_ICONS,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
                                   PROP_SYMBOLIC_ICONS,
                                   g_param_spec_boolean ("symbolic-icons", NULL, NULL,
                                                         DEFAULT_SYMBOLIC_ICONS,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
                                   PROP_MENU_IS_PRIMARY,
                                   g_param_spec_boolean ("menu-is-primary", NULL, NULL,
                                                         DEFAULT_MENU_IS_PRIMARY,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
                                   PROP_HIDE_NEW_ITEMS,
                                   g_param_spec_boolean ("hide-new-items", NULL, NULL,
                                                         DEFAULT_HIDE_NEW_ITEMS,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
                                   PROP_KNOWN_ITEMS,
                                   g_param_spec_boxed ("known-items",
                                                       NULL, NULL,
                                                       G_TYPE_PTR_ARRAY,
                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
                                   PROP_HIDDEN_ITEMS,
                                   g_param_spec_boxed ("hidden-items",
                                                       NULL, NULL,
                                                       G_TYPE_PTR_ARRAY,
                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
                                   PROP_KNOWN_LEGACY_ITEMS,
                                   g_param_spec_boxed ("known-legacy-items",
                                                       NULL, NULL,
                                                       G_TYPE_PTR_ARRAY,
                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
                                   PROP_HIDDEN_LEGACY_ITEMS,
                                   g_param_spec_boxed ("hidden-legacy-items",
                                                       NULL, NULL,
                                                       G_TYPE_PTR_ARRAY,
                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  sn_config_signals[CONFIGURATION_CHANGED] = g_signal_new (g_intern_static_string ("configuration-changed"),
                                                           G_TYPE_FROM_CLASS (object_class),
                                                           G_SIGNAL_RUN_LAST,
                                                           0, NULL, NULL,
                                                           g_cclosure_marshal_VOID__VOID,
                                                           G_TYPE_NONE, 0);

  sn_config_signals[ICONS_CHANGED] = g_signal_new (g_intern_static_string ("icons-changed"),
                                                   G_TYPE_FROM_CLASS (object_class),
                                                   G_SIGNAL_RUN_LAST,
                                                   0, NULL, NULL,
                                                   g_cclosure_marshal_VOID__VOID,
                                                   G_TYPE_NONE, 0);

  sn_config_signals[ITEM_LIST_CHANGED] = g_signal_new (g_intern_static_string ("items-list-changed"),
                                                       G_TYPE_FROM_CLASS (object_class),
                                                       G_SIGNAL_RUN_LAST,
                                                       0, NULL, NULL,
                                                       g_cclosure_marshal_VOID__VOID,
                                                       G_TYPE_NONE, 0);

  sn_config_signals[COLLECT_KNOWN_ITEMS] = g_signal_new (g_intern_static_string ("collect-known-items"),
                                                         G_TYPE_FROM_CLASS (object_class),
                                                         G_SIGNAL_RUN_LAST,
                                                         0, NULL, NULL,
                                                         g_cclosure_marshal_generic,
                                                         G_TYPE_NONE, 1, G_TYPE_POINTER);

  sn_config_signals[LEGACY_ITEM_LIST_CHANGED] = g_signal_new (g_intern_static_string ("legacy-items-list-changed"),
                                                              G_TYPE_FROM_CLASS (object_class),
                                                              G_SIGNAL_RUN_LAST,
                                                              0, NULL, NULL,
                                                              g_cclosure_marshal_VOID__VOID,
                                                              G_TYPE_NONE, 0);
}



static void
sn_config_init (SnConfig *config)
{
  config->icon_size = DEFAULT_ICON_SIZE;
  config->single_row = DEFAULT_SINGLE_ROW;
  config->square_icons = DEFAULT_SQUARE_ICONS;
  config->symbolic_icons = DEFAULT_SYMBOLIC_ICONS;
  config->hide_new_items = DEFAULT_HIDE_NEW_ITEMS;
  for (gint n = 0; n < N_SN_ITEM_TYPES; n++)
    {
      config->known_items[n] = NULL;
      config->hidden_items[n] = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
    }

  config->orientation = DEFAULT_ORIENTATION;
  config->panel_orientation = DEFAULT_PANEL_ORIENTATION;
  config->nrows = 1;
  config->panel_size = DEFAULT_PANEL_SIZE;
}



static void
sn_config_finalize (GObject *object)
{
  SnConfig *config = SN_CONFIG (object);

  for (gint n = 0; n < N_SN_ITEM_TYPES; n++)
    {
      g_list_free_full (config->known_items[n], g_free);
      g_hash_table_destroy (config->hidden_items[n]);
    }

  G_OBJECT_CLASS (sn_config_parent_class)->finalize (object);
}



static void
sn_config_free_array_element (gpointer data)
{
  GValue *value = (GValue *) data;

  g_value_unset (value);
  g_free (value);
}



static void
sn_config_collect_keys (gpointer key,
                        gpointer value,
                        gpointer array)
{
  GValue *tmp;

  tmp = g_new0 (GValue, 1);
  g_value_init (tmp, G_TYPE_STRING);
  g_value_set_string (tmp, key);
  g_ptr_array_add (array, tmp);
}



static void
sn_config_get_property (GObject *object,
                        guint prop_id,
                        GValue *value,
                        GParamSpec *pspec)
{
  SnConfig *config = SN_CONFIG (object);
  GPtrArray *array;
  GList *li;
  GValue *tmp;

  switch (prop_id)
    {
    case PROP_ICON_SIZE:
      g_value_set_int (value, config->icon_size);
      break;

    case PROP_SINGLE_ROW:
      g_value_set_boolean (value, config->single_row);
      break;

    case PROP_SQUARE_ICONS:
      g_value_set_boolean (value, config->square_icons);
      break;

    case PROP_SYMBOLIC_ICONS:
      g_value_set_boolean (value, config->symbolic_icons);
      break;

    case PROP_MENU_IS_PRIMARY:
      g_value_set_boolean (value, config->menu_is_primary);
      break;

    case PROP_HIDE_NEW_ITEMS:
      g_value_set_boolean (value, config->hide_new_items);
      break;

    case PROP_KNOWN_ITEMS:
      array = g_ptr_array_new_full (1, sn_config_free_array_element);
      for (li = config->known_items[SN_ITEM_TYPE_DEFAULT]; li != NULL; li = li->next)
        {
          tmp = g_new0 (GValue, 1);
          g_value_init (tmp, G_TYPE_STRING);
          g_value_set_string (tmp, li->data);
          g_ptr_array_add (array, tmp);
        }
      g_value_set_boxed (value, array);
      g_ptr_array_unref (array);
      break;

    case PROP_HIDDEN_ITEMS:
      array = g_ptr_array_new_full (1, sn_config_free_array_element);
      g_hash_table_foreach (config->hidden_items[SN_ITEM_TYPE_DEFAULT], sn_config_collect_keys, array);
      g_value_set_boxed (value, array);
      g_ptr_array_unref (array);
      break;

    case PROP_KNOWN_LEGACY_ITEMS:
      array = g_ptr_array_new_full (1, sn_config_free_array_element);
      for (li = config->known_items[SN_ITEM_TYPE_LEGACY]; li != NULL; li = li->next)
        {
          tmp = g_new0 (GValue, 1);
          g_value_init (tmp, G_TYPE_STRING);
          g_value_set_string (tmp, li->data);
          g_ptr_array_add (array, tmp);
        }
      g_value_set_boxed (value, array);
      g_ptr_array_unref (array);
      break;

    case PROP_HIDDEN_LEGACY_ITEMS:
      array = g_ptr_array_new_full (1, sn_config_free_array_element);
      g_hash_table_foreach (config->hidden_items[SN_ITEM_TYPE_LEGACY], sn_config_collect_keys, array);
      g_value_set_boxed (value, array);
      g_ptr_array_unref (array);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
sn_config_set_property (GObject *object,
                        guint prop_id,
                        const GValue *value,
                        GParamSpec *pspec)
{
  SnConfig *config = SN_CONFIG (object);
  gint val;
  GPtrArray *array;
  const GValue *tmp;
  gchar *name;
  guint i;

  switch (prop_id)
    {
    case PROP_ICON_SIZE:
      val = g_value_get_int (value);
      if (config->icon_size != val)
        {
          config->icon_size = val;
          g_signal_emit (G_OBJECT (config), sn_config_signals[ICONS_CHANGED], 0);
          g_signal_emit (G_OBJECT (config), sn_config_signals[CONFIGURATION_CHANGED], 0);
        }
      break;

    case PROP_SINGLE_ROW:
      val = g_value_get_boolean (value);
      if (config->single_row != val)
        {
          config->single_row = val;
          g_signal_emit (G_OBJECT (config), sn_config_signals[CONFIGURATION_CHANGED], 0);
        }
      break;

    case PROP_SQUARE_ICONS:
      val = g_value_get_boolean (value);
      if (config->square_icons != val)
        {
          config->square_icons = val;
          g_signal_emit (G_OBJECT (config), sn_config_signals[CONFIGURATION_CHANGED], 0);
        }
      break;

    case PROP_SYMBOLIC_ICONS:
      val = g_value_get_boolean (value);
      if (config->symbolic_icons != val)
        {
          config->symbolic_icons = val;
          g_signal_emit (G_OBJECT (config), sn_config_signals[CONFIGURATION_CHANGED], 0);
        }
      break;

    case PROP_MENU_IS_PRIMARY:
      val = g_value_get_boolean (value);
      if (config->menu_is_primary != val)
        {
          config->menu_is_primary = val;
          g_signal_emit (G_OBJECT (config), sn_config_signals[CONFIGURATION_CHANGED], 0);
        }
      break;

    case PROP_HIDE_NEW_ITEMS:
      val = g_value_get_boolean (value);
      if (config->hide_new_items != val)
        {
          config->hide_new_items = val;
          g_signal_emit (G_OBJECT (config), sn_config_signals[ITEM_LIST_CHANGED], 0);
          g_signal_emit (G_OBJECT (config), sn_config_signals[LEGACY_ITEM_LIST_CHANGED], 0);
        }
      break;

    case PROP_KNOWN_ITEMS:
      g_list_free_full (config->known_items[SN_ITEM_TYPE_DEFAULT], g_free);
      config->known_items[SN_ITEM_TYPE_DEFAULT] = NULL;
      array = g_value_get_boxed (value);
      if (G_LIKELY (array != NULL))
        {
          for (i = 0; i < array->len; i++)
            {
              tmp = g_ptr_array_index (array, i);
              g_assert (G_VALUE_HOLDS_STRING (tmp));
              name = g_value_dup_string (tmp);
              config->known_items[SN_ITEM_TYPE_DEFAULT] =
                g_list_append (config->known_items[SN_ITEM_TYPE_DEFAULT], name);
            }
        }
      g_signal_emit (G_OBJECT (config), sn_config_signals[ITEM_LIST_CHANGED], 0);
      break;

    case PROP_HIDDEN_ITEMS:
      g_hash_table_remove_all (config->hidden_items[SN_ITEM_TYPE_DEFAULT]);
      array = g_value_get_boxed (value);
      if (G_LIKELY (array != NULL))
        {
          for (i = 0; i < array->len; i++)
            {
              tmp = g_ptr_array_index (array, i);
              g_assert (G_VALUE_HOLDS_STRING (tmp));
              name = g_value_dup_string (tmp);
              g_hash_table_replace (config->hidden_items[SN_ITEM_TYPE_DEFAULT], name, name);
            }
        }
      g_signal_emit (G_OBJECT (config), sn_config_signals[ITEM_LIST_CHANGED], 0);
      break;

    case PROP_KNOWN_LEGACY_ITEMS:
      g_list_free_full (config->known_items[SN_ITEM_TYPE_LEGACY], g_free);
      config->known_items[SN_ITEM_TYPE_LEGACY] = NULL;
      array = g_value_get_boxed (value);
      if (G_LIKELY (array != NULL))
        {
          for (i = 0; i < array->len; i++)
            {
              tmp = g_ptr_array_index (array, i);
              g_assert (G_VALUE_HOLDS_STRING (tmp));
              name = g_value_dup_string (tmp);
              config->known_items[SN_ITEM_TYPE_LEGACY] =
                g_list_append (config->known_items[SN_ITEM_TYPE_LEGACY], name);
            }
        }
      g_signal_emit (G_OBJECT (config), sn_config_signals[LEGACY_ITEM_LIST_CHANGED], 0);
      break;

    case PROP_HIDDEN_LEGACY_ITEMS:
      g_hash_table_remove_all (config->hidden_items[SN_ITEM_TYPE_LEGACY]);
      array = g_value_get_boxed (value);
      if (G_LIKELY (array != NULL))
        {
          for (i = 0; i < array->len; i++)
            {
              tmp = g_ptr_array_index (array, i);
              g_assert (G_VALUE_HOLDS_STRING (tmp));
              name = g_value_dup_string (tmp);
              g_hash_table_replace (config->hidden_items[SN_ITEM_TYPE_LEGACY], name, name);
            }
        }
      g_signal_emit (G_OBJECT (config), sn_config_signals[LEGACY_ITEM_LIST_CHANGED], 0);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



gint
sn_config_get_icon_size (SnConfig *config)
{
  g_return_val_if_fail (SN_IS_CONFIG (config), DEFAULT_ICON_SIZE);

  if (config->icon_size > 0)
    return config->icon_size;

  return config->panel_icon_size;
}



gboolean
sn_config_get_icon_size_is_automatic (SnConfig *config)
{
  g_return_val_if_fail (SN_IS_CONFIG (config), FALSE);

  return config->icon_size == 0;
}



void
sn_config_get_dimensions (SnConfig *config,
                          gint *ret_icon_size,
                          gint *ret_n_rows,
                          gint *ret_row_size,
                          gint *ret_padding)
{
  gint panel_size, config_nrows, icon_size, hx_size, hy_size, nrows, row_size, padding;
  gboolean single_row, square_icons;

  panel_size = sn_config_get_panel_size (config);
  config_nrows = sn_config_get_nrows (config);
  icon_size = sn_config_get_icon_size (config);
  single_row = sn_config_get_single_row (config);
  square_icons = sn_config_get_square_icons (config);
  if (square_icons)
    {
      nrows = single_row ? 1 : MAX (1, config_nrows);
      hx_size = hy_size = panel_size / nrows;
    }
  else
    {
      hx_size = MIN (icon_size + 2, panel_size);
      nrows = single_row ? 1 : MAX (1, panel_size / hx_size);
      hy_size = panel_size / nrows;
    }
  icon_size = MIN (icon_size, MIN (hx_size, hy_size));

  if (icon_size % 2 != 0)
    {
      icon_size--;
    }

  row_size = hy_size;
  padding = (hy_size - icon_size) / 2;

  if (square_icons)
    padding = 0;

  if (ret_icon_size != NULL)
    *ret_icon_size = icon_size;

  if (ret_n_rows != NULL)
    *ret_n_rows = nrows;

  if (ret_row_size != NULL)
    *ret_row_size = row_size;

  if (ret_padding != NULL)
    *ret_padding = padding;
}



gboolean
sn_config_get_single_row (SnConfig *config)
{
  g_return_val_if_fail (SN_IS_CONFIG (config), DEFAULT_SINGLE_ROW);

  return config->single_row;
}



gboolean
sn_config_get_square_icons (SnConfig *config)
{
  g_return_val_if_fail (SN_IS_CONFIG (config), DEFAULT_SQUARE_ICONS);

  return config->square_icons;
}



gboolean
sn_config_get_symbolic_icons (SnConfig *config)
{
  g_return_val_if_fail (SN_IS_CONFIG (config), DEFAULT_SYMBOLIC_ICONS);

  return config->symbolic_icons;
}



gboolean
sn_config_get_menu_is_primary (SnConfig *config)
{
  g_return_val_if_fail (SN_IS_CONFIG (config), DEFAULT_MENU_IS_PRIMARY);

  return config->menu_is_primary;
}



void
sn_config_set_orientation (SnConfig *config,
                           GtkOrientation panel_orientation,
                           GtkOrientation orientation)
{
  gboolean needs_update = FALSE;

  g_return_if_fail (SN_IS_CONFIG (config));

  if (config->orientation != orientation)
    {
      config->orientation = orientation;
      needs_update = TRUE;
    }

  if (config->panel_orientation != panel_orientation)
    {
      config->panel_orientation = panel_orientation;
      needs_update = TRUE;
    }

  if (needs_update)
    g_signal_emit (G_OBJECT (config), sn_config_signals[CONFIGURATION_CHANGED], 0);
}



GtkOrientation
sn_config_get_orientation (SnConfig *config)
{
  g_return_val_if_fail (SN_IS_CONFIG (config), DEFAULT_ORIENTATION);

  return config->orientation;
}



GtkOrientation
sn_config_get_panel_orientation (SnConfig *config)
{
  g_return_val_if_fail (SN_IS_CONFIG (config), DEFAULT_PANEL_ORIENTATION);

  return config->panel_orientation;
}



void
sn_config_set_size (SnConfig *config,
                    gint panel_size,
                    gint nrows,
                    gint icon_size)
{
  gboolean needs_update = FALSE;

  g_return_if_fail (SN_IS_CONFIG (config));

  if (config->nrows != nrows)
    {
      config->nrows = nrows;
      needs_update = TRUE;
    }

  if (config->panel_size != panel_size)
    {
      config->panel_size = panel_size;
      needs_update = TRUE;
    }

  if (config->panel_icon_size != icon_size)
    {
      config->panel_icon_size = icon_size;
      needs_update = TRUE;
      g_signal_emit (G_OBJECT (config), sn_config_signals[ICONS_CHANGED], 0);
    }

  if (needs_update)
    g_signal_emit (G_OBJECT (config), sn_config_signals[CONFIGURATION_CHANGED], 0);
}



gint
sn_config_get_nrows (SnConfig *config)
{
  g_return_val_if_fail (SN_IS_CONFIG (config), 1);

  return config->nrows;
}



gint
sn_config_get_panel_size (SnConfig *config)
{
  g_return_val_if_fail (SN_IS_CONFIG (config), DEFAULT_PANEL_SIZE);

  return config->panel_size;
}



gboolean
sn_config_is_hidden (SnConfig *config,
                     SnItemType type,
                     const gchar *name)
{
  g_return_val_if_fail (SN_IS_CONFIG (config), FALSE);

  return g_hash_table_lookup_extended (config->hidden_items[type], name, NULL, NULL);
}



void
sn_config_set_hidden (SnConfig *config,
                      SnItemType type,
                      const gchar *name,
                      gboolean hidden)
{
  gchar *name_copy;

  g_return_if_fail (SN_IS_CONFIG (config));

  if (hidden)
    {
      name_copy = g_strdup (name);
      g_hash_table_replace (config->hidden_items[type], name_copy, name_copy);
    }
  else
    {
      g_hash_table_remove (config->hidden_items[type], name);
    }

  if (type == SN_ITEM_TYPE_DEFAULT)
    {
      g_object_notify (G_OBJECT (config), "hidden-items");
      g_signal_emit (G_OBJECT (config), sn_config_signals[ITEM_LIST_CHANGED], 0);
    }
  else
    {
      g_object_notify (G_OBJECT (config), "hidden-legacy-items");
      g_signal_emit (G_OBJECT (config), sn_config_signals[LEGACY_ITEM_LIST_CHANGED], 0);
    }
}



GList *
sn_config_get_known_items (SnConfig *config,
                           SnItemType type)
{
  g_return_val_if_fail (SN_IS_CONFIG (config), NULL);

  return config->known_items[type];
}



GList *
sn_config_get_hidden_legacy_items (SnConfig *config)
{
  GList *list = NULL;

  g_return_val_if_fail (SN_IS_CONFIG (config), NULL);

  list = g_hash_table_get_values (config->hidden_items[SN_ITEM_TYPE_LEGACY]);

  return list;
}



gboolean
sn_config_add_known_item (SnConfig *config,
                          SnItemType type,
                          const gchar *name)
{
  GList *li;
  gchar *name_copy;

  g_return_val_if_fail (SN_IS_CONFIG (config), FALSE);

  /* check if item is already known */
  for (li = config->known_items[type]; li != NULL; li = li->next)
    if (g_strcmp0 (li->data, name) == 0)
      return g_hash_table_contains (config->hidden_items[type], name);

  config->known_items[type] = g_list_prepend (config->known_items[type], g_strdup (name));

  if (config->hide_new_items)
    {
      name_copy = g_strdup (name);
      g_hash_table_replace (config->hidden_items[type], name_copy, name_copy);
      g_object_notify (G_OBJECT (config), type == SN_ITEM_TYPE_DEFAULT ? "hidden-items" : "hidden-legacy-items");
    }

  if (type == SN_ITEM_TYPE_DEFAULT)
    {
      g_object_notify (G_OBJECT (config), "known-items");
      g_signal_emit (G_OBJECT (config), sn_config_signals[ITEM_LIST_CHANGED], 0);
    }
  else
    {
      g_object_notify (G_OBJECT (config), "known-legacy-items");
      g_signal_emit (G_OBJECT (config), sn_config_signals[LEGACY_ITEM_LIST_CHANGED], 0);
    }

  return config->hide_new_items;
}



void
sn_config_swap_known_items (SnConfig *config,
                            SnItemType type,
                            const gchar *name1,
                            const gchar *name2)
{
  GList *li, *li_tmp;

  g_return_if_fail (SN_IS_CONFIG (config));

  for (li = config->known_items[type]; li != NULL; li = li->next)
    if (g_strcmp0 (li->data, name1) == 0)
      break;

  /* make sure that the list contains name1 followed by name2 */
  if (li == NULL || li->next == NULL || g_strcmp0 (li->next->data, name2) != 0)
    {
      panel_debug (PANEL_DEBUG_SYSTRAY, "Couldn't swap items: %s and %s", name1, name2);
      return;
    }

  /* li_tmp will contain only the removed element (name2) */
  li_tmp = li->next;
  config->known_items[type] = g_list_remove_link (config->known_items[type], li_tmp);

  /* not strictly necessary (in testing the list contents was preserved)
   * but searching for the index again should be safer */
  for (li = config->known_items[type]; li != NULL; li = li->next)
    if (g_strcmp0 (li->data, name1) == 0)
      break;

  config->known_items[type] = g_list_insert_before (config->known_items[type], li, li_tmp->data);
  g_list_free (li_tmp);

  if (type == SN_ITEM_TYPE_DEFAULT)
    {
      g_object_notify (G_OBJECT (config), "known-items");
      g_signal_emit (G_OBJECT (config), sn_config_signals[ITEM_LIST_CHANGED], 0);
    }
  else
    {
      g_object_notify (G_OBJECT (config), "known-legacy-items");
      g_signal_emit (G_OBJECT (config), sn_config_signals[LEGACY_ITEM_LIST_CHANGED], 0);
    }
}



static gboolean
sn_config_items_clear_callback (gpointer key,
                                gpointer value,
                                gpointer user_data)
{
  GHashTable *collected_known_items = user_data;
  return !g_hash_table_contains (collected_known_items, key);
}



gboolean
sn_config_items_clear (SnConfig *config)
{
  GHashTable *collected_known_items;
  guint initial_size;
  GList *new_list, *li;

  collected_known_items = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  g_signal_emit (G_OBJECT (config), sn_config_signals[COLLECT_KNOWN_ITEMS],
                 0, collected_known_items);

  initial_size = g_list_length (config->known_items[SN_ITEM_TYPE_DEFAULT]);
  new_list = NULL;
  for (li = config->known_items[SN_ITEM_TYPE_DEFAULT]; li != NULL; li = li->next)
    {
      if (g_hash_table_contains (collected_known_items, li->data))
        {
          new_list = g_list_append (new_list, g_strdup (li->data));
        }
    }
  g_list_free_full (config->known_items[SN_ITEM_TYPE_DEFAULT], g_free);
  config->known_items[SN_ITEM_TYPE_DEFAULT] = new_list;

  g_hash_table_foreach_remove (config->hidden_items[SN_ITEM_TYPE_DEFAULT],
                               sn_config_items_clear_callback,
                               collected_known_items);
  g_hash_table_destroy (collected_known_items);

  if (initial_size != g_list_length (config->known_items[SN_ITEM_TYPE_DEFAULT]))
    {
      g_object_notify (G_OBJECT (config), "known-items");
      g_object_notify (G_OBJECT (config), "hidden-items");
      g_signal_emit (G_OBJECT (config), sn_config_signals[ITEM_LIST_CHANGED], 0);

      return TRUE;
    }
  else
    {
      return FALSE;
    }
}



gboolean
sn_config_legacy_items_clear (SnConfig *config)
{
  g_list_free_full (config->known_items[SN_ITEM_TYPE_LEGACY], g_free);
  config->known_items[SN_ITEM_TYPE_LEGACY] = NULL;
  g_hash_table_remove_all (config->hidden_items[SN_ITEM_TYPE_LEGACY]);

  g_object_notify (G_OBJECT (config), "known-legacy-items");
  g_object_notify (G_OBJECT (config), "hidden-legacy-items");

  g_signal_emit (G_OBJECT (config), sn_config_signals[LEGACY_ITEM_LIST_CHANGED], 0);

  return TRUE;
}



SnConfig *
sn_config_new (XfcePanelPlugin *plugin)
{
  SnConfig *config = g_object_new (SN_TYPE_CONFIG, NULL);
  const PanelProperty properties[] = {
    { "icon-size", G_TYPE_INT },
    { "single-row", G_TYPE_BOOLEAN },
    { "square-icons", G_TYPE_BOOLEAN },
    { "symbolic-icons", G_TYPE_BOOLEAN },
    { "menu-is-primary", G_TYPE_BOOLEAN },
    { "hide-new-items", G_TYPE_BOOLEAN },
    { "known-items", G_TYPE_PTR_ARRAY },
    { "hidden-items", G_TYPE_PTR_ARRAY },
    { "known-legacy-items", G_TYPE_PTR_ARRAY },
    { "hidden-legacy-items", G_TYPE_PTR_ARRAY },
    { NULL }
  };

  panel_properties_bind (NULL, G_OBJECT (config),
                         xfce_panel_plugin_get_property_base (plugin),
                         properties, FALSE);
  g_signal_emit (G_OBJECT (config), sn_config_signals[CONFIGURATION_CHANGED], 0);

  return config;
}
