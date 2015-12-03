#!/usr/bin/perl -w
use strict;
use DBI;

sub _usage_info() {
    print "Usage: tgf_data_report.pl  [-m]\n";
    print "    Show local data files associated with TGFs in the database.\n";
    
    print "-m    Produce output for TGFs with a matching stroke (matches defined by get_match_coords)\n";
    print "-a    Show only TGFs with a matching stroke that also have position AND tte data files\n";
    
}


my $tgf_db = $ENV{"TGFDB"};
my $data_db = $ENV{"GBMDB"};

my $tgf_tbl = $ENV{"TGFDB_TABLE"};
my $pos_tbl = $ENV{"POS_TABLE"};
my $tte_tbl = $ENV{"TTE_TABLE"};

$tgf_tbl = "tgflist" unless $tgf_tbl;
$pos_tbl = "pos" unless $pos_tbl;
$tte_tbl = "tte" unless $tte_tbl;

$tgf_db = `tgftools_config --tgfdb` unless $tgf_db;
$data_db = `tgftools_config --datadb` unless $data_db;


my $arg;
my $slurp=0;
my $matchOnly=0;
my $completeOnly=0;
while (@ARGV > 0) {
	
	$_ = shift @ARGV;
	if ( /^-{2}(info|help)/i ) {
		_usage_info();
        $arg = undef;
        exit;
	} elsif ( /^-{1}(\w+)/ ) {
		$arg = $1;
        $slurp=0;
	} elsif (! $slurp) {
        $arg = undef;
    }
    
    if (! defined $arg) {
        next;
    }
    
    if ($arg eq 'm') {
        $matchOnly=1;
    } elsif ($arg eq 'c') {
        $completeOnly=1;
    } elsif ($arg eq 'a') {
        $completeOnly=1;$matchOnly=1;
    }
    
}






my $dbh = DBI->connect( "dbi:SQLite:$data_db" );
$dbh->{RaiseError} = 1;


my $sth = $dbh->prepare("ATTACH '$tgf_db' AS tgfdb");

$sth->execute();










my $tgfs_get = "  select tgfid, met, utc from '$tgf_tbl'  order by met ";

my $result = $dbh->selectall_arrayref($tgfs_get);

my $pos_qry;

my @noPosition;
my @noTTE;

print sprintf("%-15s | %3s , %3s ,  stroke match?", "TGF", "Pos", "TTE")."\n";
foreach (@$result) {
    
    my @file_count = $dbh->selectrow_array( " select count(*) from $pos_tbl where tmin <= $_->[1] AND tmax >= $_->[1] " );
    my @tte_count = $dbh->selectrow_array( " select count(*) from $tte_tbl where tmin <= $_->[1] AND tmax >= $_->[1] " );
    my @assoc_count = $dbh->selectrow_array( " select count(*) from assoc where tgfid = '$_->[0]' " );
    
    my $match = ($assoc_count[0] > 0) ? 'y' : '';
    
    next if $match eq '' && $matchOnly;
    
    if ( @file_count && $file_count[0] > 0 ) {
        #print sprintf("%-15s | %3d position", $_->[0], $file_count[0])."\n";   
    } else {
        push @noPosition, $_;
    }
    
    if ( @tte_count && $tte_count[0] > 0 ) {
        #print sprintf("%-15s | %3d TTE", $_->[0], $tte_count[0])."\n";   
    } else {
        push @noTTE, $_;
    }
    
    if ( @file_count || @tte_count ) {
        print sprintf("%-15s | %3d , %3d , %s", $_->[0], $file_count[0], $tte_count[0], $match)."\n";   
    }
    
}




print "\n\nTGFs without Position files (poshist/trigdat):\n";
foreach (@noPosition) {
    print sprintf("%-15s on %s", $_->[0], $_->[2])."\n";   
}

print "\n\nTGFs without TTE files:\n";
foreach (@noTTE) {
    print sprintf("%-15s on %s", $_->[0], $_->[2])."\n";   
}


print "\n\nData sources: \n$data_db.$pos_tbl\n$data_db.$tte_tbl\n$tgf_db.$tgf_tbl \n\n";
