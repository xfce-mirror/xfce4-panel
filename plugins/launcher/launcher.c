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

/*  This file is divided into several sections:
 *  (1) typedefs, prototypes and global variables
 *  (2) Plugin section: implementation of the Control interface
 *  (3) Launcher: handling of the launcher structure
 *  (4) Properties: Settings dialog for the launcher
 *  (5) Xml: reading and writing settings and data to xml.
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

#define BORDER           6
#define W_ARROW          18
#define MENU_ICON_SIZE   24
#define DLG_ICON_SIZE    32
#define PANEL_ICON_SIZE  48

/* Don't use icons without xfce- default */
#define NUM_CATEGORIES   (XFCE_N_BUILTIN_ICON_CATEGORIES - 4)


/* typedefs */
typedef struct _Entry Entry;
typedef struct _Launcher Launcher;

struct _Entry
{
    char *name;
    char *comment;
    char *icon;
    char *exec;

    guint terminal:1;
    guint startup:1;
};

struct _Launcher
{
    GtkTooltips *tips;

    Entry *entry;
    
    GList *items;

    GtkWidget *menu;

    GtkWidget *box;
    GtkWidget *arrowbutton;
    GtkWidget *iconbutton;
};

static XfceIconTheme *global_icon_theme = NULL;

/* prototypes */
static Launcher *launcher_new (void);

static void launcher_properties_dialog (Launcher *launcher, 
                                        GtkContainer *container, 
                                        GtkWidget *close);
static void launcher_read_xml (Launcher *launcher, xmlNodePtr node);

static void launcher_write_xml (Launcher *launcher, xmlNodePtr node);

static void entry_free (Entry *entry);


/* utility functions *
 * ----------------- */

static GdkPixbuf *
launcher_load_pixbuf (const char *file, int size)
{
    GdkPixbuf *pb = NULL;

    if (file)
    {
        if (g_path_is_absolute (file))
            pb = xfce_pixbuf_new_from_file_at_size (file, size, size, NULL);
        else
            pb = xfce_icon_theme_load (global_icon_theme, file, size);
    }

    if (!pb)
        pb = xfce_icon_theme_load_category (global_icon_theme, 0, size);

    return pb;
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

    if (e->icon)
    {
        pb = launcher_load_pixbuf (e->icon, PANEL_ICON_SIZE);
        
        xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON (launcher->iconbutton), 
                                    pb);
        g_object_unref (pb);
    }

    if (!launcher->menu)
        return;
    
    ll = GTK_MENU_SHELL (launcher->menu)->children;
    
    for (l = launcher->items; l != NULL && ll != NULL; 
         l = l->next, ll = ll->next)
    {
        GtkWidget *im;
        
        e = l->data;
        
        if (e->icon)
        {
            pb = launcher_load_pixbuf (e->icon, MENU_ICON_SIZE);

            im = gtk_image_new_from_pixbuf (pb);
            gtk_widget_show (im);
            gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (ll->data), im);
            g_object_unref (pb);
        }
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

/* destroy icon theme when unloading */
G_MODULE_EXPORT void
g_module_unload (GModule *gm)
{
    if (global_icon_theme)
    {
        g_object_unref (global_icon_theme);
        global_icon_theme = NULL;
    }
}

/* Launcher: plugin implementation *
 * ------------------------------- */

