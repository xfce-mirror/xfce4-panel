#include <sys/stat.h>
#include <gtk/gtk.h>

#include "xfce.h"
#include "module.h"
#include "callbacks.h"

/* mailcheck icons */
#include "icons/mail.xpm"
#include "icons/nomail.xpm"
#include "icons/oldmail.xpm"

GtkTooltips *tooltips;

/*****************************************************************************/

/* mailcheck module */

/* this is checked when the module is loaded */
int is_xfce_panel_module = 1;

enum
{
  NO_MAIL, 
  NEW_MAIL, 
  OLD_MAIL
};

typedef struct
{
  char *mbox;
  char *command;

  int interval;
  int timeout;
  int status;

  int size;

  GdkPixbuf *current_pb;
  GdkPixbuf *nomail_pb;
  GdkPixbuf *newmail_pb;
  GdkPixbuf *oldmail_pb;

  GtkWidget *button;
  GtkWidget *image;
}
t_mailcheck;

GdkPixbuf *
get_mailcheck_pixbuf (int id)
{
  GdkPixbuf *pb;
  
  if (id == NEW_MAIL)
    pb = gdk_pixbuf_new_from_xpm_data ((const char **) mail_xpm);
  else if (id == OLD_MAIL)
    pb = gdk_pixbuf_new_from_xpm_data ((const char **) oldmail_xpm);
  else
    pb = gdk_pixbuf_new_from_xpm_data ((const char **) nomail_xpm);
  
  return pb;
}

static void
free_mailcheck (t_mailcheck * mailcheck)
{
  g_free (mailcheck->mbox);
  g_free (mailcheck->command);

  g_object_unref (mailcheck->nomail_pb);
  g_object_unref (mailcheck->oldmail_pb);
  g_object_unref (mailcheck->newmail_pb);

  gtk_widget_destroy (mailcheck->button);
  g_free (mailcheck);
}

static void
mailcheck_set_scaled_image (t_mailcheck * mailcheck)
{
  GdkPixbuf *pb;
  int width, height, bw;

  bw = mailcheck->size == SMALL ? 0 : 2;
  width = height = icon_size (mailcheck->size);

  gtk_container_set_border_width (GTK_CONTAINER (mailcheck->button), bw);
  gtk_widget_set_size_request (mailcheck->button, width + 4, height + 4);

  pb = gdk_pixbuf_scale_simple (mailcheck->current_pb,
				width - 2 * bw,
				height - 2 * bw, GDK_INTERP_BILINEAR);
  gtk_image_set_from_pixbuf (GTK_IMAGE (mailcheck->image), pb);
  g_object_unref (pb);
}

static gboolean
check_mail (t_mailcheck * mailcheck)
{
  int mail;
  struct stat s;

  if (stat (mailcheck->mbox, &s) < 0)
    mail = NO_MAIL;
  else if (!s.st_size)
    mail = NO_MAIL;
  else if (s.st_mtime < s.st_atime)
    mail = OLD_MAIL;
  else
    mail = NEW_MAIL;

  if (mail != mailcheck->status)
    {
      mailcheck->status = mail;
      
      if (mail == NO_MAIL)
	mailcheck->current_pb = mailcheck->nomail_pb;
      else if (mail == OLD_MAIL)
	mailcheck->current_pb = mailcheck->oldmail_pb;
      else
	mailcheck->current_pb = mailcheck->newmail_pb;
      
      mailcheck_set_scaled_image (mailcheck);
    }

  return TRUE;
}

void
module_run (t_mailcheck * mailcheck)
{
  mailcheck->timeout = 
    g_timeout_add (mailcheck->interval, (GSourceFunc) check_mail, mailcheck);
}

void
module_stop (t_mailcheck * mailcheck)
{
  g_source_remove (mailcheck->timeout);
  free_mailcheck (mailcheck);
}

void
module_set_size (t_mailcheck * mailcheck, int size)
{
  mailcheck->size = size;
  mailcheck_set_scaled_image (mailcheck);
}

