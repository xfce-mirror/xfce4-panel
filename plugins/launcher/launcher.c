/* vim: set expandtab ts=8 sw=4: */

/*  $Id$
 *
 *  Copyright © 2005 Jasper Huijsmans <jasper@xfce.org>
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

#include <config.h>

#include <stdio.h>
#include <gtk/gtk.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/libxfcegui4.h>

#include <libs/xfce-arrow-button.h>

#include <panel/xfce.h>
#include <panel/settings.h>
#include <panel/plugins.h>

#include "launcher.h"
#include "launcher-dialog.h"


XfceIconTheme *launcher_theme = NULL;


/* prototypes */
static Launcher *launcher_new (void);

static void launcher_read_xml (Launcher *launcher, xmlNodePtr node);

static void launcher_write_xml (Launcher *launcher, xmlNodePtr node);


/* utility functions *
 * ----------------- */

GdkPixbuf *
launcher_load_pixbuf (Icon *icon, int size)
{
    GdkPixbuf *pb = NULL;
    
    if (icon->type == ICON_TYPE_NAME)
    {
        if (g_path_is_absolute (icon->icon.name))
            pb = xfce_pixbuf_new_from_file_at_size (icon->icon.name, 
                                                    size, size, NULL);
        else
            pb = xfce_icon_theme_load (launcher_theme, icon->icon.name, size);
    }
    else if (icon->type == ICON_TYPE_CATEGORY)
    {
        pb = xfce_icon_theme_load_category (launcher_theme, 
                                            icon->icon.category, size);
    }

    if (!pb)
        pb = xfce_icon_theme_load_category (launcher_theme, 0, size);

    return pb;
}

char *
file_uri_to_local (const char *uri)
{
    const char *s;

    s = uri;
    
    if (!strncmp (s, "file:", 5))
    {
        s += 5;
        
        if (!strncmp (s, "//", 2))
            s += 2;
    }

    /* decode %-stuff */

    return g_strdup (s);
}

/* prevent users from clicking twice because of slow app start */
static gboolean set_sensitive_cb (GtkWidget *button)
{
    if (GTK_IS_WIDGET (button))
        gtk_widget_set_sensitive (button, TRUE);
    
    return FALSE;
}

static void
after_exec_insensitive (GtkWidget *button)
{
    gtk_widget_set_sensitive (button, FALSE);
    
    g_timeout_add (INSENSITIVE_TIMEOUT, 
                   (GSourceFunc) set_sensitive_cb, button);
}


/* Plugin interface *
 * ---------------- */

static void
launcher_about (void)
{
    XfceAboutInfo *info;
    GtkWidget *dlg;
    GdkPixbuf *pb;

    info = xfce_about_info_new (_("Launcher"), "", _("Launcher Plugin"), 
                                XFCE_COPYRIGHT_TEXT ("2005", 
                                                     "Jasper Huijsmans"),
                                XFCE_LICENSE_GPL);

    xfce_about_info_set_homepage (info, "http://www.xfce.org");

    xfce_about_info_add_credit (info, "Jasper Huijsmans", "jasper@xfce.org",
                                _("Developer"));

    pb = xfce_themed_icon_load ("xfce4-panel", DLG_ICON_SIZE);
    dlg = xfce_about_dialog_new (NULL, info, pb);
    g_object_unref (pb);

    gtk_widget_set_size_request (dlg, 300, 200);

    xfce_gtk_window_center_on_monitor_with_pointer (GTK_WINDOW (dlg));

    gtk_dialog_run (GTK_DIALOG (dlg));

    gtk_widget_destroy (dlg);

    xfce_about_info_free (info);
}

static void
launcher_set_theme (Control *control, const char *theme)
{
    GList *l, *ll;
    Entry *e;
    GdkPixbuf *pb;
    Launcher *launcher = control->data;
    
    g_return_if_fail (l != NULL);

    e = launcher->entry;

    pb = launcher_load_pixbuf (&e->icon, PANEL_ICON_SIZE);
    xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON (launcher->iconbutton), pb);
    g_object_unref (pb);

    if (!launcher->menu)
        return;
    
    ll = GTK_MENU_SHELL (launcher->menu)->children;
    
    for (l = launcher->items; l != NULL && ll != NULL; 
         l = l->next, ll = ll->next)
    {
        GtkWidget *im;
        
        e = l->data;
        
        pb = launcher_load_pixbuf (&e->icon, MENU_ICON_SIZE);

        im = gtk_image_new_from_pixbuf (pb);
        gtk_widget_show (im);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (ll->data), im);

        g_object_unref (pb);
    }
}

