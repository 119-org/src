
Extra_DIST = syntax.h

libdir = $(prefix)/lib/partysip/
plugindir = $(libdir)

lib_LTLIBRARIES = libpsp_syntax.la

libpsp_syntax_la_SOURCES = syntax.c syntax_core.c

libpsp_syntax_la_LDFLAGS = -module -avoid-version -export-dynamic \
-L$(prefix)/lib -losip2 $(PPL_LIB)

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/ppl/unix  -I$(prefix)/include

AM_CFLAGS = $(PPL_FLAGS)

noinst_HEADERS = syntax.h 

