#!/usr/bin/perl

# mount the scratch drives on nodes n0.o0 - n14.o0

use Getopt::Long; 

my $dryrun=0;
my $iface = $ENV{"PTL_IFACE"};
my $src = "\/home\/raoldfi\/research\/sandia\/scalable-io\/lwfs\/trunk\/src";
my $log = "\/home\/raoldfi\/log";
my $trace = "\/home\/raoldfi\/trace";

# authr options
my $authr_cache_caps = 0;
my $authr_rack = 0;
my $authr_node = 15;
my $authr_node = 13;
my $authr_pid=124;
my $authr_nid=`rsh n$authr_node.o$authr_rack \"utcp_nid $iface\"`;
$authr_nid =~ s/Running.*\n//g;
$authr_nid =~ s/\n//g;

# ss options
my $use_threads=0;
my $num_threads=4; 
my $verbose=2;
my $iolib="sysio";

my $ss_rack = 0;
my $ss_nodestr = "5:6:7:8:9:10:11:12:13";
my @ss_pids = (122, 123);
my @ss_root = ("\/l1scratch\/raoldfi\/ss-root",
        "\/l2scratch\/raoldfi\/ss-root");
my $ss_simdisk=0;



GetOptions(
		"use-threads" => \$use_threads,
		"cache-caps" => \$authr_cache_caps,
		"num-threads=i" => \$num_threads,
		"iolib=s" => \$iolib,
		"authr=s" => \$authr_node,
		"ss-nodes=s" => \$ss_nodestr,
		"logdir=s" => \$log,
		"tracedir=s" => \$trace,
		"verbose=i" => \$verbose,
		"dryrun" => \$dryrun);


my $min_threads=$num_threads;
my $max_threads=$num_threads;
my $init_threads=$num_threads;
my $high_watermark=10;
my $low_watermark=5;

@ss_nodes = split(":", $ss_nodestr);

print(join(" ", @ss_nodes), "\n");

if ($ss_simdisk) {
    $ss_simdisk_opt = "--ss-simdisk"; 
}
else {
    $ss_simdisk_opt=""; 
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
system("mkdir -p $log") == 0
    or die "could not create log directory";

system("mkdir -p $trace") == 0
    or die "could not create trace directory";


# Start the authorization server 
@args = ("ccmd --o $authr_rack --node $authr_node",
        "\"$src\/authr\/lwfs-authr --verbose=$verbose --authr-db-clear",
        "--authr-trace --authr-trace-file=$trace/authr-$authr_node.sddf",
        "\&\> $log\/authr-$authr_node.log \& \"");
print(join(" ", @args), "\n\n");
if ($dryrun == 0) {
    system(join(" ", @args)) == 0
        or die "could not start authr on $authr_node"
}

sleep(5);


foreach $node (@ss_nodes) {

    for ($i=0; $i<2; $i++) {
		# remove the root dir so we can start from scratch
		@args = ("ccmd --o $ss_rack --node $node",
				"\"find $ss_root[i]\/ -name '0x*' | xargs rm -f\"");
        print(join(" ", @args), "\n\n");
        if ($dryrun == 0) {
            system(join(" ", @args)) == 0
                or die "could start ss $i on $node"
        }

		# start the storage service
        @args = ("ccmd --o $ss_rack --node $node",
                "\"$src\/storage\/lwfs-ss",
                "--verbose=$verbose",
                "--authr-nid=$authr_nid",
                "--authr-pid=$authr_pid",
                "$cache_caps_opt",
                "--ss-iolib=$iolib",
                "--ss-pid=$ss_pids[$i]",
                "--ss-root-dir=$ss_root[$i]",
                "--ss-trace",
                "--ss-trace-file=$trace/ss-$node-$i.sddf",
                "$ss_simdisk_opt",
                "$use_threads_opt",
                "--tp-init-thread-count=$init_threads",
                "--tp-min-thread-count=$min_threads",
                "--tp-max-thread-count=$max_threads",
                "--tp-high-watermark=$high_watermark",
                "--tp-low-watermark=$low_watermark",
                "\>\& $log\/ss-$node-$i.log \& \"");
        print(join(" ", @args), "\n\n");
        if ($dryrun == 0) {
            system(join(" ", @args)) == 0
                or die "could start ss $i on $node"
        }
    }
}