/* entry */
static void
entry_free (Entry *e)
{
    g_free (e->name);
    g_free (e->comment);
    g_free (e->icon);
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

static void
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

        pb = launcher_load_pixbuf (entry->icon, MENU_ICON_SIZE);

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

static void
launcher_set_panel_entry (Launcher *launcher)
{    
    GdkPixbuf *pb;
    char *tip;

    if (launcher->entry->icon)
    {
        pb = launcher_load_pixbuf (launcher->entry->icon, 
                                   PANEL_ICON_SIZE);

        xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON (launcher->iconbutton), 
                                    pb);
        g_object_unref (pb);
    }

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
launcher_state_changed (GtkWidget *b1, GtkStateType state, GtkWidget *b2)
{
    if (GTK_WIDGET_STATE (b2) != GTK_WIDGET_STATE (b1))
        gtk_widget_set_state (b2, GTK_WIDGET_STATE (b1));
}

static void
launcher_clicked (GtkWidget *w, Launcher *launcher)
{
    entry_exec (launcher->entry);
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
    launcher->entry->icon = g_strdup ("xfce-unknown");

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

    if (G_UNLIKELY (global_icon_theme == NULL))
    {
        global_icon_theme = 
            xfce_icon_theme_get_for_screen (
                    gtk_widget_get_screen (launcher->box));
    }
    
    return launcher;
}


/* Properties: settings dialogs *
 * ---------------------------- */

typedef struct
{
    Launcher *launcher;
    
    GtkWidget *dlg;

    GtkWidget *tree;
    GtkWidget *scroll;

    GtkWidget *up;
    GtkWidget *down;
    GtkWidget *edit;
    GtkWidget *add;
    GtkWidget *remove;
}
LauncherDialog;

typedef struct
{
    Entry *entry;
    
    gboolean info_changed;
    gboolean icon_changed;
    gboolean exec_changed;

    GtkWidget *dlg;
    
    GtkWidget *exec_name;
    GtkWidget *exec_comment;

    GtkWidget *icon_img;
    GtkWidget *icon_category;
    GtkWidget *icon_file;
    GtkWidget *icon_browse;

    GtkWidget *exec;
    GtkWidget *exec_browse;
    GtkWidget *exec_terminal;
    GtkWidget *exec_startup;
}
EntryDialog;

static char *category_icons [] = {
    N_("Default"),
    N_("Editor"),
    N_("File management"),
    N_("Utilities"),
    N_("Games"),
    N_("Help browser"),
    N_("Multimedia"),
    N_("Network"),
    N_("Graphics"),
    N_("Printer"),
    N_("Productivity"),
    N_("Sound"),
    N_("Terminal"),
    N_("Development"),
    N_("Settings"),
    N_("System"),
    N_("Windows programs")
};

/* entry dialog */
static void
update_entry_info (EntryDialog *ed)
{
    const char *text;
    
    text = gtk_entry_get_text (GTK_ENTRY (ed->exec_name));

    if (!text || !strlen (text))
    {
        if (ed->entry->name)
        {
            ed->info_changed = TRUE;
            g_free (ed->entry->name);
            ed->entry->name = NULL;
        }
    }
    else if (!ed->entry->name || strcmp (text, ed->entry->name) != 0)
    {
        ed->info_changed = TRUE;

        g_free (ed->entry->name);
        ed->entry->name = g_strdup (text);
    }

    text = gtk_entry_get_text (GTK_ENTRY (ed->exec_comment));

    if (!text || !strlen (text))
    {
        if (ed->entry->comment)
        {
            ed->info_changed = TRUE;
            g_free (ed->entry->comment);
            ed->entry->comment = NULL;
        }
    }
    else if (!ed->entry->comment || strcmp (text, ed->entry->comment) != 0)
    {
        ed->info_changed = TRUE;

        g_free (ed->entry->comment);
        ed->entry->comment = g_strdup (text);
    }
}

static void
update_entry_icon (EntryDialog *ed)
{
    const char *text;
    GdkPixbuf *pb;
    
    text = gtk_entry_get_text (GTK_ENTRY (ed->icon_file));
        
    if (!text || !strlen (text))
    {
        if (ed->entry->icon)
        {
            ed->icon_changed = TRUE;
            g_free (ed->entry->icon);
            ed->entry->icon = NULL;
        }

        pb = xfce_icon_theme_load_category (global_icon_theme, 0,                                                           DLG_ICON_SIZE);
        gtk_image_set_from_pixbuf (GTK_IMAGE (ed->icon_img), pb);
        g_object_unref (pb);
    }
    else if (!ed->entry->icon || strcmp (text, ed->entry->icon) != 0)
    {
        ed->icon_changed = TRUE;

        pb = launcher_load_pixbuf (text, DLG_ICON_SIZE);
        
        gtk_image_set_from_pixbuf (GTK_IMAGE (ed->icon_img), pb);
        g_object_unref (pb);

        g_free (ed->entry->icon);
        ed->entry->icon = g_strdup (text);
    }
}

static void
update_entry_exec (EntryDialog *ed)
{
    const char *text;
    
    text = gtk_entry_get_text (GTK_ENTRY (ed->exec));

    if (!text || !strlen (text))
    {
        if (ed->entry->exec)
        {
            ed->info_changed = TRUE;
            g_free (ed->entry->exec);
            ed->entry->exec = NULL;
        }
    }
    else if (!ed->entry->exec || strcmp (text, ed->entry->exec) != 0)
    {
        ed->exec_changed = TRUE;

        g_free (ed->entry->exec);
        ed->entry->exec = g_strdup (text);
    }
}

static gboolean
entry_lost_focus (GtkWidget *gentry, GdkEventFocus *ev, EntryDialog *ed)
{
    if (gentry == ed->exec_name || gentry == ed->exec_comment)
    {
        update_entry_info (ed);
    }
    else if (gentry == ed->icon_file)
    {
        update_entry_icon (ed);
    }
    else if (gentry == ed->exec)
    {
        update_entry_exec (ed);
    }

    return FALSE;
}

static void
check_button_toggled (GtkWidget *tb, EntryDialog *ed)
{
    if (tb == ed->exec_terminal)
        ed->entry->terminal = 
            gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (tb));
    else if (tb == ed->exec_startup)
        ed->entry->startup = 
            gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (tb));
}

