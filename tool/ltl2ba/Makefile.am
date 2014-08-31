include $(top_srcdir)/Makefile.am.global
noinst_HEADERS=BA_graph.hh KS_BA_graph.hh alt_graph.hh buchi_lang.hh \
	       formul.hh ltl.hh ltl_graph.hh opt_buchi.hh support_dve.hh \
	       DBA_graph.hh

bin_PROGRAMS = $(top_srcdir)/bin/$(BINPREFIX)ltl2ba \
	       $(top_srcdir)/bin/$(BINPREFIX)ltl2buchi

__top_srcdir__bin___BINPREFIX_ltl2buchi_SOURCES = \
   ltl2buchi.cc formul.cc ltl.cc ltl_graph.cc support_dve.cc \
   opt_buchi.cc BA_graph.cc KS_BA_graph.cc

__top_srcdir__bin___BINPREFIX_ltl2ba_SOURCES = \
   ltl2ba.cc formul.cc ltl.cc alt_graph.cc opt_buchi.cc \
   support_dve.cc ltl_graph.cc BA_graph.cc KS_BA_graph.cc \
   DBA_graph.cc

LDADD = $(SEVINE_LIB)