void
module_configure (PanelModule * pm)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *mbox_entry;
  GtkWidget *mbox_button;
  GtkWidget *command_entry;
  GtkWidget *command_button;
  GtkWidget *spinbutton;
  GtkSizeGroup *sg = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  
  int response;
  t_mailcheck *mc = (t_mailcheck *) pm->data;
  
  dialog = gtk_dialog_new_with_buttons (_("Mail check options"), NULL,
					GTK_DIALOG_MODAL,
					GTK_STOCK_APPLY, GTK_RESPONSE_OK,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					NULL);
  
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
  
  vbox = gtk_vbox_new (TRUE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), 
		      vbox, FALSE, TRUE, 0);

  hbox = gtk_hbox_new (FALSE, 4);

  label = gtk_label_new (_("Mail box:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_size_group_add_widget (sg, label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  mbox_entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (mbox_entry), mc->mbox);
  gtk_box_pack_start (GTK_BOX (hbox), mbox_entry, TRUE, TRUE, 0);

  mbox_button = gtk_button_new_with_label (" ... ");
  gtk_box_pack_start (GTK_BOX (hbox), mbox_button,
		      FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
  
  hbox = gtk_hbox_new (FALSE, 4);

  label = gtk_label_new (_("Mail command:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_size_group_add_widget (sg, label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  command_entry = gtk_entry_new ();
  gtk_tooltips_set_tip (tooltips, command_entry, 
    _("Command to run when the button on the panel is clicked"), NULL);
  gtk_entry_set_text (GTK_ENTRY (command_entry), mc->command);
  gtk_box_pack_start (GTK_BOX (hbox), command_entry, TRUE, TRUE, 0);

  command_button = gtk_button_new_with_label (" ... ");
  gtk_box_pack_start (GTK_BOX (hbox), command_button,
		      FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
  
  hbox = gtk_hbox_new (FALSE, 4);

  label = gtk_label_new (_("Interval (sec):"));
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_size_group_add_widget (sg, label);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

  spinbutton = gtk_spin_button_new_with_range (1, 600, 1);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spinbutton), mc->interval);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
  
  gtk_widget_show_all (vbox);
  
  response = gtk_dialog_run (GTK_DIALOG (dialog));
  
  if (response == GTK_RESPONSE_OK)
    {
    }
  
  gtk_widget_destroy (dialog);
}

void
module_init (PanelModule * pm)
{
  const char *mail, *logname;
  t_mailcheck *mailcheck = g_new (t_mailcheck, 1);

  mail = g_getenv ("MAIL");
  
  if (mail)
    mailcheck->mbox = g_strdup (mail);
  else
    {
      logname = g_getenv ("LOGNAME");
      mailcheck->mbox = g_strconcat ("/var/spool/mail/", logname, NULL);
    }
  
  tooltips = gtk_tooltips_new ();
 
  /* TODO : read xml config here */
  
  mailcheck->command = g_strdup ("sylpheed");

  mailcheck->interval = 5000;	/* 5 sec */
  mailcheck->timeout = 0;
  mailcheck->status = NO_MAIL;

  mailcheck->size = MEDIUM;
  
  mailcheck->nomail_pb = get_mailcheck_pixbuf (NO_MAIL);
  mailcheck->oldmail_pb = get_mailcheck_pixbuf (OLD_MAIL);
  mailcheck->newmail_pb = get_mailcheck_pixbuf (NEW_MAIL);

  mailcheck->button = gtk_button_new ();
  gtk_button_set_relief (GTK_BUTTON (mailcheck->button), GTK_RELIEF_NONE);
  mailcheck->image = gtk_image_new ();

  mailcheck->current_pb = mailcheck->nomail_pb;
  mailcheck_set_scaled_image (mailcheck);

  gtk_container_add (GTK_CONTAINER (mailcheck->button), mailcheck->image);
  gtk_container_add (GTK_CONTAINER (pm->eventbox), mailcheck->button);

  gtk_widget_show_all (mailcheck->button);

  check_mail (mailcheck);

  /* signals */
  g_signal_connect_swapped (mailcheck->button, "clicked",
			    G_CALLBACK (exec_cmd), mailcheck->command);
  gtk_tooltips_set_tip (tooltips, mailcheck->button, 
			mailcheck->command, NULL);
      
  pm->data = (gpointer) mailcheck;
  pm->main = mailcheck->button;
  pm->caption = g_strdup (_("Mail check"));
  pm->run = (gpointer) module_run;
  pm->stop = (gpointer) module_stop;
  pm->set_size = (gpointer) module_set_size;
  pm->configure = (gpointer) module_configure;
}

