/*
 * Copyright (C) 2009-2010 Nick Schermer <nick@xfce.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */



#ifndef __LIBXFCE4PANEL_DEPRECATED_H__
#define __LIBXFCE4PANEL_DEPRECATED_H__

#include <libxfce4panel/xfce-panel-plugin-provider.h>
#include <gdk/gdkx.h>
#include <stdlib.h>

G_BEGIN_DECLS

/**
 * SECTION: register-46-plugins
 * @title: Panel Plugin Register Macros (4.6 Style)
 * @short_description: The 4.6 way of registering plugins that compiled as executables
 * @include: libxfce4panel/libxfce4panel.h
 *
 * Macros to register old external panel plugins that are compiled as executables.
 **/


/*< private >*/
#define _PANEL_CLIENT_EVENT_ATOM "XFCE4_PANEL_PLUGIN_46"


/**
 * panel_slice_alloc:
 * @block_size : the number of bytes to allocate
 *
 * See g_slice_alloc() for more information.
 *
 * Returns: a pointer to the allocated memory block
 *
 * Deprecated: 4.8: Deprecated because panel depends on recent enough
 *                  version of glib. Use g_slice_alloc() instead.
 **/
#define panel_slice_alloc(block_size) (g_slice_alloc ((block_size)))

/**
 * panel_slice_alloc0:
 * @block_size : the number of bytes to allocate
 *
 * See g_slice_alloc0() for more information.
 *
 * Returns: a pointer to the allocated memory block
 *
 * Deprecated: 4.8: Deprecated because panel depends on recent enough
 *                  version of glib. Use g_slice_alloc0() instead.
 **/
#define panel_slice_alloc0(block_size) (g_slice_alloc0 ((block_size)))

/**
 * panel_slice_free1:
 * @block_size : the size of the block
 * @mem_block  : a pointer to the block to free
 *
 * See g_slice_free1() for more information.
 *
 * Deprecated: 4.8: Deprecated because panel depends on recent enough
 *                  version of glib. Use g_slice_free1() instead.
 **/
#define panel_slice_free1(block_size, mem_block) G_STMT_START{ g_slice_free1 ((block_size), (mem_block)); }G_STMT_END

/**
 * panel_slice_new:
 * @type : the type to allocate, typically a structure name
 *
 * See g_slice_new() for more information.
 *
 * Returns: a pointer to the allocated memory block
 *
 * Deprecated: 4.8: Deprecated because panel depends on recent enough
 *                  version of glib. Use g_slice_new() instead.
 **/
#define panel_slice_new(type) (g_slice_new (type))

/**
 * panel_slice_new0:
 * @type : the type to allocate, typically a structure name
 *
 * See g_slice_new0() for more information.
 *
 * Returns: a pointer to the allocated memory block
 *
 * Deprecated: 4.8: Deprecated because panel depends on recent enough
 *                  version of glib. Use g_slice_new0() instead.
 **/
#define panel_slice_new0(type) (g_slice_new0 (type))

/**
 * panel_slice_free:
 * @type : the type to allocate, typically a structure name
 * @ptr  : a pointer to the block to free
 *
 * See g_slice_free() for more information.
 *
 * Deprecated: 4.8: Deprecated because panel depends on recent enough
 *                  version of glib. Use g_slice_free() instead.
 **/
#define panel_slice_free(type, ptr) G_STMT_START{ g_slice_free (type, (ptr)); }G_STMT_END



/**
 * PANEL_PARAM_READABLE:
 *
 * Macro for #G_PARAM_READABLE with static strings.
 *
 * Deprecated: 4.8: Deprecated because panel depends on recent enough
 *                  version of glib. Use #G_PARAM_READABLE
 *                  | #G_PARAM_STATIC_STRINGS instead.
 **/
#define PANEL_PARAM_READABLE (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)

/**
 * PANEL_PARAM_WRITABLE:
 *
 * Macro for #PANEL_PARAM_WRITABLE with static strings.
 *
 * Deprecated: 4.8: Deprecated because panel depends on recent enough
 *                  version of glib. Use #G_PARAM_WRITABLE
 *                  | #G_PARAM_STATIC_STRINGS instead.
 **/
#define PANEL_PARAM_WRITABLE (G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS)

