/*  $Id$
 *
 *  Copyright (C) 2004 Jasper Huijsmans <jasper@xfce.org>
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

/**
 * Functions are described using gtk-doc style comments, even though no actual
 * documentation is extracted. The format should be self-explanatory.
 *
 * The 'xfce_control_class_init' function is the only direct access to the module
 * from the panel. It is probably easiest to start reading from there. It's
 * the last function definition in this file.
 **/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include <gtk/gtk.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfcegui4/xfce_iconbutton.h>

#include <panel/xfce.h>
#include <panel/settings.h>
#include <panel/plugins.h>

#define BORDER 6

/**
 * t_sample
 *
 * This struct contains all relevant information about the plugin. A pointer
 * to a newly allocated t_sample struct is added to every new sample #Control.
 *
 * See also xfce_plugin_init().
 **/
typedef struct
{
    int intval;
    char *stringval;
    char *elementval1;
    char *elementval2;

    GtkWidget *widget;
}
t_sample;

/**
 * sample_read_config
 *
 * @control : the #Control to read configuration for
 * @node    : an #xmlNodePtr (part of the panel config file) containing the
 *            configuration.
 **/
static void
sample_read_config (Control * control, xmlNodePtr node)
{
    xmlChar *value;
    int n;
    t_sample *sample;

    if (!node || !node->children)
	return;

    sample = (t_sample *) control->data;

    /* xml properties ... */

    value = xmlGetProp (node, (const xmlChar *) "sampleint");

    if (value)
    {
	n = (int) strtol (value, NULL, 0);

	if (n > 0)
	    sample->intval = n;

	g_free (value);
    }

    value = xmlGetProp (node, (const xmlChar *) "samplestring");

    if (value)
    {
	g_free (sample->stringval);
	sample->stringval = (char *) value;
    }

    /* xml elements (child nodes) */

    for (node = node->children; node; node = node->next)
    {
	if (xmlStrEqual (node->name, (const xmlChar *) "samplenode1"))
	{
	    /* this macro is defined in <xfce4/settings.h> */
	    value = DATA (node);

	    if (value)
	    {
		g_free (sample->elementval1);
		sample->elementval1 = (char *) value;
	    }

	}
	else if (xmlStrEqual (node->name, (const xmlChar *) "samplenode2"))
	{
	    value = DATA (node);

	    if (value)
	    {
		g_free (sample->elementval2);
		sample->elementval2 = (char *) value;
	    }
	}
    }

    /* do stuff to the widget or otherwise apply new settings */
}

/**
 * sample_write_config
 *
 * @control : the #Control to write configuration for
 * @node    : an #xmlNodePtr (part of the panel config file) containing the
 *            configuration.
 **/
static void
sample_write_config (Control * control, xmlNodePtr parent)
{
    char value[5];
    t_sample *sample;

    sample = (t_sample *) control->data;

    g_snprintf (value, 4, "%d", sample->intval);
    xmlSetProp (parent, "sampleint", value);

    xmlSetProp (parent, "samplestring", sample->stringval);

    xmlNewTextChild (parent, NULL, "samplenode1", sample->elementval1);

    xmlNewTextChild (parent, NULL, "samplenode2", sample->elementval2);
}


/**
 * sample_attach_callback
 *
 * @control  : the #Control to attach callbacks to
 * @signal   : the signal name
 * @callback : callback function
 * @data     : user data
 *
 * The plugin is expected to run g_signal_connect() on all widgets that
 * receive events, at least one. This is used, for example to connect the
 * right-click menu.
 **/
static void
sample_attach_callback (Control * control, const char *signal,
			GCallback callback, gpointer data)
{
    t_sample *sample = control->data;

    g_signal_connect (sample->widget, signal, callback, data);
}

/**
 * sample_new
 *
 * Returns a newly allocated and initialized #t_sample
 **/
static t_sample *
sample_new (void)
{
    t_sample *sample;
    GtkWidget *label;

    sample = g_new0 (t_sample, 1);

    /* initialize default settings */
    sample->intval = 42;
    sample->stringval = g_strdup ("fortytwo");
    sample->elementval1 = g_strdup ("ev1");
    sample->elementval2 = NULL;

    /* this is our main widget that catches all events */
    sample->widget = gtk_event_box_new ();
    gtk_widget_show (sample->widget);

    /* give the user something to look at */
    label = gtk_label_new ("Sample");
    gtk_container_add (GTK_CONTAINER (sample->widget), label);
    gtk_widget_show (label);

    return sample;
}

/**
 * sample_free
 *
 * free the memory allocated for a sample #Control
 *
 * @control : the #Control to free memory for.
 **/
