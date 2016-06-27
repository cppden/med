# AC_PREREQ(2.59)
AC_INIT(med_ut, 0.1)
AC_CANONICAL_HOST

AC_PROG_CXX
AC_LANG([C++])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([foreign no-installinfo no-installman silent-rules -Wall])
AM_SILENT_RULES([yes])
AM_PROG_AR

SRC_DIR=m4_esyscmd([pwd | tr -d '\n'])

CFLAGS="-O3 -Wall -Werror -funroll-loops -ffast-math -fno-strict-aliasing -pipe -pthread -rdynamic"
CXXFLAGS="-std=c++14 ${CFLAGS} -I${SRC_DIR}/include -I${SRC_DIR}/ut"
CPPFLAGS=""
LDFLAGS="-static -static-libstdc++"


#---------- substitute corresponding @var@s in Makefiles
AC_SUBST(LIBS)
AC_SUBST(CFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(CXXFLAGS)
AC_SUBST(AM_CXXFLAGS)
AC_SUBST(AM_CFLAGS)
AC_SUBST(AM_CPPFLAGS)
AC_SUBST(LDFLAGS)

AM_PROG_CC_C_O

AC_CONFIG_FILES([
	Makefile 
	ut/Makefile
])

AC_OUTPUT