/**
 * PANEL_PARAM_READWRITE:
 *
 * Macro for #G_PARAM_READWRITE with static strings.
 *
 * Deprecated: 4.8: Deprecated because panel depends on recent enough
 *                  version of glib. Use #G_PARAM_READWRITE
 *                  | #G_PARAM_STATIC_STRINGS instead.
 **/
#define PANEL_PARAM_READWRITE (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)

#define _panel_assert(expr)                  g_assert (expr)
#define _panel_assert_not_reached()          g_assert_not_reached ()
#define _panel_return_if_fail(expr)          g_return_if_fail (expr)
#define _panel_return_val_if_fail(expr, val) g_return_val_if_fail (expr, (val))


/**
 * xfce_create_panel_button:
 *
 * See xfce_panel_create_button() for more information.
 *
 * Deprecated: 4.8: Use xfce_panel_create_button() instead.
 **/
#define xfce_create_panel_button xfce_panel_create_button

/**
 * xfce_create_panel_toggle_button:
 *
 * See xfce_panel_create_toggle_button() for more information.
 *
 * Deprecated: 4.8: Use xfce_panel_create_toggle_button() instead.
 **/
#define xfce_create_panel_toggle_button xfce_panel_create_toggle_button

/**
 * xfce_panel_plugin_set_panel_hidden:
 * @plugin  : an #XfcePanelPlugin.
 * @hidden  : new blocking state of this plugin.
 *
 * See xfce_panel_plugin_block_autohide() for more information.
 *
 * Deprecated: 4.8: Use xfce_panel_plugin_block_autohide() instead.
 **/
#define xfce_panel_plugin_set_panel_hidden(plugin, hidden) \
    xfce_panel_plugin_block_autohide(plugin,hidden)

/**
 * xfce_allow_panel_customization:
 *
 * Always returns %FALSE. Plugins can be locked on a plugin basis
 * level in the future, so this function is useless.
 *
 * Deprecated: 4.8: Look at xfce_panel_plugin_get_locked().
 **/
#define xfce_allow_panel_customization  (FALSE)

/* <private >*/
#define _panel_g_type_register_simple(type_parent,type_name_static,class_size,class_init,instance_size,instance_init) \
    g_type_register_static_simple(type_parent,type_name_static,class_size,class_init,instance_size,instance_init, 0)

/**
 * XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL:
 * @construct_func : name of a function that can be cast to an
 *                   #XfcePanelPluginFunc
 *
 * Registers and initializes the plugin. This is the only thing that is
 * required to create a panel plugin.
 *
 * Deprecated: 4.8: Deprecation is not entirely true, but it should be made
 *                  clear that it is recommended to use XFCE_PANEL_PLUGIN_REGISTER().
 *                  See the <link linkend="libxfce4panel-register">Registering Plugins</link>
 *                  for more information.
 **/
#define XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL(construct_func)  \
    XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL_FULL (construct_func, NULL, NULL)

/**
 * XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL_WITH_CHECK:
 * @construct_func : name of a function that can be cast to an
 *                   #XfcePanelPluginFunc
 * @check_func :     name of a function that can be cast to an
 *                   #XfcePanelPluginCheck or %NULL
 *
 * Registers and initializes the plugin. This is the only thing that is
 * required to create a panel plugin. The @check functions is run before
 * creating the plugin, and should return FALSE if plugin creation is not
 * possible.
 *
 * Deprecated: 4.8: Deprecation is not entirely true, but it should be made
 *                  clear that it is recommended to use XFCE_PANEL_PLUGIN_REGISTER_WITH_CHECK().
 *                  See the <link linkend="libxfce4panel-register">Registering Plugins</link>
 *                  for more information.
 **/
#define XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL_WITH_CHECK(construct_func ,check_func) \
    XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL_FULL (construct_func, NULL, check_func)

/**
 * XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL_FULL:
 * @construct_func : name of a function that can be cast to an
 *                   #XfcePanelPluginFunc
 * @preinit_func :   name of a function that can be case to #XfcePanelPluginPreInit
 *                   or %NULL
 * @check_func :     name of a function that can be cast to an
 *                   #XfcePanelPluginCheck or %NULL
 *
 * Same as XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL_WITH_CHECK(), but with a
 * preinit function that is called before gtk_init(). This allows plugins
 * to initialize libraries or threads.
 *
 * Since: 4.6
 * Deprecated: 4.8: Deprecation is not entirely true, but it should be made
 *                  clear that it is recommended to use XFCE_PANEL_PLUGIN_REGISTER_FULL().
 *                  See the <link linkend="libxfce4panel-register">Registering Plugins</link>
 *                  for more information.
 **/
