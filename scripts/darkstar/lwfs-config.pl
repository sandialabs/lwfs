#!/usr/bin/perl

#!/bin/sh -x

use Getopt::Long; 

# list of servers
my $srvrstr = "n0.o0,n1.o0,n2.o0,n3.o0,n4.o0,n5.o0,n6.o0,n7.o0,n8.o0,"
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
	print ($srvrstr, "\n");
}


if (-e "clients.txt") {
	$clntstr = `cat clients.txt`;
	$clntstr =~ s/#.*\n//g; 
	$clntstr =~ s/\n/,/g; 
	$clntstr =~ s/,+$//g; 
	print ($clntstr, "\n");
}


# this gives you everything but the LSI kernel modules
my $sysarch="suse-9.1-x86_64";
my $vmname="gm-2.1.2-x86_64-2.6.5-7.111";
my $image="gm-2.1.2-x86_64-2.6.5-7.111/vmlinuz-2.6.5-7.111-x86_64";

# this configuration works... after Ruth fixed a few things.
my $sysarch="suse-9.1-x86_64";
my $vmname="lustre-1.2.8-suse9.1";
my $image="gm-2.1.2-x86_64-2.6.5-7.111/vmlinuz-2.6.5-7.111-x86_64";

# this configuration is better, it also has mpich-gm
my $sysarch="suse-9.1-x86_64";
my $vmname="raoldfi-x86_64";
my $image="gm-2.1.2-x86_64-2.6.5-7.111/vmlinuz-2.6.5-7.111-x86_64";

# this configuration is better, it also has mpich-gm
#my $sysarch="suse-9.1-x86_64";
#my $vmname="rk-x86_64";
# This needs to be a link in tftpboot 
#my $image="rk-x86_64/vmlinuz-2.6.11.4-21.10-x86_64";



$nodes = $clntstr . "," . $srvrstr; 

my $dryrun = 0;

GetOptions(
	   "sysarch=s" => \$sysarch,
	   "vmname=s" => \$vmname,
	   "image=s" => \$image,
	   "dryrun" => \$dryrun,
	   "nodes=s" => \$nodes);

# all nodes are configured the same
@nodes = split(",",$nodes);


print (join("\n", @nodes),"\n");

# Set the attributes 
@args = ("attr_mgr","--set", 
        "--image $image", 
        "--vmname $vmname", 
        "--sysarch $sysarch", @nodes);

print(join(" ", @args), "\n");
if ($dryrun == 0) {
    $res = system(join(" ",@args)); 

    print("result = $res\n");
}


@args = ("sudo", "mk_conf", "--local", "--dhcp", @nodes);
print ("\n", join(" ", @args), "\n"); 
if ($dryrun == 0) {
    system(join(" ",@args)) == 0
        or die "system @args failed: $?"
}


@args = ("sudo", "mk_conf", "--pxe", @nodes);
print ("\n", join(" ", @args), "\n"); 
if ($dryrun == 0) {
    system(join(" ",@args)) == 0
        or die "system @args failed: $?"
}

@args = ("sudo", "build_diskless", "--force", @nodes);
print ("\n", join(" ", @args), "\n"); 
if ($dryrun == 0) {
    system(join(" ",@args)) == 0
        or die "system @args failed: $?"
}

