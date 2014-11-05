#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(DiVinE-cluster, 0.8.1-rc1, [divine(a)fi.muni.cz])
AC_SUBST(abs_top_srcdir)           #enables detection of absolute path
AC_CONFIG_AUX_DIR([support/src/makeutils])
AC_LANG(C++)
AM_INIT_AUTOMAKE([tar-pax foreign])

AC_CONFIG_SRCDIR([src/sevine.h])   #only safety check - not too important

#check for os type
AC_CANONICAL_BUILD


# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_INSTALL

# Checks for libraries.
#AC_CHECK_LIB([divine], [main])
#AC_CHECK_LIB([sevine], [main])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h stddef.h stdlib.h string.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CXX_NAMESPACES
AC_CXX_HAVE_IOS_BASE
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_TYPE_SIZE_T
AX_CREATE_STDINT_H(src/divine-int.hh)

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_REALLOC
AC_TYPE_SIGNAL
AC_CHECK_FUNCS([alarm floor gettimeofday memset strchr strdup])


# Checking for presence of MPI:
ACX_MPI(,[AC_MSG_WARN([Could not find properly installed MPI. Library libdivine.a cannot be compiled. Only sequential code will be compiled!!!], 1)])

AC_DIVINE

# Checks for copy from CVS (in distribution not present - reason for silent
# include):
sinclude(ac_all_deps_of_cvs.m4)

#conditional make of support
AM_CONDITIONAL(BUILD_OS_IS_DARWIN,
[case $build_os in darwin* ) true ;; * ) false ;; esac])
AM_CONDITIONAL([BUILDSUPPORT], [test "$IS_CVS_COPY" = yes])
AM_CONDITIONAL([CVSDATAPATH], [test "$IS_CVS_COPY" = yes])
AM_CONDITIONAL([HAVEMPI], [test "x$MPI_FOUND" = "x1"])

#including macros for testing of command line parameters
#there are also set initial values modified by command line parameters
sinclude(support/src/makeutils/ac_parameters.m4)

AM_CONDITIONAL([FORCE_CVS_STYLE_COMPILATION], 
               [test "$NON_CVS_STYLE_COMPILATION" = no &&
		test "$IS_CVS_COPY" = yes])

AC_CONFIG_FILES([
                Makefile
                cli/Makefile
                pepmc2/Makefile
                lib/Makefile
		src/Makefile
		tool/Makefile
		tool/ltl2ba/Makefile
		tool/model_manipulation/Makefile
		tool/state_spaces/Makefile
		tool/owcty_reversed/Makefile
		tool/owcty/Makefile
		tool/distr_map/Makefile
		tool/utils/Makefile
        support/Makefile
        support/src/debug/Makefile
        support/src/makeutils/Makefile
        support/src/Makefile
        support/etc/doxygen/Makefile
        support/etc/Makefile
	       ])


if test "x$MPI_FOUND" != "x1"; then
	echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	echo "!!                                                            !!"
	echo "!!  MPI not found. No distributed applications and libraries  !!"
	echo "!!  will be compiled!                                         !!"
	echo "!!                                                            !!"
	echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
fi


AC_OUTPUT











