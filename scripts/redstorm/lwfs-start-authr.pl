#!/usr/bin/perl

# mount the scratch drives on nodes n0.o0 - n14.o0

use Getopt::Long; 

my $rsh = "ssh";
my $authr_server = "rsclogin01";

my $dryrun=1;
my $bindir = "\/home\/raoldfi\/software\/lwfs\/snos\/bin";
my $workdir = "\/scratch1\/raoldfi\/servers\/authr";
my $logdir = $workdir;
my $tracedir = $workdir;

# authr options
my $authr_pid=124;

# ss options
my $use_threads=1;
my $num_threads=1; 
my $verbose=2;


GetOptions(
		"srcdir" => \$srcdir,
		"logdir" => \$logdir,
		"use-threads" => \$use_threads,
		"num-threads=i" => \$num_threads,
		"verbose=i" => \$verbose,
		"dryrun" => \$dryrun,
		"authr-cache-caps" => \$authr_cache_caps);



my $min_threads=$num_threads;
my $max_threads=$num_threads;
my $init_threads=$num_threads;
my $high_watermark=10;
my $low_watermark=5;

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
system("mkdir -p $workdir") == 0
    or die "could not create log directory";

system("mkdir -p $logdir") == 0
    or die "could not create log directory";


# Start the authorization server 
@args = ("$rsh $authr_server",
	"$bindir\/lwfs-authr", 
	"--verbose=$verbose", 
	"--authr-db-clear",
	"--logfile=$logdir\/authr.log");
print(join(" ", @args), "\n\n");
if ($dryrun == 0) {
    system(join(" ", @args)) == 0
        or die "could not start authr on $authr_node";
}