static void
add_entry_info_options(EntryDialog *ed, GtkBox *box, GtkSizeGroup *sg)
{
    GtkWidget *hbox, *label;
    
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (box, hbox, FALSE, FALSE, 0);

    label = gtk_label_new (_("Name"));
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

    gtk_size_group_add_widget (sg, label);

    ed->exec_name = gtk_entry_new ();
    gtk_widget_show (ed->exec_name);
    gtk_box_pack_start (GTK_BOX (hbox), ed->exec_name, TRUE, TRUE, 0);
    if (ed->entry->name)
        gtk_entry_set_text (GTK_ENTRY (ed->exec_name), ed->entry->name);

    g_signal_connect (ed->exec_name, "focus-out-event",
                      G_CALLBACK (entry_lost_focus), ed);
    
    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (box, hbox, FALSE, FALSE, 0);

    label = gtk_label_new (_("Description"));
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

    gtk_size_group_add_widget (sg, label);

    ed->exec_comment = gtk_entry_new ();
    gtk_widget_show (ed->exec_comment);
    gtk_box_pack_start (GTK_BOX (hbox), ed->exec_comment, TRUE, TRUE, 0);
    gtk_widget_set_size_request (ed->exec_comment, 300, -1);
    if (ed->entry->comment)
        gtk_entry_set_text (GTK_ENTRY (ed->exec_comment), ed->entry->comment);
    
    g_signal_connect (ed->exec_comment, "focus-out-event",
                      G_CALLBACK (entry_lost_focus), ed);
}

static void
icon_menu_deactivated (GtkWidget *menu, EntryDialog *ed)
{
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ed->icon_browse), FALSE);
}

static void
position_icon_menu (GtkMenu * menu, int *x, int *y, gboolean * push_in, 
                    GtkWidget *b)
{
    GtkRequisition req;
    GdkScreen *screen;
    GdkRectangle geom;
    int num;

    /* wtf is this anyway? */
    *push_in = TRUE;

    if (!GTK_WIDGET_REALIZED (GTK_WIDGET (menu)))
        gtk_widget_realize (GTK_WIDGET (menu));

    gtk_widget_size_request (GTK_WIDGET (menu), &req);

    gdk_window_get_origin (b->window, x, y);

    *x += b->allocation.x;
    *y += b->allocation.y - req.height;

    screen = gtk_widget_get_screen (b);

    num = gdk_screen_get_monitor_at_window (screen, b->window);

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
popup_icon_menu (GtkWidget *tb, EntryDialog *ed)
{
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (tb)))
    {
        gtk_menu_popup (GTK_MENU (ed->icon_category), NULL, NULL, 
                        (GtkMenuPositionFunc) position_icon_menu, tb,
                        0, gtk_get_current_event_time ());
    }
}

static void
icon_menu_activated (GtkWidget *mi, EntryDialog *ed)
{
    int n = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (mi), "category"));
    char *cat, *name, *start, *end;

    n = CLAMP (n, 0, NUM_CATEGORIES);

    cat = xfce_icon_theme_lookup_category (global_icon_theme, n, 
                                           DLG_ICON_SIZE);
    
    if (!(start = strrchr (cat, G_DIR_SEPARATOR)))
        start = cat;
    else
        start++;

    end = strrchr (start, '.');
    
    name = g_strndup (start, end ? end - start : strlen (start));

    g_free (cat);
    
    if (!ed->entry->icon || strcmp (name, ed->entry->icon) != 0)
    {
        GdkPixbuf *pb;
        
        ed->icon_changed = TRUE;

        gtk_entry_set_text (GTK_ENTRY (ed->icon_file), name);

        g_free (ed->entry->icon);
        ed->entry->icon = name;

        pb = launcher_load_pixbuf (ed->entry->icon, DLG_ICON_SIZE);
        gtk_image_set_from_pixbuf (GTK_IMAGE (ed->icon_img), pb);
        g_object_unref (pb);
    }
    else
    {
        g_free (name);
    }
}

static void
icon_menu_browse (GtkWidget *mi, EntryDialog *ed)
{
    char *file, *path;
    
    path = 
        (ed->entry->icon != NULL && g_path_is_absolute (ed->entry->icon)) ?
                ed->entry->icon : NULL;
    
    file = select_file_with_preview (_("Select image file"), path, ed->dlg);

    if (file && g_file_test (file, G_FILE_TEST_IS_REGULAR))
    {
        GdkPixbuf *pb;
        
        gtk_entry_set_text (GTK_ENTRY (ed->icon_file), file);
        gtk_editable_set_position (GTK_EDITABLE (ed->icon_file), -1);
        update_entry_icon (ed);

        pb = xfce_pixbuf_new_from_file_at_size (file, DLG_ICON_SIZE,
                                                DLG_ICON_SIZE, NULL);
        if (!pb)
            pb = xfce_icon_theme_load_category (global_icon_theme, 0, 
                                                DLG_ICON_SIZE);
        gtk_image_set_from_pixbuf (GTK_IMAGE (ed->icon_img), pb);
        g_object_unref (pb);
    }

    g_free (file);
}

