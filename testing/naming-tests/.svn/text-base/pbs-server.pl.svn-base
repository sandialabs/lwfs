#!/usr/bin/perl

#PBS -l n2.p0:ppn=2+n3.o0:ppn=2+n4.o0:ppn=2+n5.o0:ppn=2+n6.o0:ppn=2+n7.o0:ppn=2+n8.o0:ppn=2+n9.o0:ppn=2+n10.o0:ppn=2+n11.o0:ppn=2+n12.o0:ppn=2+n13.o0:ppn=2+n14.o0:ppn=2+n15.o0:ppn=2+n16.o0:ppn=2+n17.o0:ppn=1+n18.o0:ppn=1+n19.o0:ppn=1

#PBS -l walltime=7:0:0
#PBS -j oe

use Getopt::Long; 

$srcdir = "/home/raoldfi/research/sandia/scalable-io/lwfs/src";
$scratch = "/lscratch/raoldfi/"; 
$scratch = "";

$rsh = "rsh";
$PTL_IFACE = "myri0"; 
$pwd = $ENV{"PBS_O_WORKDIR"};
$pwd = $ENV{"PWD"};
$mpirun = "mpirun -machinefile $pwd/machines.p0"; 


$numhosts = 35; 
$count = 10;  # number of experiments to average
$size = 1; # number of files in a directory
$precount = 250;  
$precount = 0;  
$prog = "naming-perf-tests";
$verbose = 4;

$start_servers = 0; 
$run_experiments = 0; 

GetOptions("start-servers=i" => \$start_servers,
	   "run-experiments=i" => \$run_experiments,
	   "size=i" => \$size);

print("start-servers=".$start_servers, "\n");
print("run-experiments=".$run_experiments, "\n");


$ENV{"PTL_IFACE"} = "$PTL_IFACE";

print ("############# START ###############\n"); 
print (`date`, `whoami`, "\n");
print (`pwd`);

# gather hostnames
@hosts = `$mpirun -np $numhosts mpi-hostname`;

# remove the end of line from the hostnames
foreach $host (@hosts) {
}

# sort the array of hostnames
@clients = sort(@hosts); 

foreach $client (@clients) {
	$client =~ s/\n//g;
	$client =~ s/.*://g;
}

print(join("\n", @clients), "\n"); 



# reserve the last three hosts for the servers 
$naming = pop(@clients);
$naming_nid = `$rsh $naming utcp_nid $PTL_IFACE`; 
$naming_nid =~ s/\n//g;
$naming_pid = 126; 

$ss     = pop(@clients);
$ss_nid = `$rsh $ss utcp_nid $PTL_IFACE`; 
$ss_nid =~ s/\n//g;
$ss_pid = 122; 

$authr  = pop(@clients);
$authr_nid = `$rsh $authr utcp_nid $PTL_IFACE`; 
$authr_nid =~ s/\n//g;
$authr_pid = 124; 

# count the number of clients
$np = @clients; 
print("num-clients = $np\n");


print("\n\nStarting experiments...\n");

@total_nodes = (1, 2, 4, 8, 12, 16, 20, 24, 28, 32);

# UFS (Lustre) tests 
@types = ( "ufs_create",
		"ufs_remove", 
		"ufs_stat", 
		"ufs_mkdir", 
		"ufs_rmdir");

# LWFS tests 

@types = ("ping", "create_dir", 
		"remove_dir", 
		"create_file",
		"remove_file", 
		"stat_file", 
		"stat_dir", 
		"lookup",
		"create_obj",
		"remove_obj");



@total_ops = (320, 640, 1280, 2560, 5120, 10240, 20480);
@total_ops = (32, 64, 128, 256, 512, 1024, 2048);
@total_ops = (320, 640, 1280, 2560, 5120, 10240);
@total_ops = (320, 640, 1280, 2560, 5120);


foreach $type (@types) {

	foreach $ops (@total_ops) {

		$outfile = "results/".$type."_".$ops.".out"; 

		`rm -f $outfile`; 

		foreach $nodes (@total_nodes) {

			$ops_per_node = $ops/$nodes;

			if ($start_servers > 0) {
				start_servers();
			}

			if ($nodes <= $np) {
				@args = ("cd $pwd; $mpirun", "-np","$nodes", "$prog", 
						"--verbose=$verbose", 
						"--scratch=$scratch",
						"--num-ops=$ops_per_node", 
						"--count=$count",
						"--precount=$precount",
						"--count=$count",
						"--size=$size",
						"--type=$type",
						"--result-file=$outfile",
						"--result-file-mode=a",
						"--naming-nid=$naming_nid",
						"--naming-pid=$naming_pid",
						"--authr-nid=$authr_nid",
						"--authr-pid=$authr_pid",
						"--ss-nid=$ss_nid",
						"--ss-pid=$ss_pid");

				print ("\n", join(" ", @args), "\n"); 

				if ($run_experiments > 0) {
					system(join(" ", @args)) == 0
						or kill_servers("system @args failed: $?", 1);
				}
			}

			if ($start_servers > 0) {
				kill_servers("finished with type=$type, nodes=$node", 0);
			}
		}
	}
}

