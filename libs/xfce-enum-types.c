
/* Generated data (by glib-mkenums) */

#include <xfce-enum-types.h>
#include <xfce-panel-window.h>


/* enumerations from "xfce-panel-window.h" */
GType
xfce_handle_style_get_type (void)
{
	static GType type = 0;
	if (type == 0) {
	static const GEnumValue values[] = {
	{ XFCE_HANDLE_STYLE_NONE, "XFCE_HANDLE_STYLE_NONE", "none" },
	{ XFCE_HANDLE_STYLE_BOTH, "XFCE_HANDLE_STYLE_BOTH", "both" },
	{ XFCE_HANDLE_STYLE_START, "XFCE_HANDLE_STYLE_START", "start" },
	{ XFCE_HANDLE_STYLE_END, "XFCE_HANDLE_STYLE_END", "end" },
	{ 0, NULL, NULL }
	};
	type = g_enum_register_static ("XfceHandleStyle", values);
  }
	return type;
}


/* Generated data ends here */

