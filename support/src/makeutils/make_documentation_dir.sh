#!/bin/sh
################################################################################
#
# make_documentation_dir.sh - script for simple creating of directory for
#                             doxygen documentation in DiVinE
# 
# Detailed description:
# (1) Tries to create directory given in parameter
# (2) Creates doxyfile.in according to the position in a CVS
#     New file is very simple, you can edit it and change for example
#     the footer, style sheet or any other option of Doxygen configuration
#     This file will be during compilation translated to doxyfile.local
#     using subtitution for @TOP_PATH@
# (3) Creates Makefile.am (always the same - usualy suffice without modif.)
#
################################################################################

function print_usage() {
  echo "$0 <name of directory>"
}

if [ $# -ne 1 ]; then print_usage; exit 1; fi

CURRENT="`pwd`"

DIRECTORY="$1"
echo "Creating directory $DIRECTORY"

if ! mkdir "$DIRECTORY"; then
  echo "Creation of directory "$DIRECTORY" failed";
  exit 2;
fi;
cd "$DIRECTORY"

#cutting of path to divine top from CURRENT
SUBDIR=`echo "$CURRENT" | sed s,.*\/divine[a-z0-9_-]*\/,,`
echo "Detection of subdirectory in DiVinE CVS: $SUBDIR"
echo ""
echo "Creating doxyfile.in..."

echo "@INCLUDE_PATH = @TOP_PATH@/support/etc/doxygen/" > doxyfile.in
echo "@INCLUDE = doxyfile.local" >> doxyfile.in
echo "" >> doxyfile.in
echo "@INCLUDE_PATH = @TOP_PATH@/$SUBDIR" >> doxyfile.in
echo "@INCLUDE = doxyfile.local" >> doxyfile.in
echo "" >> doxyfile.in
echo "INPUT+= ." >> doxyfile.in
echo "# HTML_FOOTER=footer.html" >> doxyfile.in
echo "# HTML_STYLESHEET=styles.css" >> doxyfile.in
echo "# DOTFILE_DIRS=./dot" >> doxyfile.in
echo "# IMAGE_PATH += ./img" >> doxyfile.in
echo "# GENERATE_TAGFILE = library_reference.tag" >> doxyfile.in
echo "" >> doxyfile.in
echo "# For other options see divine/support/doxygen" >> doxyfile.in
echo "" >> doxyfile.in

echo "doxyfile.in completed"
echo ""
echo "Creating Makefile.am..."

echo "DOXYGEN_MORE_CONFIG_FILES=../doxyfile.local" > Makefile.am
echo "" >> Makefile.am
echo "include \$(top_srcdir)/doc/Makefile.am.doxygen" >> Makefile.am
echo "" >> Makefile.am
echo "../doxyfile.local:" >> Makefile.am
echo -e "\tcd .. && \$(MAKE) doxyfile.local" >> Makefile.am
echo "" >> Makefile.am

echo "Makefile.am completed"
echo ""
echo "New documentation directory \"$DIRECTORY\" ready to use"
echo "Do not forget to add \"$DIRECTORY\" to $SUBDIR/Makefile.am and"
echo "to add \"$SUBDIR/$DIRECTORY/Makefile\" to configure.ac"
