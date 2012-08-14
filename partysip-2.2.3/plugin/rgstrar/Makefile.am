
EXTRA_DIST = rgstrar.h

libdir = $(prefix)/lib/partysip/
plugindir = $(libdir)

lib_LTLIBRARIES = libpsp_rgstrar.la

libpsp_rgstrar_la_SOURCES = rgstrar.c rgstrar_core.c

libpsp_rgstrar_la_LDFLAGS = -module -avoid-version -export-dynamic \
 -L$(prefix)/lib -losip2 $(PPL_LIB)

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/ppl/unix -I$(prefix)/include

AM_CFLAGS = $(PPL_FLAGS)

noinst_HEADERS = rgstrar.h