static GtkWidget *
create_icon_category_menu (EntryDialog *ed)
{
    GtkWidget *menu, *mi, *img;
    GdkPixbuf *pb;
    int i;

    menu = gtk_menu_new ();
    
    g_signal_connect (menu, "deactivate", 
                      G_CALLBACK (icon_menu_deactivated), ed);
    
    for (i = 1; i < NUM_CATEGORIES; ++i)
    {
        mi = gtk_image_menu_item_new_with_label (_(category_icons[i]));
        gtk_widget_show (mi);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

        g_object_set_data (G_OBJECT (mi), "category", GINT_TO_POINTER (i));

        g_signal_connect (mi, "activate", 
                          G_CALLBACK (icon_menu_activated), ed);

        pb = xfce_icon_theme_load_category (global_icon_theme, i, 
                                            MENU_ICON_SIZE);
        img = gtk_image_new_from_pixbuf (pb);
        gtk_widget_show (img);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), img);
        g_object_unref (pb);
    }

    mi = gtk_separator_menu_item_new ();
    gtk_widget_show (mi);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    mi = gtk_menu_item_new_with_label (_("Other..."));
    gtk_widget_show (mi);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    
    g_signal_connect (mi, "activate", 
                      G_CALLBACK (icon_menu_browse), ed);
    
    return menu;
}

static void
add_entry_icon_options (EntryDialog *ed, GtkBox *box, GtkSizeGroup *sg)
{
    GtkWidget *hbox, *hbox2, *arrow;
    GdkPixbuf *pb;

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (box, hbox, FALSE, FALSE, 0);

    ed->icon_browse = gtk_toggle_button_new ();
    gtk_widget_show (ed->icon_browse);
    gtk_box_pack_start (GTK_BOX (hbox), ed->icon_browse, FALSE, FALSE, 0);

    gtk_size_group_add_widget (sg, ed->icon_browse);

    ed->icon_category = create_icon_category_menu (ed);

    g_signal_connect (ed->icon_browse, "toggled", 
                      G_CALLBACK (popup_icon_menu), ed);
    
    hbox2 = gtk_hbox_new (FALSE, 2);
    gtk_widget_show (hbox2);
    gtk_container_add (GTK_CONTAINER (ed->icon_browse), hbox2);

    ed->icon_img = gtk_image_new ();
    gtk_widget_show (ed->icon_img);
    gtk_box_pack_start (GTK_BOX (hbox2), ed->icon_img, TRUE, TRUE, 0);

    pb = launcher_load_pixbuf (ed->entry->icon, DLG_ICON_SIZE);
        
    gtk_image_set_from_pixbuf (GTK_IMAGE (ed->icon_img), pb);
    g_object_unref (pb);

    arrow = gtk_arrow_new (GTK_ARROW_UP, GTK_SHADOW_NONE);
    gtk_widget_show (arrow);
    gtk_box_pack_start (GTK_BOX (hbox2), arrow, TRUE, TRUE, 0);
    
    ed->icon_file = gtk_entry_new ();
    gtk_widget_show (ed->icon_file);
    gtk_box_pack_start (GTK_BOX (hbox), ed->icon_file, TRUE, TRUE, 0);
    gtk_entry_set_text (GTK_ENTRY (ed->icon_file), ed->entry->icon);
    
    g_signal_connect (ed->icon_file, "focus-out-event",
                      G_CALLBACK (entry_lost_focus), ed);
}

static void
add_entry_exec_options (EntryDialog *ed, GtkBox *box, GtkSizeGroup *sg)
{
    GtkWidget *hbox, *label, *button, *img;

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (box, hbox, FALSE, FALSE, 0);

    label = gtk_label_new (_("Command"));
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

    gtk_size_group_add_widget (sg, label);

    ed->exec = gtk_entry_new ();
    gtk_widget_show (ed->exec);
    gtk_box_pack_start (GTK_BOX (hbox), ed->exec, TRUE, TRUE, 0);
    if (ed->entry->exec)
        gtk_entry_set_text (GTK_ENTRY (ed->exec), ed->entry->exec);
    
    g_signal_connect (ed->exec, "focus-out-event",
                      G_CALLBACK (entry_lost_focus), ed);

    button = gtk_button_new ();
    gtk_widget_show (button);
    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

    img = gtk_image_new_from_stock (GTK_STOCK_OPEN, GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (img);
    gtk_container_add (GTK_CONTAINER (button), img);

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (box, hbox, FALSE, FALSE, 0);

    label = gtk_label_new (NULL);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    gtk_size_group_add_widget (sg, label);

    ed->exec_terminal = 
        gtk_check_button_new_with_mnemonic (_("Run in _terminal"));
    gtk_widget_show (ed->exec_terminal);
    gtk_box_pack_start (GTK_BOX (hbox), ed->exec_terminal, TRUE, TRUE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ed->exec_terminal), 
                                  ed->entry->terminal);

    g_signal_connect (ed->exec_terminal, "toggled",
                      G_CALLBACK (check_button_toggled), ed);

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (box, hbox, FALSE, FALSE, 0);

    label = gtk_label_new (NULL);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    gtk_size_group_add_widget (sg, label);

    ed->exec_startup = 
        gtk_check_button_new_with_mnemonic (_("Use _startup notification"));
    gtk_widget_show (ed->exec_startup);
    gtk_box_pack_start (GTK_BOX (hbox), ed->exec_startup, TRUE, TRUE, 0);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ed->exec_startup), 
                                  ed->entry->startup);

    g_signal_connect (ed->exec_startup, "toggled",
                      G_CALLBACK (check_button_toggled), ed);
}

