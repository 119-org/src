
Extra_DIST = groups.h

libdir = $(prefix)/lib/partysip/
plugindir = $(libdir)

lib_LTLIBRARIES = libpsp_groups.la

libpsp_groups_la_SOURCES = groups.c groups_core.c

libpsp_groups_la_LDFLAGS = -module -avoid-version -export-dynamic \
 -L$(prefix)/lib -losip2 $(PPL_LIB)

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/ppl/unix -I$(prefix)/include

AM_CFLAGS = $(PPL_FLAGS)

noinst_HEADERS = groups.h 

