#!/usr/bin/perl -w
use strict;


use DBI;

our %sql_to_sprintf_types = (
    "char" => "s",
    "int"   => "d",
    "float" => "f",
    "real" => "f",
    "blob" => "s",
    "datetime" => "s"
);

our $default_width = 12;
our $min_width = 12;

sub _usage_info() {
    print "Usage: show_tgfs.pl [-s delimiter] [-w min_width] [-W where] [--desc --help]  [column names]\n";
    print "    Print the given columns from the associations table.  Default is to print all columns\n\n";
    
    print "Optional arguments\n";
    print "-s delimiter  Use delimeter between different columns of output. Default is '|'\n";
    print "-w min_width  Minimum width of any output field. Default is 12. \n";
    print "-W where      SQL expression to be used in the query of the match table.\n  The query is of the form 'SELECT ... FROM table (your expression)', so 'WHERE' is required\n";
    print "--desc     Print the available column names and quit.  These are the names useful as arguments.\n";
    
    print "\n";
}

sub _get_columns {
    
    my ($dbh, $table, $fields) = @_;
    
    my $ftext;
    if ( defined($fields) && ref($fields) eq 'ARRAY' && @$fields ) {
        $ftext = join (',', @$fields);
    } else {
        $ftext = '*';
    }
    
    my $sth = $dbh->prepare("SELECT $ftext FROM $table WHERE 1=0;"); #get just column names, no rows
    $sth->execute;
    my @cols = @{$sth->{"NAME_lc"}};
    $sth->finish;
    
    my $fmt = {};
    my $lineWidth=0;
    foreach ( @cols ) {
        my $qth = $dbh->column_info( undef, undef, $table, $_ );
        my $hash = $qth->fetchrow_hashref;
        
        my ($modifier,$width,$precision,$type);
    
        $modifier = '';
    
        if ( defined $hash->{TYPE_NAME} ) {
                $type = $sql_to_sprintf_types{lc ($hash->{TYPE_NAME}) };
        }
        else { $type = 's';}
        if ( defined $hash->{DECIMAL_DIGITS} ) { $precision = $hash->{DECIMAL_DIGITS}; } else { $precision = ''}
        if ( defined $hash->{COLUMN_SIZE} ) { $width = $hash->{COLUMN_SIZE}; }
        elsif ($type eq 'f') {
            $width = 9;   
        } elsif ('datetime' eq lc ($hash->{TYPE_NAME})) {
            $width = 19;   
        } else {
            $width = $default_width
        }
        
        $width = length($_) if ( $width < length($_) );
        
        if ( $type eq 'f' && $precision ne '') { $precision = ".".$precision  }
        if ( $width < $min_width) {$width = $min_width;}
        if ( $type eq 's' ) { $modifier = '-'; }
        
        my $fmthash;
        $fmthash->{m} = $modifier;
        $fmthash->{w} = $width;
        $fmthash->{p} = $precision;
        $fmthash->{t} = $type;
        
        push @{$fmt->{"fmt"}}, $fmthash;
        push @{$fmt->{"fmtstr"}}, "%${modifier}${width}${precision}${type}";
        $lineWidth += $width;
    }
    $fmt->{"linewidth"} = $lineWidth;
    
    
    $sth = $dbh->prepare("SELECT * FROM ${table}_units WHERE 1=0;"); #get just column names, no rows
    
     return ([@cols], $fmt) unless $sth;
    
    $sth->execute;
    my @unitsCols = @{$sth->{"NAME_lc"}};
    $sth->finish;
    
    
    
    return ([@cols], $fmt) if (@unitsCols == 0) ;
    
    my @units;
    my @desc;
    foreach (@cols) {
        
        my ($value) = $dbh->selectrow_array("SELECT $_ FROM ${table}_units"); 
        
        if ($value) {
            push @units,  $value;
        } else {
            push @units, '';
        }
           
    }
    
    return ([@cols], $fmt, [@units]);
}

my $tgf_db = $ENV{"TGFDB"};
my $data_table=$ENV{"TGFDB_TABLE"};


