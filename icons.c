#include "xfce.h"
#include "icons.h"

/* general icons */
#include "icons/unknown.xpm"

#include "icons/edit.xpm"
#include "icons/file1.xpm"
#include "icons/file2.xpm"
#include "icons/games.xpm"
#include "icons/man.xpm"
#include "icons/multimedia.xpm"
#include "icons/network.xpm"
#include "icons/paint.xpm"
#include "icons/print.xpm"
#include "icons/schedule.xpm"
#include "icons/sound.xpm"
#include "icons/terminal.xpm"

/* icons for the panel */
#include "icons/minilock.xpm"
#include "icons/miniinfo.xpm"
#include "icons/minipalet.xpm"
#include "icons/minipower.xpm"

#include "icons/handle.xpm"
#include "icons/addicon.xpm"
#include "icons/close.xpm"
#include "icons/iconify.xpm"
#include "icons/up.xpm"
#include "icons/down.xpm"

/* program icons */
#include "icons/diag_icon.xpm"
#include "icons/menu_icon.xpm"
#include "icons/xfce_icon.xpm"

/* trashcan icons */
#include "icons/trash_full.xpm"
#include "icons/trash_empty.xpm"

GdkPixbuf *xfce_icons[NUM_ICONS];
GdkPixbuf *minilock_pb, *miniinfo_pb, *minipalet_pb, *minipower_pb;
GdkPixbuf *handle_pb, *addicon_pb, *close_pb, *iconify_pb, *up_pb, *down_pb;
GdkPixbuf *diag_icon_pb, *menu_icon_pb, *xfce_icon_pb;
GdkPixbuf *trash_full_pb, *trash_empty_pb;


static void
set_icon_names (void)
{
  icon_names [DEFAULT_ICON] = _("Default");
  icon_names [EDIT_ICON] = _("Editor");
  icon_names [FILE1_ICON] = _("File management");
  icon_names [FILE2_ICON] = _("Utilities");
  icon_names [GAMES_ICON] = _("Games");
  icon_names [MAN_ICON] = _("Help browser");
  icon_names [MULTIMEDIA_ICON] = _("Multimedia");
  icon_names [NETWORK_ICON] = _("Network");
  icon_names [PAINT_ICON] = _("Graphics");
  icon_names [PRINT_ICON] = _("Printer");
  icon_names [SCHEDULE_ICON] = _("Productvity");
  icon_names [SOUND_ICON] = _("Sound");
  icon_names [TERMINAL_ICON] = _("Terminal");
}