#kill_servers("Success!");
print ("############# FINISHED! ###########\n"); 
print (`date`, "\n");
exit(0); 


sub start_servers
{
# Fork a process to start the authorization server 
	if (!defined($authr_upid = fork())) {
		die "Cannot fork server: $!";
	} 

# this is the child
	if ($authr_upid == 0) {
		print("Starting authr server...\n");
		@args = ( "$rsh $authr \"export PTL_IFACE=$PTL_IFACE;",
				"$srcdir/authr/lwfs-authr",
				"--verbose=$verbose", 
				"--dbclear", 
				"--dbfile=/tmp/acls.db >& /tmp/authr.out\"");

		print (join(" ", @args), "\n\n"); 
		exec(join(" ", @args)) == 0
			or die "system @args failed: $?"; 
		exit(0); 
	}


	sleep(3); 


# Fork a process to start the storage server 
	if (!defined($ss_upid = fork())) {
		die "Cannot fork server: $!";
	} 

# this is the child
	if ($ss_upid == 0) {
		print("Starting storage server...\n");
		@args = ("$rsh $ss \"export PTL_IFACE=$PTL_IFACE;",
				"rm -rf /tmp/ss-root;",
				"$srcdir/storage/lwfs-ss",
				"--verbose=$verbose", 
				"--authr-cache-caps", 
				"--authr-nid=$authr_nid", 
				"--authr-pid=$authr_pid", 
				"--root-dir=/tmp/ss-root >& /tmp/ss.out\" ");

		print (join(" ", @args), "\n\n"); 
		exec(join(" ",@args)) == 0
			or die "system @args failed: $?"; 

		exit(0); 
	}


	sleep(3); 


# Fork a process to start the naming server 
	if (!defined($naming_upid = fork())) {
		die "Cannot fork server: $!";
	} 

# this is the child
	if ($naming_upid == 0) {
		print("Starting naming server...\n");

		@args = ("$rsh $naming \"export PTL_IFACE=$PTL_IFACE;",
				"$srcdir/naming/lwfs-naming",
				"--verbose=$verbose", 
				"--cache-caps", 
				"--authr-nid=$authr_nid", 
				"--authr-pid=$authr_pid", 
				"--ss-nid=$ss_nid", 
				"--ss-pid=$ss_pid",
				"--naming-db-path=/usr/tmp/naming.db",
				"--naming-db-clear >& /tmp/naming.out\"");

		print (join(" ", @args), "\n\n"); 
		exec(join(" ",@args)) == 0
			or die "system @args failed: $?"; 

		exit(0); 
	}

	sleep(5); 
}


sub kill_servers {

	my ($str) = shift(@_);
	my ($kill) = shift(@_);

	print($str, "\n"); 

	if ($start_servers > 0) {

		# Kill all the LWFS services 
		print("Killing naming service...\n");
		@args = ("$srcdir/rpc/lwfs-kill",
				"--verbose=$verbose",
				"--server-nid=$naming_nid",
				"--server-pid=$naming_pid");
		print(join(" ", @args), "\n");
		system(join(" ", @args)) == 0
			or die "could not kill naming svc: $?";

		print("Waiting for naming service to complete...\n");
		waitpid($naming_upid, 0); 
		print("Finished naming!\n");


		print("Killing storage service...\n");
		@args = ("$srcdir/rpc/lwfs-kill",
				"--verbose=$verbose",
				"--server-nid=$ss_nid",
				"--server-pid=$ss_pid");
		print(join(" ", @args), "\n");
		system(@args) == 0
			or die "could not kill authr: $?";

		waitpid($ss_upid, 0); 
		print("Finished ss!\n");


		print("Killing authr service...\n");
		@args = ("$srcdir/rpc/lwfs-kill",
				"--verbose=4",
				"--server-nid=$authr_nid",
				"--server-pid=$authr_pid");
		print(join(" ", @args), "\n");
		system(join(" ", @args)) == 0
			or die "could not kill authr: $?";

		waitpid($authr_upid, 0); 
		print("Finished authr!\n");

	}
	if ($kill == 1) {
		exit(-1);
	}
}


