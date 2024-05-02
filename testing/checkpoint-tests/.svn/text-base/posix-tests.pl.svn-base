#!/usr/bin/perl

use Getopt::Long; 

my $mpirun = "mpirun -machinefile darkstar.machines";

GetOptions("authr-nid=i" => \$authr_nid,
	   "authr-pid=i" => \$authr_pid,
	   "num-pts=i" => \$num_pts);

@progs = ("posix-nton-write", "posix-nto1-write");
@num_ss = (1, 3, 5, 10, 15, 20, 25, 30); 
@nodes = (1, 2, 4, 8, 16, 32, 64, 128);


$num_trials = 5; 
$bytes_per_op = 67108864;  
$ops_per_trial = 8;

$scratch_dir = "/lscratch/raoldfi";

foreach $node (@nodes) {
    foreach $ss (@num_ss) {
        foreach $prog (@progs) {

            $result_file  = ".\/output\/$prog-$node-$ss.out";
            $tmpfile = ".\/output\/$prog-$node-$ss.processing";

            $scratch_file = "$scratch_dir\/stripe$ss\/test";

            if (-e $result_file) {
                print ("File $result_file exists, skipping test\n"); 
            }
            else {
                print ("Running test $prog-$node-$ss\n");

                @args = ("$mpirun", "-np","$node", "$prog", 
                        "--verbose=2", 
                        "--num-trials=$num_trials",
                        "--ops-per-trial=$ops_per_trial",
                        "--bytes-per-op=$bytes_per_op",
                        "--result-file=$tmpfile",
                        "--result-file-mode=w",
                        "--remove-file",
                        "--result-file-mode=w",
                        "--scratch-file=$scratch_file",
                        "--num-ss=$ss");

                print ("\n", join(" ", @args), "\n"); 
                system(join(" ",@args)) == 0
                    or die "system @args failed: $?";

                `mv $tmpfile $result_file`; 
            }
        }
    }
}
