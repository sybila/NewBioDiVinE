#!/bin/bash
################################################################################
#
# make_documentation_index.sh - script for creation of tree-structured
#                               index.html of documentation
#                            
# 
# Detailed description:
# (1) Searches for doxyfile.in files of divine/doc
# (2) Tries to find a special comment beginning with "#node: " or "#branch: "
# (3) Extracts a description of the documentation from it
# (4) Adds the description to index.html in divine/doc
# (5) This way finally creates the tree-structured index of documentation
#     contained in DiVinE
#
################################################################################

function print_usage() {
  echo "This script created index.html for a documentation:"
  echo "Usage: $0 path css"
  echo "path ... path to the top level directory od documentation (divine/doc)"
  echo "css  ... cascading stylesheet file to use"
}

function go_deeper() {
  CURRENTLEVEL="$1"
  for FILE in $CURRENTLEVEL; do
    NTYPE="node";
    echo "Trying $FILE:"
    FULLNAME=`sed '/^#node: .*$/p;d' < "$FILE"`
    
#    echo "FULLNAME1 = $FULLNAME"
    if [ -z "$FULLNAME" ]; then
      NTYPE="branch"
      FULLNAME=`sed '/^#branch: .*$/p;d' < "$FILE"`
    fi
#    echo "FULLNAME2 = $FULLNAME"
    
    if ! [ -z "$FULLNAME" ]; then
      NAME=`echo "$FULLNAME" | sed 's/^#'"$NTYPE"': //' | sed 's/ *-.*$//'`
      DESCRIPTION=`echo "$FULLNAME" | sed 's/^#'"$NTYPE"': //' | sed 's/^[^-]*- *//'`
#      echo "NTYPE = $NTYPE"
      echo "Extracted name: $NAME"
      echo ""
      
      #search for doxyfile.in in subdirectories:
      BRANCH_DIR=`dirname "$FILE" | sed 's/^\.\///'`
      NEXTLEVEL="`find $BRANCH_DIR -mindepth 2 -maxdepth 2 -type f -name doxyfile.in`"
      #writing to the body of index.html:
      if [ "$NTYPE" = "node" ]; then
        echo "<li><a href="$BRANCH_DIR/html/index.html">$NAME</a> - $DESCRIPTION" >> index.html
      else
        echo "<li>$NAME - $DESCRIPTION" >> index.html
      fi
      #if there are subdirectories containing doxyfile.in files,
      #call recursively
      if ! [ -z "$NEXTLEVEL" ]; then 
        echo "<ul>" >>index.html
        go_deeper "$NEXTLEVEL"
        echo "</ul>" >>index.html
      fi
      echo "</li>" >>index.html
    else
      echo "No information extracted - documentation will not be listed"
      echo ""
    fi
  
  done

}

if [ $# -ne 2 ]; then print_usage; exit 1; fi

TOP_DOC_PATH="$1"
CSS="$2"

cd "$TOP_DOC_PATH"

cp "$CSS" ./styles.css

#Writing a header of HTML file:
echo '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">' >index.html
echo '<html><head><meta http-equiv="Content-Type" content="text/html;charset=iso-8859-1">' >>index.html
echo '<title>DiVinE Documentation</title>' >> index.html
echo '<link href="styles.css" rel="stylesheet" type="text/css">' >> index.html
echo '</head><body>' >> index.html
echo '<div class="qindex">&nbsp;</div>' >> index.html
echo '<h1>DiVinE Documentation</h1>' >> index.html

echo "Searching for files doxyfile.in in $TOP_DOC_PATH"
echo "Extracting documentation description from doxyfile.in files:"
echo ""
NEXTLEVEL=`find . -mindepth 2 -maxdepth 2 -type f -name doxyfile.in`
go_deeper "$NEXTLEVEL";

#Writing a footer of HTML file:
echo '<hr size="1"><small><b>DiVinE Documentation, 2006 developed in <a href="http://www.fi.muni.cz/paradise/">ParaDiSe</a> laboratory, <a href="http://www.fi.muni.cz/">Faculty of Informatics</a>, <a href="http://www.muni.cz/">Masaryk University</a></b></small>' >> index.html
echo '</body></html>' >> index.html

