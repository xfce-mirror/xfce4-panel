/*  xfce4
 *
 *  Copyright (C) 2004 Edscott Wilson Garcia <edscott@users.sourceforge.net>
 *  Copyright (C) 2004 Jasper Huijsmans (jasper@xfce.org)
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
#include <config.h>
#endif

#include "xfcombo.h"

#ifdef HAVE_LIBDBH

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <gmodule.h>
#include <dbh.h>

#include <libxfce4util/debug.h>
#include <libxfce4util/i18n.h>
#include <libxfce4util/util.h>

#define RUN_FLAG_FILE "xffm.runflag.2.dbh"
#define RUN_DBH_FILE "xffm.runlist.2.dbh"

#if 0
typedef struct _CompletionCombo CompletionCombo;

/* Treate everything as private, read-only.
 *
 * Use API to change members of this struct 
*/
struct _CompletionCombo
{
    GtkWidget *combo;
    GtkWidget *entry;

    gmodule *gmodule;

    xfc_combo_functions_t *fun;

    xfc_combo_info_t *info;
    
    gboolean widget_destroyed;
    
    /* callbacks */
    void (*cancel)(CompletionCombo *combo, gpointer data);
    gpointer cancel_data;
    void (*activate)(CompletionCombo *combo, gpointer data);
    gpointer activate_data;
    void (*complete)(CompletionCombo *combo, gpointer data);
    gpointer complete_data;
};

CompletionCombo *completion_combo_new (GtkCombo *combo);

void completion_combo_destroy (CompletionCombo *combo);

void completion_combo_set_cancel_callback (CompletionCombo *combo,
					   void (*callback)(CompletionCombo*,
					       		    gpointer),
					   gpointer data);

void completion_combo_set_activate_callback (CompletionCombo *combo,
					     void (*callback)(CompletionCombo*,
					       		      gpointer),
					     gpointer data);

void completion_combo_set_complete_callback (CompletionCombo *combo,
					     void (*callback)(CompletionCombo*,
					       		      gpointer),
					     gpointer data);

#define CC_SET_COMBO(combo,token)					\
	(CompletionCombo *)combo->fun->xfc_set_combo (			\
		(CompletionCombo *)combo->info, token);

#define CC_SET_BLANK(combo)						\
	(CompletionCombo *)combo->fun->xfc_set_blank (			\
		(CompletionCombo *)combo->info);

#define CC_SET_ENTRY(combo,text)					\
	(CompletionCombo *)combo->fun->xfc_set_entry (			\
		(CompletionCombo *)combo->info, text);

#define CC_READ_HISTORY(combo)						\
	(CompletionCombo *)combo->fun->xfc_read_history (		\
		(CompletionCombo *)combo->info, 			\
		(CompletionCombo *)combo->info->active_dbh_file);

#define CC_CLEAR_HISTORY(combo,text)					\
	(CompletionCombo *)combo->fun->xfc_clear_history (		\
		(CompletionCombo *)combo->info);

#define CC_IS_IN_HISTORY(combo,text) 					\
	(CompletionCombo *)combo->fun->xfc_is_in_history (		\
		(CompletionCombo *)combo->info->active_dbh_file, text);

#define CC_SAVE_TO_HISTORY(combo,text)					\
	(CompletionCombo *)combo->fun->xfc_save_to_history (		\
		(CompletionCombo *)combo->info->active_dbh_file, text);

#define CC_REMOVE_FROM_HISTORY(combo,text)				\
	(CompletionCombo *)combo->fun->xfc_remove_from_history (	\
		(CompletionCombo *)combo->info->active_dbh_file, text);

