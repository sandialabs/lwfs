#!/usr/bin/perl

# mount the scratch drives on nodes n0.o0 - n14.o0

use Getopt::Long; 

my $dryrun=0;
my $authr_nodestr = "n10.o0";


GetOptions(
       "node=s" => \$authr_nodestr,
	   "dryrun" => \$dryrun);


print("killing lwfs-authr on node $authr_nodestr\n");


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


# Stop the authorization server 
@args = ("ccmd --$authr_nodetype $authr_racknum --node $authr_nodenum",
        "\"", 
		"pkill -SIGINT \'lwfs-authr\';",
		"\"");
print(join(" ", @args), "\n\n");
if ($dryrun == 0) {
    system(join(" ", @args)) == 0
        or die "could not stop authr on $authr_node";
}

@args = ("ccmd --$authr_nodetype $authr_racknum --node $authr_nodenum",
        "\"", 
        "pkill -SIGKILL \'lwfs-authr\';",
		"\"");
print(join(" ", @args), "\n\n");
if ($dryrun == 0) {
    system(join(" ", @args)) == 0
        or die "could not stop authr on $authr_node";
}

