
EXTRA_DIST = udp.h

libdir = $(prefix)/lib/partysip/
plugindir = $(libdir)

lib_LTLIBRARIES = libpsp_udp.la

libpsp_udp_la_SOURCES = udp.c udp_core.c

libpsp_udp_la_LDFLAGS = -module -avoid-version -export-dynamic \
 -L$(prefix)/lib -losip2 $(PPL_LIB)

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/ppl/unix -I$(prefix)/include

AM_CFLAGS = $(PPL_FLAGS)

noinst_HEADERS = udp.h

