/* Convenience header for conditional use of GNU <libintl.h>.
   Copyright (C) 1995-1998, 2000-2002 Free Software Foundation, Inc.
*/
#ifndef _LIBGETTEXT_H
#define _LIBGETTEXT_H 1

/* NLS can be disabled through the configure --disable-nls option.  */
#if ENABLE_NLS

/* Get declarations of GNU message catalog functions.  */
# include <libintl.h>

#else

/* Disabled NLS.
   The casts to 'const char *' serve the purpose of producing warnings
   for invalid uses of the value returned from these functions.
   On pre-ANSI systems without 'const', the config.h file is supposed to
   contain "#define const".  */
# define gettext(Msgid) ((const char *) (Msgid))
# define dgettext(Domainname, Msgid) ((const char *) (Msgid))
# define dcgettext(Domainname, Msgid, Category) ((const char *) (Msgid))
# define ngettext(Msgid1, Msgid2, N) \
    ((N) == 1 ? (const char *) (Msgid1) : (const char *) (Msgid2))
# define dngettext(Domainname, Msgid1, Msgid2, N) \
    ((N) == 1 ? (const char *) (Msgid1) : (const char *) (Msgid2))
# define dcngettext(Domainname, Msgid1, Msgid2, N, Category) \
    ((N) == 1 ? (const char *) (Msgid1) : (const char *) (Msgid2))
# define textdomain(Domainname) ((const char *) (Domainname))
# define bindtextdomain(Domainname, Dirname) ((const char *) (Dirname))
# define bind_textdomain_codeset(Domainname, Codeset) ((const char *) (Codeset))

#endif

/* A pseudo function call that serves as a marker for the automated
   extraction of messages, but does not call gettext().  The run-time
   translation is done at a different place in the code.
   The argument, String, should be a literal string.  Concatenated strings
   and other string expressions won't work.
   The macro's expansion is not parenthesized, so that it is suitable as
   initializer for static 'char[]' or 'const char[]' variables.  */
#define gettext_noop(String) String

#endif /* _LIBGETTEXT_H */

/* gettext convenience */
#define _(x) gettext(x)
#define N_(x) gettext_noop(x)

