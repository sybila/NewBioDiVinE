include $(top_srcdir)/Makefile.am.global

#DIST_SUBDIRS=jar

#if JAVA_OK
#NIPSSCRIPTS= $(top_srcdir)/bin/$(BINPREFIX)bymoc_asm.pl \
#              $(top_srcdir)/bin/$(BINPREFIX)bymoc_disasm.pl \
#              $(top_srcdir)/bin/$(BINPREFIX)nips_asm.pl \
#              $(top_srcdir)/bin/$(BINPREFIX)pml2s.sh \
#              $(top_srcdir)/bin/$(BINPREFIX)nips_disasm.pl
#endif

#bin_PROGRAMS = $(top_srcdir)/bin/$(BINPREFIX)syntax \
#	       $(top_srcdir)/bin/$(BINPREFIX)dve2mcrl2 \
#	       $(top_srcdir)/bin/$(BINPREFIX)dve2promela \
#	       $(top_srcdir)/bin/$(BINPREFIX)draw_automata
bin_SCRIPTS = $(top_srcdir)/bin/$(BINPREFIX)combine
#	      $(top_srcdir)/bin/$(BINPREFIX)preprocessor \
#	      $(NIPSSCRIPTS)
#dist_noinst_SCRIPTS = combine preprocessor pml2s.sh nips_disasm.pl nips_asm.pl
#dist_pkgdata_DATA = m4_defs nips_asm_help.pl nips_asm_instr.pl
#dist_pkgjar_DATA = jar/dtdparser121.jar jar/jdom.jar jar/junit.jar jar/pml2s.jar jar/sablecc.jar
CLEANFILES=$(bin_SCRIPTS)  #no "+=", but it is understood this way

#noinst_HEADERS = *.hh # Uncomment if necessary
#__top_srcdir__bin___BINPREFIX_syntax_SOURCES = syntax.cc # Modify, if necessary
#__top_srcdir__bin___BINPREFIX_draw_automata_SOURCES = draw_automata.cc  # Modify, if necessary
#__top_srcdir__bin___BINPREFIX_dve2mcrl2_SOURCES = dve2mcrl2.cc
#__top_srcdir__bin___BINPREFIX_dve2promela_SOURCES = dve2promela.cc

LDADD = $(SEVINE_LIB) #$(PROMELA_LIB)

$(top_srcdir)/bin/$(BINPREFIX)combine: combine
	$(COMMON_SUBSTITUTIONS) <combine >$(top_srcdir)/bin/$(BINPREFIX)combine
	chmod '+x' $(top_srcdir)/bin/$(BINPREFIX)combine

#$(top_srcdir)/bin/$(BINPREFIX)preprocessor: preprocessor
#	$(COMMON_SUBSTITUTIONS) <preprocessor >$(top_srcdir)/bin/$(BINPREFIX)preprocessor
#	chmod '+x' $(top_srcdir)/bin/$(BINPREFIX)preprocessor

#$(top_srcdir)/bin/$(BINPREFIX)bymoc_asm.pl: $(top_srcdir)/bin/$(BINPREFIX)nips_asm.pl
#	cp $(top_srcdir)/bin/$(BINPREFIX)nips_asm.pl $(top_srcdir)/bin/$(BINPREFIX)bymoc_asm.pl
#	chmod '+x' $(top_srcdir)/bin/$(BINPREFIX)bymoc_asm.pl

#$(top_srcdir)/bin/$(BINPREFIX)bymoc_disasm.pl: $(top_srcdir)/bin/$(BINPREFIX)nips_disasm.pl
#	cp $(top_srcdir)/bin/$(BINPREFIX)nips_disasm.pl $(top_srcdir)/bin/$(BINPREFIX)bymoc_disasm.pl
#	chmod '+x' $(top_srcdir)/bin/$(BINPREFIX)bymoc_disasm.pl

#$(top_srcdir)/bin/$(BINPREFIX)nips_asm.pl: nips_asm.pl
#	sed -e 's,nips_asm_help.pl,@PERL_MODULES_PATH@\/nips_asm_help.pl,' <nips_asm.pl |  sed -e 's,nips_asm_instr.pl,@PERL_MODULES_PATH@\/nips_asm_instr.pl,' | $(COMMON_SUBSTITUTIONS) >$(top_srcdir)/bin/$(BINPREFIX)nips_asm.pl
#	chmod '+x' $(top_srcdir)/bin/$(BINPREFIX)nips_asm.pl


#$(top_srcdir)/bin/$(BINPREFIX)nips_disasm.pl: nips_disasm.pl
#	sed -e 's,nips_asm_help.pl,@PERL_MODULES_PATH@\/nips_asm_help.pl,' <nips_disasm.pl | sed -e 's,nips_asm_instr.pl,@PERL_MODULES_PATH@\/nips_asm_instr.pl,' | $(COMMON_SUBSTITUTIONS) >$(top_srcdir)/bin/$(BINPREFIX)nips_disasm.pl
#	chmod '+x' $(top_srcdir)/bin/$(BINPREFIX)nips_disasm.pl

#$(top_srcdir)/bin/$(BINPREFIX)pml2s.sh: pml2s.sh
#	$(COMMON_SUBSTITUTIONS) <pml2s.sh >$(top_srcdir)/bin/$(BINPREFIX)pml2s.sh
#	chmod '+x' $(top_srcdir)/bin/$(BINPREFIX)pml2s.sh

