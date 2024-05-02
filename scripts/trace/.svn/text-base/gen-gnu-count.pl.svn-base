#!/usr/bin/perl

use Getopt::Long; 
use List::Util qw(first max maxstr min minstr reduce shuffle sum);

my $lw = 1;

# command line arguments
my $src = "lwfs-trace.xml";
my $dest = "lwfs-trace.gnu";
my $eps = "lwfs-trace.eps";
my $scriptdir = ".";


GetOptions("scriptdir=s" => \$scriptdir,
	   "src=s" => \$src,
	   "eps=s" => \$eps,
	   "dest=s" => \$dest);

$countstr = `xsltproc $scriptdir/unique-counts.xsl $src`;

@counts = split(":", $countstr);


open (MYOUTFILE, ">$dest"); 

# Print the gnuplot header
#print MYOUTFILE ("set format y \"Thread % g\"\n");
print MYOUTFILE ("set format y\n"); # reset y format
print MYOUTFILE ("set ytics 1\n");
print MYOUTFILE ("set yrange [0:] noreverse\n");
print MYOUTFILE ("set xrange [:] noreverse\n");
print MYOUTFILE ("set xlabel \'Elapsed Time (sec)\'\n\n");
print MYOUTFILE ("set ylabel \'Count\'\n\n");

# Output the plot commands
foreach $count (@counts) {
	print ("Generating \"$count.dat\"...\n");
	@args = ("xsltproc --stringparam name \"$count\" ",
			"-o \"$count.dat\"",
			"$scriptdir/extract-count.xsl $src");
	print (join(" ", @args), "\n"); 
	system(join(" ",@args)) == 0
		or die "system @args failed: $?";


	print ("Adding plot command to \"$dest\"...\n");

	print MYOUTFILE ("plot ");
	print MYOUTFILE ("\'$count.dat\' using 1:2 ",
		"title \'$count\' ",
		"with boxes ",
		"lw $lw\n");

	# footer 
	print MYOUTFILE ("\n");
	print MYOUTFILE ("# set terminal postscript eps enhanced color solid\n");
	print MYOUTFILE ("# set output \"$eps\"\n");
	print MYOUTFILE ("replot\n");
	print MYOUTFILE ("pause -1\n\n");
}

print MYOUTFILE "\n";


close(MYOUTFILE);
