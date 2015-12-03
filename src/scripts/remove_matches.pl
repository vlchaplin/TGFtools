#!/usr/bin/perl
use strict;
use DBI;
my $db = $ENV{"TGFDB"};

if ( ! defined($db) ) {
    print "Environment variable TGFDB not set\n";
    exit 0;
}

my $dbh = DBI->connect( "dbi:SQLite:$db" );
$dbh->{RaiseError} = 1;

if (!defined $dbh) {
   die "Cannot connect to database\n";
}

if ( @ARGV == 0 ) {
    print "Usage:  remove_matches.pl  tgfid  [utc_pattern  [fsec_pattern] ]\n\n";
    print "Removes TGF<->stroke matches from the 'assoc' table in \$TGFDB.\n";
    print "Specify TGF with a sqlite pattern matching the tgfid column.\n";
    print "Optionally, specify stroke events with a sqlite pattern matching the UTC time(s), or additionaly, the fsec parameter(s).\n";
    print "Use '%' as a wildcard character.\n";
    print "See remove_strokes.pl for more info on specifying stroke events.\n\n";
    print "Examples:\n";
    print " `remove_strokes.pl  '091001%'  '%' `\n\t--remove all stroke associations for TGFs on October 1, 2009\n";
    print " `remove_strokes.pl  '%'`\n\t--clear the associations table\n";
    exit 0;
}

my ($tgfid, $utc_patt, $fsec_patt)  = @ARGV;

$utc_patt = '%' unless defined $utc_patt;
$fsec_patt = '%' unless defined $fsec_patt;



my $whr = " WHERE tgfid like '$tgfid' AND sutc like '$utc_patt' AND sfsec like '$fsec_patt'; ";

my $first = $dbh->selectall_arrayref( "SELECT tgfid, sutc, sfsec FROM assoc $whr");
my $n=1;
foreach my $rw (@$first) {
    print sprintf("%5d| %15s <-> '%s+%f'", $n++, @$rw )."\n";
}

if ( @$first == 0 ) {
    print "Rows effected: 0\n";
    exit 0;
}

print "Delete (y/n)?  ";

while (1) {
    my $answ = <STDIN>;

    if ( $answ =~ /y/i ) {  
        my $stm = "delete FROM assoc $whr";
        print $stm."\n";
        my $nrows = $dbh->do( $stm ) or die $dbh->errstr;
        print "Rows effected: ".sprintf("%d", $nrows)."\n";
        exit 0;
    }
    
    elsif ( $answ =~ /n/i ) {
        print "Rows effected: 0\n";
        exit 0;
    }
    
    else  {
        print "Delete (y/n)?  ";
    }
}