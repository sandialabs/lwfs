#!/usr/bin/perl


use Getopt::Long; 

# Defaults
my $num_trials = 5;
my $ops_per_trial = 5;
my $np = 0;
my $np_max = 1;
my $prog = "test";
my $mpirun = "mpirun";
my $result_suffix = 0;
my $result_dir = "result";
my $mpirun = "mpirun";
my $dryrun = 0;
my $extra_args = "";


GetOptions("num-trials:i" => \$num_trials,
	   "np:i" => \$np,
	   "np-max:i" => \$np_max,
	   "prog=s" => \$prog,
	   "mpirun:s" => \$mpirun,
	   "result-suffix:s" => \$result_suffix,
	   "result-dir:s" => \$result_dir,
	   "dryrun" => \$dryrun,
	   "prog-args:s" => \$prog_args);

print "Unprocessed by Getopt::Long\n" if $ARGV[0];
foreach (@ARGV) {
    print "$_\n";
}

# If $np is not set, $np increases from 1 to $np_max by powers of 2
if ($np != 0) {
    $np_max = $np;
    $np_init = $np; 
}
else {
    $np_init = 1; 
}


# $np doubles after every iteration
for ($np=$np_init; $np <= $np_max; $np=$np*2) {

    # remove all directory information from the progname
    $progname = $prog;
    $progname =~ s/.*\///g;

    $result_file = "$result_dir\/${progname}_np=$np";
    if ($result_suffix) {
	$result_file = "$result_file"."_$result_suffix";
    }
    else {
	$result_file = "$result_file".".out";
    }



    # calculate the number of trials remaining
    if (-e "$result_file") {
	$lines = `grep -v \% $result_file | wc -l`;
	$lines =~ s/\n//g;

	$trials_left = $num_trials - $lines; 
	$result_mode = "a";

	print ("File $result_file exists, $trials_left ",
		"trials remaining \n"); 
    }
    else {
	$trials_left = $num_trials;
	$result_mode = "w";

	print ("Creating $result_file, $trials_left ",
		"trials remaining \n"); 
    }


    if ($trials_left > 0) {

	# Generate the arguments for the job
	@args = ("$mpirun", "-np","$np", "$prog", 
		"--num-trials=$trials_left",
		"--result-file=$result_file",
		"--result-file-mode=$result_mode",
		"$prog_args");

	print ("\n", join(" ", @args), "\n"); 
	if (!$dryrun) {
	    $res = system(join(" ",@args)); 

	    if ($res != 0) {
		die("\nFAILED TO COMPLETE $result_file\n");
	    }
	}
    }
}
