# plugindir is set in configure

plugin_LTLIBRARIES = libgstteletext.la

# sources used to compile this plug-in
libgstteletext_la_SOURCES = gstteletextdec.c teletext.c

# flags used to compile this plugin
# add other _CFLAGS and _LIBS as needed
libgstteletext_la_CFLAGS = $(GST_CFLAGS) $(ZVBI_CFLAGS)
libgstteletext_la_LIBADD = $(GST_LIBS) $(ZVBI_LIBS)
libgstteletext_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstteletext_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gstteletextdec.h