$data_table = "tgflist" unless $data_table;


my $dbh = DBI->connect( "dbi:SQLite:$tgf_db" );
$dbh->{RaiseError} = 0;




my @sql_col_expr;
my $slurp=0;
my $arg = undef;
my $desc_and_quit;
my $whr = "";

my $psfix = "|";
my $sep = "|";

my $all=0;
my @defaul_cols = qw(tgfid met utc);

while (@ARGV > 0) {
	
	$_ = shift @ARGV;
	if ( /^-{2}(info|help)/i ) {
		_usage_info();
        $arg = undef;
        exit;
	} elsif ( /^-{2}(desc)/i ) {
		$desc_and_quit = 1;
        $slurp=0;
        $arg = undef;
        next;
	} elsif ( /^-{1}(\w+)/ ) {
		$arg = $1;
        $slurp=0;
	} elsif (! $slurp) {
        $arg = undef;
    }
    
    if (! defined $arg) {
        push @sql_col_expr, $_;
        next;
    }
    
    if ($arg eq 's') {
        unless($slurp) {
            $slurp=1;
            next;
        }
        $sep = $_;
        $arg = undef;
        next;
    }
    if ($arg eq 'w') {
        unless($slurp) {
            $slurp=1;
            next;
        }
        $min_width = $_;
        $arg = undef;
        next;
    }
    if ($arg eq 'W') {
        unless($slurp) {
            $slurp=1;
            next;
        }
        $whr = $_;
        $arg = undef;
        next;
    }
    if ($arg eq 'all') {
        $all=1;
        $arg = undef;
        next;
        
    }
    
}


if ($all) {
    @sql_col_expr = ();
}

my ($all_fields, $format, $units) = _get_columns($dbh, $data_table, \@sql_col_expr);

if ( $desc_and_quit ) {
 
    print "----------------------------------------\n";
    print "Database: '$tgf_db',  Table: '$data_table'\n";
    print "----------------------------------------\n";
    
    if ( defined($units) ) {
       my $nf = @$all_fields; 
        
        my $fldlength=20;
        my $unitlength=12;
        my $sep = '';
        
        map {$sep .= '-' } ( 1 .. ($fldlength+$unitlength+3));
        
        print $sep."\n";
        print sprintf("|%-${fldlength}s|%-${unitlength}s|", "field", "unit")."\n";
        print $sep."\n";
        for ( 0 .. $nf-1 ) {
            print sprintf("|%-${fldlength}s|%-${unitlength}s|", $all_fields->[$_], $units->[$_] )."\n";
        }
        print $sep."\n";
    } else {
        print "COLUMNS:\n";
        print join("\n", @$all_fields)."\n";
        print "----------------------------------------\n";
    }
    exit;
}

if ($all) {
    @sql_col_expr = @$all_fields;
} elsif  ( @sql_col_expr==0 ) {
    
    push @sql_col_expr, @defaul_cols;
    
    (undef, $format, $units) = _get_columns($dbh, $data_table, \@sql_col_expr);
    
}


my $stmt = "SELECT ".join(',', @sql_col_expr)." from $data_table $whr";
my $result = $dbh->selectall_arrayref($stmt);

unless ( $result && @$result ) {
    print "Empty set.\n\n";
    exit;
}

my $nfields = @{$result->[0]};
my $break = '';
map {$break .= '-' } ( 1 .. ($format->{"linewidth"} + length($sep)*($nfields-1)) );
$break = $psfix.$break.$psfix;

my $fmt = $psfix.join($sep, @{$format->{"fmtstr"}} ).$psfix;

my $header = $psfix.join($sep,
                        map { sprintf("%".$format->{"fmt"}->[$_]->{"w"}."s", $sql_col_expr[$_] )} ( 0 .. $nfields-1)
                        ).$psfix;

print $break."\n";
print $header."\n";
print $break."\n";



foreach (@$result) {
    map { $_ = 0 unless defined $_ } @{$_};
    print sprintf( $fmt, @{$_} )."\n";
}
print $break."\n";

