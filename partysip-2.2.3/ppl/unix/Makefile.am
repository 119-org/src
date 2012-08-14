
SUBDIRS = ppl

lib_LTLIBRARIES = libppl.la

AM_CFLAGS = $(PPL_FLAGS)

libppl_la_SOURCES =      \
pplinit.c     \
ppldso.c      \
pplsocket.c   \
ppltime.c     \
pplpipe.c     \
pplgetopt.c   \
ppldnsv6.c    \
pplmd5.c      \
ppldbm.c      \
ppluinfo.c

libppl_la_LDFLAGS = -export-dynamic -version-info $(LIBPPL_SO_VERSION) \
-L$(prefix)/lib -losip2 $(PPL_LIB)

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/ppl/unix -I$(prefix)/include


