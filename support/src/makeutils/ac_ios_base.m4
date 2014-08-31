dnl @synopsis AC_CXX_HAVE_IOS_BASE
dnl
dnl Check if ios_base is present. In old versions of compiler, ios is used
dnl instead of ios_base
dnl
dnl @category Cxx
dnl @author Pavel Simecek <xsimece1@fi.muni.cz>
dnl @version 2005-03-21
dnl @license GPLWithACException

AC_DEFUN([AC_CXX_HAVE_IOS_BASE],
[AC_CACHE_CHECK(whether the compiler has ios_base,
ac_cv_cxx_have_ios_base,
[AC_REQUIRE([AC_CXX_NAMESPACES])
  AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  AC_TRY_COMPILE([#include <ios>
#ifdef HAVE_NAMESPACES
using namespace std;
#endif
],[ ios_base::iostate my_iostate; return 0; ],
  ac_cv_cxx_have_ios_base=yes, ac_cv_cxx_have_ios_base=no)
  AC_LANG_RESTORE
])
if test "$ac_cv_cxx_have_ios_base" = no; then
   AC_DEFINE(ios_base,ios,[define if the compiler has ios_base])
fi
])

