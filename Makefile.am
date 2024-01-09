SUBDIRS = polkitgtk src po docs

EXTRA_DIST = \
	autogen.sh		\
	TODO			\
	HACKING

DISTCHECK_CONFIGURE_FLAGS="--disable-introspection --disable-maintainer-mode"

clean-local :
	rm -f *~ po/*~
