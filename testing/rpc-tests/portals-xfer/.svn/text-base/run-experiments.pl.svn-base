#!/usr/bin/perl

use Getopt::Long;

my $rsh = ssh; 
my $dir = $ENV{"PWD"};
my $server_exec = "portals-xfer";
my $client_exec = "portals-xfer";
my $dryrun = 1;
my $num_pts = 23;
my $num_reqs = 64;
my $count = 20; 
my $output_file = "portals-xfer.out";
my $server_nid = 0;
my $server_pid = 128;
my $iface = $ENV{'PTL_IFACE'}; 
my $server = "localhost";

GetOptions("server=s" => \$server,
		"rsh=s" => \$rsh,
		"server-nid=i" => \$server_nid,
		"output-file=s" => \$output_file,
		"dryrun=i" => \$dryrun,
		"iface=s" => \$output_file,
		"num-pts=i" => \$num_pts);


`rm -f $output_file`;

$server_nid = `$rsh $server utcp_nid $iface`;
$server_nid =~ s/\n//g;

$myname = `hostname`;
$myname =~ s/\n//g;


$start = 0;
$end = $num_pts;

	for ($i=$start; $i<$end; $i++) {
		$x[$i] = 2**$i;

		$kidpid = fork();

		if (!defined($kidpid)) {
			die "Cannot fork: $!";
		}
		elsif ($kidpid == 0) {  
			# this branch is a child (start the server)
			`$rsh $server pkill -SIGKILL $server_exec`;
			print("\n\n$Starting server on $server ...\n");
			@args = ("$rsh $server $dir/$server_exec --server --count=".$num_reqs*($count+1));
			print(join(" ", @args), "\n");
			if ($dryrun == 0) {
				#system(join(" ", @args)) == 0
				#	or die "system @args failed: $?"
				
				exec(join(" ", @args));

				# if exec fails, fall through to next statement
				die "exec @args failed: $?"
			}
		}

		else {
			# this branch is parent
			sleep(3); 
			print("\n\nStarting client on $myname ...\n");
			@args = ("$client_exec",
					"--len=$x[$i]",
					"--server-nid=".$server_nid,
					"--server-pid=".$server_pid,
					"--num-reqs=".$num_reqs,
					"--count=$count",
					"--result-file=$output_file",
					"--result-file-mode=a",
					"");

			print (join(" ", @args), "\n");
			if ($dryrun == 0) {
				system(join(" ", @args)) == 0
					or die "system @args failed: $?"
			}

			# wait for the server to complete
			waitpid($kidpid,0);
		}
	}

# Kill the server process




exit;





#------------- CUT HERE ----------------




($myid != -1) or die "could not assign id to process"; 


print("$myid: server_nid=$server_nid\n");


$start = 0;
$end = $num_pts;

for ($i=$start; $i<$num_pts; $i++) {
	$x[$i] = 2**$i;

	if ($myid == 0) {
		sleep(5); 
		print("$myid: Starting server on $myname ...\n");
		@args = ("portals-xfer", 
				"--server", 
				"--count=".2*$count);
		print(join(" ", @args), "\n");
		system(@args) == 0
			or die "system @args failed: $?"
	}

	else {
		sleep(8); 
		print("$myid: Starting client on $myname ...\n");
		@args = ("portals-xfer", 
				"--len=$x[$i]", 
				"--count=20",
				"--server-nid=".$server_nid,
				"--server-pid=128",
				"--result-file=$output_file",
				"--result-file-mode=a");

		print (join(" ", @args), "\n"); 
		system(@args) == 0
			or die "system @args failed: $?"
	}
}

