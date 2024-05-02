#!/usr/bin/perl

# mount the scratch drives on nodes n0.o0 - n14.o0
# 
# Usage:  lwfs-mount --nodes="n0.o0:n1.o0:..."

use Getopt::Long; 

my $dryrun=0;
my $nodestr="n5.o0,n6.o0,n7.o0,n8.o0,n9.o0,n10.o0,n11.o0,n12.o0,n13.o0,n14.o0,n15.o0";


if (-e "servers.txt") {
	$nodestr = `cat servers.txt`;
	$nodestr =~ s/#.*\n//g; 
	$nodestr =~ s/\n/,/g; 
	$nodestr =~ s/,+$//g; 
	print ($nodestr, "\n");
}

my @lsi_devs=("\/dev\/scsi\/sdh4-0c0i0l0", "\/dev\/scsi\/sdh5-0c0i0l1");
my @dirs=("\/l1scratch", "\/l2scratch");


# Ge the command-line options
GetOptions("dryrun" => \$dryrun,
        "devs=s" => \$devstr,
        "nodes=s" => \$nodestr);

@lsi_nodes = split(",", $nodestr);


print (join(" ", @lsi_nodes), "\n");


foreach $node (@lsi_nodes) {

	$rack = $node;
	$rack =~ s/n.*\..//;

	$nid = $node;
	$nid =~ s/n//;
	$nid =~ s/\..*//;

	$arch = $node; 
	$arch =~ s/.*\.//;
	$arch =~ s/o.*/o/;
	$arch =~ s/p.*/p/;

    for ($i = 0; $i < 2; $i++) {
        @args = ("sudo ccmd --$arch $rack --node $nid",
                "\"mount -t ext3 $lsi_devs[$i] $dirs[$i];\"");
        print(join(" ", @args), "\n\n");
        if ($dryrun == 0) {
            system(join(" ", @args)) == 0
                or die "could not mount $dir[$i] on $node"
        }
    }
}

foreach $node (@lsi_nodes) {

	$rack = $node;
	$rack =~ s/n.*\..//;

	$nid = $node;
	$nid =~ s/n//;
	$nid =~ s/\..*//;

	$arch = $node; 
	$arch =~ s/.*\.//;
	$arch =~ s/o.*/o/;
	$arch =~ s/p.*/p/;

    for ($i = 0; $i < 2; $i++) {
        @args = ("sudo ccmd --$arch $rack --node $nid",
                "\"if \[ ! -e $dirs[$i] \]; then",
                "mkdir $dirs[$i]\/raoldfi; ",
                "chown raoldfi $dirs[$i]\/raoldfi; fi;\"");
        print(join(" ", @args), "\n");
        if ($dryrun == 0) {
            system(join(" ", @args)) == 0
                or die "could not make $dir[$i]\/raoldfi on $node"
        }
    }
}



#my @ql_devs=("\/dev\/scsi\/sdh0-0c0i0l0", "\/dev\/scsi\/sdh1-0c0i0l1");
#my @ql_nodes = ("12", "13", "14", "15");

#foreach $node (@ql_nodes) {
#    for ($i = 0; $i < 2; $i++) {
#        @args = ("sudo ccmd --0 --node $node",
#                "\"mount -t ext3 $ql_devs[$i] $dirs[$i];\"");
#        print(join(" ", @args), "\n\n");
#    }
#}


