#include "launcher-item-info.h"
#include "launcher.h"

#include <gio/gio.h>
#include <libxfce4util/libxfce4util.h>
#include <xfconf/xfconf.h>



gchar *
launcher_get_item_name (GarconMenuItem *item)
{
  const gchar *name = garcon_menu_item_get_name (item);

  if (xfce_str_is_empty (name))
    return g_strdup (_("Unnamed item"));

  return g_strdup (name);
}



gchar *
launcher_get_item_name_markup (GarconMenuItem *item)
{
  gchar *name = launcher_get_item_name (item);
  const gchar *comment = garcon_menu_item_get_comment (item);
  gchar *markup = NULL;

  if (!xfce_str_is_empty (comment))
    markup = g_markup_printf_escaped ("<b>%s</b>\n%s", name, comment);
  else
    markup = g_markup_printf_escaped ("<b>%s</b>", name);
  g_free (name);
  return markup;
}



GIcon *
launcher_get_item_icon (GarconMenuItem *item)
{
  return launcher_plugin_tooltip_icon (garcon_menu_item_get_icon_name (item));
}



gchar *
launcher_get_item_tooltip (GarconMenuItem *item)
{
  GFile *file = garcon_menu_item_get_file (item);
  gchar *tooltip = g_file_get_parse_name (file);

  g_clear_object (&file);
  return tooltip;
}
