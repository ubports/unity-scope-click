AC_INIT(unity-scope-click, 0.0.1, https://launchpad.net/unity-scope-click)
AC_COPYRIGHT([Copyright 2013 Canonical])

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)

#####################################################
# Silent build rules
#####################################################
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_PREREQ(2.59)

AC_CONFIG_HEADERS([config.h])

#####################################################
# Init the other things we depend on
#####################################################
AM_MAINTAINER_MODE
AM_PROG_VALAC([0.18.0])
AS_IF([test -z "$VALAC"], [AC_MSG_ERROR(["No valac compiler found."])])
AC_PROG_CC
AM_PROG_CC_C_O
AC_HEADER_STDC

LT_INIT
AC_CONFIG_MACRO_DIR([m4])

###########################
# gcov coverage reporting
###########################
m4_include([m4/gcov.m4])
AC_TDD_GCOV
AM_CONDITIONAL([HAVE_GCOV], [test "x$ac_cv_check_gcov" = xyes])
AM_CONDITIONAL([HAVE_LCOV], [test "x$ac_cv_check_lcov" = xyes])
AM_CONDITIONAL([HAVE_GCOVR], [test "x$ac_cv_check_gcovr" = xyes])
AC_SUBST(COVERAGE_CFLAGS)
AC_SUBST(COVERAGE_LDFLAGS)

#####################################################
# Check for module and library dependancies
#####################################################
GLIB_REQUIRED=2.32
PKG_CHECK_MODULES(SCOPE_DAEMON,
                  unity >= 7.0.0
                  json-glib-1.0
                  gio-unix-2.0
                  libsoup-2.4
                  oauth
                  libaccounts-glib
                  libsignon-glib
                  libnm-glib
                  gee-1.0)

AC_SUBST(SCOPE_DAEMON_CFLAGS)
AC_SUBST(SCOPE_DAEMON_LIBS)

#####################################################
# local install for distcheck and stand-alone running
#####################################################
with_localinstall="no"
AC_ARG_ENABLE(localinstall,
              AS_HELP_STRING([--enable-localinstall],
                             [Install all of the files locally instead of in system directories (for distcheck)]),
              with_localinstall=$enableval,
              with_localinstall=no)

AM_CONDITIONAL([HAVE_LOCALINSTALL], [test "x$with_localinstall" = "xyes"])

#####################################################
# Expand variables needed for config.vala
#####################################################
AS_AC_EXPAND(PREFIX, $prefix)
AC_SUBST(PREFIX)

AS_AC_EXPAND(DATADIR, $datarootdir)
AC_SUBST(DATADIR)

AS_AC_EXPAND(LIBEXECDIR, $libexecdir)
AC_SUBST(LIBEXECDIR)

AS_AC_EXPAND(LIBDIR, $libdir)
AC_SUBST(LIBDIR)

#####################################################
# Look for dbus service dir
#####################################################
if test "x$with_localinstall" = "xyes"; then
	DBUSSERVICEDIR="${datadir}/dbus-1/services/"
else
	DBUSSERVICEDIR=`$PKG_CONFIG --variable=session_bus_services_dir dbus-1`
fi
AC_SUBST(DBUSSERVICEDIR)

#####################################################
# Look for correct Scopes dir
#####################################################
if test "x$with_localinstall" = "xyes"; then
	SCOPESDIR="${datadir}/unity/scopes"
else
	SCOPESDIR=`$PKG_CONFIG --variable=scopesdir unity`
fi
AC_SUBST(SCOPESDIR)

#####################################################
# Create the Makefiles
#####################################################
AC_CONFIG_FILES([
  Makefile
  data/Makefile
  src/Makefile
])
AC_OUTPUT

#####################################################
# Output the results
#####################################################
AC_MSG_NOTICE([

  Unity Click Packages Daemon $VERSION
  -------------------------------

  Prefix          : ${prefix}

  Local install   : ${with_localinstall}

  Extra CFlags    : ${CPPFLAGS} $MAINTAINER_CFLAGS
  Extra ValaFlags : ${CPPFLAGS} $MAINTAINER_VALAFLAGS

  Scopes Directory: ${SCOPESDIR}

  Testing
    Coverage reporting : ${use_gcov}

])
