#!/usr/bin/perl
#PBS -l nodes=2:ppn=1:pentium
#PBS -l walltime=2:0:0
#PBS -j oe

#cd $PBS_O_WORKDIR;

use Getopt::Long; 


$mpirun = "mpiexec"; 

$hostnames = "localhost,remotehost"; 
GetOptions("hosts=s" => \$hostnames);

@hosts = split(",", $hostnames); 

print(join("-", @hosts), "\n"); 

$pwd = $ENV{"PBS_O_WORKDIR"};
`cd $pwd`;

print ("############# START ###############\n"); 
print (`date`, `whoami`, "\n");
print (`pwd`);


# gather hostnames
@hosts = `$mpirun -np 2 hostname`;

# remove the end of line from the hostnames
foreach $host (@hosts) {
	$host =~ s/\n//g;
}

$hostnames = join(",", @hosts); 


# get the nid of the server 
$nid = `rsh $hosts[0] utcp_nid myri0`; 
$nid =~ s/\n//g;


@args = ("cd $pwd; mpiexec -np 2 run-experiments.pl --server-nid=$nid --hosts=$hostnames");
print(join(" ", @args), "\n");
system(@args) == 0
	or die "system @args failed: $?"; 

print ("############# FINISHED! ###########\n"); 
print (`date`, "\n");

