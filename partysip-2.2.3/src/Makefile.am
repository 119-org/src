
INSTALL_DIR  = $(prefix)
bin_PROGRAMS = partysip

partysip_SOURCES = main.c psp_config.c psp_utils.c psp_module.c \
psp_plugin.c osip_msg.c   psp_request.c   psp_osip.c   \
tlp.c        tlp_plugin.c \
imp.c        imp_plugin.c \
psp_core.c   psp_core2.c  psp_core3.c \
psp_core4.c  psp_core5.c  \
psp_resolv.c psp_nat.c    \
sfp_fsm.c    sfp_plugin.c sfp_branch.c sfp.c sfp_fsm2.c \
ntservice.c  nt_svc.c proxyfsm.h

#partysip_LDFLAGS = -O -L$(prefix)/lib -losip2 -module -avoid-version -export-dynamic
partysip_LDFLAGS = -L$(prefix)/lib -losip2 -avoid-version -export-dynamic

partysip_LDADD = $(top_builddir)/ppl/unix/libppl.la \
$(PARTYSIP_LIB)  $(PTHREAD_LIBS)

INCLUDES =  -I$(top_srcdir)/partysip -I$(top_srcdir)/ppl/unix/ \
-I$(top_srcdir) -I$(prefix)/include

AM_CFLAGS = $(PARTYSIP_FLAGS) -DPSP_SERVER_PREFIX="\"$(prefix)\"" -DCONFIG_DIR="\"$(CONFIG_DIR)\"" $(PTHREAD_CFLAGS)

install-exec-local:
	echo "Installing script file for plugin: partysip-config";
	$(mkinstalldirs) $(DESTDIR)$(bindir)
	@chmod 0755 $(DESTDIR)$(bindir)
	$(INSTALL_SCRIPT) partysip-config $(DESTDIR)$(bindir)/partysip-config


