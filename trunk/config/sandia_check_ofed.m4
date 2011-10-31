# -*- Autoconf -*-
#
# Copyright (c)      2011  Sandia Corporation
#

# SANDIA_CHECK_EV([action-if-found], [action-if-not-found])
# ------------------------------------------------------------------------------
AC_DEFUN([SANDIA_CHECK_OFED], [
  AC_ARG_WITH([ofed],
    [AS_HELP_STRING([--with-ofed=[path]],
       [Location of OFED library])])

  saved_CPPFLAGS="$CPPFLAGS"
  saved_LDFLAGS="$LDFLAGS"
  saved_LIBS="$LIBS"

  AS_IF([test "$with_ofed" != "no"],
    [AS_IF([test ! -z "$with_ofed" -a "$with_ofed" != "yes"],
       [check_ofed_dir="$with_ofed"])
     AS_IF([test ! -z "$with_ofed_libdir" -a "$with_ofed_libdir" != "yes"],
       [check_ofed_libdir="$with_ofed_libdir"])
     OMPI_CHECK_PACKAGE([ofed],
                        [infiniband/ib.h],
                        [ibverbs],
                        [ibv_open_device],
                        [],
                        [$check_ofed_dir],
                        [$check_ofed_libdir],
                        [check_ofed_happy="yes"],
                        [check_ofed_happy="no"])
	 AS_IF([test "x$check_ofed_happy" = xno],
     	   [OMPI_CHECK_PACKAGE([ofed],
                               [infiniband/verbs.h],
							   [ibverbs],
							   [ibv_open_device],
							   [],
							   [$check_ofed_dir],
							   [$check_ofed_libdir],
							   [check_ofed_happy="yes"],
							   [check_ofed_happy="no"])])],
    [check_ofed_happy="no"])

  CPPFLAGS="$saved_CPPFLAGS"
  LDFLAGS="$saved_LDFLAGS"
  LIBS="$saved_LIBS"

  AC_SUBST(ofed_CPPFLAGS)
  AC_SUBST(ofed_LDFLAGS)
  AC_SUBST(ofed_LIBS)

  AS_IF([test "$check_ofed_happy" = "yes"],
    [$1],
    [AS_IF([test ! -z "$with_ofed" -a "$with_ofed" != "no"],
      [AC_MSG_ERROR([OFED support requested but not found.  Aborting])])
     $2])
])