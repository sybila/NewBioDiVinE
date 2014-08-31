include $(top_srcdir)/Makefile.am.global

noinst_SCRIPTS=doxyfile.local
CLEANFILES=doxyfile.local
noinst_DATA=doxyfile.in

do_subst=sed -e 's,[@]PACKAGE_VERSION[@],$(PACKAGE_VERSION),g; \
	         s,[@]TOP_PATH[@],$(TOP_PATH),g'

doxyfile.local: doxyfile.in
	$(do_subst) < doxyfile.in > doxyfile.local

