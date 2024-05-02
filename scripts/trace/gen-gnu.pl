#!/usr/bin/perl

use Getopt::Long; 
use List::Util qw(first max maxstr min minstr reduce shuffle sum);

my $lw = 2;
my $bar = 2;

# command line arguments
my $src = "lwfs-trace.xml";
my $dest = "lwfs-trace.gnu";
my $eps = "lwfs-trace.eps";
my $scriptdir = ".";


GetOptions("scriptdir=s" => \$scriptdir,
	   "src=s" => \$src,
	   "eps=s" => \$eps,
	   "dest=s" => \$dest);

$intervalstr = `xsltproc $scriptdir/unique-intervals.xsl $src`;
$pidstr      = `xsltproc $scriptdir/unique-pids.xsl $src`;

@intervals = split(":", $intervalstr);
@pids = split(":", $pidstr);

$ymin = (min @pids) - 0.5; 
$ymax = (max @pids) + 0.5; 


open (MYOUTFILE, ">$dest"); 

# Print the gnuplot header
print MYOUTFILE ("set format y \"Thread % g\"\n");
print MYOUTFILE ("set ytics 1\n");
print MYOUTFILE ("set yrange [$ymin:$ymax] reverse\n");
print MYOUTFILE ("set xrange [:] noreverse\n");
print MYOUTFILE ("set bar $bar\n");
print MYOUTFILE ("set xlabel \'Elapsed Time (sec)\'\n\n");

# Output the plot commands
foreach $interval (@intervals) {
	print ("Generating \"$interval.dat\"...\n");
	@args = ("xsltproc --stringparam name \"$interval\" ",
			"-o \"$interval.dat\"",
			"$scriptdir/extract-interval.xsl $src");
	#print (join(" ", @args), "\n"); 
	system(join(" ",@args)) == 0
		or die "system @args failed: $?";


	print ("Adding plot command to \"$dest\"...\n");

	if ($interval eq $intervals[0]) {
		print MYOUTFILE ("plot ");
	}
	else {
		print MYOUTFILE ("replot ");
	}

	print MYOUTFILE ("\'$interval.dat\' using 2:1:2:3 ",
		"title \'$interval\' ",
		"with xerrorbars ",
		"lw $lw pt 0 ps 0\n");
}

print MYOUTFILE "\n";

# Output the borders for each thread
foreach $pid (@pids) {
	print MYOUTFILE "# Boundary for Thread $pid\n";
	$ymin = $pid-0.5; 
	$ymax = $pid+0.5; 
	print MYOUTFILE "replot $ymin notitle lt 0\n";
	print MYOUTFILE "replot $ymax notitle lt 0\n\n";
}



# Print the footer 
print MYOUTFILE ("\n");
print MYOUTFILE ("# set terminal postscript eps enhanced color solid\n");
print MYOUTFILE ("# set output \"$eps\"\n");
print MYOUTFILE ("replot\n");

close(MYOUTFILE);
