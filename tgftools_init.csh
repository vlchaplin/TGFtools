#!/bin/csh

#This script will fail as written unless TGFTOOLS, PERLLIB, and PERL5LIB are
#all defined prior to running.  Boo Cshell

setenv PATH ${PATH}:"$TGFTOOLS"/bin:"$TGFTOOLS"/bin/scripts
setenv TGFDB "$TGFTOOLS"/GbmTgfs.db
setenv GBMDB "$TGFTOOLS"/GbmDataArc.db

setenv PERLLIB "$PERLLIB":$TGFTOOLS/lib/perl
setenv PERL5LIB "$PERL5LIB":$TGFTOOLS/lib/perl