static void
launcher_set_size (Control *control, int size)
{
    Launcher *launcher = control->data;

    gtk_widget_set_size_request (launcher->iconbutton, 
                                 icon_size [size], icon_size [size]);
}

static void
launcher_set_orientation (Control *control, int orientation)
{
    GtkWidget *box;
    GtkArrowType type;
    Launcher *launcher = control->data;

    box = launcher->box;
    
    if (HORIZONTAL == orientation)
    {
        launcher->box = gtk_hbox_new (FALSE, 0);
        type = GTK_ARROW_UP;
    }
    else
    {
        launcher->box = gtk_vbox_new (FALSE, 0);
        type = GTK_ARROW_LEFT;
    }
    
    gtk_widget_show (launcher->box);

    gtk_widget_reparent (launcher->iconbutton, launcher->box);
    gtk_widget_reparent (launcher->arrowbutton, launcher->box);

    gtk_widget_destroy (box);

    gtk_container_add (GTK_CONTAINER (control->base), launcher->box);

    xfce_arrow_button_set_arrow_type (
            XFCE_ARROW_BUTTON (launcher->arrowbutton), type);
}

static void
launcher_create_options (Control * control, GtkContainer *container, 
                         GtkWidget *done)
{
    Launcher *launcher = control->data;

    launcher_properties_dialog (launcher, container, done);
}

static void
launcher_read_config (Control *control, xmlNodePtr node)
{
    Launcher *launcher = control->data;

    launcher_read_xml (launcher, node);
}

static void
launcher_write_config (Control *control, xmlNodePtr node)
{
    Launcher *launcher = control->data;

    launcher_write_xml (launcher, node);
}

static void
launcher_attach_callback (Control * control, const char *signal,
			GCallback callback, gpointer data)
{
    Launcher *launcher = control->data;

    g_signal_connect (launcher->iconbutton, signal, callback, data);
    g_signal_connect (launcher->arrowbutton, signal, callback, data);
}

static void
launcher_free (Control * control)
{
    GList *l;
    Launcher *launcher = (Launcher *) control->data;

    g_object_unref (launcher->tips);
    
    for (l = launcher->items; l != NULL; l = l->next)
    {
        Entry *e = l->data;

        entry_free (e);
    }

    g_list_free (launcher->items);

    if (launcher->menu)
        gtk_widget_destroy (launcher->menu);
    
    g_free (launcher);
}

static gboolean
create_launcher_control (Control * control)
{
    Launcher *launcher = launcher_new ();

    gtk_widget_set_size_request (control->base, -1, -1);
    gtk_container_add (GTK_CONTAINER (control->base), launcher->box);

    control->data = launcher;

    return TRUE;
}

G_MODULE_EXPORT void
xfce_control_class_init (ControlClass * cc)
{
    xfce_textdomain (GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");

    cc->name = "launcher";
    cc->caption = _("Alternative Launcher");

    cc->create_control = (CreateControlFunc) create_launcher_control;

    cc->free = launcher_free;
    cc->attach_callback = launcher_attach_callback;
    
    cc->read_config = launcher_read_config;
    cc->write_config = launcher_write_config;

    cc->create_options = launcher_create_options;

    cc->set_orientation = launcher_set_orientation;
    cc->set_size = launcher_set_size;
    cc->set_theme = launcher_set_theme;

    cc->about = launcher_about;

    control_class_set_unloadable (cc, TRUE);
}

/* Macro that checks panel API version */
XFCE_PLUGIN_CHECK_INIT

/* Launcher: plugin implementation *
 * ------------------------------- */

/* entry */
Entry *
entry_new (void)
{
    return g_new0 (Entry, 1);
}

void
entry_free (Entry *e)
{
    g_free (e->name);
    g_free (e->comment);
    if (e->icon.type == ICON_TYPE_NAME)
        g_free (e->icon.icon.name);
    g_free (e->exec);

    g_free (e);
}

static void
entry_exec (Entry *entry)
{
    GError *error = NULL;
    
    if (!entry->exec || !strlen (entry->exec))
        return;
    
    xfce_exec (entry->exec, entry->terminal, entry->startup, &error);
        
    if (error)
    {
        xfce_err (_("Could not run \"%s\":\n%s"), 
                  entry->name, error->message);

        g_error_free (error);
    }
}

/* menu */
static void
launcher_menu_deactivated (GtkWidget *menu, GtkToggleButton *tb)
{
    gtk_toggle_button_set_active (tb, FALSE);
}

static void
launcher_menu_item_activate (GtkWidget *mi, Entry *entry)
{
    entry_exec (entry);
}

static void
launcher_create_menu (Launcher *launcher)
{
    launcher->menu = gtk_menu_new ();

    g_signal_connect (launcher->menu, "deactivate", 
                      G_CALLBACK (launcher_menu_deactivated),
                      launcher->arrowbutton);
}

static void
launcher_destroy_menu (Launcher *launcher)
{
    gtk_widget_destroy (launcher->menu);

    launcher->menu = NULL;
}

void
launcher_recreate_menu (Launcher *launcher)
{
    GList *l;
    
    if (launcher->menu)
        launcher_destroy_menu (launcher);

    if (!launcher->items)
    {
        gtk_widget_hide (launcher->arrowbutton);
        return;
    }
    
    launcher_create_menu (launcher);
    
    for (l = launcher->items; l != NULL; l = l->next)
    {
        GtkWidget *mi, *img;
        GdkPixbuf *pb;
        Entry *entry;
        
        entry = l->data;
        
        mi = gtk_image_menu_item_new_with_label (entry->name);
        gtk_widget_show (mi);
        gtk_menu_shell_prepend (GTK_MENU_SHELL (launcher->menu), mi);

        pb = launcher_load_pixbuf (&entry->icon, MENU_ICON_SIZE);

        img = gtk_image_new_from_pixbuf (pb);
        gtk_widget_show (img);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), img);
        g_object_unref (pb);

        g_signal_connect (mi, "activate", 
                          G_CALLBACK (launcher_menu_item_activate), entry);

        if (entry->comment)
            gtk_tooltips_set_tip (launcher->tips, mi, entry->comment, NULL);
    }
}

