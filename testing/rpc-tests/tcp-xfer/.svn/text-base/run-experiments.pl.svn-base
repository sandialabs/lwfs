#!/usr/bin/perl

use Getopt::Long;

my $rsh = rsh; 
my $dir = $ENV{"PWD"};
my $server = "localhost";
my $iface = $ENV{"PTL_IFACE"};
my $server_exec = "tcp-xfer";
my $client_exec = "tcp-xfer";
my $dryrun = 1;
my $num_pts = 23;
my $num_reqs = 64;
my $count = 20; 
my $output_file = "tcp-xfer.out";
GetOptions(
		"server=s" => \$server,
		"output-file=s" => \$output_file,
		"dryrun=i" => \$dryrun,
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

for ($i=$start; $i<$end; $i++) {
	$x[$i] = 2**$i;

	$kidpid = fork();

	if (!defined($kidpid)) {
		die "Cannot fork: $!";
	}
	elsif ($kidpid == 0) {  
		# this branch is a child (start the server)
		`$rsh $server pkill -SIGKILL $server_exec`;
		print("$Starting server on $server ...\n");
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
		print("Starting client on $myname ...\n");
		@args = ("$client_exec",
				"--len=$x[$i]",
				"--count=$count",
				"--num-reqs=$num_reqs",
				"--result-file=$output_file",
				"--result-file-mode=a",
				"$remote_server",
				"");

		print (join(" ", @args), "\n");
		if ($dryrun == 0) {
			system(join(" ", @args)) == 0
				or die "system @args failed: $?"
		}

		waitpid($kidpid,0);
	}
}

