EXTRA_DIST = partysip.conf partysipd

pkgdatadir = $(prefix)/$(PACKAGE)

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(CONFIG_DIR);
	@chmod 0755 $(DESTDIR)$(CONFIG_DIR);
	@echo "Installing admin files : partysip.conf";
	@if test -f $(DESTDIR)$(CONFIG_DIR)/partysip.conf; then \
	  echo "********************************************************"; \
	  echo "**** File partysip.conf has already been installed. ****"; \
	  echo "****   MERGE CHANGES MANUALLY in partysip.conf.     ****"; \
	  echo "********************************************************"; \
	  echo "Installing new admin file: partysip.conf.$(PSP_VERSION)";  \
	  echo "********************************************************"; \
	  $(INSTALL_DATA) $(top_srcdir)/conf/partysip.conf $(DESTDIR)$(CONFIG_DIR)/partysip.conf.$(PSP_VERSION); \
	else \
	  echo "********************************************************"; \
	  echo "****         First installation detected.           ****"; \
	  echo "****      Please, edit the file partysip.conf.      ****"; \
	  echo "********************************************************"; \
	  $(INSTALL_DATA) $(top_srcdir)/conf/partysip.conf $(DESTDIR)$(CONFIG_DIR)/; \
	fi