static void
sample_free (Control * control)
{
    t_sample *sample = (t_sample *) control->data;

    /* free resources, stop timeouts, etc. */
    g_free (sample->stringval);
    g_free (sample->elementval1);
    g_free (sample->elementval2);

    /* don't forget to free the memory for the t_sample struct itself */
    g_free (sample);
}

/**
 * sample_set_theme
 *
 * @control : #Control to set theme for
 * @theme   : theme name
 *
 * If your plugin uses icons, you should consider if you want to 
 * support icon themes. You can uses functions in panel/icons.h for this.
 **/
static void
sample_set_theme (Control * control, const char *theme)
{
    t_sample *sample;

    sample = (t_sample *) control->data;

    /* update the icons */
}

/* options dialog */

/**
 * SampleDialog
 *
 * struct to hold relevant information for a settings dialog for our
 * control.
 **/
typedef struct
{
    t_sample *sample;

    /* control dialog */
    GtkWidget *dialog;

    /* options */
    GtkWidget *int_spin;
    GtkWidget *string_entry;
    GtkWidget *ev1_entry;
    GtkWidget *ev2_entry;
}
SampleDialog;

/**
 * free_sample_dialog
 *
 * @sd : the #SampleDialog to free resources of
 *
 * Free memory allocated for the options dialog.
 **/
static void
free_sample_dialog (SampleDialog * sd)
{
    /* gtk widgets are handle by gtk */

    /* we have to free the SampleDialog struct */
    g_free (sd);
}

/**
 * sample_apply_options
 *
 * @sd : #SampleDialog to apply options for.
 *
 * This is our simple all-in-one callback for when one of the options changes.
 * In more complex situation you will probably want to have a separate
 * callback for each option.
 **/
static void
sample_apply_options (SampleDialog * sd)
{
    const char *tmp;
    t_sample *sample = sd->sample;

    sample->intval =
	gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (sd->int_spin));

    tmp = gtk_entry_get_text (GTK_ENTRY (sd->string_entry));

    if (tmp && *tmp)
    {
	g_free (sample->stringval);
	sample->stringval = g_strdup (tmp);
    }

    tmp = gtk_entry_get_text (GTK_ENTRY (sd->ev1_entry));

    if (tmp && *tmp)
    {
	g_free (sample->elementval1);
	sample->elementval1 = g_strdup (tmp);
    }

    tmp = gtk_entry_get_text (GTK_ENTRY (sd->ev2_entry));

    if (tmp && *tmp)
    {
	g_free (sample->elementval2);
	sample->elementval2 = g_strdup (tmp);
    }

    /* apply new settings */
}

/**
 * entry_lost_focus
 *
 * This shows a way to update plugin settings when the user leaves a text
 * entry, by connecting to the "focus-out" event on the entry.
 **/
static gboolean
entry_lost_focus (SampleDialog * sd)
{
    sample_apply_options (sd);

    /* NB: needed to let entry handle focus-out as well */
    return FALSE;
}

/**
 * int_changed
 *
 * @spin : #GtkSpinButton that changed
 * @sd   : #SampleDialog struct
 *
 * Update the settings when a spinbutton changes.
 **/
static void
int_changed (GtkSpinButton * spin, SampleDialog * sd)
{
    sd->sample->intval = gtk_spin_button_get_value_as_int (spin);
}

/**
 * sample_create_options
 *
 * @control   : #Control to create dialog for
 * @container : #GtkContainer to pack our option widgets into.
 * @done      : the close button on the dialog as #GtkWidget
 *
 * The plugin should create the widgets to set options and connect the
 * appropriate signals. The plugin is expected to use instant-apply of the
 * settings as much as possible. At least for settings that affect the
 * appearance.
 **/
