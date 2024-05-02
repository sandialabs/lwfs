#!/usr/bin/perl


# gather hostnames
@hosts = `mpiexec -np 35 hostname`;

# remove the end of line from the hostnames
foreach $host (@hosts) {
	$host =~ s/\n//g;
	$host =~ s/n(.\.p0)/n0\1/g;
}

# sort the array of hostnames
@clients = sort(@hosts); 

foreach $client (@clients) {
	$client =~ s/n0/n/g;
}

print(join("\n", @clients));

