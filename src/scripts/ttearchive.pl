#!/usr/bin/perl


use strict;
use DBI;
use FITSUtils;
use Astro::FITS::CFITSIO qw(:shortnames :constants);

use GbmTime;
use File::Basename;
use Cwd 'abs_path';
use POSIX "floor";

$|=1;

my $gbm_db = $ENV{"GBMDB"};

my $dbh = DBI->connect( "dbi:SQLite:$gbm_db" );
$dbh->{RaiseError} = 1;

my $create_stmt = qq(
   
CREATE TABLE IF NOT EXISTS tte (
filename char(40),
path char(120),
utc_start DATETIME,
utc_stop DATETIME,
tmin FLOAT,
tmax FLOAT,
det char(6),
type char(12),
UNIQUE (filename)
);

);

my $stmt = $dbh->prepare($create_stmt);
$stmt->execute();

my @ttefiles = @ARGV;

my $fptr;

my $keystr = FitsKeywordIO::new;
$keystr->hash_alloc("DETNAM", TSTRING );
$keystr->hash_alloc("TSTART", TDOUBLE, "EVENTS" );
$keystr->hash_alloc("TSTOP", TDOUBLE, "EVENTS" );

my $filetype = 'tte';

foreach my $tte (@ttefiles) {
    my $status=0;
    print "$tte\n";
    
    if ( $keystr->readFile($tte) ) { next; }
 
    my $tstart = $keystr->get("TSTART");
    my $tstop = $keystr->get("TSTOP");
    my $det = $keystr->get("DETNAM");
    
    next unless ( $tstart || $tstop || $det );
    
    
    my ($sec,$min,$hour,$mday,$mon,$year) = GbmTime::met2utc($tstart);
    $year += 1900;
    $mon++;
    my $utc_start = sprintf("%04d-%02d-%02d %02d:%02d:%02d", $year, $mon, $mday, $hour, $min, $sec);
    
    ($sec,$min,$hour,$mday,$mon,$year) = GbmTime::met2utc($tstop);
    $year += 1900;
    $mon++;
    my $utc_stop = sprintf("%04d-%02d-%02d %02d:%02d:%02d", $year, $mon, $mday, $hour, $min, $sec);
    
    if ( insert_file($dbh, basename($tte), dirname(abs_path($tte)), $utc_start, $utc_stop, $tstart, $tstop, $det, $filetype) ) {
        print "inserted\n";
    } else {
        print "failed\n";
    }
}


sub insert_file {
    my ($dbh, $filename, $filepath, $utmin, $utmax, $tmin, $tmax, $det, $filetype) = @_;
    
    my $ins = qq(INSERT or REPLACE INTO tte VALUES ('$filename', '$filepath', '$utmin', '$utmax', $tmin, $tmax, '$det', '$filetype') );
    
    #print $ins."\n";
    
    my $sth = $dbh->prepare($ins);
    return $sth->execute();
}


sub _printFitsError {
	my ($status) = @_;
	open (my $stream, ">&STDOUT" );
    ffrprt( $stream,  $status);
    close ($stream);
    return;
}

