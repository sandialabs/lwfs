#!/usr/bin/perl

use Getopt::Long; 

my $mpirun = "mpirun -machinefile darkstar.machines";

GetOptions("authr-nid=i" => \$authr_nid,
	   "authr-pid=i" => \$authr_pid,
	   "num-pts=i" => \$num_pts);

@prog_list = ("posix-nton-create");
@num_stripe_list = (2, 4, 8, 12, 16, 20, 24, 28); 
@num_node_list = (1, 2, 4, 8, 16, 32, 64);

$num_trials = 5; 
$bytes_per_op = 1;  
$ops_per_trial = 1024;

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

                # Count OSTs
                `touch $scratch_file`;
                $num_osts = `lfs getstripe $scratch_file | grep " ACTIVE" | wc -l`;
                $num_osts =~ s/\n//g;
                `rm $scratch_file`;
                
                if ($num_osts !=  $required_osts) {
                    print ("$num_osts active OSTs. We need $required_osts ... skipping test!\n");
                }
                else {

                    print ("$num_osts active OSTs. We need $required_osts ... we're good to go!\n");
                    print ("Running test $prog-$num_nodes-$num_stripes-$required_osts\n");

                    @args = ("$mpirun", "-np","$num_nodes", "$prog", 
                            "--verbose=2", 
                            "--num-trials=$trials_left",
                            "--ops-per-trial=$ops_per_trial",
                            "--bytes-per-op=$bytes_per_op",
                            "--result-file=$result_file",
                            "--result-file-mode=$result_mode",
                            "--remove-file",
                            "--scratch-file=$scratch_file",
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
}
