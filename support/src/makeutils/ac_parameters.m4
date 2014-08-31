CXXFLAGS=
CPPFLAGS=

AC_SUBST(DEFAULT_CXXFLAGS,"-ansi -Wall")
AC_SUBST(DEFAULT_CPPFLAGS,[])

CUSTOM_CXXFLAGS=$DEFAULT_CXXFLAGS
CUSTOM_CPPFLAGS=$DEFAULT_CPPFLAGS

# pridelat volbu 

################################################################################
# check - flag for setting the level of self-checking
################################################################################

AC_ARG_ENABLE(
   [checks],
   AS_HELP_STRING([--enable-checks=ARG],
                  [select level of self-checking:
                  (1) onlyfast = only constant time checks enabled,
                  (2) slow = also non-constant time checks enabled,
                  (3) no = no checks enabled
                  (default: no)
                  It is recommended to run your application using
                  DiVinE Library at least once with DiVinE compiled with
                  --enable-checks=slow to prove, that your application
                  uses DiVinE Library correctly]),
    PARAMVALUE="${enableval}"
   ,
    PARAMVALUE=no
)

AC_SUBST(CHECKS_SLOW_CPPFLAGS,["-DDIVINE_SLOW_CHECKS -DDIVINE_FAST_CHECKS"])
AC_SUBST(CHECKS_ONLYFAST_CPPFLAGS,[-DDIVINE_FAST_CHECKS])
if test x"${PARAMVALUE}" = xslow; then
     CUSTOM_CPPFLAGS="$CPPFLAGS $CHECKS_SLOW_CPPFLAGS"
elif test x"${PARAMVALUE}" = xonlyfast; then
     CUSTOM_CPPFLAGS="$CPPFLAGS $CHECKS_ONLYFAST_CPPFLAGS"
elif test ! x"${PARAMVALUE}" = xno; then
     AC_MSG_ERROR(Unknown argument ${PARAMVALUE} given to --enable-checks)
# if PARAMVALUE=no, then nothing to do.
fi

################################################################################
# debug - flag for enabling/disabling optimisations, debugging
#         informations and run-time messages
################################################################################

AC_SUBST(DEBUG_YES_CXXFLAGS,[-g])
AC_SUBST(DEBUG_YES_CPPFLAGS,[-DDIVINE_DEBUG_MESSAGES])
AC_SUBST(DEBUG_NO_CPPFLAGS,[-O3])

AC_ARG_ENABLE(
   [debug],
   AS_HELP_STRING([--enable-debug],
                  [--enable-debug implies compiled debugging informations,
                   run-time messages and no optimizations (defaultly disabled)]),
   CUSTOM_CXXFLAGS="$CXXFLAGS $DEBUG_YES_CXXFLAGS"
   CUSTOM_CPPFLAGS="$CPPFLAGS $DEBUG_YES_CPPFLAGS",
   CUSTOM_CXXFLAGS="$CXXFLAGS $DEBUG_NO_CPPFLAGS"
)

################################################################################
# non-cvs-compile - flag for enabling other than debugging style of compilation
#                   in a copy of CVS
################################################################################

AC_ARG_ENABLE(
   [non-cvs-compilation-flags],
   AS_HELP_STRING([--enable-non-cvs-compilation-flags],
                  [Only works, when you are using CVS copy of DiVinE.
		   Makes --enable-checks and --enable-debug
		   working. In a copy of CVS these debugging and
		   checking levels are overriden to be proper for
		   easy development]),
   NON_CVS_STYLE_COMPILATION=yes,
   NON_CVS_STYLE_COMPILATION=no
)

AC_SUBST(CUSTOM_CXXFLAGS)
AC_SUBST(CUSTOM_CPPFLAGS)


################################################################################
# gui - flag for disabling compilation of GUI (defaultly enabled)
################################################################################


AC_ARG_ENABLE(gui,
                AS_HELP_STRING(--disable-gui,[Turn off compilation of GUI]),
                [case "${enableval}" in
                      yes) GUIDIR=dwi ;;
                       no) GUIDIR= ;;
                        *) AC_MSG_ERROR(bad value ${enableval} for --enable-without-gui) ;;
                  esac],[GUIDIR=dwi])

AC_SUBST(GUIDIR)