CompletionCombo *
completion_combo_new (void)
{
    CompletionCombo *cc;
    gpointer symbol;

    cc = g_new0 (CompletionCombo,1);

    cc->combo = gtk_combo_new();
    cc->entry = GTK_COMBO (cc->combo)->entry;

    cc->gmodule = get_combo_module();

    if (!g_module_find_symbol (cc->gmodule, "module_init", &symbol))
    {
	goto failed;
    }
    else
    {
	xfc_combo_fun_t *init (void);

	init = symbol;

	if (!(cc->fun = init ()))
	    goto failed;
    }

    cc->info = cc->fun->xfc_init_combo (GTK_COMBO (cc->combo));
    
    /* set all user data to the CompletionCombo */
    cc->info->cancel_user_data = cc->info->activate_user_data =
	cc->fun->extra_key_data = cc;
    
    cc->info->cancel_func = completion_combo_cancel;
    cc->info->activate_function = completion_combo_activate;
    cc->fun->extra_key_completion = completion_combo_complete;
    
    return cc;

failed:
    gtk_widget_destroy (cc->combo);
    g_free (cc);

    return NULL;
}

void completion_combo_destroy (CompletionCombo *combo);

void completion_combo_set_cancel_callback (CompletionCombo *combo,
					   void (*callback)(CompletionCombo*,
					       		    gpointer),
					   gpointer data);

void completion_combo_set_activate_callback (CompletionCombo *combo,
					     void (*callback)(CompletionCombo*,
					       		      gpointer),
					     gpointer data);

void completion_combo_set_complete_callback (CompletionCombo *combo,
					     void (*callback)(CompletionCombo*,
					       		      gpointer),
					     gpointer data);
#endif
static int refcount = 0;

static xfc_combo_functions *xfc_fun = NULL;
static GModule *xfc_cm = NULL;

void
unload_xfc (void)
{
    xfc_fun = NULL;

    DBG ("unloading module \"libxfce4_combo\"");
    
    if (!g_module_close (xfc_cm))
    {
	g_warning ("Failed to unload module \"libxfce4_combo\"");
    }

    xfc_cm = NULL;
}

xfc_combo_functions *
load_xfc (void)
{
    xfc_combo_functions *(*module_init) (void);
    gpointer symbol;
    gchar *library, *module;

    if (xfc_fun)
	return xfc_fun;

    library = g_strconcat ("libxfce4_combo.", G_MODULE_SUFFIX, NULL);
    module = g_build_filename (LIBDIR, "modules", library, NULL);

    if (!(xfc_cm = g_module_open (module, 0)))
    {
	g_warning ("cannot load xfce4-module %s\n", module);

	goto failed;
    }

    if (!g_module_symbol (xfc_cm, "module_init", &symbol))
    {
	g_error ("Cannot initialize module \"libxfce4_combo\"");

	goto failed;
    }

    module_init = symbol;

    xfc_fun = module_init ();
 
    DBG ("module %s successfully loaded", library);
    
    g_free (library);
    g_free (module);
    
    return xfc_fun;
    
failed:
    g_free (library);
    g_free (module);
    
    return NULL;
}

/* no saving to history from panel for the moment. Saving should only
 * occur after a succesfull execution, which is not determined during
 * the configuration step (where the combo module is being applied)
 * */
#if 0
static void
save_flags (gchar * in_cmd, gboolean interm, gboolean hold)
{
    DBHashTable *runflags;
    int *flags;
    GString *gs;
    gchar *file;
    
    file = xfce_get_userfile ("xffm", RUN_FLAG_FILE, NULL);

    if ((runflags = DBH_open (file)) == NULL)
    {
	if ((runflags = DBH_create (file, 11)) == NULL)
	{
	    g_warning ("Cannot create %s\n", file);
	    g_free (file);
	    return;
	}
    }
    
    g_free (file);
    
    gs = g_string_new (in_cmd);
    sprintf ((char *) DBH_KEY (runflags), "%10u", g_string_hash (gs));
    g_string_free (gs, TRUE);
    
    flags = (int *) runflags->data;
    flags[0] = interm;
    flags[1] = hold;
    
    DBH_set_recordsize (runflags, 2 * sizeof (int));
    DBH_update (runflags);
    DBH_close (runflags);

    DBG ("flags saved in dbh file for %s\n", in_cmd);
}
#endif