static gboolean
entry_properties_dialog (Entry *entry, GtkWindow *parent)
{
    EntryDialog *ed;
    GtkWidget *header, *vbox, *align;
    GtkSizeGroup *sg;
    gboolean changed = FALSE;

    ed = g_new0 (EntryDialog, 1);
    
    ed->entry = entry;
    
    ed->dlg = gtk_dialog_new_with_buttons (_("Edit launcher"), parent,
                                           GTK_DIALOG_NO_SEPARATOR,
                                           GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
                                           NULL);
    
    header = xfce_create_header (NULL, _("Edit launcher"));
    gtk_widget_show (header);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (ed->dlg)->vbox), header,
                        FALSE, TRUE, 0);
    gtk_widget_set_size_request (header, -1, DLG_ICON_SIZE);

    vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_widget_show (vbox);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (ed->dlg)->vbox), vbox,
                        FALSE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);

    sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

    add_entry_info_options (ed, GTK_BOX (vbox), sg);

    align = gtk_alignment_new (0, 0, 0, 0);
    gtk_widget_show (align);
    gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 0);
    gtk_widget_set_size_request (align, BORDER, BORDER);
    
    add_entry_icon_options (ed, GTK_BOX (vbox), sg);

    align = gtk_alignment_new (0, 0, 0, 0);
    gtk_widget_show (align);
    gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 0);
    gtk_widget_set_size_request (align, BORDER, BORDER);
    
    add_entry_exec_options (ed, GTK_BOX (vbox), sg);
    
    g_object_unref (sg);
    
    gtk_dialog_run (GTK_DIALOG (ed->dlg));

    gtk_widget_hide (ed->dlg);
   
    update_entry_info (ed);
    
    update_entry_icon (ed);
    
    update_entry_exec (ed);
    
    changed = ed->info_changed || ed->icon_changed || ed->exec_changed;
    
    gtk_widget_destroy (ed->dlg);
    
    g_free (ed);

    return changed;
}

/* launcher dialog */
static void
set_panel_icon (LauncherDialog *ld)
{
    GdkPixbuf *pb;

    pb = launcher_load_pixbuf (ld->launcher->entry->icon, PANEL_ICON_SIZE);
    
    xfce_iconbutton_set_pixbuf (XFCE_ICONBUTTON (ld->launcher->iconbutton),
                                pb);
    g_object_unref (pb);
}

static void
launcher_dialog_destroyed (LauncherDialog *ld)
{
    if (ld->launcher->iconbutton)
    {
        launcher_recreate_menu (ld->launcher);
        launcher_set_panel_entry (ld->launcher);
    }

    g_free (ld);
}

static void
treeview_destroyed (GtkWidget * tv)
{
    GtkTreeModel *store;

    store = gtk_tree_view_get_model (GTK_TREE_VIEW (tv));
    gtk_list_store_clear (GTK_LIST_STORE (store));
}

static gboolean
treeview_dblclick (GtkWidget * tv, GdkEventButton * evt, LauncherDialog *ld)
{
    if (evt->type == GDK_2BUTTON_PRESS)
    {
        GtkTreeSelection *sel;
        GtkTreeModel *model;
        GtkTreeIter iter;
        Entry *e;

        sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));
        gtk_tree_selection_get_selected (sel, &model, &iter);

        gtk_tree_model_get (model, &iter, 0, &e, -1);

        if (!e)
            return FALSE;
        
        if (G_UNLIKELY (!ld->dlg))
            ld->dlg = gtk_widget_get_toplevel (ld->tree);

        if (entry_properties_dialog (e, GTK_WINDOW (ld->dlg)) && 
            e == ld->launcher->entry)
        {
            set_panel_icon (ld);
        }
        
        return TRUE;
    }

    return FALSE;
}

static void
cursor_changed (GtkTreeView * tv, LauncherDialog *ld)
{
    GtkTreeSelection *sel;
    GtkTreeModel *model;
    GtkTreeIter iter;
    Entry *e;

    sel = gtk_tree_view_get_selection (tv);
    gtk_tree_selection_get_selected (sel, &model, &iter);

    gtk_tree_model_get (model, &iter, 0, &e, -1);

    if (e)
    {
        GList *l;
        
	gtk_widget_set_sensitive (ld->up, TRUE);
	gtk_widget_set_sensitive (ld->down, TRUE);
	gtk_widget_set_sensitive (ld->edit, TRUE);
        gtk_widget_set_sensitive (ld->remove, TRUE);

        if (e == ld->launcher->entry)
        {
            gtk_widget_set_sensitive (ld->remove, FALSE);
            gtk_widget_set_sensitive (ld->up, FALSE);

            if (!ld->launcher->items)
                gtk_widget_set_sensitive (ld->down, FALSE);
        }
        else
        {
            for (l = ld->launcher->items; l != NULL; l = l->next)
            {
                if (e == (Entry *)l->data)
                {
                    if (!l->next)
                        gtk_widget_set_sensitive (ld->down, FALSE);

                    break;
                }
            }
        }
    }
    else
    {
	gtk_widget_set_sensitive (ld->up, FALSE);
	gtk_widget_set_sensitive (ld->down, FALSE);
	gtk_widget_set_sensitive (ld->edit, FALSE);
	gtk_widget_set_sensitive (ld->remove, FALSE);
    }
}

