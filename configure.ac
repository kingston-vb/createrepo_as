# Copyright (C) 2014 Richard Hughes <richard@hughsie.com>
AC_PREREQ(2.63)

AC_INIT([createrepo_as],[0.1.0],[http://www.hughski.com/])
AC_CONFIG_SRCDIR(src)
AM_INIT_AUTOMAKE([1.9 no-dist-gzip dist-xz tar-ustar foreign])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_PROG_LIBTOOL

# enable nice build output on automake1.11
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_PROG_CC
AC_PROG_INSTALL
LT_INIT
AM_PROG_CC_C_O

dnl ---------------------------------------------------------------------------
dnl - Extra verbose warning switches
dnl ---------------------------------------------------------------------------
if test "$GCC" = "yes"; then
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wall"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wcast-align -Wno-uninitialized"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wmissing-declarations"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wpointer-arith"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wcast-align"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wwrite-strings"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Winit-self"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wreturn-type"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wformat-nonliteral"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wformat-security"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wmissing-include-dirs"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wmissing-format-attribute"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wclobbered"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wempty-body"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wignored-qualifiers"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wsign-compare"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wtype-limits"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wuninitialized"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Waggregate-return"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wdeclaration-after-statement"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wshadow"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wno-strict-aliasing"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Winline"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wmissing-parameter-type"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Woverride-init"
else
	WARNINGFLAGS_C=""
fi
AC_SUBST(WARNINGFLAGS_C)

PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.36.0 gobject-2.0 gthread-2.0 gio-2.0 >= 2.25.9 gio-unix-2.0 gmodule-2.0)
PKG_CHECK_MODULES(LIBARCHIVE, libarchive)
PKG_CHECK_MODULES(GDKPIXBUF, gtk+-3.0 gdk-pixbuf-2.0)
PKG_CHECK_MODULES(SQLITE, sqlite3)
PKG_CHECK_MODULES(LIBSOUP, libsoup-2.4)
PKG_CHECK_MODULES(APPSTREAM, appstream-glib >= 0.1.5)
PKG_CHECK_MODULES(FREETYPE, pango fontconfig freetype2 >= 9.10.0)

# rpm (default enabled)
AC_ARG_ENABLE(rpm, AS_HELP_STRING([--disable-rpm],[Disable rpm support]), enable_rpm=$enableval)
if test x$enable_rpm != xno; then
	PKG_CHECK_MODULES(RPM, rpm, HAVE_RPM="yes", HAVE_RPM="no")
	if test "x$HAVE_RPM" = "xyes"; then
		AC_DEFINE(HAVE_RPM, 1, [define if RPM is installed])
	else
		if test x$enable_rpm = xyes; then
			AC_MSG_ERROR([rpm enabled but not found])
		fi
	fi
else
	HAVE_RPM=no
fi
AM_CONDITIONAL(HAVE_RPM, test x$HAVE_RPM = xyes)

AC_CONFIG_FILES([
Makefile
src/Makefile
src/plugins/Makefile
])
AC_OUTPUT
