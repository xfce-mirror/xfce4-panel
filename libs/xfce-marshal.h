
#ifndef ___xfce_marshal_MARSHAL_H__
#define ___xfce_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* BOOLEAN:ENUM (./xfce-marshal.list:1) */
extern void _xfce_marshal_BOOLEAN__ENUM (GClosure     *closure,
                                         GValue       *return_value,
                                         guint         n_param_values,
                                         const GValue *param_values,
                                         gpointer      invocation_hint,
                                         gpointer      marshal_data);

/* BOOLEAN:BOOLEAN (./xfce-marshal.list:2) */
extern void _xfce_marshal_BOOLEAN__BOOLEAN (GClosure     *closure,
                                            GValue       *return_value,
                                            guint         n_param_values,
                                            const GValue *param_values,
                                            gpointer      invocation_hint,
                                            gpointer      marshal_data);

/* BOOLEAN:INT,INT,INT (./xfce-marshal.list:3) */
extern void _xfce_marshal_BOOLEAN__INT_INT_INT (GClosure     *closure,
                                                GValue       *return_value,
                                                guint         n_param_values,
                                                const GValue *param_values,
                                                gpointer      invocation_hint,
                                                gpointer      marshal_data);

/* VOID:INT,INT (./xfce-marshal.list:4) */
extern void _xfce_marshal_VOID__INT_INT (GClosure     *closure,
                                         GValue       *return_value,
                                         guint         n_param_values,
                                         const GValue *param_values,
                                         gpointer      invocation_hint,
                                         gpointer      marshal_data);

G_END_DECLS

#endif /* ___xfce_marshal_MARSHAL_H__ */

