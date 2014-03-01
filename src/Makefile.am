AM_CPPFLAGS =						\
	$(GLIB_CFLAGS)					\
	-DG_LOG_DOMAIN=\"Cra\"

noinst_PROGRAMS =					\
	createrepo_as
createrepo_as_SOURCES =					\
	cra-plugin.c					\
	cra-plugin.h					\
	cra-plugin-loader.c				\
	cra-plugin-loader.h				\
	cra-main.c
createrepo_as_LDADD =					\
	$(GLIB_LIBS)
createrepo_as_CFLAGS =					\
	$(WARNINGFLAGS_C)

plugindir = $(libdir)/cra-plugins
plugin_LTLIBRARIES =					\
	libcra_plugin_gstreamer.la

libcra_plugin_gstreamer_la_SOURCES = cra-plugin-gstreamer.c
libcra_plugin_gstreamer_la_LIBADD = $(GLIB_LIBS)
libcra_plugin_gstreamer_la_LDFLAGS = -module -avoid-version
libcra_plugin_gstreamer_la_CFLAGS = $(GLIB_CFLAGS) $(WARNINGFLAGS_C)

-include $(top_srcdir)/git.mk