static void
render_icon (GtkTreeViewColumn * col, GtkCellRenderer * cell,
	     GtkTreeModel * model, GtkTreeIter * iter, gpointer data)
{
    Entry *entry;
    GdkPixbuf *pb;

    gtk_tree_model_get (model, iter, 0, &entry, -1);

    if (entry)
    {
        pb = launcher_load_pixbuf (entry->icon, DLG_ICON_SIZE);
        g_object_set (cell, "pixbuf", pb, NULL);
        g_object_unref (pb);
    }
    else
    {
        g_object_set (cell, "pixbuf", NULL, NULL);
    }
}

static void
render_text (GtkTreeViewColumn * col, GtkCellRenderer * cell,
	     GtkTreeModel * model, GtkTreeIter * iter, GtkWidget * treeview)
{
    Entry *entry;

    gtk_tree_model_get (model, iter, 0, &entry, -1);

    if (entry)
    {
        char *text;

        if (entry->comment)
        {
            text = g_strdup_printf ("<b>%s</b>\n%s", entry->name, 
                                    entry->comment);
        }
        else
        {
            text = g_strdup_printf ("<b>%s</b>", entry->name);
        }
        
        g_object_set (cell, "markup", text, NULL);
        g_free (text);
    }
    else
    {
        g_object_set (cell, "markup", "", NULL);
    }
}

static void
launcher_dialog_add_item_tree (LauncherDialog *ld, GtkBox *box)
{
    GtkWidget *tv;
    GtkCellRenderer *cell;
    GtkTreeViewColumn *col;
    GtkListStore *store;
    GtkTreeModel *model;
    GtkTreePath *path;
    GtkTreeIter iter;
    Entry *e;
    GList *l;
    int n;

    ld->scroll = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_show (ld->scroll);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (ld->scroll),
				    GTK_POLICY_NEVER, 
                                    GTK_POLICY_NEVER);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (ld->scroll),
					 GTK_SHADOW_IN);
    gtk_box_pack_end (box, ld->scroll, TRUE, TRUE, 0);
    
    store = gtk_list_store_new (1, G_TYPE_POINTER);
    model = GTK_TREE_MODEL (store);

    ld->tree = tv = gtk_tree_view_new_with_model (model);
    gtk_widget_show (tv);
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (tv), TRUE);
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tv), FALSE);
    gtk_container_add (GTK_CONTAINER (ld->scroll), tv);

    g_signal_connect (tv, "destroy", G_CALLBACK (treeview_destroyed), NULL);

    g_object_unref (G_OBJECT (store));

    /* create the view */
    col = gtk_tree_view_column_new ();
    gtk_tree_view_append_column (GTK_TREE_VIEW (tv), col);

    cell = gtk_cell_renderer_pixbuf_new ();
    gtk_tree_view_column_pack_start (col, cell, FALSE);
    gtk_tree_view_column_set_cell_data_func (col, cell,
					     (GtkTreeCellDataFunc)
					     render_icon, NULL, NULL);

    cell = gtk_cell_renderer_text_new ();
    gtk_tree_view_column_pack_start (col, cell, TRUE);
    gtk_tree_view_column_set_cell_data_func (col, cell,
					     (GtkTreeCellDataFunc)
					     render_text, tv, NULL);

    /* fill model */
    e = ld->launcher->entry;
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter, 0, e, -1);
    
    for (l = ld->launcher->items, n = 0; l != NULL; l = l->next, n++)
    {
        if (n == 7)
        {
            GtkRequisition req;

            gtk_widget_size_request (tv, &req);

            gtk_widget_set_size_request (tv, -1, req.height);

            gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (ld->scroll),
                                            GTK_POLICY_NEVER, 
                                            GTK_POLICY_ALWAYS);
        }

	e = l->data;
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, 0, e, -1);
    }

    g_signal_connect (tv, "cursor_changed", G_CALLBACK (cursor_changed),
		      ld);

    g_signal_connect (tv, "button-press-event",
		      G_CALLBACK (treeview_dblclick), ld);
    
    path = gtk_tree_path_new_from_string ("0");
    gtk_tree_view_set_cursor (GTK_TREE_VIEW (tv), path, NULL, FALSE);
    gtk_tree_path_free (path);
}