#define XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL_FULL(construct_func, preinit_func, check_func) \
  static GdkAtom          _xpp_atom = GDK_NONE; \
  static gdouble          _xpp_alpha = 1.00; \
  static guint            _xpp_bg_style = 0; \
  static GdkColor         _xpp_bg_color = { 0, }; \
  static const gchar     *_xpp_bg_image = NULL; \
  static cairo_pattern_t *_xpp_bg_image_cache = NULL; \
  static gboolean         _xpp_debug = FALSE; \
  static gint             _xpp_retval = PLUGIN_EXIT_FAILURE; \
  \
  static void \
  _xpp_quit_main_loop (void) \
  { \
     static gboolean quiting = FALSE; \
     \
     if (!quiting) \
       { \
         quiting = TRUE; \
         gtk_main_quit (); \
       } \
  } \
  \
  static gboolean \
  _xpp_client_event (GtkWidget       *plug, \
                     GdkEventClient  *event, \
                     XfcePanelPlugin *xpp) \
  { \
    XfcePanelPluginProvider          *provider = XFCE_PANEL_PLUGIN_PROVIDER (xpp); \
    gint                              value; \
    XfcePanelPluginProviderPropType   type; \
    \
    g_return_val_if_fail (XFCE_IS_PANEL_PLUGIN (xpp), TRUE); \
    g_return_val_if_fail (GTK_IS_PLUG (plug), TRUE); \
    g_return_val_if_fail (_xpp_atom != GDK_NONE, TRUE); \
    g_return_val_if_fail (XFCE_IS_PANEL_PLUGIN_PROVIDER (xpp), TRUE); \
    \
    if (event->message_type == _xpp_atom) \
      { \
        type = event->data.s[0]; \
        value = event->data.s[1]; \
        \
        switch (type) \
          { \
          case PROVIDER_PROP_TYPE_ACTION_REMOVED: \
            xfce_panel_plugin_provider_removed (provider); \
            break; \
            \
          case PROVIDER_PROP_TYPE_ACTION_SAVE: \
            xfce_panel_plugin_provider_save (provider); \
            break; \
            \
          case PROVIDER_PROP_TYPE_SET_BACKGROUND_ALPHA: \
            _xpp_alpha = value / 100.00; \
            if (gtk_widget_is_composited (plug)) \
              gtk_widget_queue_draw (plug); \
            break; \
            \
          case PROVIDER_PROP_TYPE_SET_LOCKED: \
            xfce_panel_plugin_provider_set_locked (provider, !!value); \
            break; \
            \
          case PROVIDER_PROP_TYPE_SET_MODE: \
            xfce_panel_plugin_provider_set_mode (provider, value); \
            break; \
            \
          case PROVIDER_PROP_TYPE_SET_NROWS: \
            xfce_panel_plugin_provider_set_nrows (provider, value); \
            break; \
            \
          case PROVIDER_PROP_TYPE_SET_SCREEN_POSITION: \
            xfce_panel_plugin_provider_set_screen_position (provider, value); \
            break; \
            \
          case PROVIDER_PROP_TYPE_SET_SENSITIVE: \
            gtk_widget_set_sensitive (plug, !!value); \
            break; \
            \
          case PROVIDER_PROP_TYPE_SET_SIZE: \
            xfce_panel_plugin_provider_set_size (provider, value); \
            break; \
            \
          case PROVIDER_PROP_TYPE_ACTION_SHOW_ABOUT: \
            xfce_panel_plugin_provider_show_about (provider); \
            break; \
            \
          case PROVIDER_PROP_TYPE_ACTION_SHOW_CONFIGURE: \
            xfce_panel_plugin_provider_show_configure (provider); \
            break; \
            \
          case PROVIDER_PROP_TYPE_ACTION_QUIT_FOR_RESTART: \
            _xpp_retval = PLUGIN_EXIT_SUCCESS_AND_RESTART; \
          case PROVIDER_PROP_TYPE_ACTION_QUIT: \
            _xpp_quit_main_loop (); \
            break; \
            \
          case PROVIDER_PROP_TYPE_SET_BACKGROUND_COLOR: \
            _xpp_bg_color.red = event->data.s[1]; \
            _xpp_bg_color.green = event->data.s[2]; \
            _xpp_bg_color.blue = event->data.s[3]; \
            _xpp_bg_style = 1; \
            gtk_widget_queue_draw (plug); \
            break; \
            \
          case PROVIDER_PROP_TYPE_ACTION_BACKGROUND_UNSET: \
            _xpp_bg_style = 0; \
            gtk_widget_queue_draw (plug); \
            break; \
            \
          case PROVIDER_PROP_TYPE_ACTION_ASK_REMOVE: \
            xfce_panel_plugin_provider_ask_remove (provider); \
            break; \
            \
          default: \
            g_warning ("Received unknow client event %u", type); \
            break; \
          } \
        \
        return FALSE; \
      } \
    \
    return TRUE; \
  } \
  \
  static void \
  _xpp_provider_signal (GtkWidget *xpp, \
                        guint      message, \
                        GtkWidget *plug) \
  { \
    GdkEventClient  event; \
    GdkWindow      *window; \
    gint            result; \
    \
    g_return_if_fail (GTK_IS_PLUG (plug)); \
    g_return_if_fail (XFCE_IS_PANEL_PLUGIN (xpp)); \
    g_return_if_fail (gtk_widget_get_realized (plug)); \
    g_return_if_fail (_xpp_atom != GDK_NONE); \
    g_return_if_fail (gtk_widget_get_realized (xpp)); \
    \
    if (_xpp_debug) \
      g_printerr ("xfce4-panel(%s): send provider signal %d\n", \
                  xfce_panel_plugin_get_name (XFCE_PANEL_PLUGIN (xpp)), message); \
    \
    event.type = GDK_CLIENT_EVENT; \
    event.window = gtk_widget_get_window (plug); \
    event.send_event = TRUE; \
    event.message_type = _xpp_atom; \
    event.data_format = 16; \
    event.data.s[0] = message; \
    event.data.s[1] = 0; \
    \
    window = gtk_plug_get_socket_window (GTK_PLUG (plug)); \
    g_return_if_fail (GDK_IS_WINDOW (window)); \
    \
    gdk_error_trap_push (); \
    gdk_event_send_client_message ((GdkEvent *) &event, GDK_WINDOW_XID (window)); \
    gdk_flush (); \
    result = gdk_error_trap_pop (); \
    if (G_UNLIKELY (result != 0)) \
      g_warning ("Failed to send provider-signal %d: X error code %d", message, result); \
  } \
  \
  static void \
  _xpp_realize (XfcePanelPlugin *xpp, \
                GtkPlug         *plug) \
  { \
    GtkWidget *ebox; \
    \
    g_return_if_fail (XFCE_IS_PANEL_PLUGIN (xpp)); \
    g_return_if_fail (GTK_IS_PLUG (plug)); \
    g_return_if_fail (gtk_widget_get_realized (plug)); \
    \
    if (_xpp_debug) \
      g_printerr ("xfce4-panel(%s): calling plugin construct function\n", \
                  xfce_panel_plugin_get_name (xpp)); \
    \
    g_signal_handlers_disconnect_by_func (G_OBJECT (xpp), \
        G_CALLBACK (_xpp_realize), NULL); \
    \
    g_signal_connect (G_OBJECT (xpp), "provider-signal", \
        G_CALLBACK (_xpp_provider_signal), plug); \
    \
    ((XfcePanelPluginFunc) construct_func) (xpp); \
    \
    ebox = gtk_bin_get_child (GTK_BIN (xpp)); \
    if (ebox != NULL && GTK_IS_EVENT_BOX (ebox)) \
      gtk_event_box_set_visible_window (GTK_EVENT_BOX (ebox), FALSE); \
  } \
  \
  static gboolean \
  _xpp_expose_event (GtkWidget      *plug, \
                     GdkEventExpose *event) \
  { \
    cairo_t        *cr; \
    const GdkColor *color; \
    gdouble         real_alpha; \
    GdkPixbuf      *pixbuf; \
    GError         *error = NULL; \
    \
    if (!GTK_WIDGET_DRAWABLE (plug)) \
      return FALSE; \
    \
    if (G_UNLIKELY (_xpp_bg_style == 2)) \
      { \
        cr = gdk_cairo_create (gtk_widget_get_window (plug)); \
        cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE); \
        gdk_cairo_rectangle (cr, &event->area); \
        cairo_clip (cr); \
        \
        if (G_LIKELY (_xpp_bg_image_cache != NULL)) \
          { \
            cairo_set_source (cr, _xpp_bg_image_cache); \
            cairo_paint (cr); \
          } \
        else \
          { \
            /* load the image in a pixbuf */ \
            pixbuf = gdk_pixbuf_new_from_file (_xpp_bg_image, &error); \
            if (G_LIKELY (pixbuf != NULL)) \
              { \
                gdk_cairo_set_source_pixbuf (cr, pixbuf, 0, 0); \
                g_object_unref (G_OBJECT (pixbuf)); \
                \
                _xpp_bg_image_cache = cairo_get_source (cr); \
                cairo_pattern_reference (_xpp_bg_image_cache); \
                cairo_pattern_set_extend (_xpp_bg_image_cache, CAIRO_EXTEND_REPEAT); \
                cairo_paint (cr); \
              } \
            else \
              { \
                /* print error message */ \
                g_warning ("Background image disabled, \"%s\" could not be loaded: %s", \
                           _xpp_bg_image, error != NULL ? error->message : "No error"); \
                g_error_free (error); \
                \
                /* disable background image */ \
                _xpp_bg_style = 0; \
              } \
          } \
        \
        cairo_destroy (cr); \
      } \
    else \
      { \
        real_alpha = gtk_widget_is_composited (plug) ? _xpp_alpha : 1.00; \
        \
        if (_xpp_bg_style == 1 || real_alpha < 1.00) \
          { \
            if (G_LIKELY (_xpp_bg_style == 0)) \
              color = &(gtk_widget_get_style (plug)->bg[GTK_STATE_NORMAL]); \
            else \
              color = &_xpp_bg_color; \
            \
            cr = gdk_cairo_create (gtk_widget_get_window (plug)); \
            cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE); \
            cairo_set_source_rgba (cr, \
                                   color->red / 65535.00, \
                                   color->green / 65535.00, \
                                   color->blue / 65535.00, \
                                   real_alpha); \
            gdk_cairo_rectangle (cr, &event->area); \
            cairo_fill (cr); \
            cairo_destroy (cr); \
          } \
      } \
    \
    return FALSE; \
  } \
  \
  static void \
  _xpp_plug_embedded (GtkPlug *plug) \
  { \
    g_return_if_fail (GTK_IS_PLUG (plug)); \
    \
    if (!gtk_plug_get_embedded (plug)) \
      _xpp_quit_main_loop (); \
  } \
  \
  gint \
  main (gint argc, gchar **argv) \
  { \
    GtkWidget       *plug; \
    GdkScreen       *screen; \
    GtkWidget       *xpp; \
    gint             unique_id; \
    GdkNativeWindow  socket_id; \
    GdkColormap     *colormap = NULL; \
    const gchar     *value; \
    gchar           *base_name; \
    \
    value = g_getenv ("PANEL_DEBUG"); \
    if (G_UNLIKELY (value != NULL)) \
      { \
        _xpp_debug = TRUE; \
        \
        base_name = g_filename_display_basename (argv[0]);\
        g_printerr ("xfce4-panel(%s): compiled against libxfce4panel %s\n", \
                    base_name, LIBXFCE4PANEL_VERSION); \
        g_free (base_name); \
      } \
    \
    if (G_UNLIKELY (argc < PLUGIN_ARGV_ARGUMENTS)) \
      { \
        g_critical ("Not enough arguments are passed to the plugin"); \
        return PLUGIN_EXIT_ARGUMENTS_FAILED; \
      } \
    \
    if (G_UNLIKELY (preinit_func != NULL)) \
      { \
        if (!((XfcePanelPluginPreInit) preinit_func) (argc, argv)) \
          return PLUGIN_EXIT_PREINIT_FAILED; \
      } \
    \
    gtk_init (&argc, &argv); \
    \
    if (check_func != NULL) \
      { \
        screen = gdk_screen_get_default (); \
        if (!((XfcePanelPluginCheck) check_func) (screen)) \
          return PLUGIN_EXIT_CHECK_FAILED; \
      } \
    \
    _xpp_atom = gdk_atom_intern_static_string (_PANEL_CLIENT_EVENT_ATOM); \
    \
    socket_id = strtol (argv[PLUGIN_ARGV_SOCKET_ID], NULL, 0); \
    plug = gtk_plug_new (socket_id); \
    gtk_widget_set_name (GTK_WIDGET (plug), "XfcePanelWindowExternal"); \
    g_signal_connect (G_OBJECT (plug), "embedded", \
        G_CALLBACK (_xpp_plug_embedded), NULL); \
    g_signal_connect (G_OBJECT (plug), "expose-event", \
        G_CALLBACK (_xpp_expose_event), NULL); \
    \
    gtk_widget_set_app_paintable (plug, TRUE); \
    \
    screen = gtk_widget_get_screen (plug); \
    colormap = gdk_screen_get_rgba_colormap (screen); \
    if (colormap != NULL) \
      gtk_widget_set_colormap (plug, colormap); \
    \
    unique_id = strtol (argv[PLUGIN_ARGV_UNIQUE_ID], NULL, 0); \
    xpp = g_object_new (XFCE_TYPE_PANEL_PLUGIN, \
                        "name", argv[PLUGIN_ARGV_NAME], \
                        "unique-id", unique_id, \
                        "display-name", argv[PLUGIN_ARGV_DISPLAY_NAME], \
                        "comment", argv[PLUGIN_ARGV_COMMENT],  \
                        "arguments", argv + PLUGIN_ARGV_ARGUMENTS, NULL); \
    gtk_container_add (GTK_CONTAINER (plug), xpp); \
    g_signal_connect_after (G_OBJECT (xpp), "realize", \
        G_CALLBACK (_xpp_realize), plug); \
    g_signal_connect_after (G_OBJECT (xpp), "destroy", \
        G_CALLBACK (_xpp_quit_main_loop), NULL); \
    gtk_widget_show (xpp); \
    \
    if (*argv[PLUGIN_ARGV_BACKGROUND_IMAGE] != '\0') \
      { \
        _xpp_bg_image = argv[PLUGIN_ARGV_BACKGROUND_IMAGE]; \
        _xpp_bg_style = 2; \
      } \
    \
    g_signal_connect (G_OBJECT (plug), "client-event", \
       G_CALLBACK (_xpp_client_event), xpp); \
    gtk_widget_show (plug); \
    \
    gtk_main (); \
    \
    if (_xpp_retval != PLUGIN_EXIT_SUCCESS_AND_RESTART) \
      _xpp_retval = PLUGIN_EXIT_SUCCESS; \
    \
    if (_xpp_bg_image_cache != NULL) \
      cairo_pattern_destroy (_xpp_bg_image_cache); \
    \
    if (GTK_IS_WIDGET (plug)) \
      gtk_widget_destroy (plug); \
    \
    return _xpp_retval; \
  }



