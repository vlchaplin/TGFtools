#!/usr/bin/perl


use strict;
use DBI;

use GbmTime;
use File::Basename;
use POSIX "floor";

use constant {
	REPLACE => "REPLACE",
	IGNORE => "IGNORE",
	ABORT => "ABORT"
};

$|=1;

sub _usage_info() {
	
	print "Usage: add_tgfs.pl  -nx --inc [-sep pattern] --help\n";
	print "Add TGFs to the table by MET.\n";
	print "-n       Dry run.  Does everything except adding to the database.\n";
	print "-x       Do not sanity check METs.\n";
	print "-inc    Auto increment conflicting IDs (A,B,C...).  Only used if conflict action = IGNORE (default)\n";
	print "         This can also be given during input as <inc>[on,off,yes,no, '']</inc>, so a file can define it's own behavior.  \n";
	print "-sep   pattern\n";
	print "         Set the field separation pattern.  Default is ',\\s*', which splits on commas possibly followed by white spaces.\n ";
	print "         This pattern is used in the Perl split() function.  \n";
	print "         This can also be given during input as <sep>pattern</sep>, so a file can define it's own format.  \n";
	print "--override     Ignore commands issued in a file or during program input. Does not effect format commands.\n";
}

my $gbm_db = $ENV{"TGFDB"};

my $tgf_tbl = $ENV{"TGFDB_TABLE"};

$tgf_tbl = "tgflist" unless $tgf_tbl;

my $arg;
my $slurp=0;

my $auto_increment_ids=0;
my $dryrun=0;
my $metnocheck=0;
my $override =0;

my %fmtParams = ("sep" => ",\\s*", "conflict" => IGNORE );

while (@ARGV > 0) {
	
	$_ = shift @ARGV;
	if ( /^-{1}(\w+)/ ) {
		$arg = $1;
        $slurp=0;
	} elsif ( /^-{2}(info|help)/i ) {
		_usage_info();
        exit;
	} elsif ( /^-{2}(\w+)/i ) {
		$arg = $1;
        $slurp=0;
	}
	
	unless( defined $arg) {
		$slurp=0;
		next;
	}
	
	if ($arg eq 'n') {
		$dryrun=1;
	}
	elsif ($arg eq 'x') {
		$metnocheck=1;
	}
	elsif ($arg eq 'sep') {
		unless($slurp) {
			$slurp=1;
			next;
		}
		$fmtParams{"sep"} = $arg;
		$arg=undef;
	} elsif ($arg eq 'inc') {
		$auto_increment_ids=1;
	} elsif ($arg eq 'override') {
		$override=1;
	}
	
}






my $dbh = DBI->connect( "dbi:SQLite:$gbm_db" );
$dbh->{RaiseError} = 0;

sub create_tgf_table {
	my ($dbh, $table) = @_;
	
	my $create_stmt = qq(
   
	CREATE TABLE IF NOT EXISTS $table (
	tgfid char(15),
	utc DATETIME(19),
	fsec FLOAT(9),
	met FLOAT(16),
	tstart FLOAT(16) DEFAULT NULL,
	tstop FLOAT(16) DEFAULT NULL,
	lcXs FLOAT(9) DEFAULT NULL,
	lcXe FLOAT(9) DEFAULT NULL,
	lcDx FLOAT(9) DEFAULT NULL,
	pos_x FLOAT(9) DEFAULT NULL,
	pos_y FLOAT(9) DEFAULT NULL,
	pos_z FLOAT(9) DEFAULT NULL,
	qs FLOAT DEFAULT NULL,
	qx FLOAT DEFAULT NULL,
	qy FLOAT DEFAULT NULL,
	qz FLOAT DEFAULT NULL,
	PRIMARY KEY(tgfid)
	);
	
	);
	
	if ( ! $dryrun ) {
		my $stmt = $dbh->prepare( $create_stmt );
		$stmt->execute;	
	}
	
}
=junk
 
ALTER TABLE tgflist ADD COLUMN lcXs FLOAT DEFAULT NULL;
ALTER TABLE tgflist ADD COLUMN lcXe FLOAT DEFAULT NULL;
ALTER TABLE tgflist ADD COLUMN lcDx FLOAT DEFAULT NULL

=cut

#my %tgffields = ( "tgfid" => undef, "utc" => undef, "fsec" => undef, "met" => undef, "path"=> undef );