static void
tree_button_clicked (GtkWidget *b, LauncherDialog *ld)
{
    GtkTreeSelection *sel;
    GtkTreeModel *model;
    GtkTreeIter iter, iter2;
    GtkTreePath *path;
    Entry *e;

    sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (ld->tree));
    gtk_tree_selection_get_selected (sel, &model, &iter);

    gtk_tree_model_get (model, &iter, 0, &e, -1);

    if (b == ld->up)
    {
        if (!e)
            return;

        path = gtk_tree_model_get_path (model, &iter);

        if (gtk_tree_path_prev (path))
        {
            gtk_tree_model_get_iter (model, &iter2, path);

            gtk_list_store_swap (GTK_LIST_STORE (model), &iter, &iter2);

            gtk_tree_view_set_cursor (GTK_TREE_VIEW (ld->tree), path, NULL, 
                                      FALSE);

            if (!gtk_tree_path_prev (path))
            {
                Entry *tmp = ld->launcher->entry;
                
                ld->launcher->items = g_list_remove (ld->launcher->items, e);

                ld->launcher->entry = e;

                ld->launcher->items = g_list_prepend (ld->launcher->items, 
                                                      tmp);

                set_panel_icon (ld);
            }
            else
            {
                GList *l;

                for (l = ld->launcher->items; l != NULL; l = l->next)
                {
                    if ((Entry *)l->data == e)
                    {
                        GList *l2 = l->prev;                    
                        
                        l->prev = l2->prev;
                        l2->prev = l;
                        l2->next = l->next;
                        l->next = l2;
                        
                        if (l2 == ld->launcher->items)
                            ld->launcher->items = l;
                        
                        break;
                    }
                }
            }
        }
            
        gtk_tree_path_free (path);
    }
    else if (b == ld->down)
    {
        if (!e)
            return;

        path = gtk_tree_model_get_path (model, &iter);
        gtk_tree_path_next (path);
        
        if (gtk_tree_model_get_iter (model, &iter2, path))
        {
            gtk_list_store_swap (GTK_LIST_STORE (model), &iter, &iter2);

            gtk_tree_view_set_cursor (GTK_TREE_VIEW (ld->tree), path, NULL, 
                                      FALSE);

            if (e == ld->launcher->entry)
            {
                Entry *tmp;

                tmp = ld->launcher->items->data;

                ld->launcher->items = g_list_remove (ld->launcher->items, tmp);

                ld->launcher->items = g_list_prepend (ld->launcher->items, 
                                                      ld->launcher->entry);
                
                ld->launcher->entry = tmp;

                set_panel_icon (ld);
            }
            else
            {
                GList *l;

                for (l = ld->launcher->items; l != NULL; l = l->next)
                {
                    if ((Entry *)l->data == e)
                    {
                        GList *l2 = l->next;                    
                        
                        l2->prev = l->prev;
                        l->prev = l2;
                        l->next = l2->next;
                        l2->next = l;

                        if (l == ld->launcher->items)
                            ld->launcher->items = l2;
                        
                        break;
                    }
                }
            }
        }

        gtk_tree_path_free (path);
    }
    else if (b == ld->edit)
    {
        if (!e)
            return;
        
        if (G_UNLIKELY (!ld->dlg))
            ld->dlg = gtk_widget_get_toplevel (ld->tree);

        if (entry_properties_dialog (e, GTK_WINDOW (ld->dlg)) && 
            e == ld->launcher->entry)
        {
            set_panel_icon (ld);
        }
    }
    else if (b == ld->add)
    {
        GList *l;
        int n;
        Entry *e2;
        
        iter2 = iter;
        
        e2 = g_new0 (Entry, 1);
        e2->name = g_strdup (_("New item"));
        e2->comment = g_strdup (_("This item has not yet been configured"));
        e2->icon = g_strdup ("xfce-unknown");
        
        if (e == ld->launcher->entry)
        {
            ld->launcher->items = g_list_prepend (ld->launcher->items, e2);
        }
        else
        {
            for (n = 1, l = ld->launcher->items; l != NULL; l = l->next, ++n)
            {
                if (e == (Entry *)l->data)
                {
                    ld->launcher->items = 
                        g_list_insert (ld->launcher->items, e2, n);

                    break;
                }
            }
        }

        if (g_list_length (ld->launcher->items) == 7)
        {
            GtkRequisition req;

            gtk_widget_size_request (ld->tree, &req);

            gtk_widget_set_size_request (ld->tree, -1, req.height);

            gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (ld->scroll),
                                            GTK_POLICY_NEVER, 
                                            GTK_POLICY_ALWAYS);
        }

        gtk_list_store_insert_after (GTK_LIST_STORE (model), &iter, &iter2);
        gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, e2, -1);

        path = gtk_tree_model_get_path (model, &iter);
        gtk_tree_view_set_cursor (GTK_TREE_VIEW (ld->tree), path, NULL, FALSE);
        gtk_tree_path_free (path);

        gtk_widget_show (ld->launcher->arrowbutton);
        
        if (G_UNLIKELY (!ld->dlg))
            ld->dlg = gtk_widget_get_toplevel (ld->tree);

        entry_properties_dialog (e2, GTK_WINDOW (ld->dlg));
    }
    else if (b == ld->remove)
    {
        if (!e || e == ld->launcher->entry)
            return;

        if (gtk_list_store_remove (GTK_LIST_STORE (model), &iter))
        {
            path = gtk_tree_model_get_path (model, &iter);
        }
        else
        {
            path = gtk_tree_path_new_from_string ("0");
        }
        
        gtk_tree_view_set_cursor (GTK_TREE_VIEW (ld->tree), path, NULL, FALSE);
        
        gtk_tree_path_free (path);
            
        ld->launcher->items = g_list_remove (ld->launcher->items, e);

        entry_free (e);

        if (g_list_length (ld->launcher->items) == 7)
        {
            gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (ld->scroll),
                                            GTK_POLICY_NEVER, 
                                            GTK_POLICY_NEVER);

            gtk_widget_set_size_request (ld->tree, -1, -1);
        }

        if (!ld->launcher->items)
            gtk_widget_hide (ld->launcher->arrowbutton);
    }

    cursor_changed (GTK_TREE_VIEW (ld->tree), ld);
}

