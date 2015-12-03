package FITSUtils;
use strict;
use warnings;

use Astro::FITS::CFITSIO qw( :shortnames :constants );

use List::Util qw(max);

sub _printError {
	my ($status) = @_;
	open (my $stream, ">&STDOUT" );
    ffrprt( $stream,  $status);
    close ($stream);
    return;
}

sub list2mask {
	
	return 0 if @_==0;
	
	my $mask = 0;
	
	map { $mask = ( $mask | (2**$_))  if $_ <= 15; } @_;
	
	return $mask;
}

sub mask2list {
    my ($mask) = @_;
	
	return unless defined $mask;
	
    my @dets;
    my $decbinOrder = 1;
    for (0 .. 15 ) {
        push @dets, $_  if $mask & ($decbinOrder) ;
        $decbinOrder*=2;
    }
    return @dets;
}

sub transposeAddrSort {
	my (@sortedAddressList) = @_;
	
	my $max_addr = max( @sortedAddressList );
	my @irank = map { $max_addr+1 } 0 .. $max_addr;
	my $n = 0;
	my @visited = map { 0 } @irank;
	
	map { unless ($visited[$_]) { $irank[$_] = $n++ ; $visited[$_]=1; }  } @sortedAddressList;
	
	#print "Sorted address list: ".join(", ", @sortedAddressList)."\n";
	#print "Ranked structure: ".join(", ", @irank)."\n";	
	
	return @irank;
}

sub presortDetListUnion {
	
	my ($sz1, $sz2) = ( shift, shift );
	my @list1 = @_[0 .. $sz1-1];
	my @list2 = @_[$sz1 .. $sz1+$sz2-1];
	
	
	#print "List 1: ".join(", ", @list1)."\n";
	#print "List 2: ".join(", ", @list2)."\n";
	
	my @addrRank = transposeAddrSort( @list1, @list2 );
	
	my $union = ( list2mask( @list1 ) | list2mask( @list2 )  );
	
	my @uSet = mask2list( $union );
	@uSet = sort { $addrRank[$a] <=> $addrRank[$b] }  @uSet ;
	
	#print "Union = ".join(", ", @uSet)."\n";
	
	return @uSet;
};

sub getDetMask {
    my ($tfile) = @_;
    
    my $fptr;
    my $status;
    my $det_mask;
    
	my $keyStruct = FitsKeywordIO::new();
	
	$keyStruct->hash_alloc("DET_MASK", TSTRING );
    $keyStruct->readFile( $tfile );
    
    $det_mask = $keyStruct->get("DET_MASK");
    return -1 unless defined $det_mask;

    my $padz = 14 - length($det_mask);
    $det_mask .= sprintf("%0".( $padz )."d", 0);
    
    #print $det_mask."\n";
    
   return unpack("v", pack("b16", $det_mask));
}

sub fmtScalePrecisFloat {

	my ($sec,$fmt,$usefmt,$ndec_add) = @_;
	
	my $ord;
	
	if ($sec <= 0) { $ord = 0; }
	else { $ord = int( log( $sec ) ); }
	
	$ndec_add = 0 unless defined $ndec_add;
	
	my $frmtstr;
	my $string;
	
	if (! defined $usefmt) {
		if ($ord >= 1) {
			$frmtstr = "%0.${ndec_add}f";
		} else {
			$frmtstr = "%0.".(abs($ord)+$ndec_add)."f"
		}
	} else {
		$frmtstr = $usefmt;
	}
	
	$string = sprintf( $frmtstr, $sec );
	
	if (ref($fmt) eq 'SCALAR') { $$fmt = $frmtstr }
	
	return $string;
};

sub FitsKeywordIO::new {

	my $self = {};
	$self->{"extmap"} = {};
	$self->{"buffer"} = {};
	bless $self, "FitsKeywordIO";
	return $self;
	
};

sub separate_archives {
	
	my @archives;
	my @files;
	
	foreach (@_) {
		
		if ( /\.tar$/ ) {
			push @archives, $_;
		} elsif ( /\.t.*gz$/) {
			push @archives, $_;
		} elsif ( /(gz$|zip$)/) {
			push @archives, $_;
		} else {
			push @files, $_;
		}
	}
	
	
	return ([@files], [@archives]);
}

