#!/usr/bin/perl

use Getopt::Long; 

my $dryrun = 0; 
my $mpirun = "mpirun -machinefile darkstar-clnts.machines";
my $iface = $ENV{"PTL_IFACE"};

my $in_transit = 50;
my $authr_node="n10.o0";
my $authr_pid=124;
my $ss_file="ss-list.txt";
my $verbose="2";
my $local=0;

$type = 1;  # async reqs
$num_trials = 5; 
$bytes_per_op = 67108864;  
$ops_per_trial = 8;

$bytes_per_op = 16777216;
$ops_per_trial = 32;

GetOptions("authr-node=s" => \$authr_node,
	   "authr-pid=i" => \$authr_pid,
	   "verbose=i" => \$authr_pid,
	   "type=i" => \$type,
	   "in-transit=i" => \$in_transit,
	   "ops-per-trial=i" => \$ops_per_trial,
	   "num-trials=i" => \$num_trials,
	   "bytes-per-op=i" => \$bytes_per_op,
	   "dryrun" => \$dryrun,
	   "local" => \$local,
	   "ss-file=s" => \$ss_list);

# extract the node ID
my $authr_nid=`rsh $authr_node \"utcp_nid $iface\"`;
$authr_nid =~ s/\n//g;

@prog_list = ("lwfs-nton-write");
@num_stripe_list = (1, 2, 4, 8, 12, 16, 20, 24, 28); 
@num_node_list = (1, 2, 4, 8, 16, 28, 32, 48, 64);

# this is what we plotted in the IPDPS paper
@num_node_list = (1, 2, 4, 8, 16, 32, 64);
@num_stripe_list = (2, 4, 8, 16, 28); 

@num_node_list = (1, 2, 4, 8, 16, 32, 64);
@num_stripe_list = (1,2,4,8,16);

$type = 1; 

if ($local != 0) {
    $authr_opts = "--authr-local";
    $ss_opts = "--ss-local --ss-root-dir=\/usr\/tmp\/ss-root";
    $ss_opts = "--ss-local --ss-root-dir=\/l2scratch\/raoldfi\/ss-root";
}
else {
    $authr_opts = "--authr-nid=$authr_nid --authr-pid=$authr_pid";
    $ss_opts = "--ss-server-file=$ss_file";
}




$scratch_dir = "/lscratch/raoldfi";

foreach $num_nodes (@num_node_list) {
    foreach $num_stripes (@num_stripe_list) {

        $required_osts = $num_stripes; 

        #if ($num_nodes >= $num_stripes) {

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
                            "--type=$type",
                            "--in-transit=$in_transit",
                            "--remove-file",
                            "--scratch-file=$scratch_file",
                            "$authr_opts",
                            "$ss_opts",
                            "--ss-num-servers=$required_osts");

                    print ("\n", join(" ", @args), "\n"); 
                    if ($dryrun == 0) {
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
