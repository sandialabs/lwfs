#!/usr/bin/perl

# Must have a server running for this to work.

use Getopt::Long;

my $rsh = ssh; 
my $dir = $ENV{"PWD"};
my $iface = $ENV{"PTL_IFACE"};
my $server = "localhost";
my $server_exec = "rpc-xfer-server";
my $client_exec = "rpc-xfer";
my $dryrun = 1;
my $num_pts = 23;
my $num_reqs = 64; 
my $count = 20; 
my $output_file = "rpc-xfer.out";
GetOptions(
		"server=s" => \$server,
		"output-file=s" => \$output_file,
		"dryrun=i" => \$dryrun,
		"rsh=s" => \$rsh,
		"num-pts=i" => \$num_pts);

`rm -f $output_file`;


$i = 0; 

$myname = `hostname`;
$myname =~ s/\n//g;

if ($iface == "myri0") {
	$remote_server = "$iface\.$server";
}
else {
	$remote_server = $server;
}

# Start the server process
#`$rsh $servername $dir/$server_exec &`;


$start = 0;
$end = $num_pts;

@types = (0, 1);
@output_file = ("rpc-xfer-0.out", "rpc-xfer-1.out");

foreach $type (@types) {
	for ($i=$start; $i<$num_pts; $i++) {
		$x[$i] = 2**$i;

		$kidpid = fork();

		if (!defined($kidpid)) {
			die "Cannot fork: $!";
		}
		elsif ($kidpid == 0) {  
			# this branch is a child (start the server)
			`$rsh $server pkill -9 $server_exec`;
			print("$Starting server on $server ...\n");
			@args = ("$rsh $server $dir/$server_exec", $num_reqs*($count+1));
			print(join(" ", @args), "\n");
			if ($dryrun == 0) {
				exec(join(" ", @args));

				# if exec fails, fall through to next statement
				die "exec @args failed: $?"
			}
		}

		else {
			# this branch is parent
			sleep(3); 
			print("Starting client on $myname ...\n");
			@args = ("rpc-xfer",
					"--len=$x[$i]",
					"--count=$count",
					"--num-reqs=$num_reqs",
					"--result-file=$output_file[$type]",
					"--result-file-mode=a",
					"--server=$remote_server",
					"");

			print (join(" ", @args), "\n");
			if ($dryrun == 0) {
				system(join(" ", @args)) == 0
					or die "system @args failed: $?"
			}

			waitpid($kidpid,0);
		}
	}
}

# Kill the server process