/**
 * XFCE_PANEL_PLUGIN_REGISTER_INTERNAL:
 * @construct_func : name of a function that can be cast to an
 *                   #XfcePanelPluginFunc
 *
 * See XFCE_PANEL_PLUGIN_REGISTER() for more information.
 *
 * Deprecated: 4.8: Use XFCE_PANEL_PLUGIN_REGISTER() instead.
 **/
#define XFCE_PANEL_PLUGIN_REGISTER_INTERNAL(construct_func)  \
    XFCE_PANEL_PLUGIN_REGISTER (construct_func)

/**
 * XFCE_PANEL_PLUGIN_REGISTER_INTERNAL_WITH_CHECK:
 * @construct_func : name of a function that can be cast to an
 *                   #XfcePanelPluginFunc
 * @check_func :     name of a function that can be cast to an
 *                   #XfcePanelPluginCheck or %NULL
 *
 * See XFCE_PANEL_PLUGIN_REGISTER() for more information.
 *
 * Deprecated: 4.8: use XFCE_PANEL_PLUGIN_REGISTER_WITH_CHECK() instead.
 **/
#define XFCE_PANEL_PLUGIN_REGISTER_INTERNAL_WITH_CHECK(construct_func ,check_func) \
    XFCE_PANEL_PLUGIN_REGISTER_WITH_CHECK (construct_func, check_func)

G_END_DECLS

#endif /* !__LIBXFCE4PANEL_DEPRECATED_H__ */