sub parse_wwlln_info {
    my $line = shift;
    
    my ($date,$time, $lat, $lon) = split /,/, $line;

    $date =~ s/^\s+//g;
    $date =~ /(\d+)\/(\d+)\/(\d+)/;
    
    $date= join "-", map { sprintf("%02d", $_) } ($1,$2,$3) ;

    
    my $isec = substr( $time, 0, 8 );
    my $fsec = substr( $time, 8 );
    $fsec = sprintf("%0.6f", $fsec);
    
    return ( $date." ".$isec, $fsec, $lat, $lon );
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

sub insert_tgf {
    my ($dbh, $cols, @vals) = @_;
    
	my ($id, $utc, $fsec) = @vals;
	
    $vals[2] = sprintf("%0.6f", $vals[2]);
	
	#check for genuine conflicts.  reprocessing the same TGF
	my $cnt = "select tgfid, met from $tgf_tbl where tgfid=${id} ";
	
	#print $cnt."\n";
	
	my @result = $dbh->selectrow_array($cnt);
	if ( @result > 0 && $fmtParams{"conflict"} eq IGNORE  ) {
		
		my $delta_t = $vals[3] - $result[1];
		
		if ( abs($delta_t ) <= 2e-03 ) {
			print " Appears to be a duplicate of TGF '$result[0]'.  conflict action = IGNORE -> skipping.\n ";
			return -1;
		} 
		
		print "! Conflicts with '$result[0]', ".sprintf('%0.6f', $delta_t)." seconds apart\n";
		
		if ($auto_increment_ids) {
			
			if ( $vals[0] =~ /.*'(.*\d+)([[:upper:]]+)'.*$/) {
				my $itr = $2;
				$itr++;
				$vals[0] = " '$1$itr' ";
				
			} else {
				$vals[0] =~ s/'(.*)'.*$/'$1A'/
			}
			
			print "Incremented ID: ".$vals[0]."\n";
			
			return insert_tgf($dbh, $cols, @vals);
			
		} else {
			print "Skipping\n";
			return -1;
		}
		
	}
	if ( @result > 0 && $fmtParams{"conflict"} eq REPLACE  ) {
		print "  Overwritting '$result[0]' entry\n";
	}
	
    my $str = "INSERT or ".$fmtParams{"conflict"}." INTO $tgf_tbl $cols VALUES (".join(",", @vals).")";
    
    #print "$str\n";
    
	if ( ! $dryrun ) {
		my $sth = $dbh->prepare($str);
		my $status= $sth->execute();
		if ($status == 1) {
			print "Inserted $vals[0]\n";
		}
		return $status;
	}

    return -1;
}

sub insert_lightning_ev {
    my ($dbh, $utc, $fsec, $id, $lon, $lat, $src) = @_;
    
    $fsec = sprintf("%0.6f", $fsec);
    my $str = "INSERT or IGNORE INTO lightning VALUES ('$utc', $fsec, $id, $lon, $lat, '$src')";
    
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

create_tgf_table( $dbh, $tgf_tbl );


my $quit = 0;
my $met;
my $status=0;
my $n=1;
my $fail=0;

my $id;

print "Add TGF event by specifying MET (max precision=microsecond): \n";
print sprintf("  > %-30s", "met")." for default id ('yyymmddfff')  OR\n";
print sprintf("  > %-30s", "met,  , tstart, tstop")." for default id with tstart & tstop (MET) OR\n";
print sprintf("  > %-30s", "met, id, tstart, tstop")." time, id string (no spaces),  start,end (MET) \n";
print sprintf("  > %-30s", "q"). " Quit\n\n";

print "$n> ";
while (! $quit && $fail < 8) {
    $_ = <STDIN>;
    
    chomp;
    
    next if ($_ =~ /^#/);
    
    if (substr($_,0,1) eq '<' ) {
        my ($tag, $data) =  _readInlineTag($_);
        next unless defined $data;
        
		if ( (!$override) && $tag =~ /table/i) {
			
			if ( $data =~ /^\$(\w+)/) {
				$tgf_tbl = $ENV{"$1"}; 
				print "expanding $data = '$tgf_tbl'\n";
				$tgf_tbl  = "tgflist" unless $tgf_tbl ;
			} elsif ( $data ) {
				$tgf_tbl = $data; 	
			} else {
				die "Unable to parse table name from <table>...</table> tag\n";
			}
			
			create_tgf_table($dbh, $tgf_tbl );
			print "** Changed TGF Table: '$tgf_tbl' **\n";
			
			print "$n> "; 
			next;
		}
		
		if ( (!$override) && $tag =~ /inc/) {
			
			#turn on auto increment if the field matches 'y', 'on', or is empty.
			if ( length($data) == 0 || $data =~ /y|on|\s+/i) {
				print "Auto increment enabled\n";
				$auto_increment_ids = 1;
			} elsif ( $data =~ /n|off/i && $auto_increment_ids) {
				print "Auto increment disabled\n";
				$auto_increment_ids = 0;
			}
			
			next;
		}

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
    
    if (@par==1) {
        $met = shift @par;
        
        if ( (! $metnocheck) && (length($met) < 9 ||  ($met+0 < 237427201)) ) {
            print "MET argument '$met' is not properly formatted. Must be at least 9 digits > 237427201\n";
            print "$n> "; 
            next;
        }
        
        my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday) = GbmTime::met2utc($met);
        $year += 1900;
        $mon++;
        my $utc = sprintf("%04d-%02d-%02d %02d:%02d:%02d", $year, $mon, $mday, $hour, $min, $sec);
        $id = join "", met2bn($met);
        
        my $fsec = $met - floor($met);
        
        my $path = '\0';
        
        $status= insert_tgf( $dbh, "(tgfid, utc, fsec, met)",  (" '$id' ", " '$utc' ", $fsec, $met) );
        
        
        
    } elsif (@par==2) {
        $met = shift @par;
        $id = shift @par;
        
		if ( length( $id ) == 0 ) {
			$id = join "", met2bn($met);
		} else {
			$id =~ s/\s//g;
		}
		
        my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday) = GbmTime::met2utc($met);
        $year += 1900;
        $mon++;
        my $utc = sprintf("%04d-%02d-%02d %02d:%02d:%02d", $year, $mon, $mday, $hour, $min, $sec);
        
        my $fsec = $met - floor($met);
        my $path = '\0';
        
        $status= insert_tgf( $dbh, "(tgfid, utc, fsec, met)",  ( " '$id' ", " '$utc' ", $fsec, $met)  );
        
    } elsif (@par>=4) {
        $met = shift @par;
        $id = shift @par;
        my ($tstart, $stop) = @par;
		
		if ( length( $id ) == 0 ) {
			$id = join "", met2bn($met);
		} else {
			$id =~ s/\s//g;
		}
		
        my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday) = GbmTime::met2utc($met);
        $year += 1900;
        $mon++;
        my $utc = sprintf("%04d-%02d-%02d %02d:%02d:%02d", $year, $mon, $mday, $hour, $min, $sec);
        
        my $fsec = $met - floor($met);
        my $path = '\0';
    
        
        $status= insert_tgf( $dbh, "(tgfid, utc, fsec, met, tstart, tstop)",  (" '$id' ", " '$utc' ", $fsec, $met, $tstart, $stop) );
        
    } else {
        $status=0;
    }
    
    if ($status==1) {
        $n++;
        print "Ok\n";
        #print "ok\n";
        print "$n> ";
    } elsif ($status == -1 ) {
		print "\n$n> ";
	} else {
        $fail++;
        
        print "\n$n> ";
    }
       
}

print "\n\n";

exit if $quit;

=head1
print "\n\nProccessing WWLLN info: $wwlln_file\n";
open(my $fh, "<".$wwlln_file) or die "$!\n";;

my $sst = $dbh->prepare($strokes_crt);
$sst->execute();

while (<$fh>) {
    chomp;
    next if /^#/;
    
    my ($utc, $fsec, $lat, $lon) = parse_wwlln_info($_);
    
    my $associations = find_gbm_event_id( $dbh, $utc, $fsec, 300.0 );
    
    if ( @$associations == 0 ) {
        insert_lightning_ev( $dbh,$utc, $fsec, "NULL", $lon, $lat, "wwlln")
    }
    
    foreach my $ev ( @$associations ) {
        print "GBM match ---> ".join(" | ", @$ev)."\n";
        insert_lightning_ev( $dbh,$utc, $fsec, "'$ev->[0]'", $lon, $lat, "wwlln");
    }
    
}
close $fh;
=cut
