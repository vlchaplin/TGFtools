#!/usr/bin/perl
use strict;
use DBI;
my $db = $ENV{"TGFDB"};
my $tgf_tbl = $ENV{"TGFDB_TABLE"};
$tgf_tbl = "tgflist" unless $tgf_tbl;



if ( ! defined($db) ) {
    print "Environment variable TGFDB not set\n";
    exit 0;
}

print "TGF Table = '$tgf_tbl'\n";

my $dbh = DBI->connect( "dbi:SQLite:$db" );
$dbh->{RaiseError} = 1;

if (!defined $dbh) {
   die "Cannot connect to database\n";
}

if ( @ARGV == 0 || (! defined($ARGV[0])) ) {
    print "Usage: remove_tgf.pl  tgfid\n";
    print "Remove TGF events specified with a sqlite pattern matching the tgfid column\n";
    print "Use '%' as a wildcard character.\n";
    exit 0;
}

my ($tgfid_patt)  = @ARGV;

my $first = $dbh->selectall_arrayref( "select tgfid, utc, fsec from $tgf_tbl where  tgfid like '$tgfid_patt' ");
my $n=1;
foreach my $rw (@$first) {
    print sprintf("%5d| %15s  '%s+%f'", $n++, @$rw )."\n";
   # print join (" ", map { sprintf("%20s", $_) } @$rw )."\n";
}

if ( @$first == 0 ) {
    print "Rows effected: 0\n";
    exit 0;
}

print "Delete (y/n)?  ";


while (1) {
    my $answ = <STDIN>;

    if ( $answ =~ /y/i ) {  
        my $stm = "delete from $tgf_tbl where tgfid like '$tgfid_patt'";
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


