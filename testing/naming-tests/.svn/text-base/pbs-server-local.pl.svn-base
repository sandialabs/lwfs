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


$numhosts = 1; 
$count = 10;  # number of experiments to average
$size = 1; # number of files in a directory
$precount = 250;  
$precount = 0;  
$prog = "naming-perf-tests";
$verbose = 3;

$run_experiments = 0; 

GetOptions(
	   "run-experiments=i" => \$run_experiments,
	   "size=i" => \$size);

print("run-experiments=".$run_experiments, "\n");


$ENV{"PTL_IFACE"} = "$PTL_IFACE";

print ("############# START ###############\n"); 
print (`date`, `whoami`, "\n");
print (`pwd`);


print("\n\nStarting experiments...\n");

@total_nodes = (1, 2, 4, 8, 12, 16, 20, 24, 28, 32);

# UFS (Lustre) tests 
@types = ( "ufs_create",
		"ufs_remove", 
		"ufs_stat", 
		"ufs_mkdir", 
		"ufs_rmdir");

# LWFS tests 

@types = ("create_dir", 
		"remove_dir", 
		"create_file",
		"remove_file", 
		"stat_file", 
		"stat_dir", 
		"lookup",
		"create_obj",
		"remove_obj");

@types = ("create_dir", "remove_dir", "create_file", "remove_file");



@total_ops = (320, 640, 1280, 2560, 5120, 10240, 20480);
@total_ops = (320, 640, 1280, 2560, 5120, 10240);
@total_ops = (320, 640, 1280, 2560, 5120);

@total_ops = (2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096);



foreach $type (@types) {

	$outfile = "results/".$type."_local.out"; 
	`rm -f $outfile`; 

	foreach $ops (@total_ops) {

		@total_nodes = (1);
		foreach $nodes (@total_nodes) {

			$ops_per_node = $ops/$nodes;

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
					"--naming-local",
					"--naming-db-clear",
					"--naming-db-path=/tmp/naming.db",
					"--authr-local",
					"--authr-db-clear",
					"--authr-db-path=/tmp/acls.db",
					"--ss-local");

			print ("\n", join(" ", @args), "\n"); 

			if ($run_experiments > 0) {
				system(join(" ", @args)) == 0
					or die "system @args failed: $?";
			}

		}
	}
}

