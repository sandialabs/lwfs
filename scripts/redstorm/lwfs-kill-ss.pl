#!/usr/bin/perl

# mount the scratch drives on nodes n0.o0 - n14.o0

use Getopt::Long; 

my $dryrun=0;
my $nodetype = "o";
my $rack = 0;
my $nodestr = "0,1,3,4,5,6,7,8,9,10,11,12,13,15";

if (-e "servers.txt") {
	$nodestr = `cat servers.txt`;
	$nodestr =~ s/#.*\n//g; 
	$nodestr =~ s/\n/,/g; 
	$nodestr =~ s/,+$//g; 
	print ($nodestr, "\n");
}


GetOptions(
       "rack=s" => \$rack,
       "type=s" => \$nodetype,
       "nodes=s" => \$nodestr,
	   "dryrun" => \$dryrun);

@nodes = split(",",$nodestr);

foreach $node (@nodes) {
	$node =~ s/n//g;
	$node =~ s/\..*//g;
}

&kill_ss($nodetype,$rack,join(",",@nodes));





sub kill_ss {
	$nodetype = $_[0];
	$rack = $_[1];
	$nodelist = $_[2];

	@nodes = split(",",$nodelist);


	@args = ("ccmd --$nodetype $rack --node $nodelist",
			"\"", 
			"pkill -SIGKILL \'lwfs-ss\';",
			"\"");
	print(join(" ", @args), "\n\n");
	if ($dryrun == 0) {
		system(join(" ", @args)) == 0
			or die "could not stop authr on $authr_node";
	}
}

