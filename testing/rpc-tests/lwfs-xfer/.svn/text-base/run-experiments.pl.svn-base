#!/usr/bin/perl

use Getopt::Long; 
use POSIX ":sys_wait_h";

my $mpirun = "mpirun";
my $kill_server = "lwfs-kill";
my $server_exec = "lwfs-xfer-server";
my $client_exec = "lwfs-xfer-client";
my $num_threads = 5; 
my $num_trials = 5; 
my $num_reqs = 256;
my $rsh = "rsh";
my $num_trials = 10;
my $num_pts = 21; 
my $output_file = "lwfs-xfer.out";
my $server = "localhost";
my $server_nid = 0;
my $server_pid = 122;
my $iface = $ENV{'PTL_IFACE'};
my $dryrun = 0;
my $use_threads = 1;

GetOptions(
	   "server=s" => \$server,
	   "output-file=s" => \$output_file,
	   "rsh=s" => \$rsh,
	   "num-pts=i" => \$num_pts,
	   "use-threads=i" => \$use_threads,
	   "mpirun=s" => \$mpirun,
	   "dryrun=i" => \$dryrun);


# Get the nid of the server
$server_nid = `$rsh $server utcp_nid $iface`;
$server_nid =~ s/Running.*\n//g;
$server_nid =~ s/\n//g;


$dir = $ENV{'PWD'};

print ("server_nid=$server_nid\n");

@outfiles = ("output/lwfs-xfer-args-blk",
		"output/lwfs-xfer-args-noblk", 
		"output/lwfs-xfer-data-blk", 
		"output/lwfs-xfer-data-noblk");

@num_node_list = (1, 2, 4, 8, 16, 32, 64);
@num_node_list = (1);

@num_pts = (1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 
        4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288);  
# 1048576,  2097152, 4194304, 8388608);


@types = (0, 1, 2, 3);  # only test blocking versions
@types = (2, 3);  # only test blocking versions

if ($use_threads == 0) {
	$use_threads_str = "";
}
else {
	$use_threads_str = "--use-threads";
}


$i=-1; 
$start = 0;


foreach $type (@types) {
    
    print ("\n====================================\n");
    print ("Starting experiments for type $type\n\n");

	foreach $num_nodes (@num_node_list) {

		foreach  $pts (@num_pts) {
			$i++;

			print ("\n------------------------------------\n");
			$outfile = $outfiles[$type]."-$num_nodes-$pts.out"; 

			if (-e $outfile) {
				$lines = `grep -v \% $outfile | wc -l`;
				$lines =~ s/\n//g;

				$trials_left = $num_trials - $lines;
				$result_mode = "a";

				print ("File $outfile exists, $trials_left ",
						"trials remaining \n");
			}
			else {
				$trials_left = $num_trials; 
				$result_mode = "w";
				print ("Creating $outfile, $trials_left ",
						"trials remaining \n");
			}

			if ($trials_left > 0) {


				$kidpid = fork(); 

				if (!defined($kidpid)) {
					die "Cannot fork: $!";
				}

				elsif ($kidpid == 0) {
					# start the server
					print("Starting server on $server ...\n");
					@args = ("$rsh $server",
							"$dir/$server_exec",
							"--pid=$server_pid",
							"--verbose=2",
							"$use_threads_str",
							"--tp-min-thread-count=$num_threads",
							"--tp-max-thread-count=$num_threads",
							"--tp-init-thread-count=$num_threads",
							#"--count=".(($trials_left*$num_reqs)+1),
							"");

					print(join(" ", @args), "\n");
					if ($dryrun == 0) {
						exec(join(" ", @args)); 

						die "exec @args failed: $?"
					}
				}

				else {
					# start the client 
					sleep(3);  

					print("Starting client ...\n");

					@args = ("$mpirun -np $num_nodes",
							"$dir/$client_exec", 
							"--verbose=2", 
							"--len=$pts", 
							"--count=".$trials_left,
							"--num-reqs=$num_reqs",
							"--type=$type",
							"--result-file=$dir/$outfile",
							"--result-file-mode=$result_mode",
							"--server-nid=$server_nid",
							"--server-pid=$server_pid",
							"");

					print (join(" ", @args), "\n"); 
					if ($dryrun == 0) {
						system(join(" ",@args)) == 0
							or die "exec @args failed: $?"
					}

					print("\nFinished test $i ...\n\n");

					# kill the server process
					@args = ("$dir/$kill_server",
							"--server-nid=$server_nid",
							"--server-pid=$server_pid",
							"--verbose=2",
							"");

					print (join(" ", @args), "\n"); 
					if ($dryrun == 0) {
						system(join(" ",@args)) == 0
							or die "exec @args failed: $?"
					}

					waitpid($kidpid, 0); 
				}
			}
		}
	}

    print ("Finished experiments for type $type\n");
}