sub get_unpacking_map {
	
	my $opts = shift @_;
	
	if ( ref($opts) ne 'ARRAY') {
		unshift @_, $opts;
		$opts='';
	} else {
		$opts = $opts->[0];
	}
	

	
	my @filemap;
	
	foreach (@_) {
		
		if ( /\.tar$/i ) {
			my @contents = `tar $opts -tf $_`;
			chomp (@contents);
			push @filemap, [$_, @contents];
			
		} elsif ( /\.t.*gz$/i) {
			my @contents = `tar $opts -tzf $_`;
			chomp (@contents);
			push @filemap, [$_, @contents];
		} elsif ( /(.*)\.(gz$|zip$)/i) {
			#my @contents =`gunzip -v $_`;
			push @filemap, [$_, ($1)]
		} 
	}
	
	return @filemap;
}

sub expand_archive {
	
	my ($opts, $file, @targets) = @_;
	
	my @expand;
	
	foreach ( @targets ) {
		push @expand, $_ unless -e $_;
	}
	
	my $targetlist = join(' ', @expand);
	
	if ( $file=~ /\.tar$/ ) {
		`tar $opts -xvf $file $targetlist`;		
	} elsif ( $file=~ /\.t.*gz$/) {
		print "tar $opts -zvxf $file $targetlist\n";
		`tar $opts -zvxf $file $targetlist`;
	} elsif ( $file=~ /(.*)\.(gz$|zip$)/) {
		`gunzip $file`;
		push @targets, $1;
	} else {
		@targets = ();	
	}
	
	return ([@targets], [@expand]);
}

sub partition_arc_files {
	
	my @files;
	my @archived;
	my $narchives=0;
	
	foreach (@_) {
		
		if ( /\.tar$/ ) {
			$narchives++;
			push @files,  `tar -tf $_`;
		} elsif ( /\.t.*gz$/) {
			$narchives++;
			push @files, `tar -tzf $_`;
		} elsif ( /(gz$|zip$)/) {
			push @files, `gunzip $_`;
		} else {
			push @files, $_;
		}
	}
	
	if( $narchives > 0 ) {
		($narchives, @files) = partition_arc_files(@files);
	}
		
	return ($narchives, @files);
}

sub FitsKeywordIO::hash_alloc {

	my $self = shift @_;
	my ($key, $type, $extname, $value) = @_;
	
	$extname = "Primary" if (! defined $extname);
	$extname = "Primary" if length($extname) == 0;
	
	my $buffer = $self->{"buffer"};
	my $map = $self->{"extmap"};
	
	push @{$map->{$key}}, $extname;
	
	$buffer->{$extname}->{$key}->{"ttype"} = $type;
	$buffer->{$extname}->{$key}->{"value"} = $value;
	
};

sub FitsKeywordIO::readFile {
	my $self = shift @_;
	my $fitsFile = shift @_;
	
	my $status = 0;
	ffopen( my $fptr, $fitsFile, READONLY, $status );
	if ($status != 0) {
        print "FitsKeywordIO::read(): $fitsFile \n";
  		_printError( $status );
        return $status;
    }
    
    my $buffer = $self->{"buffer"};
    
    foreach my $extname ( keys %$buffer ) {
    	if ($extname eq 'Primary') {
    		ffmahd($fptr, 1, ANY_HDU, $status);
    	} else {
    		ffmnhd($fptr, ANY_HDU, $extname, 0, $status);
    	}
     	if ($status != 0) {
     		_printError($status);
     		$status=0;
    		next;
    	}
    	my $list = $buffer->{$extname};
    	foreach my $key ( keys %$list ) {
    		ffgky($fptr, $list->{$key}->{"ttype"}, $key, my $val,undef, $status);
    		
    		if ($status == 0) {
    			$list->{$key}->{"value"} = $val;
    		} else {
    			$list->{$key}->{"value"} = undef;
    			$status=0;
    		}
    		#print $extname." => $key => ".$list->{$key}->{"value"}."\n";
    	}
    }
    
    ffclos($fptr,$status);
    return $status;
}

sub FitsKeywordIO::get {
	my $self = shift @_;
	my ($key) = @_;
	
	my $buffer = $self->{"buffer"};
	my $map = $self->{"extmap"};
	
	return undef unless exists($map->{$key});
	
	my @exts = @{$map->{$key}};
	
	if (@exts == 1) {
		 return $buffer->{$exts[0]}->{$key}->{"value"};
	}
	
	my @vals;
	foreach (@exts) {
		push @vals, $buffer->{$_}->{$key}->{"value"};
	}
	
	return ( [@exts], [@vals] );
}


1;