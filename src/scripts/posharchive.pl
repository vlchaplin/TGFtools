#!/usr/bin/perl -w
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
my $wwlln_file = "strike_pos_details.txt";

my $dbh = DBI->connect( "dbi:SQLite:$gbm_db" );
$dbh->{RaiseError} = 1;

my $table = $ENV{"POS_TABLE"};
$table = "pos" unless $table;


my $create_stmt = qq(
   
CREATE TABLE IF NOT EXISTS $table (
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

my $keystr = FitsKeywordIO::new;
$keystr->hash_alloc("FILETYPE", TSTRING );
$keystr->hash_alloc("TSTART", TDOUBLE );
$keystr->hash_alloc("TSTOP", TDOUBLE );


my ($regular_files, $arcfiles) = FITSUtils::separate_archives(@ARGV);

my $tar_opt = qq(--wildcards --exclude "*tte*" --exclude "*tcat*" --exclude "*pha*");

my @arcfilemap = FITSUtils::get_unpacking_map( [$tar_opt],   @$arcfiles);


foreach my $unpack (@arcfilemap) {
    
    print "$unpack->[0] => \n";
    
    my ($files, $unpacked) = FITSUtils::expand_archive( $tar_opt, @$unpack );
    
    
    foreach (@$files) {
        print " $_\n";
        read_and_insert_file( $_, $_, abs_path($unpack->[0])  );   
    }
    
    
    print "\nremoving temporary copies...\n" if @$unpacked;
    `rm @$unpacked` if @$unpacked;
    
}





foreach my $file (@$regular_files) {
    print "\n";
    print "$file\n";
    
    read_and_insert_file($file, basename($file), dirname(abs_path($file)));
}

sub insert_file {
    my ($dbh, $filename, $filepath, $utmin, $utmax, $tmin, $tmax, $det, $filetype) = @_;
    
    my $ins = qq(INSERT or REPLACE INTO $table VALUES ('$filename', '$filepath', '$utmin', '$utmax', $tmin, $tmax, '$det', '$filetype') );
    
    #print $ins."\n";
    
    my $sth = $dbh->prepare($ins);
    return $sth->execute();
}

sub read_and_insert_file {
    my ($file, $dbfile, $dbpath) = @_;
    
    my $status=0;
    if ( $keystr->readFile($file) ) { return 1; }
 
    my $tstart = $keystr->get("TSTART");
    my $tstop = $keystr->get("TSTOP");
    my $filetype = $keystr->get("FILETYPE");

    if ( $filetype eq 'GLAST POS HIST') {
        $filetype = 'poshist';
    } elsif ( $filetype eq 'TRIGDAT') {
        $filetype = 'trigdat';
    } else {
        print "$file\n";
        print "Does not appear to be a GBM position file type\n";
        return 1;
    }
    
    return 1 unless ( $tstart || $tstop );
    
    my ($sec,$min,$hour,$mday,$mon,$year) = GbmTime::met2utc($tstart);
    $year += 1900;
    $mon++;
    my $utc_start = sprintf("%04d-%02d-%02d %02d:%02d:%02d", $year, $mon, $mday, $hour, $min, $sec);
    
    ($sec,$min,$hour,$mday,$mon,$year) = GbmTime::met2utc($tstop);
    $year += 1900;
    $mon++;
    my $utc_stop = sprintf("%04d-%02d-%02d %02d:%02d:%02d", $year, $mon, $mday, $hour, $min, $sec);
    #dirname(abs_path($file))
    if ( insert_file($dbh, $dbfile, $dbpath, $utc_start, $utc_stop, $tstart, $tstop, '', $filetype) ) {
        print "inserted\n";
        return 0;
    } else {
        print "failed\n";
        return 2;
    }
    
    
}

