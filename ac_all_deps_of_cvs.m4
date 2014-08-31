dnl This file contains a check, whether compilation is made in the
dnl copy of CVS or not.
dnl In case of copy of CVS it runs more tests (needed for creating
dnl sources and data normally present in distribution)

AC_CHECK_FILE([support/src/makeutils/cvs_test],
[
 dnl Print out a message
 AC_MSG_NOTICE([running in a copy of CVS, more checks will run:])
 
 dnl Checks for programs.
 AC_PROG_YACC
 AC_PROG_LEX
# ACX_DOXYGEN
# ACX_ANT
 
 dnl if we are in copy of CVS, variable IS_CVS_COPY in makefiles
 dnl will expand to "yes"
 AC_SUBST(IS_CVS_COPY,[yes])
 dnl if we are in copy of CVS, we need to create also makefiles in
 dnl doc and support
], dnl end of part for the case of successful search for file "cvs"

[
 dnl if we are not in copy of CVS, variable IS_CVS_COPY in makefiles
 dnl will expand to "no"
 AC_SUBST(IS_CVS_COPY,[no])
]) dnl end of part for the case, when file "cvs is not found


