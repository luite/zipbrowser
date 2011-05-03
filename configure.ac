#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)

# Application version:
AC_INIT([zipbrowser], 4.0)

AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_HEADERS(config.h)

# Language support.  TODO: comment these when not needed
GETTEXT_PACKAGE=$PACKAGE
AC_SUBST(GETTEXT_PACKAGE)
AC_PROG_INTLTOOL
AM_GLIB_GNU_GETTEXT

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_HEADER_STDC
AC_STDC_HEADERS
AC_PROG_INSTALL

dnl ----- Args -------------------------------------------------------------

AC_ARG_ENABLE(doxygen-docs,     
  [  --enable-doxygen-docs   build DOXYGEN API documentation (requires Doxygen)], 
     enable_doxygen_docs=$enableval,enable_doxygen_docs=no)

AC_ARG_ENABLE(epaper,
  [  --enable-epaper  enable support for epaper display [default=yes] ],
     enable_epaper=$enableval, enable_epaper=yes )

dnl ----- Checks for libraries ---------------------------------------------

dnl ------- GTK, GLib ------------------------------------------------------
PKG_CHECK_MODULES(DEPS, gtk+-2.0 >= 2.2 glib-2.0 >= 2.2 gconf-2.0 >= 2.0 gthread-2.0 webkit-1.0)
AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

dnl ------- eReader libraries-----------------------------------------------
PKG_CHECK_MODULES(LIBERXX, libergtk >= 0.1 liberipc >= 0.1 libermetadb >= 0.1 liberkeyb-client >= 0.1)
AC_SUBST(LIBERXX_CFLAGS)
AC_SUBST(LIBERXX_LIBS)

PKG_CHECK_MODULES(HTTPD, libmicrohttpd >= 0.4.6)
AC_SUBST(HTTPD_CFLAGS)
AC_SUBST(HTTPD_LIBS)

dnl ------- MACHINE_NAME definition ----------------------------------------
AC_MSG_CHECKING([machine definition])
MACHINE_NAME=${MACHINE_NAME:-dr1000s}
AC_SUBST(MACHINE_NAME)
AC_MSG_RESULT($MACHINE_NAME)


case $MACHINE_NAME in
    dr1000s)
        MACHINE_IS_DR1000S=1
        ;;
    dr1000sw)
        MACHINE_IS_DR1000SW=1
        ;;
    dr800s)
        MACHINE_IS_DR800S=1
        ;;
    dr800sw)
        MACHINE_IS_DR800SW=1
        ;;
    dr800sg|qemuarm)
        MACHINE_IS_DR800SG=1
        ;;
    *)
        AC_MSG_WARN([Unhandled MACHINE_NAME set: $MACHINE_NAME])
        MACHINE_IS_DR1000S=1
        ;;
esac

AC_DEFINE_UNQUOTED(MACHINE_NAME,           "${MACHINE_NAME}",            [String name of machine])
AC_DEFINE_UNQUOTED(MACHINE_IS_DR1000S,     ${MACHINE_IS_DR1000S:-0},     [Whether we are compiling for DR1000S machine])
AC_DEFINE_UNQUOTED(MACHINE_IS_DR1000SW,    ${MACHINE_IS_DR1000SW:-0},    [Whether we are compiling for DR1000SW machine])
AC_DEFINE_UNQUOTED(MACHINE_IS_DR800S,      ${MACHINE_IS_DR800S:-0},      [Whether we are compiling for DR800S machine])
AC_DEFINE_UNQUOTED(MACHINE_IS_DR800SW,     ${MACHINE_IS_DR800SW:-0},     [Whether we are compiling for DR800SW machine])
AC_DEFINE_UNQUOTED(MACHINE_IS_DR800SG,     ${MACHINE_IS_DR800SG:-0},     [Whether we are compiling for DR800SG machine])

AM_CONDITIONAL(MACHINE_IS_DR800S, test x$MACHINE_IS_DR800S = x1)
AM_CONDITIONAL(MACHINE_IS_DR800SW, test x$MACHINE_IS_DR800SW = x1)
AM_CONDITIONAL(MACHINE_IS_DR800SG, test x$MACHINE_IS_DR800SG = x1)

dnl ------ Doxygen docs ----------------------------------------------------

if test x$enable_doxygen_docs = xyes ; then

  AC_PATH_PROG(DOXYGEN, doxygen, no)

  AC_MSG_CHECKING([whether to build Doxygen documentation])

  if test x$DOXYGEN = xno ; then
    have_doxygen=no
  else
    have_doxygen=yes
  fi

  if test x$have_doxygen = xno; then
    AC_MSG_ERROR([Building Doxygen docs explicitly required, but Doxygen not found])
  fi

  AC_MSG_RESULT(yes)

fi

AM_CONDITIONAL(HAVE_DOXYGEN, test x$enable_doxygen_docs = xyes)


dnl ------------------------------------------------------------------------

AC_CONFIG_FILES([
    Makefile
    include/Makefile
    src/Makefile
    po/Makefile.in
    data/Makefile
    doc/Makefile
    doc/Doxyfile
])
AC_OUTPUT

dnl ==========================================================================
echo "
        App zipbrowser $VERSION
        =====================

        prefix:                             ${prefix}
        source code location:               ${srcdir}

        Building with Debug:                ${enable_debug}
        Building with epaper support:       ${enable_epaper}

        Building with API Documentation:    ${enable_doxygen_docs}

"
