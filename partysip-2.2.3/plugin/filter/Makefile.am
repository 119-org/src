
Extra_DIST = filter.h

libdir = $(prefix)/lib/partysip/
plugindir = $(libdir)

lib_LTLIBRARIES = libpsp_filter.la

libpsp_filter_la_SOURCES = filter.c filter_core.c

libpsp_filter_la_LDFLAGS = -module -avoid-version -export-dynamic \
 -L$(prefix)/lib -losip2 $(PPL_LIB)

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/ppl/unix -I$(prefix)/include

AM_CFLAGS = $(PPL_FLAGS)

noinst_HEADERS = filter.h 

