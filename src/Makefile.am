SUBDIRS = plugins

AM_CPPFLAGS =						\
	$(APPSTREAM_CFLAGS)				\
	$(GDKPIXBUF_CFLAGS)				\
	$(GLIB_CFLAGS)					\
	$(LIBARCHIVE_CFLAGS)				\
	$(LIBSOUP_CFLAGS)				\
	$(RPM_CFLAGS)					\
	$(SQLITE_CFLAGS)				\
	-DG_LOG_DOMAIN=\"Cra\"

noinst_PROGRAMS =					\
	createrepo_as
createrepo_as_SOURCES =					\
	cra-app.c					\
	cra-app.h					\
	cra-screenshot.c				\
	cra-screenshot.h				\
	cra-context.c					\
	cra-context.h					\
	cra-package.c					\
	cra-package.h					\
	cra-utils.c					\
	cra-utils.h					\
	cra-plugin.c					\
	cra-plugin.h					\
	cra-plugin-loader.c				\
	cra-plugin-loader.h				\
	cra-main.c
createrepo_as_LDADD =					\
	$(APPSTREAM_LIBS)				\
	$(GDKPIXBUF_LIBS)				\
	$(GLIB_LIBS)					\
	$(LIBARCHIVE_LIBS)				\
	$(LIBSOUP_LIBS)					\
	$(RPM_LIBS)

createrepo_as_CFLAGS =					\
	$(WARNINGFLAGS_C)

CLEANFILES =					\
	*.tar					\
	*.xml.gz

-include $(top_srcdir)/git.mk
