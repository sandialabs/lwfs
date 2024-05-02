#!/usr/bin/perl

use Getopt::Long; 


my $prog = "mpiexec";
my $num_pts = 21; 
my $output_file = "mpi-xfer.out";
GetOptions("output-file=s" => \$output_file,
	   "num-pts=i" => \$num_pts,
	   "prog=s" => \$prog);

`rm -f $output_file`; 

for ($i=0; $i<$num_pts; $i++) {
	$x[$i] = 2**$i;
	@args = ("$prog -np 2", "mpi-xfer", 
			"--len=$x[$i]", 
			"--count=20",
			"--result-file=$output_file",
			"--result-file-mode=a",
			"--type=0");
	
	`$prog -np 2 mpi-xfer --len=$x[$i] --count=20 --result-file=$output_file --result-file-mode=a --type=0`;

	print (join(" ", @args), "\n"); 
	#system(@args); 
	#system(@args) == 0
	#	or die "system @args failed: $?"
}

