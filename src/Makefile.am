AM_CPPFLAGS =						\
	$(GLIB_CFLAGS)					\
	-DG_LOG_DOMAIN=\"Cra\"

noinst_PROGRAMS =					\
	createrepo_as

createrepo_as_SOURCES =					\
	cra-main.c

createrepo_as_LDADD =					\
	$(GLIB_LIBS)

createrepo_as_CFLAGS =					\
	$(WARNINGFLAGS_C)

-include $(top_srcdir)/git.mk
