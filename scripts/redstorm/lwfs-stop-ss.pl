#!/usr/bin/perl

# mount the scratch drives on nodes n0.o0 - n14.o0

use Getopt::Long; 

my $dryrun=0;
my $ss_nodetype = "o";
my $ss_rack = 0;
my $ss_nodestr = "0,1,3,4,5,6,7,8,9,10,11,12,13,15";


GetOptions(
       "rack=s" => \$ss_rack,
       "type=s" => \$ss_nodetype,
       "nodes=s" => \$ss_nodestr,
	   "dryrun" => \$dryrun);


&kill_ss($ss_nodetype,$ss_rack,$ss_nodestr);





sub kill_ss {
	$nodetype = $_[0];
	$rack = $_[1];
	$nodelist = $_[2];

	@nodes = split(",",$nodelist);


	@args = ("ccmd --$nodetype $rack --node $nodelist",
			"\"", 
			"pkill -SIGINT \'lwfs-ss\';",
			"\"");
	print(join(" ", @args), "\n\n");
	if ($dryrun == 0) {
		system(join(" ", @args)) == 0
			or die "could not stop authr on $authr_node";
	}
}