void
launcher_update_panel_entry (Launcher *launcher)
{    
    GdkPixbuf *pb;
    char *tip;

    pb = launcher_load_pixbuf (&launcher->entry->icon, PANEL_ICON_SIZE);
    xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON (launcher->iconbutton), pb);
    g_object_unref (pb);

    if (launcher->entry->name)
    {
        if (launcher->entry->comment)
            tip = g_strdup_printf ("%s\n%s", launcher->entry->name,
                                   launcher->entry->comment);
        else
            tip = g_strdup (launcher->entry->name);

        gtk_tooltips_set_tip (launcher->tips, launcher->iconbutton, tip, NULL);

        g_free (tip);
    }
}

static void
launcher_position_menu (GtkMenu * menu, int *x, int *y, gboolean * push_in,
                        GtkWidget *b)
{
    GtkWidget *widget;
    GtkRequisition req;
    GdkScreen *screen;
    GdkRectangle geom;
    int num;

    widget = b->parent->parent;
    
    /* wtf is this anyway? */
    *push_in = TRUE;

    if (!GTK_WIDGET_REALIZED (GTK_WIDGET (menu)))
        gtk_widget_realize (GTK_WIDGET (menu));

    gtk_widget_size_request (GTK_WIDGET (menu), &req);

    gdk_window_get_origin (widget->window, x, y);

    widget = b->parent;
 
    switch (xfce_arrow_button_get_arrow_type (XFCE_ARROW_BUTTON (b)))
    {
        case GTK_ARROW_UP:
            *x += widget->allocation.x;
            *y += widget->allocation.y - req.height;
            break;
        case GTK_ARROW_DOWN:
            *x += widget->allocation.x;
            *y += widget->allocation.y + widget->allocation.height;
            break;
        case GTK_ARROW_LEFT:
            *x += widget->allocation.x - req.width;
            *y += widget->allocation.y - req.height
                + widget->allocation.height;
            break;
        case GTK_ARROW_RIGHT:
            *x += widget->allocation.x + widget->allocation.width;
            *y += widget->allocation.y - req.height
                + widget->allocation.height;
            break;
    }

    screen = gtk_widget_get_screen (widget);

    num = gdk_screen_get_monitor_at_window (screen, widget->window);

    gdk_screen_get_monitor_geometry (screen, num, &geom);

    if (*x > geom.x + geom.width - req.width)
        *x = geom.x + geom.width - req.width;
    if (*x < geom.x)
        *x = geom.x;

    if (*y > geom.y + geom.height - req.height)
        *y = geom.y + geom.height - req.height;
    if (*y < geom.y)
        *y = geom.y;
}

static void
launcher_toggle_menu (GtkToggleButton *b, Launcher *launcher)
{
    if (gtk_toggle_button_get_active (b) && launcher->items)
    {
        gtk_menu_popup (GTK_MENU (launcher->menu), NULL, NULL, 
                        (GtkMenuPositionFunc) launcher_position_menu, b,
                        0, gtk_get_current_event_time ());
    }
}

