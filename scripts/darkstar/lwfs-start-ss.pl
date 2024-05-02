#!/usr/bin/perl

# mount the scratch drives on nodes n0.o0 - n14.o0

use Getopt::Long; 

my $dryrun=0;
my $iface = $ENV{"PTL_IFACE"};
my $src = "\/home\/raoldfi\/research\/sandia\/scalable-io\/lwfs\/trunk\/src";
my $logdir = "\/home\/raoldfi\/log";

# authr options
my $authr_cache_caps = 1;
my $authr_nodestr = "n10.o0";
my $authr_pid=124;

# ss options
my $use_threads=1;
my $num_threads=10; 
my $verbose=2;

my $ss_trace = 0;
my $ss_tracedir = "\/home\/raoldfi\/trace";
my $ss_iolib="aio";
my $ss_nodetype = "o"; 
my $ss_rack = 0;
my $ss_nodestr = "5,6,7,8,11,12,13,15";

if (-e "servers.txt") {
	$ss_nodestr = `cat servers.txt`;
	$ss_nodestr =~ s/#.*\n//g; 
	$ss_nodestr =~ s/\n/,/g; 
	$ss_nodestr =~ s/,+$//g; 
}



my @ss_pids = (122, 123);
my @ss_root = ("\/l1scratch\/raoldfi\/ss-root",
        "\/l2scratch\/raoldfi\/ss-root");
my $ss_simdisk=0;



GetOptions(
		"srcdir" => \$srcdir,
		"logdir" => \$logdir,
		"use-threads" => \$use_threads,
		"num-threads=i" => \$num_threads,
		"verbose=i" => \$verbose,
		"dryrun" => \$dryrun, 
		"authr-node=s" => \$authr_nodestr,
		"authr-cache-caps" => \$authr_cache_caps,
		"ss-trace" => \$ss_trace,
		"ss-tracedir" => \$ss_tracedir,
		"ss-rack=i" => \$ss_rack,
		"ss-nodetype=s" => \$ss_nodetype,
		"ss-nodenums=s" => \$ss_nodestr,
		"ss-iolib=s" => \$ss_iolib,
		"ss-nodes=s" => \$ss_nodestr);


@ss_nodelist = split(",",$ss_nodestr);

# extract the node ID
foreach $node (@ss_nodelist) {
	$node =~ s/n//g;
	$node =~ s/\..*//g;
}

# Get the nid of the authr server
my $authr_nid=`rsh $authr_nodestr \"utcp_nid $iface\"`;
$authr_nid =~ s/\n//g;

# Set the threadpool options
my $min_threads=$num_threads;
my $max_threads=$num_threads;
my $init_threads=$num_threads;
my $high_watermark=10;
my $low_watermark=5;

if ($ss_trace) {
    $ss_trace_opt = "--ss-trace"; 
}
else {
    $ss_trace_opt="";
}

if ($use_threads) {
    $use_threads_opt = "--use-threads"; 
}
else {
    $use_threads_opt="";
}

if ($authr_cache_caps) {
	$cache_caps_opt = "--authr-cache-caps";
}
else {
	$cache_caps_opt = "";
}


# Make directories
system("mkdir -p $logdir") == 0
    or die "could not create log directory";

system("mkdir -p $ss_tracedir") == 0
    or die "could not create trace directory";




&start_ss($ss_nodetype,$ss_rack,join(",",@ss_nodelist),0);
&start_ss($ss_nodetype,$ss_rack,join(",",@ss_nodelist),1);











sub start_ss {
	$nodetype = $_[0];
	$rack = $_[1];
	$nodelist = $_[2];
	$i = $_[3];

	@nodes = split(",",$nodelist);


	# remove the root dir so we can start from scratch
	@args = ("ccmd --$nodetype $rack --node $nodelist",
			"\"find $ss_root[$i]\/ -name '0x*' | xargs rm -f\"");
	print(join(" ", @args), "\n\n");
	if ($dryrun == 0) {
		system(join(" ", @args)) == 0
			or die "could start ss $i on $node"
	}

	foreach $node (@nodes) {

		# start the storage service
		@args = ("ccmd --$nodetype $rack --node $node",
				"\"$src\/storage\/lwfs-ss",
				"--verbose=$verbose",
				"--authr-nid=$authr_nid",
				"--authr-pid=$authr_pid",
				"$cache_caps_opt",
				"--ss-iolib=$ss_iolib",
				"--ss-pid=$ss_pids[$i]",
				"--ss-root-dir=$ss_root[$i]",
				"$ss_trace_opt",
				"--ss-trace-file=$ss_tracedir/ss-n$node.$nodetype$rack-$i.sddf",
				"$ss_simdisk_opt",
				"$use_threads_opt",
				"--tp-init-thread-count=$init_threads",
				"--tp-min-thread-count=$min_threads",
				"--tp-max-thread-count=$max_threads",
				"--tp-high-watermark=$high_watermark",
				"--tp-low-watermark=$low_watermark",
				"\>\& $logdir\/ss-n$node.$nodetype$rack-$i.log \& \"");
		print(join(" ", @args), "\n\n");
		if ($dryrun == 0) {
			system(join(" ", @args)) == 0
				or die "could start ss $i on $node"
		}
	}
}
