#!/usr/bin/perl

use Getopt::Long; 

# These are defaults for the 2cab experiments
my $mpirun = "yod";
my $result_dir = "./results"; 
my $num_trials = 10; 
my $bytes_per_op = 67108864;   
my $ops_per_trial = 8;   # 64 MB*8 ops (is that enough?)

GetOptions("authr-nid=i" => \$authr_nid,
	   "authr-pid=i" => \$authr_pid,
	   "num-pts=i" => \$num_pts,
	   "dryrun" => \$dryrun);

@progs = ("../posix-nton-write");
#push(@progs, "../posix-nto1-write");

# 2Cab has 8 OSTs
$num_ost = 8;
$stripe_size = 1048576;
@num_stripes = (1, 2, 4, 6, 8); 
$np_max = 128;


$scratch_dir = "/scratch1/raoldfi";


foreach $num_stripe (@num_stripes) {
    foreach $prog (@progs) {

	$result_suffix = "stripes=${num_stripe}_ost=${num_ost}.out";
	$scratch_file = "$scratch_dir\/stripe$num_stripe\/test";

	@prog_args = ( "--ops-per-trial=$ops_per_trial",
		"--bytes-per-op=$bytes_per_op",
		"--scratch-file=$scratch_file",
		"--remove-file",
		"--num-stripe=$num_stripe", 
		"--stripe-size=$stripe_size", 
		"--num-ost=$num_ost");

	@args = ("test-harness.pl", 
		"--mpirun=$mpirun",
		"--np-max=$np_max",
		"--prog=$prog",
		"--num-trials=$num_trials",
		"--result-dir=$result_dir",
		"--result-suffix=$result_suffix",
		"--prog-args=\"".join(" ",@prog_args)."\"");

	if ($dryrun) {
	    push(@args,"--dryrun");
	}

	print ("\n", join(" ", @args), "\n"); 
	system(join(" ",@args)) == 0
	    or die "system @args failed: $?";
    }
}
