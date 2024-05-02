#include <stdio.h>
#include <mpi.h>

int main(int argc, char *argv[])
{
	char name[100];
	int length;
	int size, rank; 

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size); 
	MPI_Comm_rank(MPI_COMM_WORLD, &rank); 

	MPI_Get_processor_name(name, &length);
	printf("%03d:%s\n", rank, name);
	MPI_Finalize();

	return 0;
}
