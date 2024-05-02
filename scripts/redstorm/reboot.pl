#!/usr/bin/perl

#!/bin/sh -x

use Getopt::Long; 

my $reboot = "power"; 
my $sleep = "1"; 

# list of servers
my $srvrstr = "n0.o0,n1.o0,n2.o0,n3.o0,n4.o0,n5.o0,n6.o0,n7.o0,n8.o0,"
            . "n9.o0,n10.o0,n11.o0,n12.o0,n13.o0,n14.o0,n15.o0";
my $srvrstr = "n5.o0,n6.o0,n7.o0,n8.o0,"
            . "n9.o0,n10.o0,n11.o0,n12.o0,n13.o0,n14.o0,n15.o0";

# list of clients
my $clntstr = "n0.o1,n1.o1,n2.o1,n3.o1,n4.o1,n5.o1,n6.o1,n7.o1,n8.o1,n9.o1,n10.o1,"
            . "n11.o1,n12.o1,n13.o1,n14.o1,n15.o1,n16.o1,n17.o1,n18.o1,n19.o1,"
            . "n16.o0,n17.o0,n18.o0,n19.o0";


if (-e "servers.txt") {
	$srvrstr = `cat servers.txt`;
	$srvrstr =~ s/#.*\n//g; 
	$srvrstr =~ s/\n/,/g; 
	$srvrstr =~ s/,+$//g; 
}


if (-e "clients.txt") {
	$clntstr = `cat clients.txt`;
	$clntstr =~ s/#.*\n//g; 
	$clntstr =~ s/\n/,/g; 
	$clntstr =~ s/,+$//g; 
}


my $nodestr = $clntstr . "," . $srvrstr;


my $dryrun = 0;

GetOptions("reboot=s" => \$reboot,
        "sleep=i" => \$sleep,
        "dryrun" => \$dryrun,
        "nodes=s" => \$nodestr);

@nodes = split(",",$nodestr);

print (join("\n", @nodes), "\n");


foreach $node (@nodes) {
    @args = ("sudo cboot --$reboot $node");
    print(join(" ", @args), "\n");
	if (not $dryrun) {
		system(join(" ",@args)) == 0
			or die "system @args failed: $?";
	}

    @args = ("sleep $sleep");
	if (not $dryrun) {
		system(join(" ",@args)) == 0
			or die "system @args failed: $?";
	}
}
