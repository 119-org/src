
EXTRA_DIST = ls_localdb.h

libdir = $(prefix)/lib/partysip/
plugindir = $(libdir)

lib_LTLIBRARIES = libpsp_ls_localdb.la

libpsp_ls_localdb_la_SOURCES = ls_localdb.c ls_localdb_core.c

libpsp_ls_localdb_la_LDFLAGS = -module -avoid-version -export-dynamic \
 -L$(prefix)/lib -losip2 $(PPL_LIB)

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/ppl/unix  -I$(prefix)/include

AM_CFLAGS = $(PPL_FLAGS)

noinst_HEADERS = ls_localdb.h 