void
create_pixbufs (void)
{
  set_icon_names ();
  
  /* general icons */
  xfce_icons[UNKNOWN_ICON] =
    gdk_pixbuf_new_from_xpm_data ((const char **) unknown_xpm);
  xfce_icons[EDIT_ICON] =
    gdk_pixbuf_new_from_xpm_data ((const char **) edit_xpm);
  xfce_icons[FILE1_ICON] =
    gdk_pixbuf_new_from_xpm_data ((const char **) file1_xpm);
  xfce_icons[FILE2_ICON] =
    gdk_pixbuf_new_from_xpm_data ((const char **) file2_xpm);
  xfce_icons[GAMES_ICON] =
    gdk_pixbuf_new_from_xpm_data ((const char **) games_xpm);
  xfce_icons[MAN_ICON] =
    gdk_pixbuf_new_from_xpm_data ((const char **) man_xpm);
  xfce_icons[MULTIMEDIA_ICON] =
    gdk_pixbuf_new_from_xpm_data ((const char **) multimedia_xpm);
  xfce_icons[NETWORK_ICON] =
    gdk_pixbuf_new_from_xpm_data ((const char **) network_xpm);
  xfce_icons[PAINT_ICON] =
    gdk_pixbuf_new_from_xpm_data ((const char **) paint_xpm);
  xfce_icons[PRINT_ICON] =
    gdk_pixbuf_new_from_xpm_data ((const char **) print_xpm);
  xfce_icons[SCHEDULE_ICON] =
    gdk_pixbuf_new_from_xpm_data ((const char **) schedule_xpm);
  xfce_icons[SOUND_ICON] =
    gdk_pixbuf_new_from_xpm_data ((const char **) sound_xpm);
  xfce_icons[TERMINAL_ICON] =
    gdk_pixbuf_new_from_xpm_data ((const char **) terminal_xpm);

  /* small buttons on central panel */
  minilock_pb = gdk_pixbuf_new_from_xpm_data ((const char **) minilock_xpm);
  miniinfo_pb = gdk_pixbuf_new_from_xpm_data ((const char **) miniinfo_xpm);
  minipalet_pb = gdk_pixbuf_new_from_xpm_data ((const char **) minipalet_xpm);
  minipower_pb = gdk_pixbuf_new_from_xpm_data ((const char **) minipower_xpm);

  /* move handles */
  handle_pb = gdk_pixbuf_new_from_xpm_data ((const char **) handle_xpm);
  addicon_pb = gdk_pixbuf_new_from_xpm_data ((const char **) addicon_xpm);
  close_pb = gdk_pixbuf_new_from_xpm_data ((const char **) close_xpm);
  iconify_pb = gdk_pixbuf_new_from_xpm_data ((const char **) iconify_xpm);

  /* popup buttons */
  up_pb = gdk_pixbuf_new_from_xpm_data ((const char **) up_xpm);
  down_pb = gdk_pixbuf_new_from_xpm_data ((const char **) down_xpm);

  /* app icons */
  diag_icon_pb = gdk_pixbuf_new_from_xpm_data ((const char **) diag_icon_xpm);
  menu_icon_pb = gdk_pixbuf_new_from_xpm_data ((const char **) menu_icon_xpm);
  xfce_icon_pb = gdk_pixbuf_new_from_xpm_data ((const char **) xfce_icon_xpm);

  /* trash icons */
  trash_full_pb = 
      gdk_pixbuf_new_from_xpm_data ((const char **) trash_full_xpm);
  trash_empty_pb = 
      gdk_pixbuf_new_from_xpm_data ((const char **) trash_empty_xpm);
}

GdkPixbuf *
get_pixbuf_from_id (int id)
{
  GdkPixbuf *pb;

  if (id >= UNKNOWN_ICON && id < NUM_ICONS)
    pb = xfce_icons[id];
  else
    switch (id)
      {
      case MINILOCK_ICON:
	pb = minilock_pb;
	break;
      case MINIINFO_ICON:
	pb = miniinfo_pb;
	break;
      case MINIPALET_ICON:
	pb = minipalet_pb;
	break;
      case MINIPOWER_ICON:
	pb = minipower_pb;
	break;
      case HANDLE_ICON:
	pb = handle_pb;
	break;
      case ADDICON_ICON:
	pb = addicon_pb;
	break;
      case CLOSE_ICON:
	pb = close_pb;
	break;
      case ICONIFY_ICON:
	pb = iconify_pb;
	break;
      case UP_ICON:
	pb = up_pb;
	break;
      case DOWN_ICON:
	pb = down_pb;
	break;
      case DIAG_ICON:
	pb = diag_icon_pb;
	break;
      case MENU_ICON:
	pb = menu_icon_pb;
	break;
      case XFCE_ICON:
	pb = xfce_icon_pb;
	break;
      default:
	pb = xfce_icons[UNKNOWN_ICON];
	break;
      }

  g_object_ref (pb);
  return pb;
}

/* for modules */
GdkPixbuf *
get_trashcan_pixbuf (int id)
{
  GdkPixbuf *pb;
  
  if (id == TRASH_FULL_ICON)
    pb = trash_full_pb;
  else
    pb = trash_empty_pb;
  
  g_object_ref (pb);
  return pb;
}
