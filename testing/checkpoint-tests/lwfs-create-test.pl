#!/usr/bin/perl

use Getopt::Long; 

my $mpirun = "mpirun -machinefile darkstar-clnts.machines";
my $iface = $ENV{"PTL_IFACE"};
my $authr_nodestr="n10.o0";
my $authr_pid=124;
my $ss_file="ss-list.txt";
my $verbose="2";
my $dryrun=0;
my $in_transit=100;

GetOptions("authr-node=i" => \$authr_nodestr,
	   "authr-pid=i" => \$authr_pid,
	   "verbose=i" => \$authr_pid,
	   "in-transit=i" => \$in_transit,
	   "dryrun" => \$dryrun,
	   "ss-server-file=s" => \$ss_file);

$authr_nid = `rsh $authr_nodestr \"utcp_nid $iface\"`;
$authr_nid =~ s/\n//g;

@prog_list = ("lwfs-nton-create");
@num_stripe_list = (2, 4, 8, 16, 25); 
@num_node_list = (1, 2, 4, 8, 16, 32, 64);

@num_stripe_list = (1, 2, 4, 8, 16);
@num_node_list = (1, 2, 4, 8, 16, 32, 64);
#@num_node_list = (16, 8, 4, 2, 1);

$num_trials = 5; 
$ops_per_trial = 6400;
$ops_per_trial = 12800;


foreach $num_nodes (@num_node_list) {
    foreach $num_stripes (@num_stripe_list) {

        $required_osts = $num_stripes; 

        #if ($num_nodes >= $num_stripes) {

            foreach $prog (@prog_list) {

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
                            "--result-file=$result_file",
                            "--result-file-mode=$result_mode",
                            "--remove-file",
                            "--authr-nid=$authr_nid",
                            "--authr-pid=$authr_pid",
                            "--ss-server-file=$ss_file",
                            "--type=1",
                            "--in-transit=$in_transit",
                            "--ss-num-servers=$required_osts");

                    print ("\n", join(" ", @args), "\n"); 
					if (not $dryrun) {
						$res = system(join(" ",@args)); 

						if ($res != 0) {
							die("\nFAILED TO COMPLETE $result_file\n");
						}
					}
                }
            }
        #}
    }
}
