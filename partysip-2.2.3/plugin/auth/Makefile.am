
EXTRA_DIST = auth.h

libdir = $(prefix)/lib/partysip/
plugindir = $(libdir)

lib_LTLIBRARIES = libpsp_auth.la

libpsp_auth_la_SOURCES = auth.c auth_core.c

libpsp_auth_la_LDFLAGS = -module -avoid-version -export-dynamic \
-L$(prefix)/lib -losip2 $(PPL_LIB)

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/ppl/unix -I$(prefix)/include

AM_CFLAGS = $(PPL_FLAGS)

noinst_HEADERS = auth.h

