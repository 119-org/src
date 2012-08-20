
INSTALL_DIR  = $(prefix)
bin_PROGRAMS = psp_users

psp_users_SOURCES = psp_users.c

psp_users_LDFLAGS = -O -L$(prefix)/lib -losip2 -module -avoid-version -export-dynamic
psp_users_LDADD = $(top_builddir)/ppl/unix/libppl.la \
$(PARTYSIP_LIB)  $(PTHREAD_LIBS) $(top_builddir)/src/psp_config.o $(top_builddir)/src/psp_utils.o

INCLUDES =  -I$(top_srcdir)/partysip -I$(top_srcdir)/ppl/unix/ \
-I$(top_srcdir) -I$(prefix)/include

AM_CFLAGS = $(PARTYSIP_FLAGS) -DPSP_SERVER_PREFIX="\"$(prefix)\"" -DCONFIG_DIR="\"$(CONFIG_DIR)\"" $(PTHREAD_CFLAGS)