/* launcher */
static void
launcher_entry_data_received (GtkWidget *w, GList *data, gpointer user_data)
{
    Launcher *launcher = user_data;
    GList *l;
    GString *execute;
    char *file;

    if (!data || !data->data)
        return;
    
    execute = g_string_sized_new (100);

    if (launcher->entry->terminal)
        g_string_append_c (execute, '"');

    /* XXX Fix the quoting (if necessary) */
    g_string_append (execute, launcher->entry->exec);

    for (l = data; l != NULL; l = l->next)
    {
        file = file_uri_to_local ((char *) l->data);

        if (file)
        {
            g_string_append_c (execute, ' ');
            g_string_append_c (execute, '"');
            g_string_append (execute, file);
            g_string_append_c (execute, '"');

            g_free (file);
        }
    }

    if (launcher->entry->terminal)
        g_string_append_c (execute, '"');

    exec_cmd (execute->str, launcher->entry->terminal, 
              launcher->entry->startup);
    
    after_exec_insensitive (launcher->iconbutton);
    
    g_string_free (execute, TRUE);
}

static void
launcher_state_changed (GtkWidget *b1, GtkStateType state, GtkWidget *b2)
{
    if (GTK_WIDGET_STATE (b2) != GTK_WIDGET_STATE (b1)
        && GTK_WIDGET_STATE (b1) != GTK_STATE_INSENSITIVE)
    {
        gtk_widget_set_state (b2, GTK_WIDGET_STATE (b1));
    }
}

static void
launcher_clicked (GtkWidget *w, Launcher *launcher)
{
    entry_exec (launcher->entry);
    after_exec_insensitive (w);
}

static Launcher *
launcher_new (void)
{
    Launcher *launcher;

    launcher = g_new0 (Launcher, 1);
    
    launcher->tips = gtk_tooltips_new ();
    g_object_ref (launcher->tips);
    gtk_object_sink (GTK_OBJECT (launcher->tips));
    
    launcher->entry = g_new0 (Entry, 1);
    launcher->entry->name = g_strdup (_("New item"));
    launcher->entry->comment = 
        g_strdup (_("This item has not yet been configured"));

    launcher->box = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (launcher->box);

    launcher->iconbutton = xfce_iconbutton_new ();
    gtk_widget_show (launcher->iconbutton);
    gtk_box_pack_start (GTK_BOX (launcher->box), launcher->iconbutton,
                        TRUE, TRUE, 0);
    gtk_widget_set_size_request (launcher->iconbutton, 
                                 icon_size [SMALL], icon_size [SMALL]);
    gtk_button_set_relief (GTK_BUTTON (launcher->iconbutton), 
                           GTK_RELIEF_NONE);
    gtk_button_set_focus_on_click (GTK_BUTTON (launcher->iconbutton), FALSE);

    launcher->arrowbutton = xfce_arrow_button_new (GTK_ARROW_UP);
    gtk_box_pack_start (GTK_BOX (launcher->box), launcher->arrowbutton,
                        FALSE, FALSE, 0);
    gtk_widget_set_size_request (launcher->arrowbutton, W_ARROW, W_ARROW);
    gtk_button_set_relief (GTK_BUTTON (launcher->arrowbutton), 
                           GTK_RELIEF_NONE);
    gtk_button_set_focus_on_click (GTK_BUTTON (launcher->arrowbutton), FALSE);
    
    g_signal_connect (launcher->iconbutton, "clicked",
                      G_CALLBACK (launcher_clicked), launcher);
    
    g_signal_connect (launcher->arrowbutton, "toggled",
                      G_CALLBACK (launcher_toggle_menu), launcher);
    
    g_signal_connect (launcher->iconbutton, "state-changed",
                      G_CALLBACK (launcher_state_changed), 
                      launcher->arrowbutton);
    
    g_signal_connect (launcher->arrowbutton, "state-changed",
                      G_CALLBACK (launcher_state_changed), 
                      launcher->iconbutton);

    g_signal_connect (launcher->iconbutton, "destroy", 
                      G_CALLBACK (gtk_widget_destroyed), 
                      &(launcher->iconbutton));

    dnd_set_drag_dest (launcher->iconbutton);
    dnd_set_callback (launcher->iconbutton, 
                      DROP_CALLBACK (launcher_entry_data_received), launcher);

    if (G_UNLIKELY (!launcher_theme))
        launcher_theme = xfce_icon_theme_get_for_screen (
                gtk_widget_get_screen (launcher->iconbutton));

    return launcher;
}


