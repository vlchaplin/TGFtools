#!/usr/bin/perl


use strict;
use DBI;

use GbmTime;
use File::Basename;
use POSIX "floor";

$|=1;

my %fmtParams = ("sep" => ",\\s*");

my $gbm_db = $ENV{"TGFDB"};
my $tgf_tbl = $ENV{"TGFDB_TABLE"};

$tgf_tbl = "tgflist" unless $tgf_tbl;

my $dbh = DBI->connect( "dbi:SQLite:$gbm_db" );
$dbh->{RaiseError} = 1;

my $strokes_crt = qq(
   
CREATE TABLE IF NOT EXISTS lightning (
utc DATETIME,
fsec FLOAT,
tgfid char(12) NOT NULL DEFAULT '-',
lon FLOAT,
lat FLOAT,
source char(10) DEFAULT '-',
UNIQUE (utc ASC,fsec ASC)
);

);


sub parse_wwlln_info {
    my $line = shift;
    
    my ($date,$time, $lat, $lon) = split /$fmtParams{"sep"}/, $line;

	


    $date =~ s/^\s+//g;
    $date =~ /(\d+)\/(\d+)\/(\d+)/;
    
    $date= join "-", map { sprintf("%02d", $_) } ($1,$2,$3) ;

    
    my $isec = substr( $time, 0, 8 );
    my $fsec = substr( $time, 8 );
    $fsec = sprintf("%0.6f", $fsec);
    
    return ( $date." ".$isec, $fsec, $lat, $lon );
}

sub parse_input_src {
    my  ($utcf, $lon, $lat, $src) = @_;

    $utcf =~ /(\d+)-(\d+)-(\d+).(\d+):(\d+):(\d+)(\.*\d*)/;
    
    return unless ( $1 || $2 || $3 || $4 || $5 || $6 );
    
    my $date= join "-", map { sprintf("%02d", $_) } ($1,$2,$3) ;
    my $time= join ":", map { sprintf("%02d", $_) } ($4,$5,$6) ;
    
    my $fsec = sprintf("%0.6f", $7);
    
    return ( $date." ".$time, $fsec, $lon, $lat, $src );
}

sub find_gbm_event_id {
    my ($dbh, $stroke_utc, $stroke_fsec, $hsec_intval) = @_;
        
    my $srctime = $stroke_utc.".".substr($stroke_fsec,2);
        
    my $qry = "SELECT tgfid, met, ".
        "strftime('%Y-%m-%d %H:%M:%f',utc,'+' || ($hsec_intval+fsec) || ' seconds') as tUp". ", ".
        "strftime('%Y-%m-%d %H:%M:%f',utc,'-' || ($hsec_intval+fsec) || ' seconds') as tLw".
        " FROM $tgf_tbl "
        ."WHERE '$srctime' >= tLw AND '$srctime' <= tUp; ";
    
    #print $qry."\n";
    
    
    my $events = $dbh->selectall_arrayref( $qry );
 
    
    print $srctime."\n";
    
    return $events;
}

sub insert_lightning_ev {
    my ($dbh, $utc, $fsec, $id, $lon, $lat, $src) = @_;
    
    $fsec = sprintf("%0.6f", $fsec);
    my $str = "INSERT or REPLACE INTO lightning VALUES ('$utc', $fsec, $id, $lon, $lat, '$src')";
    print $str."\n";
    my $sth = $dbh->prepare($str);
    return $sth->execute();
}

sub _readInlineTag {
    my ($line) = @_;
    
    my $b11 = index($line, '<');
    my $b12 = index($line, '>', $b11+1);
    return -1 if $b11 == -1 || $b12 == -1;
    
    my $tag1 = substr($line, $b11+1, $b12 - $b11-1);
    
    my $b21 = index($line, '</', $b12+1);
    my $b22 = index($line, '>',   $b21+1);
    return  -1 if $b21 == -1 || $b22 == -1;
    
    my $tag2 = substr($line, $b21+1, $b22 - $b21-1);
    
    return -1 unless defined($tag1) && defined($tag2);
    return -1 if ("/".$tag1 ne $tag2) ;
    
    my $data = substr($line, $b12+1, $b21 - $b12-1);
    
    return ($tag1, $data, $tag2);
    
}

sub _isTagStart {
    my ($tag, $tlen, $str) = @_;
    return "<$tag>" eq substr($str, 0, $tlen+2);
}

sub _isTagEnd {
    my ($tag, $tlen, $str) = @_;
    return "</$tag>" eq substr($str, 0, $tlen+3);
}

sub _usage_info() {
	
	print "Usage: add_strokes.pl [-vcw wwlln_info.txt] [-s window] --help\n";
	print "Add lightning stroke information via STDIN or a formatted text file.\n";
	print "After each stroke is added the TGF list is searched for associations in time.\n\n";
	print "Optional arguments:\n";
	print "   -vcw  events.txt\nProcess WWLLN stroke events formatted like:\n";
	print "#            date   UT           WWLLN_Lat  WWLLN_Lon Acc  #Stn    Fermi_Lon  Fermi_Lat\n".
     		"2008/10/01,09:24:44.909828, 11.2940, 163.1578, 13.2,  7     162.67      10.47\n\n";
    print "   -s    Look for associations with GBM Tgfs listed in \$TGFDB within +/- window seconds [default=600]\n\n";

}