static void
recover_flags (gchar * in_cmd, gboolean * interm, gboolean * hold)
{
    DBHashTable *runflags;
    int *flags;
    GString *gs;
    char *file;
    
    file = xfce_get_userfile ("xffm", RUN_FLAG_FILE, NULL);

    if ((runflags = DBH_open (file)) == NULL)
    {
	g_free (file);
	*interm = *hold = FALSE;
	return;
    }
    
    g_free (file);
    
    gs = g_string_new (in_cmd);
    sprintf ((char *) DBH_KEY (runflags), "%10u", g_string_hash (gs));
    g_string_free (gs, TRUE);
    
    flags = (int *) runflags->data;
    DBH_load (runflags);
    *interm = (flags[0] != 0);
    *hold = (flags[1] != 0);
    DBH_close (runflags);

    DBG ("flags recovered from dbh file for %s, interm=%d hold=%d\n",
	 in_cmd, *interm, *hold);
}

static int
extra_key_completion (gpointer data)
{
    xfc_combo_info_t *info = data;
    ComboCallback callback;

    callback = g_object_get_data (G_OBJECT (info->combo), "callback");

    if (callback)
    {
	gpointer data;
	
	data = g_object_get_data (G_OBJECT (info->combo), "data");

	callback (info, data);

	return 1;
    }

    return 0;
}

/* exported interface */
xfc_combo_info_t *
create_completion_combo (ComboCallback completion_cb, gpointer data)
{
    xfc_combo_info_t *combo_info = NULL;

    if (!xfc_fun)
    {
	xfc_fun = load_xfc ();
	refcount = 0;
    }
    
    if (xfc_fun != NULL)
    {
	GtkWidget *command_combo;
	char *f;

	refcount++;
	
	DBG ("refcount: %d", refcount);

	command_combo = gtk_combo_new ();

	if (completion_cb)
	{
	    g_object_set_data (G_OBJECT (command_combo), "callback", 
		    	       completion_cb);

	    if (data)
	    {
		g_object_set_data (G_OBJECT (command_combo), "data", 
				   data);
	    }
	}

	combo_info = xfc_fun->xfc_init_combo ((GtkCombo *) command_combo);

	combo_info->activate_func = NULL;
	
	f = xfce_get_userfile ("xffm", RUN_DBH_FILE, NULL);
	
	xfc_fun->extra_key_completion = NULL;
	xfc_fun->extra_key_data = NULL;
	
	if (access (f, F_OK) == 0)
	{
	    xfc_fun->xfc_read_history (combo_info, f);
	    xfc_fun->xfc_set_combo (combo_info, NULL);
	}
	
	g_free (f);
	
	xfc_fun->extra_key_completion = extra_key_completion;
	xfc_fun->extra_key_data = combo_info;
    }

    return combo_info;
}

void 
destroy_completion_combo (xfc_combo_info_t *info)
{
    if (xfc_fun)
    {
	xfc_fun->xfc_destroy_combo (info);
	refcount--;
    }

    DBG ("refcount: %d", refcount);

    if (refcount < 1 && xfc_fun)
    {
	unload_xfc ();
    }
}

void 
completion_combo_set_text (xfc_combo_info_t *info, const char *text)
{
    if (xfc_fun)
	xfc_fun->xfc_set_entry (info, (char *)text);
}

gboolean 
history_check_in_terminal (const char *command)
{
    gboolean interm, hold;

    recover_flags ((char *)command, &interm, &hold);
    
    return interm;
}
#else 
/* ! HAVE_LIBDBH 
 *
 * These are just stubs to keep the compiler happy ;-) 
*/

xfc_combo_info_t *
create_completion_combo (ComboCallback completion_cb, gpointer cb)
{
    return NULL;
}

void 
destroy_completion_combo (xfc_combo_info_t *info)
{
    /* */
}

void 
completion_combo_set_text (xfc_combo_info_t *info, const char *text)
{
    /* */
}

gboolean history_check_in_terminal (const char *command)
{
    return FALSE;
}

#endif /* HAVE_LIBDBH */

