include $(top_srcdir)/Makefile.am.global

bin_SCRIPTS = $(top_srcdir)/bin/$(BINPREFIX)plotlog
dist_noinst_SCRIPTS = plotlog
CLEANFILES=$(bin_SCRIPTS)

$(top_srcdir)/bin/$(BINPREFIX)plotlog: plotlog
#	You can substitute following lines by copying,
#	if you do not need substitutions
#	cp plotlog $(top_srcdir)/bin/$(BINPREFIX)plotlog
#	Otherwise, use substitutions, which are set in Makefile.am.global
	$(COMMON_SUBSTITUTIONS) <plotlog >$(top_srcdir)/bin/$(BINPREFIX)plotlog
	chmod '+x' $(top_srcdir)/bin/$(BINPREFIX)plotlog

