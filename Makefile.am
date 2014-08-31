include $(top_srcdir)/Makefile.am.global

EXTRA_DIST=README bin/README WHATS_NEW
SUBDIRS=lib tool src support cli pepmc2
DIST_SUBDIRS=$(SUBDIRS)

#if DONTBUILDGUI
#  DIST_SUBDIRS += $(GUIDIR)
#else
#  SUBDIRS += $(GUIDIR)
#endif
  

# Custom target that should delete all Automake/Autoconf products
.PHONY: mrproper
mrproper: distclean
	@sh -c \
	'TO_DELETE="autom4te.cache aclocal.m4 configure config.status config.log"; \
	 for FILE in $$TO_DELETE; do \
	   if [ -e "$$FILE" ]; then \
	     echo "Deleting $$FILE"; \
	     rm -Rf "$$FILE"; \
	   fi; \
	 done; \
	 echo "Searching for files Makefile.in"; \
	 if FILES=`find . -name "Makefile.in"`; then \
	   echo "Deleting all files Makefile.in"; \
	   rm $$FILES; \
	 else \
	   echo "Problem during searching of Makefile.in files." 1>&2; \
	   echo "Files Makefile.in will no be deleted." 1>&2; \
	 fi;'

# Creating of all documentation:
.PHONY: doc
doc:
	cd doc && $(MAKE) all

.PHONY: doc-clean
doc-clean:
	cd doc && $(MAKE) clean

# dist-hook is used to modify the content of directory with distribution
# before it is packed
.PHONY: dist-hook
dist-hook: doc-clean doc
# 	Copying of documentation to the directory of distribution:
	echo "Searching for all Doxygen documentation in doc/refs and doc/tutorials:"
#	First create the list of html directories (it is assumed that they are
#	created by Doxygen)
#	Then gradually takes all found directories and distributes them
#	BASEDIR=directory containing found html directory - "/html" is cut
#	TARGETDIR=directory inside the directory of distribution, where to copy
#	          this part of documentation
#	BASENAME=name of directory where documentation of placed (last directory
#	         in BASEDIR)
	HTMLS=`find doc/refs -type d -name html` && \
	HTMLS="$$HTMLS "`find doc/tutorials -type d -name html` && \
	for ONEDIR in $$HTMLS; do \
	  BASEDIR=`dirname "$$ONEDIR"`; \
	  TARGETDIR="$(distdir)/$$BASEDIR"; \
	  BASENAME=`basename "$$BASEDIR"`; \
	  echo "Copying $$ONEDIR to $$TARGETDIR"; \
	  cp -r "$$ONEDIR" "$$TARGETDIR"; \
	  echo "Copying $$BASEDIR/$$BASENAME.pdf to $$TARGETDIR"; \
	  test -e "$$BASEDIR/$$BASENAME.pdf" && \
	  cp "$$BASEDIR/$$BASENAME.pdf" "$$TARGETDIR"; \
	done && \
	echo "Doxygen documentation successfully distributed" 
#	
#	Distribution of examples:
	for EXAMPLES in examples examples2; do \
		if test -e "$$EXAMPLES"; then \
			echo "Copying $$EXAMPLES"; \
			cp -r "$$EXAMPLES" "$(distdir)/"; \
		fi; \
	done
		

#We do not want the documentation to be maked always (hence divine/doc is
#only in DIST_SUBDIRS) but we want to install it
install-data-hook:
	echo "Installing documentation"
	cd doc && $(MAKE) install-data
#	Installing examples:
	$(INSTALL) -d "$(pkgdatadir)"
	for EXAMPLES in examples examples2; do \
		if test -e "$$EXAMPLES"; then \
			echo "Installing $$EXAMPLES"; \
			cp -r "$$EXAMPLES" "$(pkgdatadir)/"; \
		fi; \
	done
	
uninstall-hook:
	echo "Uninstalling documentation in $(pkgdocdir)"
	for EXAMPLES in examples examples2; do \
		rm -rf "$(pkgdatadir)/$$EXAMPLES"; \
	done
	rm -rf $(pkgdocdir)

