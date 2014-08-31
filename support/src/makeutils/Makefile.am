include $(top_srcdir)/Makefile.am.global

noinst_SCRIPTS=$(top_srcdir)/bin/make_program_am.sh \
	       $(top_srcdir)/bin/make_script_am.sh \
	       $(top_srcdir)/bin/make_documentation_dir.sh

CLEANFILES=$(noinst_SCRIPTS)
#sources of scripts are not cimprised in a distribution, because these
#scripts are not needed in a distribution


$(top_srcdir)/bin/make_program_am.sh: make_program_am.sh
	cp make_program_am.sh $(top_srcdir)/bin/

$(top_srcdir)/bin/make_script_am.sh: make_script_am.sh
	cp make_script_am.sh $(top_srcdir)/bin/

$(top_srcdir)/bin/make_documentation_dir.sh: make_documentation_dir.sh
	cp make_documentation_dir.sh $(top_srcdir)/bin/
	chmod +x $(top_srcdir)/bin/make_documentation_dir.sh