static void
launcher_dialog_add_buttons (LauncherDialog *ld, GtkBox *box)
{
    GtkWidget *hbox, *b, *img;

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_end (box, hbox, FALSE, FALSE, 0);

    ld->up = b = gtk_button_new ();
    gtk_widget_show (b);
    gtk_box_pack_start (GTK_BOX (hbox), b, FALSE, FALSE, 0);

    g_signal_connect (b, "clicked", G_CALLBACK (tree_button_clicked), ld);

    img = gtk_image_new_from_stock (GTK_STOCK_GO_UP, GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (img);
    gtk_container_add (GTK_CONTAINER (b), img);

    ld->down = b = gtk_button_new ();
    gtk_widget_show (b);
    gtk_box_pack_start (GTK_BOX (hbox), b, FALSE, FALSE, 0);

    g_signal_connect (b, "clicked", G_CALLBACK (tree_button_clicked), ld);

    img = gtk_image_new_from_stock (GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_BUTTON);
    gtk_widget_show (img);
    gtk_container_add (GTK_CONTAINER (b), img);

    ld->edit = b = gtk_button_new_from_stock (GTK_STOCK_EDIT);
    gtk_widget_show (b);
    gtk_box_pack_start (GTK_BOX (hbox), b, FALSE, FALSE, 0);

    g_signal_connect (b, "clicked", G_CALLBACK (tree_button_clicked), ld);

    ld->add = b = gtk_button_new_from_stock (GTK_STOCK_ADD);
    gtk_widget_show (b);
    gtk_box_pack_start (GTK_BOX (hbox), b, FALSE, FALSE, 0);

    g_signal_connect (b, "clicked", G_CALLBACK (tree_button_clicked), ld);

    ld->remove = b = gtk_button_new_from_stock (GTK_STOCK_REMOVE);
    gtk_widget_show (b);
    gtk_box_pack_start (GTK_BOX (hbox), b, FALSE, FALSE, 0);

    g_signal_connect (b, "clicked", G_CALLBACK (tree_button_clicked), ld);

    gtk_widget_set_sensitive (b, FALSE);
}

static void
launcher_dialog_add_explanation (GtkBox *box)
{
    GtkWidget *hbox, *img, *label;

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (box, hbox, FALSE, FALSE, 0);
    
    img = gtk_image_new_from_stock (GTK_STOCK_DIALOG_INFO, 
                                    GTK_ICON_SIZE_LARGE_TOOLBAR);
    gtk_widget_show (img);
    gtk_box_pack_start (GTK_BOX (hbox), img, FALSE, FALSE, 0);
    
    label = 
        gtk_label_new (_("The first item in the list is shown on the panel. "
                         "Additional items will appear in a menu."));
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
}

static void
launcher_properties_dialog (Launcher * launcher, GtkContainer * container,
                            GtkWidget * close)
{
    LauncherDialog *ld;
    GtkWidget *vbox;

    ld = g_new0 (LauncherDialog, 1);
    
    ld->launcher = launcher;
    
    vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_widget_show (vbox);
    gtk_container_add (container, vbox);

    g_signal_connect_swapped (vbox, "destroy", 
                              G_CALLBACK (launcher_dialog_destroyed), ld);

    launcher_dialog_add_explanation (GTK_BOX (vbox));
    
    launcher_dialog_add_buttons (ld, GTK_BOX (vbox));
 
    launcher_dialog_add_item_tree (ld, GTK_BOX (vbox));
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
            value = DATA (child);

            if (value)
                entry->icon = (char *) value;
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

    launcher_set_panel_entry (launcher);

    launcher_recreate_menu (launcher);
}

static void
entry_write_xml (Entry *entry, xmlNodePtr node)
{
    xmlNodePtr parent, child;
    char value[2];

    parent = xmlNewTextChild (node, NULL, "launcher", NULL);

    if (entry->name)
        xmlNewTextChild (parent, NULL, "name", entry->name);

    if (entry->comment)
        xmlNewTextChild (parent, NULL, "comment", entry->comment);

    if (entry->icon)
        xmlNewTextChild (parent, NULL, "icon", entry->icon);

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

