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
    print "Usage: remove_strokes.pl  utc_pattern  fsec_pattern\n";
    print "Specify stroke events with a sqlite pattern matching their UTC time(s) and, optionally, fsec parameter(s)\n";
    print "Use '%' as a wildcard character.\n";
    print "Examples:\n";
    print " `remove_strokes.pl  '2010-10-26%' `\n\t--remove all stroke events on October 26th, 2010\n";
    print " `remove_strokes.pl  '2010-10-26 00:00:%'  '0.9%'`\n\t--remove stroke events from the first minute on October 26th, 2010 with fsec like '0.9%'\n";
    exit 0;
}

my ($utc_patt, $fsec_patt)  = @ARGV;

$fsec_patt = '%' unless defined $fsec_patt;

my $check = "SELECT FROM lightning WHERE utc like '$utc_patt' AND fsec like '$fsec_patt';";

my $whr = " WHERE utc like '$utc_patt' AND fsec like '$fsec_patt'; ";

my $first = $dbh->selectall_arrayref( "SELECT source, utc, fsec FROM lightning $whr");
my $n=1;
foreach my $rw (@$first) {
    print sprintf("%5d| %15s  '%s+%f'", $n++, @$rw )."\n";
}

if ( @$first == 0 ) {
    print "Rows effected: 0\n";
    exit 0;
}

print "Delete (y/n)?  ";


while (1) {
    my $answ = <STDIN>;

    if ( $answ =~ /y/i ) {  
        my $stm = "delete FROM lightning $whr";
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