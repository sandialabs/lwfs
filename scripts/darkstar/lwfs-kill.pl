#!/usr/bin/perl

# mount the scratch drives on nodes n0.o0 - n14.o0

use Getopt::Long; 

my $dryrun=0;
my $iface = $ENV{"PTL_IFACE"};
my $src = "\/home\/raoldfi\/research\/sandia\/scalable-io\/lwfs\/src";
my $log = "\/home\/raoldfi\/log";
my $signal = "SIGKILL";

# authr options
my $authr_rack = 0;
my $authr_node = 15;
my $authr_node = 13;
my $authr_pid=124;
my $authr_nid=`rsh n$authr_node.o$authr_rack \"utcp_nid $iface\"`;
$authr_nid =~ s/Running.*\n//g;
$authr_nid =~ s/\n//g;

# ss options
my $use_threads=0;
my $min_threads=4;
my $max_threads=4;
my $init_threads=4;
my $high_watermark=10;
my $low_watermark=1;

my $ss_rack = 0;
my $ss_nodestr = "5:6:7:8:9:10:11:12:13";
my @ss_pids = (122, 123);
my @ss_root = ("\/l1scratch\/raoldfi\/ss-root",
        "\/l2scratch\/raoldfi\/ss-root");
my $ss_simdisk=0;



GetOptions(
       "signal=s" => \$signal,
       "authr=s" => \$authr_node,
       "ss-nodes=s" => \$ss_nodestr,
	   "dryrun" => \$dryrun);

@ss_nodes = split(":", $ss_nodestr);

print("killing lwfs-ss on nodes \"", join(" ", @ss_nodes), "\"\n\n");

# Kill the authorization server 
@args = ("ccmd --o $authr_rack --node $authr_node",
        "\"pkill -$signal \"lwfs-authr\"\"");
print(join(" ", @args), "\n\n");
if ($dryrun == 0) {
    system(join(" ", @args)) == 0
        or die "could not start authr on $authr_node"
}

foreach $node (@ss_nodes) {

    @args = ("ccmd --o $ss_rack --node $node",
        "\"pkill -$signal \"lwfs-ss\"\"");
    for ($i=0; $i<2; $i++) {
        print(join(" ", @args), "\n\n");
        if ($dryrun == 0) {
            system(join(" ", @args)) == 0
                or die "could start ss $i on $node"
        }
    }
}