/* xml handling *
 * ------------ */

static Entry*
create_entry_from_xml (xmlNodePtr node)
{
    xmlNodePtr child;
    xmlChar *value;
    Entry *entry;
    
    entry = g_new0 (Entry, 1);
    
    for (child = node->children; child != NULL; child = child->next)
    {
        if (xmlStrEqual (child->name, (const xmlChar *) "name"))
        {
            value = DATA (child);

            if (value)
                entry->name = (char *) value;
        }
        else if (xmlStrEqual (child->name, (const xmlChar *) "comment"))
        {
            value = DATA (child);

            if (value)
                entry->comment = (char *) value;
        }
        else if (xmlStrEqual (child->name, (const xmlChar *) "icon"))
        {
            if ((value = xmlGetProp (child, "category")))
            {
                entry->icon.type = ICON_TYPE_CATEGORY;
		entry->icon.icon.category = 
                    CLAMP ((int) strtol (value, NULL, 0), 0, NUM_CATEGORIES);
		g_free (value);
            }
            else if ((value = xmlGetProp (child, "name")))
            {
                entry->icon.type = ICON_TYPE_NAME;
                entry->icon.icon.name = (char *) value;
            }
            else if ((value = DATA (child)))
            {
                entry->icon.type = ICON_TYPE_NAME;
                entry->icon.icon.name = (char *) value;
            }
        }
        else if (xmlStrEqual (child->name, (const xmlChar *) "exec"))
        {
            int n;
            
            value = DATA (child);

            if (value)
                entry->exec = (char *) value;

	    value = xmlGetProp (child, "term");
            n = -1;
	    if (value)
	    {
		n = (int) strtol (value, NULL, 0);
		g_free (value);
	    }

	    entry->terminal = (n == 1);

	    value = xmlGetProp (child, "sn");
            n = -1;
	    if (value)
	    {
		n = (int) strtol (value, NULL, 0);
		g_free (value);
	    }

	    entry->startup = (n == 1);
        }
    }

    return entry;
}

static void
launcher_read_old_xml (Launcher *launcher, xmlNodePtr node)
{
}

static void
launcher_read_xml (Launcher *launcher, xmlNodePtr node)
{
    xmlNodePtr child;

    if (!node || !node->children)
        return;
    
    if (!xmlStrEqual (node->children->name, (const xmlChar *) "launcher"))
    {
        launcher_read_old_xml (launcher, node);
        return;
    }
    
    child = node->children;
    
    entry_free (launcher->entry);
    launcher->entry = create_entry_from_xml (child);
    
    for (child = child->next; child != NULL; child = child->next)
    {
        Entry *entry = NULL;
        
        if (!xmlStrEqual (child->name, (const xmlChar *) "launcher"))
            continue;
    
        entry = create_entry_from_xml (child);
        launcher->items = g_list_append (launcher->items, entry);

        gtk_widget_show (launcher->arrowbutton);
    }

    launcher_update_panel_entry (launcher);

    launcher_recreate_menu (launcher);
}

static void
entry_write_xml (Entry *entry, xmlNodePtr node)
{
    xmlNodePtr parent, child;
    char value[3];

    parent = xmlNewTextChild (node, NULL, "launcher", NULL);

    if (entry->name)
        xmlNewTextChild (parent, NULL, "name", entry->name);

    if (entry->comment)
        xmlNewTextChild (parent, NULL, "comment", entry->comment);

    if (entry->icon.type != ICON_TYPE_NONE)
    {
        child = xmlNewTextChild (parent, NULL, "icon", NULL);

        if (entry->icon.type == ICON_TYPE_CATEGORY)
        {
            snprintf (value, 3, "%d", entry->icon.icon.category);
            xmlSetProp (child, "category", value);
        }
        else
        {
            xmlSetProp (child, "name", entry->icon.icon.name);
        }
    }

    if (entry->exec)
    {
        child = xmlNewTextChild (parent, NULL, "exec", entry->exec);

        snprintf (value, 2, "%d", entry->terminal);
        xmlSetProp (child, "term", value);

        snprintf (value, 2, "%d", entry->startup);
        xmlSetProp (child, "sn", value);
    }
}

static void
launcher_write_xml (Launcher *launcher, xmlNodePtr node)
{
    GList *l;

    entry_write_xml (launcher->entry, node);

    for (l = launcher->items; l != NULL; l = l->next)
    {
        entry_write_xml ((Entry *)l->data, node);
    }
}