my $quit = 0;
my $met;
my $status=0;
my $n=1;
my $fail=0;


my $arg=undef;
my $slurp=0;
my @vc_wwlln_files;
my $seconds_tolerance=600;
while (@ARGV > 0) {
	
	$_ = shift @ARGV;
	if ( /^-{1}(\w+)/ ) {
		$arg = $1;
        $slurp=0;
	} elsif ( /^-{2}(info|help)/i ) {
		_usage_info();
        exit;
	}
	
	if ($arg eq 'vcw') {
		if (! $slurp) {
			$slurp=1;
			next;
		}
		print "Stroke info file: $_\n";
		push @vc_wwlln_files, $_;
		next;
	}
	
	if ($arg eq '-s') {
		print "Window seconds: $_\n";
		$seconds_tolerance = $_;
		$slurp=0;
		$arg=undef;
		next;
	}
}

`echo 'q' | add_tgfs.pl`;
my $sst = $dbh->prepare($strokes_crt);
$sst->execute();

my $associations = [];

if ( @vc_wwlln_files > 0 ) {

	foreach (@vc_wwlln_files) {
	
		open(my $fh, "<".$_) or print "$_: $!\n";
		next unless $fh;
	
		while (<$fh>) {
			chomp;
			next if /^#/;
			next if $_ =~ /^\s+\s$/;
			
			if (substr($_,0,1) eq '<' ) {
				my ($tag, $data) =  _readInlineTag($_);
				next unless defined $data;
			
			if (! exists $fmtParams{$tag} ) {
				print "unrecognized tag used: $tag\n";
			} else {
				print "setting $tag = $data\n";
				$fmtParams{$tag} = $data;   
			}
				next;
			}
			
			my ($utc, $fsec, $lat, $lon) = parse_wwlln_info($_);
			
			#$associations = find_gbm_event_id( $dbh, $utc, $fsec, $seconds_tolerance );
			
			if ( @$associations == 0 ) {
				insert_lightning_ev( $dbh,$utc, $fsec, "NULL", $lon, $lat, "wwlln")
			}
			
		#	foreach my $ev ( @$associations ) {
		#		print "GBM match ---> ".join(" | ", @$ev)."\n";
		#		insert_lightning_ev( $dbh,$utc, $fsec, "'$ev->[0]'", $lon, $lat, "wwlln");
		#	}
			
		}
		
		close $fh;	
	
	}
	
	
	exit;
}

print "Add event:\n> utc, lon, lat, src\nor 'q' quit:\n\n";
print "Format:\n";
print "\tutc:  yyyy-mm-dd hh:mm:ss.ffffff \n";
print "\tlon:   ddd.ddd  (East longitutde in degrees)\n";
print "\tlat:   ddd.ddd  (latitude in degrees)\n";
print "\tsrc:   string    (location source, e.g., wwlln)\n";
print "Example:\n";
print "1> 2008-10-01 09:24:44.909828,   163.2,  11.294,  wwlln\n\n";
print "$n> ";
while (! $quit && $fail < 8) {
    $_ = <STDIN>;
    
    chomp;
    
    next if ($_ =~ /^#/);
    
    if (substr($_,0,1) eq '<' ) {
        my ($tag, $data) =  _readInlineTag($_);
        next unless defined $data;
        
        if (! exists $fmtParams{$tag} ) {
            print "unrecognized tag used: $tag\n";
        } else {
            print "setting $tag = $data\n";
            $fmtParams{$tag} = $data;   
        }
        print "$n> "; 
        next;
    }
    
    if ($_ =~ /^q/i ) {
        print"q \n";
        $quit=1;
        last;
    }
    if ($_ eq '') {
        $fail++;
        next;
    }
    
    my @par = split /$fmtParams{"sep"}/;
   # next if (@par==0);
    
	
	
    if (@par != 4) {
        print "Error parsing input\n";
        print "$n> "; 
        next;
    }

    
    my ($datetime, $fsec, $lon, $lat, $src) = parse_input_src( @par );
    
    #my $associations = find_gbm_event_id( $dbh, $datetime, $fsec, 300.0 );
    
	$associations=[];
    if ( @$associations == 0 ) {
        insert_lightning_ev( $dbh, $datetime, $fsec, "'-'", $lon, $lat, $src)
    }
    
    foreach my $ev ( @$associations ) {
        print "GBM match ---> ".join(" | ", @$ev[(0..1)])."\n";
        insert_lightning_ev( $dbh, $datetime, $fsec, "'$ev->[0]'", $lon, $lat, $src);
    }
    
    $n++;
    print "ok\n";
    print "$n> ";
       
}

exit if $quit;