static void
sample_create_options (Control * control, GtkContainer * container,
		       GtkWidget * done)
{
    GtkWidget *vbox, *hbox, *label;
    GtkSizeGroup *sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
    SampleDialog *sd;
    t_sample *sample;

    xfce_textdomain (GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");

    sd = g_new0 (SampleDialog, 1);

    sd->sample = sample = (t_sample *) control->data;

    sd->dialog = gtk_widget_get_toplevel (done);

    /* don't set a border width, the dialog will take care of that */

    vbox = gtk_vbox_new (FALSE, BORDER);
    gtk_widget_show (vbox);

    /* spin button */

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new ("Integer value:");
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_size_group_add_widget (sg, label);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    sd->int_spin = gtk_spin_button_new_with_range (1, 600, 1);
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (sd->int_spin),
			       sample->intval);
    gtk_widget_show (sd->int_spin);
    gtk_box_pack_start (GTK_BOX (hbox), sd->int_spin, FALSE, FALSE, 0);

    g_signal_connect (sd->int_spin, "value-changed",
		      G_CALLBACK (int_changed), sd);

    /* entries */

    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new ("Text:");
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_size_group_add_widget (sg, label);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    sd->string_entry = gtk_entry_new ();
    gtk_entry_set_text (GTK_ENTRY (sd->string_entry), sample->stringval);
    gtk_widget_show (sd->string_entry);
    gtk_box_pack_start (GTK_BOX (hbox), sd->string_entry, TRUE, TRUE, 0);

    g_signal_connect (sd->string_entry, "focus-out",
		      G_CALLBACK (entry_lost_focus), sd);


    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new ("More text:");
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_size_group_add_widget (sg, label);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    sd->ev1_entry = gtk_entry_new ();
    gtk_entry_set_text (GTK_ENTRY (sd->ev1_entry), sample->elementval1);
    gtk_widget_show (sd->ev1_entry);
    gtk_box_pack_start (GTK_BOX (hbox), sd->ev1_entry, TRUE, TRUE, 0);

    g_signal_connect (sd->ev1_entry, "focus-out",
		      G_CALLBACK (entry_lost_focus), sd);


    hbox = gtk_hbox_new (FALSE, BORDER);
    gtk_widget_show (hbox);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new ("Even more text:");
    gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
    gtk_size_group_add_widget (sg, label);
    gtk_widget_show (label);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

    sd->ev2_entry = gtk_entry_new ();
    gtk_entry_set_text (GTK_ENTRY (sd->ev2_entry), sample->elementval2);
    gtk_widget_show (sd->ev2_entry);
    gtk_box_pack_start (GTK_BOX (hbox), sd->ev2_entry, TRUE, TRUE, 0);

    g_signal_connect (sd->ev2_entry, "focus-out",
		      G_CALLBACK (entry_lost_focus), sd);


    /* update settings when dialog is closed */

    g_signal_connect_swapped (done, "clicked",
			      G_CALLBACK (sample_apply_options), sd);

    g_signal_connect_swapped (sd->dialog, "destroy-event",
			      G_CALLBACK (free_sample_dialog), sd);

    /* add widgets to dialog */

    gtk_container_add (container, vbox);
}

/**
 * create_sample_control
 *
 * Create a new instance of the plugin.
 * 
 * @control : #Control parent container
 *
 * Returns %TRUE on success, %FALSE on failure.
 **/
static gboolean
create_sample_control (Control * control)
{
    t_sample *sample = sample_new ();

    gtk_container_add (GTK_CONTAINER (control->base), sample->widget);

    control->data = sample;

    return TRUE;
}

/**
 * xfce_control_class_init
 *
 * Ideally, this should be the only exported symbol in the plugin, since this
 * is the only function that is directly accessed from the panel.
 *
 * Here you set up the virtual function table for the plugin and specify its
 * behaviour (e.g. uniqueness).
 * 
 * @cc : #ControlClass to initialize
 **/
G_MODULE_EXPORT void
xfce_control_class_init (ControlClass * cc)
{
    xfce_textdomain (GETTEXT_PACKAGE, LOCALEDIR, "UTF-8");

    /* Must be present: 
       - name            : unique id 
       - caption         : display name 
       - create_control  : create a new instance
       - attach_callback : connect a signal (e.g. right-click menu)
       - free            : free allocated resources of Control instance
     */
    cc->name = "sample";
    cc->caption = "Sample";

    cc->create_control = (CreateControlFunc) create_sample_control;

    cc->attach_callback = sample_attach_callback;
    cc->free = sample_free;

    /* Optional, leave as NULL to get default behaviour
       - read_config     : read configuration from xml
       - write_config    : write configuration to xml
       - create_options  : create widgets for the options dialog
       - set_size        : set size (SMALL, NORMAL, LARGE or HUGE)
       - set_orientation : set orientation (HORIZONTAL or VERTICAL)
       - set_theme       : set icon theme
       - about           : show an about dialog
     */
    cc->read_config = sample_read_config;
    cc->write_config = sample_write_config;

    cc->create_options = sample_create_options;

    cc->set_theme = sample_set_theme;

    /* cc->set_size = NULL;
     * cc->set_orientation = NULL;
     * cc->about = NULL;
     */

    /* Additional API calls */

    /* use if there should be only one instance per screen */
    control_class_set_unique (cc, TRUE);

    /* use if the gmodule should not be unloaded *
     * (usually because of library issues)       */
    control_class_set_unloadable (cc, FALSE);

    /* use to set an icon to represent the module        *
     * (you could even update it when the theme changes) */
    if (1)
    {
	GdkPixbuf *pixbuf;

	pixbuf = xfce_icon_theme_load (xfce_icon_theme_get_for_screen (NULL),
                                       "sampleicon.png", 48);

        if (pixbuf)
        {
            control_class_set_icon (cc, pixbuf);
            g_object_unref (pixbuf);
        }
    }
}

/* Macro that checks panel API version */
XFCE_PLUGIN_CHECK_INIT
