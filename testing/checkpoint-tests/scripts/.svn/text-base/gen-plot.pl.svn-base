#!/usr/bin/perl

# -------------------------------
# This script takes a list of (name,data-file) pairs, where 
# the name is the title of the plot and the data file contains 
# three columns (x, y, err), and plots into a single 
# file using gnuplot. 
# 
# E.g., 
# gen-plot.pl "label1,data1.dat" "label2,data2.dat"
#
# Will generate two plots (labeled 'label1' and 'label2')

use IO::File;
use Getopt::Long;

my $title="Plot Title Here";
my $legend="top left";
my $xlabel="X axis";
my $ylabel="Y axis";
my $epsfile="tmp.eps";
my $jpegfile="tmp.jpg";
my $outfile="tmp.gnu";
my $linewidth = 2;
my $color = "color";  # color or bw
my $yrange = "*:*";
my $xrange = "*:*";
my $xlog = "false";
my $ylog = "false";
my $size = "noratio 1,1";

GetOptions("data=s" => \$datastr,
        "title=s" => \$title, 
        "xrange=s" => \$xrange, 
        "yrange=s" => \$yrange, 
        "xlog=s" => \$xlog, 
        "ylog=s" => \$ylog, 
        "xlabel=s" => \$xlabel, 
        "ylabel=s" => \$ylabel, 
        "legend=s" => \$legend, 
        "color=s" => \$color, 
        "linewidth=i" => \$linewidth, 
        "size=s" => \$size,
		"epsfile=s" => \$epsfile,
		"jpegfile=s" => \$jpegfile,
        "o=s" => \$outfile);

$numArgs = $#ARGV + 1;

$out = "\n";

# define gnuplot linestyles
foreach $argnum (0 .. $#ARGV) {
    $i = $argnum+1; 
    $out = $out."set style line $i linetype $i linewidth $linewidth\n";
}
$out = $out."\n";

# generate the load statements
foreach $argnum (0 .. $#ARGV) {
    @pair = split(/,/, $ARGV[$argnum]); 
    $name = @pair[0];
    $file = @pair[1];

    $ls = $argnum+1;

    if ($argnum == 0) {
        $plotcmd = "plot";
    }
    else {
        $plotcmd = "replot";
    }

    $out = $out."$plotcmd \"$file\" using 1:2 with linesp linestyle $ls title \"$name\", \\\n";
    $out = $out."     \"$file\" using 1:2:3 with yerrorbars linestyle $ls title \"\"\;\n\n";
}


$out = $out."set size $size\;\n";
$out = $out."set xrange [$xrange]\;\n";
$out = $out."set yrange [$yrange]\;\n";

if ($xlog eq "true") {
    $out = $out."set logscale x\;\n";
}

if ($ylog eq "true") {
    $out = $out."set logscale y\;\n";
}

#$out = $out."set key $legend Left box 3\;\n";
$out = $out."set key $legend box lt -1\;\n";
$out = $out."set ylabel \"$ylabel\"\;\n";
$out = $out."set xlabel \"$xlabel\"\;\n";
$out = $out."set title \"$title\"\;\n";
$out = $out."set term postscript eps $color lw $linewidth\;\n";
$out = $out."set output \"$epsfile\"\;\n";
$out = $out."replot\;\n";
$out = $out."set term jpeg\;\n";
$out = $out."set output \"$jpegfile\"\;\n";
$out = $out."replot\;\n";

$fd = IO::File->new("$outfile", "w");

select($fd);
print ($out, "\n");

# Now run octave to actually generate the data
#`gnuplot -noraise $tmpfile`;

