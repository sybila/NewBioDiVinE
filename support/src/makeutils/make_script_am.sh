#!/bin/bash
################################################################################
#
# make_script_am.sh - script for simple creating of Makefile.am for program
#                     in DiVinE
#
################################################################################
if test "x$1" = "x-h"; then
	echo "This script prints the template of a content of Makefile.am for given scripts"
	echo "on the standard output."
	echo ''
	echo "Usage: $0 [script1] [script2] ..."
	echo ''
	exit 0
fi

echo 'include $(top_srcdir)/Makefile.am.global'
echo ''

#Scipts will be placed in CVS bin directory, after compliation
#(and in installation bin directory after installation)
echo -n 'bin_SCRIPTS ='
for SCRIPTSOURCE in $*; do
	echo -n ' $(top_srcdir)/bin/$(BINPREFIX)'"$SCRIPTSOURCE";
done
echo ''

#We need to have sources of scripts in distribution
echo -n 'dist_noinst_SCRIPTS ='
for SCRIPTSOURCE in $*; do echo -n " $SCRIPTSOURCE"; done
echo ''

#They also have to be possible to clean (defaultly, they are not):
echo 'CLEANFILES=$(bin_SCRIPTS)'
echo ''

#Rules for making of scripts (using sed):
for SCRIPTSOURCE in $*; do
	SCRIPT='$(BINPREFIX)'"$SCRIPTSOURCE"
	echo '$(top_srcdir)/bin/'"$SCRIPT: $SCRIPTSOURCE";
	echo -e '#\tYou can substitute following lines by copying,'
	echo -e '#\tif you do not need substitutions'
	echo -e '#\tcp '"$SCRIPTSOURCE"' $(top_srcdir)/bin/'"$SCRIPT"
	echo -e '#\tOtherwise, use substitutions, which are set in Makefile.am.global'
	echo -ne '\t' #TAB
	echo '$(COMMON_SUBSTITUTIONS) <'"$SCRIPTSOURCE"' >$(top_srcdir)/bin/'"$SCRIPT"
	echo -ne '\t' #TAB
	echo "chmod '+x' "'$(top_srcdir)/bin/'"$SCRIPT"
	echo ''
done

