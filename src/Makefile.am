
NULL =

FULL_LIBEXECDIR=$(libexecdir)

libexec_PROGRAMS = polkit-gnome-authentication-agent-1

polkitgnomemarshal.h polkitgnomemarshal.c : Makefile.am $(srcdir)/polkitgnomemarshal.list
	glib-genmarshal --prefix=_polkit_gnome_marshal $(srcdir)/polkitgnomemarshal.list --header > polkitgnomemarshal.h.tmp && mv polkitgnomemarshal.h.tmp polkitgnomemarshal.h
	(echo "#include \"polkitgnomemarshal.h\""; glib-genmarshal --prefix=_polkit_gnome_marshal $(srcdir)/polkitgnomemarshal.list --body) > polkitgnomemarshal.c.tmp && mv polkitgnomemarshal.c.tmp polkitgnomemarshal.c
	touch polkitgnomemarshal.stamp

BUILT_SOURCES = polkitgnomemarshal.h polkitgnomemarshal.c

polkit_gnome_authentication_agent_1_SOURCES = 						\
	polkitgnomelistener.h			polkitgnomelistener.c			\
	polkitgnomeauthenticator.h		polkitgnomeauthenticator.c		\
	polkitgnomeauthenticationdialog.h	polkitgnomeauthenticationdialog.c	\
	main.c										\
	$(BUILT_SOURCES)								\
	$(NULL)

polkit_gnome_authentication_agent_1_CPPFLAGS = 		\
	-I$(top_srcdir)					\
	-DG_LOG_DOMAIN=\"polkit-gnome-1\"		\
	-DDATADIR=\""$(pkgdatadir)"\"			\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-DPOLKIT_AGENT_I_KNOW_API_IS_SUBJECT_TO_CHANGE	\
	$(DISABLE_DEPRECATED)				\
	$(AM_CPPFLAGS)

polkit_gnome_authentication_agent_1_CFLAGS = 		\
	$(GTK_CFLAGS)					\
	$(GCONF_CFLAGS)					\
	$(POLKIT_AGENT_CFLAGS)				\
	$(POLKIT_GOBJECT_CFLAGS)			\
	$(WARN_CFLAGS)					\
	$(AM_CFLAGS)

polkit_gnome_authentication_agent_1_LDFLAGS = 		\
	$(AM_LDFLAGS)

polkit_gnome_authentication_agent_1_LDADD = 		\
	$(GTK_LIBS)					\
	$(GCONF_LIBS)					\
	$(POLKIT_AGENT_LIBS)				\
	$(POLKIT_GOBJECT_LIBS)				\
	$(INTLLIBS)

EXTRA_DIST = polkitgnomemarshal.list

clean-local :
	rm -f *~
