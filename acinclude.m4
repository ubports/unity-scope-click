dnl AS_AC_EXPAND(VAR, CONFIGURE_VAR)
dnl
dnl example
dnl AS_AC_EXPAND(SYSCONFDIR, $sysconfdir)
dnl will set SYSCONFDIR to /usr/local/etc if prefix=/usr/local

AC_DEFUN([AS_AC_EXPAND],
[
  EXP_VAR=[$1]
  FROM_VAR=[$2]

  dnl first expand prefix and exec_prefix if necessary
  prefix_save=$prefix
  exec_prefix_save=$exec_prefix

  dnl if no prefix given, then use /usr/local, the default prefix
  if test "x$prefix" = "xNONE"; then
    prefix=$ac_default_prefix
  fi
  dnl if no exec_prefix given, then use prefix
  if test "x$exec_prefix" = "xNONE"; then
    exec_prefix=$prefix
  fi

  full_var="$FROM_VAR"
  dnl loop until it doesn't change anymore
  while true; do
    new_full_var="`eval echo $full_var`"
    if test "x$new_full_var"="x$full_var"; then break; fi
    full_var=$new_full_var
  done

  dnl clean up
  full_var=$new_full_var
  AC_SUBST([$1], "$full_var")

  dnl restore prefix and exec_prefix
  prefix=$prefix_save
  exec_prefix=$exec_prefix_save
])
# Checks for existence of coverage tools:
#  * gcov
#  * lcov
#  * genhtml
# 
# Sets ac_cv_check_gcov to yes if tooling is present
# and reports the executables to the variables LCOV, and GENHTML.
AC_DEFUN([AC_TDD_GCOV],
[
  AC_ARG_ENABLE(gcov,
  AS_HELP_STRING([--enable-gcov],
		 [enable coverage testing with gcov]),
  [use_gcov=$enableval], [use_gcov=no])

  if test "x$use_gcov" = "xyes"; then
  # we need gcc:
  if test "$GCC" != "yes"; then
    AC_MSG_ERROR([GCC is required for --enable-gcov])
  fi

  # Check if ccache is being used
  AC_CHECK_PROG(SHTOOL, shtool, shtool)
  case `$SHTOOL path $CC` in
    *ccache*[)] gcc_ccache=yes;;
    *[)] gcc_ccache=no;;
  esac

  if test "$gcc_ccache" = "yes" && (test -z "$CCACHE_DISABLE" || test "$CCACHE_DISABLE" != "1"); then
    AC_MSG_ERROR([ccache must be disabled when --enable-gcov option is used. You can disable ccache by setting environment variable CCACHE_DISABLE=1.])
  fi

  lcov_version_list="1.6 1.7 1.8 1.9 1.10"
  AC_CHECK_PROG(LCOV, lcov, lcov)
  AC_CHECK_PROG(GENHTML, genhtml, genhtml)

  if test "$LCOV"; then
    AC_CACHE_CHECK([for lcov version], glib_cv_lcov_version, [
      glib_cv_lcov_version=invalid
      lcov_version=`$LCOV -v 2>/dev/null | $SED -e 's/^.* //'`
      for lcov_check_version in $lcov_version_list; do
        if test "$lcov_version" = "$lcov_check_version"; then
          glib_cv_lcov_version="$lcov_check_version (ok)"
        fi
      done
    ])
  else
    lcov_msg="To enable code coverage reporting you must have one of the following lcov versions installed: $lcov_version_list"
    AC_MSG_ERROR([$lcov_msg])
  fi

  case $glib_cv_lcov_version in
    ""|invalid[)]
      lcov_msg="You must have one of the following versions of lcov: $lcov_version_list (found: $lcov_version)."
      AC_MSG_ERROR([$lcov_msg])
      LCOV="exit 0;"
      ;;
  esac

  if test -z "$GENHTML"; then
    AC_MSG_ERROR([Could not find genhtml from the lcov package])
  fi

  ac_cv_check_gcov=yes
  ac_cv_check_lcov=yes

  # Remove all optimization flags from CFLAGS
  changequote({,})
  CFLAGS=`echo "$CFLAGS" | $SED -e 's/-O[0-9]*//g'`
  changequote([,])

  COVERAGE_CFLAGS="-O0 -fprofile-arcs -ftest-coverage"
  COVERAGE_CXXFLAGS="-O0 -fprofile-arcs -ftest-coverage"	
  COVERAGE_LDFLAGS="-lgcov"
  COVERAGE_VALAFLAGS="--debug"

fi
]) # AC_TDD_GCOV
