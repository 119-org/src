
# prepare for automake 1.5:
#AUTOMAKE_OPTIONS= dist-bzip2 1.5

EXTRA_DIST = README COPYING-2

SUBDIRS = ppl src partysip plugin scripts windows doc conf tools debian

rpm:
	rpm -ba partysip.spec

upload: dist
	@echo "######## Maintainer only ##########";
	@echo "building files for upload in ftp://savannah.gnu.org/incoming/savannah/partysip";
	@echo "building: $(PACKAGE)-$(VERSION).tar.gz.sig";
	gpg -b $(PACKAGE)-$(VERSION).tar.gz

indent:
	indent */*.h
	indent */*.c
	indent */*/*.h
	indent */*/*.c
	indent */*/*/*.h
	rm -rf */*.h~
	rm -rf */*.c~
	rm -rf */*/*.h~
	rm -rf */*/*.c~
	rm -rf */*/*/*.h~


