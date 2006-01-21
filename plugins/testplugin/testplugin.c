/* test plugin */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <gtk/gtk.h>
#include <libxfcegui4/libxfcegui4.h>

#include <libxfce4panel/xfce-panel-plugin.h>

#define PLUGIN_NAME "testplugin"


/* Panel Plugin Interface */

static void test_construct (XfcePanelPlugin *plugin);

XFCE_PANEL_PLUGIN_REGISTER_EXTERNAL(test_construct);


/* internal functions */

static void
test_orientation_changed (XfcePanelPlugin *plugin, GtkOrientation orientation, 
                     GtkWidget *label)
{
    gtk_label_set_angle (GTK_LABEL (label), 
            (orientation == GTK_ORIENTATION_HORIZONTAL) ? 0 : 90);
}

static void 
test_free_data (XfcePanelPlugin *plugin)
{
    DBG ("Free data: %s", PLUGIN_NAME);
    gtk_main_quit ();
}

static void 
test_save (XfcePanelPlugin *plugin)
{
    char *file;
    XfceRc *rc;
    
    DBG ("Save: %s", PLUGIN_NAME);

    if (!(file = xfce_panel_plugin_save_location (plugin, TRUE)))
        return;

    rc = xfce_rc_simple_open (file, FALSE);
    g_free (file);

    if (rc)
    {
        xfce_rc_write_entry (rc, "string", "stringvalue");
        xfce_rc_write_bool_entry (rc, "bool", TRUE);
        xfce_rc_write_int_entry (rc, "int", 12);

        xfce_rc_close (rc);
    }
}

static void
test_configure (XfcePanelPlugin *plugin)
{
    DBG ("Configure: %s", PLUGIN_NAME);
}

static gboolean 
test_set_size (XfcePanelPlugin *plugin, int size)
{
    DBG ("Set size to %d: %s", size, PLUGIN_NAME);

    if (xfce_panel_plugin_get_orientation (plugin) == 
        GTK_ORIENTATION_HORIZONTAL)
    {
        gtk_widget_set_size_request (GTK_WIDGET (plugin), -1, size);
    }
    else
    {
        gtk_widget_set_size_request (GTK_WIDGET (plugin), size, -1);
    }

    return TRUE;
}

/* create widgets and connect to signals */ 

static void 
test_construct (XfcePanelPlugin *plugin)
{
    GtkWidget *button;
    char *file;
    XfceRc *rc;
    GtkOrientation orientation = xfce_panel_plugin_get_orientation (plugin);
    
    xfce_textdomain (GETTEXT_PACKAGE, LOCALEDIR, "UTF-8"); 

    DBG ("Construct: %s", PLUGIN_NAME);
    
    DBG ("Properties: size = %d, panel_position = %d", 
         xfce_panel_plugin_get_size (plugin),
         xfce_panel_plugin_get_screen_position (plugin));

    if ((file = xfce_panel_plugin_lookup_rc_file (plugin)) != NULL)
    {
        rc = xfce_rc_simple_open (file, TRUE);
        g_free (file);

        if (rc)
            xfce_rc_close (rc);
    }
    
    button = 
        gtk_button_new_with_label (xfce_panel_plugin_get_display_name (plugin));
    gtk_widget_show (button);
    gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);

    gtk_label_set_angle (GTK_LABEL (GTK_BIN (button)->child), 
            (orientation == GTK_ORIENTATION_HORIZONTAL) ? 0 : 90);
    
    gtk_container_add (GTK_CONTAINER (plugin), button);

    xfce_panel_plugin_add_action_widget (plugin, button);

    g_signal_connect (plugin, "orientation-changed", 
                      G_CALLBACK (test_orientation_changed), 
                      GTK_BIN (button)->child);

    g_signal_connect (plugin, "free-data", 
                      G_CALLBACK (test_free_data), NULL);

    g_signal_connect (plugin, "save", 
                      G_CALLBACK (test_save), NULL);

    g_signal_connect (plugin, "size-changed", 
                      G_CALLBACK (test_set_size), GTK_BIN (button)->child);

    xfce_panel_plugin_menu_show_configure (plugin);
    g_signal_connect (plugin, "configure-plugin", 
                      G_CALLBACK (test_configure), NULL);
}

