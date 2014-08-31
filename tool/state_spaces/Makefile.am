include $(top_srcdir)/Makefile.am.global

bin_PROGRAMS = $(top_srcdir)/bin/$(BINPREFIX)predot
#bin_PROGRAMS = $(top_srcdir)/bin/$(BINPREFIX)generator 
bin_SCRIPTS = $(top_srcdir)/bin/$(BINPREFIX)draw_ss
dist_noinst_SCRIPTS = draw_state_space
CLEANFILES=$(bin_SCRIPTS)

#noinst_HEADERS = *.hh # Uncomment if necessary

#__top_srcdir__bin___BINPREFIX_generator_SOURCES = generator.cc  # Modify, if necessary
__top_srcdir__bin___BINPREFIX_predot_SOURCES = predot.cc # Modify, if necessary


LDADD = $(SEVINE_LIB) $(PROMELA_LIB)

$(top_srcdir)/bin/$(BINPREFIX)draw_ss: draw_state_space
#	You can substitute following lines by copying,
#	if you do not need substitutions
#	cp draw_state_space $(top_srcdir)/bin/$(BINPREFIX)draw_state_space
#	Otherwise, use substitutions, which are set in Makefile.am.global
	$(COMMON_SUBSTITUTIONS) <draw_state_space >$(top_srcdir)/bin/$(BINPREFIX)draw_ss
	chmod '+x' $(top_srcdir)/bin/$(BINPREFIX)draw_ss


