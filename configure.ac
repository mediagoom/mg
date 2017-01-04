#
#
AC_PREREQ(2.59)
AC_INIT([libmg], [0.0.3])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects]) #
AC_CONFIG_MACRO_DIR([m4])
#AC_ENABLE_SHARED
#AC_ENABLE_STATIC

AM_SILENT_RULES([yes])


AC_DEFUN([AC_PROG_FLAVOR], [AC_CHECK_PROG(FLAVOR,[flavor],[yes],[no])])

AC_PROG_FLAVOR

if test x"${FLAVOR}" == x"no" ; then
    AC_MSG_ERROR([Please compile and install flavor before trying to build mg.])
fi


CFLAGS=${CFLAGS-""}
CXXFLAGS=${CXXFLAGS-""} 


AC_DEFINE(MG_ERRROR_LOG)

AC_PROG_CXX
AC_PROG_CC

#AM_PROG_CC_C_O
#CC_CHECK_CFLAGS_APPEND([-g])
#CC_CHECK_CFLAGS_APPEND([-Wall])
#CC_CHECK_CFLAGS_APPEND([-Wsign-compare])
#CC_CHECK_CFLAGS_APPEND([-Wextra])
#CC_CHECK_CFLAGS_APPEND([-Wno-unused-parameter])
#AC_PROG_LIBTOOL
AC_PROG_RANLIB
AM_PROG_AR

#LT_INIT

# TODO(bnoordhuis) Check for -pthread vs. -pthreads
AC_CHECK_LIB([uv], [uv_default_loop])
AC_CHECK_LIB([pthread], [pthread_mutexattr_settype])
AC_CHECK_LIB([gypaes], [aes_init])
#AC_CHECK_LIB([socket], [socket])
#AM_PROG_AR

AC_ARG_ENABLE(debug,
	AS_HELP_STRING([--enable-debug],
               [enable debugging, default: no]),
	       [case "${enableval}" in
	    	yes) debug=true ;;
		no)  debug=false ;;
				      *)   AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
				      esac],
				      [debug=false])
AM_CONDITIONAL(DEBUG, test x"$debug" = x"true")


# use the C++ compiler for the following checks
AC_LANG([C++])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([string])
AC_CHECK_HEADERS([iostream])


AC_CONFIG_FILES([Makefile
		src/Makefile
		src/mg/core/Makefile
		src/mg/media/Makefile
		src/b64/Makefile
		src/mgcli/Makefile
		test/Makefile])
AC_OUTPUT
