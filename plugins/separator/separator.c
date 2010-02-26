/* $Id$ */
/*
 * Copyright (c) 2005-2007 Jasper Huijsmans <jasper@xfce.org>
 * Copyright (c) 2007-2008 Nick Schermer <nick@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif



typedef struct _XfceSeparatorClass XfceSeparatorClass;
typedef struct _XfceSeparator      XfceSeparator;
typedef enum   _XfceSeparatorMode  XfceSeparatorStyle;



static void     xfce_separator_class_init                (XfceSeparatorClass  *klass);
static void     xfce_separator_init                      (XfceSeparator       *separator);
static void     xfce_separator_finalize                  (GObject             *object);
static void     xfce_separator_configure_plugin          (XfcePanelPlugin     *plugin);
static void     xfce_separator_load                      (XfceSeparator       *separator);



struct _XfceSeparatorClass
{
  /* parent class */
  XfcePanelPluginClass __parent__;
};

struct _XfceSeparator
{
  /* parent type */
  XfcePanelPlugin __parent__;

  /* separator style */
  XfceSeparatorStyle style;

  /* if the separator should expand */
  guint              expand : 1;
};

enum _XfceSeparatorMode
{
  XFCE_SEPARATOR_MODE_TRANSPARENT,
  XFCE_SEPARATOR_MODE_SEPARATOR,
  XFCE_SEPARATOR_MODE_HANDLE,
  XFCE_SEPARATOR_MODE_DOTS
};



XFCE_PANEL_DEFINE_TYPE (XfceSeparator, xfce_separator, XFCE_TYPE_PANEL_PLUGIN);



static void
xfce_separator_class_init (XfceSeparatorClass *klass)
{
  XfcePanelPluginClass *plugin_class;

  plugin_class = XFCE_PANEL_PLUGIN_CLASS (klass);
  plugin_class->save = xfce_separator_save;
  plugin_class->configure_plugin = xfce_separator_configure_plugin;
}



static void
xfce_separator_init (XfceSeparator *separator)
{
  /* initialize the default values */
  separator->style = XFCE_SEPARATOR_MODE_SEPARATOR;
  separator->expand = FALSE;

  /* show the properties dialog */
  xfce_panel_plugin_menu_show_configure (XFCE_PANEL_PLUGIN (clock));

  /* read the user settings */
  xfce_separator_load (separator);


}



static void
xfce_separator_save (XfcePanelPlugin *plugin)
{
  XfceSeparator *separator = XFCE_SEPARATOR (plugin);
  gchar         *filename;
  XfceRc        *rc;

  /* get the config file */
  filename = xfce_panel_plugin_save_location (plugin, TRUE);
  if (G_LIKELY (filename))
    {
      /* open rc file */
      rc = xfce_rc_simple_open (filename, FALSE);

      /* cleanup */
      g_free (filename);

      if (G_LIKELY (rc))
        {
          /* save the settings */
          xfce_rc_write_int_entry (rc, "Expand", separator->expand);
          xfce_rc_write_bool_entry (rc, "Style", separator->style);

          /* close the rc file */
          xfce_rc_close (rc);
        }
    }
}



static void
xfce_separator_configure_plugin (XfcePanelPlugin *plugin)
{

}



static void
xfce_separator_load (XfceSeparator *separator)
{
  gchar  *filename;
  XfceRc *rc;

  /* config filename */
  filename = xfce_panel_plugin_lookup_rc_file (XFCE_PANEL_PLUGIN (separator));
  if (G_LIKELY (filename))
    {
      /* open rc file (readonly) */
      rc = xfce_rc_simple_open (filename, TRUE);

      /* cleanup */
      g_free (filename);

      if (G_LIKELY (rc))
        {
          /* read the settings */
          separator->expand = xfce_rc_read_bool_entry (rc, "Expand", FALSE);
          separator->style = xfce_rc_read_int_entry (rc, "Style", XFCE_SEPARATOR_MODE_SEPARATOR);

          /* close the rc file */
          xfce_rc_close (rc);
        }
    }
}



G_MODULE_EXPORT void
xfce_panel_plugin_register_types (XfcePanelModule *panel_module)
{
  /* register the separator type */
  xfce_separator_register_type (panel_module);
}



XFCE_PANEL_PLUGIN_REGISTER_OBJECT (XFCE_TYPE_SEPARATOR)
