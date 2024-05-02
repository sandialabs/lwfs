#!/usr/bin/perl
#PBS -l nodes=2:ppn=1:pentium
#PBS -l walltime=2:0:0
#PBS -j oe

#cd $PBS_O_WORKDIR;

$pwd = $ENV{"PBS_O_WORKDIR"};

print ("############# START ###############\n"); 
print (`date`, `whoami`, "\n");
print (`pwd`);


@args = ("cd $pwd; run-experiments.pl");

print(join(" ", @args), "\n");
system(@args) == 0
	or die "system @args failed: $?"; 


print ("############# FINISHED! ###########\n"); 
print (`date`, "\n");

