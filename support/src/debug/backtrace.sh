#!/bin/bash
##########################################################################################
#
#  Script for listing of backtrace of program using executable and core file.
#  Requirements: unix (bash) shell, gdb, mktemp
#
#  Call: backtrace [executable] [core file]
#
##########################################################################################

usage() {
  echo "$0 [executable] [core file]"
}

BASENAME=''
DIRNAME=`pwd`

while [ "$DIRNAME" != '/' ] && [ "$BASENAME" != 'divine' ]; do
  BASENAME=`basename "$DIRNAME"`
  DIRNAME=`dirname "$DIRNAME"`
done

if [ "$BASENAME" != 'divine' ]; then
  echo "Directory \"divine\" not found in the current path. Run this script"
  echo "only from DiVinE CVS!"
  exit 1;
fi

DIVINE_PATH="$DIRNAME/$BASENAME"

if [ -z $1 ]; then echo "Not enough parameters."; usage; exit 1; fi
if [ -z $2 ]; then echo "Not enough parameters."; usage; exit 1; fi
if [ -z $DIVINE_PATH ]; then echo "DIVINE_PATH is not set!"; exit 1; fi

if TEMPFILE=`mktemp -t backtrace.XXXXXXX`; then echo "OK">/dev/null;
else echo "Call of mktemp failed"; exit 1; fi;

if gdb -batch -x "$DIVINE_PATH"/support/src/backtrace/backtrace.gdb -se "$1" -c "$2" 2>"$TEMPFILE"; then echo "OK">/dev/null;
else echo "Problems during runnig gdb:"; cat "$TEMPFILE"; fi;

rm "$TEMPFILE"

