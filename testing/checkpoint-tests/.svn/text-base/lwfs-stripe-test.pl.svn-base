#!/usr/bin/perl

use Getopt::Long; 

my $mpirun = "mpirun -machinefile darkstar.machines";

my $authr_nid=3232236687;
my $authr_pid=124;
my $ss_file="ss-list.txt";
my $verbose="2";

GetOptions("authr-nid=i" => \$authr_nid,
	   "authr-pid=i" => \$authr_pid,
	   "verbose=i" => \$authr_pid,
	   "ss-file=s" => \$ss_list);

@prog_list = ("lwfs-nton-stripe");
@num_stripe_list = (2, 4, 8, 12, 16, 20, 24, 28); 
@num_node_list = (1, 2, 4, 8, 16, 32, 64);

$num_trials = 5; 
$bytes_per_op = 67108864;  
$ops_per_trial = 8;

$scratch_dir = "/lscratch/raoldfi";

foreach $num_nodes (@num_node_list) {
    foreach $num_stripes (@num_stripe_list) {

        $required_osts = $num_stripes; 

        foreach $prog (@prog_list) {

            $scratch_file = "$scratch_dir\/stripe$num_stripes\/test";

            $result_file  = ".\/output\/$prog-$num_nodes-$num_stripes-$required_osts.out";

            if (-e $result_file) {

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
            }

            if ($trials_left > 0) {

                print ("$num_osts active OSTs. We need $required_osts ... we're good to go!\n");
                print ("Running test $prog-$num_nodes-$num_stripes-$required_osts\n");

                @args = ("$mpirun", "-np","$num_nodes", "$prog", 
                        "--verbose=$verbose", 
                        "--num-trials=$trials_left",
                        "--ops-per-trial=$ops_per_trial",
                        "--bytes-per-op=$bytes_per_op",
                        "--result-file=$result_file",
                        "--result-file-mode=$result_mode",
                        "--remove-file",
                        "--scratch-file=$scratch_file",
                        "--authr-nid=$authr_nid",
                        "--authr-pid=$authr_pid",
                        "--ss-file=$ss_file",
                        "--num-ss=$required_osts");

                print ("\n", join(" ", @args), "\n"); 
                $res = system(join(" ",@args)); 

                if ($res != 0) {
                    die("\nFAILED TO COMPLETE $result_file\n");
                }
            }
        }
    }
}
