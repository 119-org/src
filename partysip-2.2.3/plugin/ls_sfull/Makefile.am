
EXTRA_DIST = ls_sfull.h

libdir = $(prefix)/lib/partysip/
plugindir = $(libdir)

lib_LTLIBRARIES = libpsp_ls_sfull.la

libpsp_ls_sfull_la_SOURCES = ls_sfull.c ls_sfull_core.c

libpsp_ls_sfull_la_LDFLAGS = -module -avoid-version -export-dynamic  \
 -L$(prefix)/lib -losip2 $(PPL_LIB)

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/ppl/unix -I$(prefix)/include

AM_CFLAGS = $(PPL_FLAGS)

noinst_HEADERS = ls_sfull.h 

