#!/usr/bin/perl

# mount the scratch drives on nodes n0.o0 - n14.o0

use Getopt::Long; 

my $dryrun=0;
my $iface = $ENV{"PTL_IFACE"};
my $srcdir = "\/home\/raoldfi\/research\/sandia\/scalable-io\/lwfs\/trunk\/src";
my $logdir = "\/home\/raoldfi\/log";
my $tracedir = "\/home\/raoldfi\/trace";

# authr options
my $authr_cache_caps = 0;
my $authr_nodestr = "n10.o0";
my $authr_pid=124;

# ss options
my $use_threads=1;
my $num_threads=1; 
my $verbose=2;


GetOptions(
		"srcdir" => \$srcdir,
		"logdir" => \$logdir,
		"tracedir" => \$tracedir,
		"use-threads" => \$use_threads,
		"num-threads=i" => \$num_threads,
		"verbose=i" => \$verbose,
		"dryrun" => \$dryrun,
		"authr-node=s" => \$authr_nodestr,
		"authr-cache-caps" => \$authr_cache_caps);


# get the Portals node ID
my $authr_nid=`rsh $authr_nodestr \"utcp_nid $iface\"`;
$authr_nid =~ s/Running.*\n//g;
$authr_nid =~ s/\n//g;

my $min_threads=$num_threads;
my $max_threads=$num_threads;
my $init_threads=$num_threads;
my $high_watermark=10;
my $low_watermark=5;

@ss_nodes = split(",", $ss_nodestr);

# Darkstar nodes encode the node number, node type, and rack number
# in a string n[$nodenum].[$type][$rack]

# extract the node number
$authr_nodenum = $authr_nodestr;
$authr_nodenum =~ s/n//g;      # remove the first n
$authr_nodenum =~ s/\..*//g;  # remove the .*


# extract the node type
$authr_nodetype = $authr_nodestr;
$authr_nodetype =~ s/n//g;        # remove n
$authr_nodetype =~ s/.*\.o.*/o/g; # replace *.o* with o 
$authr_nodetype =~ s/.*\.p.*/p/g; # replace *.p* with p


# extract the rack number
$authr_racknum = $authr_nodestr;
$authr_racknum =~ s/n.*\..//g;      # remove the first n


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

system("mkdir -p $tracedir") == 0
    or die "could not create trace directory";


# Start the authorization server 
@args = ("ccmd --$authr_nodetype $authr_racknum --node $authr_nodenum", "\"",
        "$srcdir\/authr\/lwfs-authr",
		"--verbose=$verbose",
		"--authr-db-clear",
        "--authr-trace",
		"--authr-trace-file=$tracedir/authr-$authr_nodestr.sddf",
        "\&\> $logdir\/authr-$authr_nodestr.log",
		"\& \"");
print(join(" ", @args), "\n\n");
if ($dryrun == 0) {
    system(join(" ", @args)) == 0
        or die "could not start authr on $authr_node";
}
