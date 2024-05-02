#!/usr/bin/perl

use IO::File;
use Getopt::Long;

my $xcol=1;
my $ycol=7;
my $outfile="tmp.data";

# exclude the min and max timing
my $olympic=0;

GetOptions(
		"olympic" => \$olympic,
		"xcol=i" => \$xcol,
        "ycol=i" => \$ycol,
        "o=s" => \$outfile);

$numArgs = $#ARGV + 1;

#print ("$numArgs arguments\n");

#foreach $argnum (0 .. $#ARGV) {
#    print "$ARGV[$argnum]\n";
#}

print ("\nxcol=$xcol, ycol=$ycol\n");

# Now generate a matlab file that will convert the data
$tmpfile = "/usr/tmp/plot";

$out = "";
# generate the load statements
foreach $argnum (0 .. $#ARGV) {
    $i = $argnum+1;
	if ($olympic) {
		$out = $out."d$i = load $ARGV[$argnum]\;\n";
		$out = $out."[x mini] = min(d$i(1,$xcol))\;\n";
		$out = $out."[x maxi] = max(d$i(1,$xcol))\;\n";
		$out = $out."data($i,1) = d$i(1,$xcol)\;\n";
		$out = $out."data($i,2) = mean(d$i(:,$ycol))\;\n";
		$out = $out."data($i,3) = std(d$i(:,$ycol))\;\n\n";
	}
	else {
		$out = $out."d$i = load $ARGV[$argnum]\;\n";
		$out = $out."data($i,1) = d$i(1,$xcol)\;\n";
		$out = $out."data($i,2) = mean(d$i(:,$ycol))\;\n";
		$out = $out."data($i,3) = std(d$i(:,$ycol))\;\n\n";
	}
}


$out = $out."[s i] = sort(data, 1)\;\n\n";
$out = $out."sorted = data(i(:,1),:)\;\n\n";
$out = $out."save -text $outfile sorted\;\n\n";

$fd = IO::File->new("$tmpfile.m", "w");

select($fd);
print ($out, "\n");

# Now run octave to actually generate the data
`octave < $tmpfile.m`;

