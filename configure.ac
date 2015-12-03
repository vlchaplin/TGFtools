#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([tgftools], [1.2], [chapliv@uah.edu], [tgftools])

AM_INIT_AUTOMAKE([tgftools], [1.2])

AC_PREFIX_DEFAULT(/usr/local/tgftools)

#AC_CONFIG_SRCDIR([])
#AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for library functions.
AC_HEADER_STDC
AC_HEADER_STDBOOL
AC_CHECK_FUNCS([strptime])

AC_ARG_ENABLE([prog-vers],
  [AS_HELP_STRING([--enable-prog-vers],
    [append the package version to the built programs (Default=no)])],
  [
  putvernum=true
  AC_MSG_NOTICE([will append version names to build products]) ],
  [
  putvernum=false
  ]
   )
   
AC_ARG_ENABLE([dislin],
  [AS_HELP_STRING([--enable-dislin],
    [also build the plotting program 'make_data_plot'. DISLIN must be defined (Default=yes)])],
  [
  use_dislin=false
  if test x$enableval = xyes; then
  use_dislin=true
  AC_CHECK_LIB( Xm, xmMainWindowClassRec, [], 
  	[
  	use_dislin=false
  	AC_MSG_ERROR( make sure libXm (the OpenMotif linked with Dislin) is in your linker path (LDFLAGS=-L<path>) or '--disable-dislin' )
  	] )
  fi
  ],
  [
     use_dislin=true
  AC_CHECK_LIB( Xm, xmMainWindowClassRec, [], 
  	[
  	use_dislin=false
  	AC_MSG_ERROR( make sure libXm (the OpenMotif linked with Dislin) is in your linker path (LDFLAGS=-L<path>) or '--disable-dislin' )
  	] )
  ]
 )

AC_ARG_WITH([sqlite3],
  [AS_HELP_STRING([--with-sqlite3],
    [location of the SQLite installation, containing include/ and lib/. Default is /usr/local])],
  [SQLITE_CFLAGS="-I$withval/include"
   SQLITE_LDFLAGS="-L$withval/lib"],
  [SQLITE_CFLAGS=''
   SQLITE_LDFLAGS=''])

AC_ARG_WITH([sqlite3-include],
  [AS_HELP_STRING([--with-sqlite3-include],
    [location of the SQLite header, if not in /usr/local/include])],
  [SQLITE_CFLAGS="-I$withval"]
  )

AC_ARG_WITH([sqlite3-lib],
  [AS_HELP_STRING([--with-sqlite3-lib],
    [location of the SQLite library, if not in /usr/local/lib])],
  [SQLITE_LDFLAGS="-L$withval"]
  )
  
AC_ARG_WITH([heasoft],
  [AS_HELP_STRING([--with-heasoft],
    [location of HEASOFT installation. Overwrites HEADAS. Assumes this directory contains lib/ and include/])],
  [LHEAS_CFLAGS="-I$withval/include"
   LHEAS_LDFLAGS="-L$withval/lib"
   noLHEASflags=false],
  [noLHEASflags=true])
 
AC_ARG_WITH([cfitsio],
  [AS_HELP_STRING([--with-cfitsio],
    [location of CFITSIO installation. This has the same effect as --with-heasoft.])],
  [LHEAS_CFLAGS="-I$withval/include"
   LHEAS_LDFLAGS="-L$withval/lib"
   noLHEASflags=false],
  [])
  
AC_ARG_WITH([cfitsio-include],
  [AS_HELP_STRING([--with-cfitsio-include],
    [location of fitsio.h if non-standard installation])],
  [LHEAS_CFLAGS="-I$withval/include"
   noLHEASflags=false],
  [])

AC_ARG_WITH([cfitsio-lib],
  [AS_HELP_STRING([--with-cfitsio-lib],
    [location of cfitsio library if non-standard installation])],
  [LHEAS_LDFLAGS="-L$withval/lib"
   noLHEASflags=false],
  [])

AC_CHECK_PROG(WHICH_RM, rm, rm)
AC_CHECK_PROG(WHICH_ENV, env, env)

AC_ARG_WITH([rspgen],
  [AS_HELP_STRING([--with-rspgen],
    [full path to a directory containing gbmrsp.exe and inputs/, without trailing '/'])],
  [GBMRSP="$withval/"
   AC_CHECK_FILE(${GBMRSP}gbmrsp.exe, [], [AC_MSG_ERROR(gbmrsp.exe must exist at the given location)])],
  [
   GBMRSP="./"
   runtimeGBMRSP=true
   AC_MSG_WARN(gbmrsp.exe directory not given: built programs will read run-time environment \$GBMRSPDIR)
   ])

AC_ARG_WITH([drmdb],
  [AS_HELP_STRING([--with-drmdb],
    [full path to GBMDRMdb002 without trailing '/', e.g. '/path/to/GBMDRMdb002'])],
  [DRMDB="$withval/"
   AC_CHECK_FILE(${DRMDB}, [], [AC_MSG_ERROR(GBMDRMdb002 must exist at the given location)])
   ],
  [
   DRMDB="GBMDRMdb002"
   AC_MSG_WARN(GBMDRMdb002 location not given: built programs will read run-time environment \$GBMDRMDB002)
   runtimeDRMDB=true
   ])
AC_ARG_WITH([datadb],
  [AS_HELP_STRING([--with-datadb],
    [full path to an existing database of GBM data files.  One is created by default in <prefix>])],
  [DATADB="$withval/"
   defaultDATADB=false],
  [
   defaultDATADB=true
   ])

AM_CONDITIONAL(PUTVERSN, test x$putvernum = xtrue)
AM_CONDITIONAL(GBMRSP_DEF, test x$runtimeGBMRSP = xtrue)
AM_CONDITIONAL(DRMDB_DEF, test x$runtimeDRMDB = xtrue)
AM_CONDITIONAL(USE_HEADAS, test x$noLHEASflags = xtrue)
AM_CONDITIONAL(DEF_DATADB, test x$defaultDATADB = xtrue)
AM_CONDITIONAL(USE_DISLIN, test x$use_dislin = xtrue)

AC_SUBST([SQLITE_CFLAGS])
AC_SUBST([SQLITE_LDFLAGS])
AC_SUBST([LHEAS_CFLAGS])
AC_SUBST([LHEAS_LDFLAGS])
AC_SUBST([GBMRSP])
AC_SUBST([DRMDB])
AC_SUBST([DATADB])

AC_ARG_VAR(WHICH_RM, Program which unlinks files. Default is rm)
AC_ARG_VAR(WHICH_ENV, Program which sets environment variables. Default is env)
AC_ARG_VAR(HEADAS, Location of heasoft. Overwritten by '--with-heasoft'.)
AC_ARG_VAR(DISLIN, Location of dislin.)

AC_SUBST([HEADAS])
AC_SUBST([DISLIN])


#AC_CHECK_FILE(${GBMRSP}gbmrsp.exe, [], [AC_MSG_ERROR(gbmrsp.exe must exist at the given location)])
#AC_CHECK_FILE(${DRMDB}, [], [AC_MSG_ERROR(GBMDRMdb002 must exist at the given location)])


AC_PROG_MAKE_SET
AC_CONFIG_FILES([Makefile])
AC_OUTPUT