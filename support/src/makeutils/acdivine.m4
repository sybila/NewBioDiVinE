AC_DEFUN([AC_DIVINE], [
AC_PREREQ(2.50) dnl for AC_LANG_CASE

#network buffer size
AC_ARG_VAR(NETBUFFERSIZE,[Size of the buffer for sending messages between workstations])
if test x = x"$NETBUFFERSIZE"; then
	NETBUFFERSIZE=8192
fi
AC_SUBST(NETBUFFERSIZE)

])dnl AC_DIVINE

