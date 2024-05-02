#!/usr/bin/perl

#PBS -l n2.p0:ppn=2+n3.o0:ppn=2+n4.o0:ppn=2+n5.o0:ppn=2+n6.o0:ppn=2+n7.o0:ppn=2+n8.o0:ppn=2+n9.o0:ppn=2+n10.o0:ppn=2+n11.o0:ppn=2+n12.o0:ppn=2+n13.o0:ppn=2+n14.o0:ppn=2+n15.o0:ppn=2+n16.o0:ppn=2+n17.o0:ppn=1+n18.o0:ppn=1+n19.o0:ppn=1

#PBS -l walltime=7:0:0
#PBS -j oe

use Getopt::Long; 

$srcdir = "/home/raoldfi/research/sandia/scalable-io/lwfs/src";
$scratch = "/lscratch/raoldfi/"; 

$rsh = "rsh";
$PTL_IFACE = "myri0"; 
$pwd = $ENV{"PBS_O_WORKDIR"};
$pwd = $ENV{"PWD"};
$mpirun = "mpirun -machinefile $pwd/machines.o0"; 


$numhosts = 32; 
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


# count the number of clients
$np = @clients; 
print("num-clients = $np\n");


print("\n\nStarting experiments...\n");

@total_nodes = (1, 2, 4, 8, 16, 32);


# LWFS tests 
@types = ("create_dir",
		"ping",
		"create_file",
		"remove_file",
		"stat_file",
		"remove_dir",
		"stat_dir",
		"create_obj",
		"remove_obj",
		"lookup");

# UFS (Lustre) tests 
@types = ( "ufs_create",
		"ufs_remove", 
		"ufs_stat", 
		"ufs_mkdir", 
		"ufs_rmdir");


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
						"--ufs-test");

				print ("\n", join(" ", @args), "\n"); 

				if ($run_experiments > 0) {
					system(join(" ", @args)) == 0
						or kill_servers("system @args failed: $?", 1);
				}
			}

		}
	}
}

print ("############# FINISHED! ###########\n"); 
print (`date`, "\n");
exit(0); 

