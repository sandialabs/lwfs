#!/usr/bin/perl

my $mpirun = "mpiexec";
my $prog = "ss-perf";

use Getopt::Long; 

my $num_pts = 23;

my $authr_nid = 167838780;
my $authr_pid = 124;

my $server_nid = 0;
GetOptions("authr-nid=i" => \$authr_nid,
	   "authr-pid=i" => \$authr_pid,
	   "num-pts=i" => \$num_pts);

my $num_ss = 4; 
@ss_nid =(167838759, 167838758, 167838757, 167838756);
@ss_pid =(122, 122, 122, 122);

@nodes = (8, 12, 16, 20);
@outfiles = ("lwfs-ss-write.out",
		"lwfs-ss-read.out");

@types = (1, 2);  # test write and read

foreach $type (@types) {

	#`rm $outfiles[$type-1]`; 

	foreach $node (@nodes) {

		for ($i=0; $i<$num_pts; $i++) {
			$x[$i] = 2**$i;

			for ($j=1; $j<=$num_ss; $j++) {
				@args = ("$mpirun", "-np","$node", "$prog", 
						"--verbose=2", 
						"--num-ops=1", 
						"--bufsize=$x[$i]",
						"--count=10",
						"--type=$type",
						"--result-file=$outfiles[$type-1]",
						"--result-file-mode=a",
						"--authr-nid=$authr_nid",
						"--authr-pid=$authr_pid",
						"--num-ss=$j",
						"--ss-nid-0=$ss_nid[0]",
						"--ss-pid-0=$ss_pid[0]",
						"--ss-nid-1=$ss_nid[1]",
						"--ss-pid-1=$ss_pid[1]",
						"--ss-nid-2=$ss_nid[2]",
						"--ss-pid-2=$ss_pid[2]",
						"--ss-nid-3=$ss_nid[3]",
						"--ss-pid-3=$ss_pid[3]");

				print ("\n", join(" ", @args), "\n"); 
				#system(@args) == 0
				#	or die "system @args failed: $?"
			}
		}
	}
}

