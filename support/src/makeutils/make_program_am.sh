#!/bin/sh
################################################################################
#
# make_program_am.sh - script for simple creating of Makefile.am for program
#                      in DiVinE
#
################################################################################
echo 'include $(top_srcdir)/Makefile.am.global'
echo ''

#Programs to build
echo -n 'bin_PROGRAMS ='
for PROGRAM in $*; do 
	echo -n ' $(top_srcdir)/bin/$(BINPREFIX)'"$PROGRAM"
done
echo ''

ALL_HH_FILES=`echo *.hh`
echo "#noinst_HEADERS = $ALL_HH_FILES # Uncomment if necessary"

ALL_CC_FILES=`echo *.cc`
for PROGRAM in $*; do
	echo "__top_srcdir__bin___BINPREFIX_${PROGRAM}_SOURCES = $ALL_CC_FILES # Modify, if necessary"
done
echo ''
echo 'LDADD = $(DIVINE_LIB) # Choose SEVINE_LIB or DIVINE_LIB'
echo ''

