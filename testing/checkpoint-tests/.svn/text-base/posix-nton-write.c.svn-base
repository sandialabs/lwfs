#include "mpi.h"
#include "posix-opts.h"
#include "support/logger/logger.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_AIO_TRUE
#include <aio.h>
#endif


#define BUFSIZE 1048576

volatile int pending=0;


static void print_args(
	FILE *fp, 
	struct gengetopt_args_info *args_info,
	const char *prefix) 
{
    time_t rawtime; 
    time(&rawtime); 

    fprintf(fp, "%s %s", prefix, ctime(&rawtime));
    fprintf(fp, "%s ------------ Options -----------\n",prefix);

    fprintf(fp, "%s \tasync = %s\n",prefix ,
	    ((args_info->async_flag)? "true":"false"));
    if (args_info->async_flag) {
	fprintf(fp, "%s \tin-transit = %d\n", prefix,
		args_info->in_transit_arg);
    }
    fprintf(fp, "%s \tnum-trials = %d\n",prefix ,
	    args_info->num_trials_arg);
    fprintf(fp, "%s \tbytes-per-op = %lu\n",prefix ,
	    (unsigned long)args_info->bytes_per_op_arg);
    fprintf(fp, "%s \tops-per-trial = %d\n",prefix ,
	    args_info->ops_per_trial_arg);
    fprintf(fp, "%s \tnum-ost = %d\n",prefix ,
	    args_info->num_ost_arg);
    fprintf(fp, "%s \tresult-file = %s\n",prefix ,
	    ((args_info->result_file_arg)? args_info->result_file_arg: "stdout"
	    ));
    fprintf(fp, "%s \tresult-file-mode = %s\n",prefix ,
	    args_info->result_file_mode_arg);
    fprintf(fp, "%s \tscratch-file = %s\n",prefix ,
	    args_info->scratch_file_arg);
    fprintf(fp, "%s \n", prefix);

    fprintf(fp, "%s-----------------------------------\n",prefix);
}


#ifdef HAVE_AIO_TRUE

/**
 * @brief Signal handler for write requests. 
 */
void write_action(int signo, siginfo_t *info, void *context)
{
    struct aiocb *aiocb = (struct aiocb *)info->si_value.sival_ptr; 
    long int bytes_written;

    fprintf(stderr, "Entered write_action\n");
    fprintf(stderr, "  signo = %d\n", signo);
    fprintf(stderr, "  si_code = %d\n", info->si_code);
    fprintf(stderr, "  si_value.sival_ptr = %lx hex\n", 
            (unsigned long)aiocb);

    /* wait for the request to complete */
    while (aio_error(aiocb) == EINPROGRESS);

    /* get the result of the function */
    bytes_written = aio_return(aiocb);

    /* free the aiocb structure */
    free(aiocb); 
    pending--;
    fflush(stderr); 
}

static void write_callback(sigval_t s)
{
	struct aiocb *aiocb = (struct aiocb *)s.sival_ptr; 
    long int bytes_written;

    fprintf(stderr, "Entered write_callback\n");
    fprintf(stderr, "  s.sival_ptr = %lx hex\n", (unsigned long)aiocb);

    /* wait for the request to complete */
    while (aio_error(aiocb) == EINPROGRESS);

    /* get the result of the function */
    bytes_written = aio_return(aiocb);

    /* free the aiocb structure */
    free(aiocb); 
    pending--;
    fflush(stderr); 
}
#endif



/**
  * @brief Each process writes a buffer to a separate file.
  *
  * This is supposed to simulate a checkpoint operation.
  */
static void nton_write(
	MPI_Comm comm, 
	const char *fname,
	const ssize_t bufsize, 
	const int num_ops, 
	const int remove_file,
	double *t_total,
	double *t_open)
{
    int myrank, i;
    double t_start; 
    char filename[256]; 
    char *buf = NULL;
    int fd; 

    MPI_Comm_rank(comm, &myrank);

    buf = (char *)malloc(sizeof(*buf)*bufsize);
    if (!buf) {
        fprintf(stderr, "unable to allocate buffer\n");
        MPI_Abort(MPI_COMM_WORLD, -1); 
    }

    for (i=0; i<bufsize; i++) {
        buf[i] = myrank*bufsize + i; 
    }
    sprintf(filename, "%s-%04d", fname, myrank); 

    /* if the file exists, delete it */
    if (access(filename, W_OK) == 0) {
        remove(filename);
    }

    /* Time the open command */
    MPI_Barrier(comm); 
    t_start = MPI_Wtime(); 
    if ((fd = open(filename, O_WRONLY | O_CREAT , 0666)) == -1) {
        fprintf(stderr, "%d: could not open file: %s\n",
                myrank, strerror(errno));
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
    *t_open = MPI_Wtime()-t_start; 

    /* Time the write commands */
    for (i=0; i<num_ops; i++) {
        ssize_t bytes_written; 
        bytes_written = write(fd, buf, bufsize);
        if (bytes_written != bufsize) {
            fprintf(stderr, "%d: unable to write buffer\n", myrank);
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
    }
    if (fsync(fd) != 0) {
        fprintf(stderr, "%d: unable to sync file\n", myrank);
        MPI_Abort(MPI_COMM_WORLD, -1);
    }


    /* time the close operation */
    if (close(fd) != 0) {
        fprintf(stderr, "%d: unable to close file\n", myrank);
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    *t_total = MPI_Wtime() - t_start; 

    if (remove_file) {
        remove(filename); 
    }

    free(buf); 

    return;
}

#ifdef HAVE_AIO_TRUE

static void nton_aio_write(
	MPI_Comm comm, 
	const char *fname,
	const ssize_t bufsize, 
	const int num_ops, 
	const int remove_file,
	double *t_total,
	double *t_open)
{
    int myrank, i;
    double t_start; 
    char filename[256]; 
    char *buf = NULL;
    int fd; 

    MPI_Comm_rank(comm, &myrank);

    buf = (char *)malloc(sizeof(*buf)*bufsize);
    if (!buf) {
        fprintf(stderr, "unable to allocate buffer\n");
        MPI_Abort(MPI_COMM_WORLD, -1); 
    }

    for (i=0; i<bufsize; i++) {
        buf[i] = myrank*bufsize + i; 
    }
    sprintf(filename, "%s-%04d", fname, myrank); 

    /* if the file exists, delete it */
    if (access(filename, W_OK) == 0) {
        remove(filename);
    }

    /* Time the open command */
    MPI_Barrier(comm); 
    t_start = MPI_Wtime(); 
    if ((fd = open(filename, O_WRONLY | O_CREAT | O_DIRECT , 0666)) == -1) {
        fprintf(stderr, "%d: could not open file: %s\n",
                myrank, strerror(errno));
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
    *t_open = MPI_Wtime()-t_start; 

    /* Time the write commands */
    {
        int rc; 
        struct aiocb sync_aiocb; 
        struct aiocb *wait_list[num_ops];
        int total_bytes = 0;

        for (i=0; i<num_ops; i++) {
            wait_list[i] = NULL;

            /* this gets freed in the signal handler */
            struct aiocb *aiocb = malloc(sizeof(struct aiocb));

            memset(aiocb, 0, sizeof(struct aiocb)); 

            /* set the signal handler for the request */

			/* if we want to use signals, uncomment the following */
            //aiocb->aio_sigevent.sigev_notify = SIGEV_SIGNAL;
            //aiocb->aio_sigevent.sigev_signo = SIGUSR2;  /* completion */

			/* if we want a thread to spawn on completion, do the following */
            aiocb->aio_sigevent.sigev_notify = SIGEV_THREAD;
            aiocb->aio_sigevent.sigev_notify_function = write_callback;

            aiocb->aio_sigevent.sigev_value.sival_ptr = aiocb; 

            /* set parameters for the io_req */
            aiocb->aio_offset = i*bufsize;
            aiocb->aio_fildes = fd;
            aiocb->aio_buf = buf; 
            aiocb->aio_nbytes = bufsize; 

            rc = aio_write(aiocb);
            if (rc != 0) {
                fprintf(stderr, "%d: unable to schedule write\n", myrank);
                MPI_Abort(MPI_COMM_WORLD, -1);
            }
            wait_list[i] = aiocb; 
            pending++;
        }

        /* wait for each io op complete */
		/*
        {
            int bytes_read = 0;

            for (i=0; i<num_ops; i++) {
				int rc; 

                while ((rc = aio_error(wait_list[i]))== EINPROGRESS);

				if (rc != 0) {
					fprintf(stderr, "%d: error with write request: %s\n",
						myrank, strerror(rc));
					MPI_Abort(MPI_COMM_WORLD, -1);
				}

                bytes_read = aio_return(wait_list[i]); 

                if (bytes_read == -1) {
                    fprintf(stderr, "%d: error with write request: %s\n", 
                            myrank, strerror(errno));
                    MPI_Abort(MPI_COMM_WORLD, -1);
                }

                total_bytes += bytes_read; 
            }
        }
		*/

        memset(&sync_aiocb, 0, sizeof(struct aiocb));
        sync_aiocb.aio_fildes = fd;
        if (aio_fsync(O_DSYNC, &sync_aiocb) != 0) {
            fprintf(stderr, "%d: unable to schedule fsync file\n", myrank);
            MPI_Abort(MPI_COMM_WORLD, -1);
        }

        /* wait for sync to complete */
        while (aio_error(&sync_aiocb) == EINPROGRESS);

        if (aio_return(&sync_aiocb) != 0) { 
            fprintf(stderr, "%d: fsync returned error: %s\n", 
                    myrank, strerror(errno));
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
        fprintf(stderr, "FSYNC COMPLETED, pending=%d, bytes_read=%d\n", pending, (int)total_bytes);
    }

    /* time the close operation */
    if (close(fd) != 0) {
        fprintf(stderr, "%d: unable to close file\n", myrank);
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    *t_total = MPI_Wtime() - t_start; 

    if (remove_file) {
        remove(filename); 
    }

    free(buf); 

    return;
}

#endif


/* include the source code for printing statistics */
#include "print-stats.c"


int main(int argc, char *argv[])
{
    /* local variables */
    char hostname[256];
    int myrank = 0; 
    int i; 
    struct gengetopt_args_info args_info; 
    FILE *result_fp; 


#ifdef HAVE_AIO_TRUE
    /*^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
    struct sigaction sig_act; 

    sigemptyset(&sig_act.sa_mask);  /* block only current signal */

    /*  If the SA_SIGINFO flag is set in the sa_flags field then
     *  the sa_sigaction field of sig_act structure specifies the
     *  signal catching function:
     */
    sig_act.sa_flags = SA_SIGINFO;
    sig_act.sa_sigaction = write_action;

    /*  If the SA_SIGINFO flag is NOT set in the sa_flags field
     *  then the the sa_handler field of sig_act structure specifies
     *  the signal catching function, and the signal handler will be
     *  invoked with 3 arguments instead of 1:
     *     sig_act.sa_flags = 0;
     *     sig_act.sa_handler = sig_handler;
     */

    /* * * *  Estab. signal handler for SIGUSR1 signal * * * */

    printf("Establish Signal Handler for SIGUSR2\n");
    if (sigaction (SIGUSR2, /* Set action for SIGUSR2       */
		&sig_act,                 /* Action to take on signal     */
		0))                       /* Don't care about old actions */
	perror("sigaction");
    /*^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/
#endif


    /* Get the command-line options (do this from all nodes?) */
    if (cmdline_parser(argc, argv, &args_info) != 0)
	exit(1); 

    /* Initialize MPI */
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);



    gethostname(hostname, sizeof(hostname));
    fprintf(stderr, "%d:%s starting...\n", myrank, hostname);

    MPI_Barrier(MPI_COMM_WORLD);

    if ((myrank == 0) && (args_info.result_file_arg!=NULL)) {
	result_fp = fopen(args_info.result_file_arg, args_info.result_file_mode_arg);
	if (!result_fp) {
	    fprintf(stderr, "could not open result file \"%s\"\n", 
		    args_info.result_file_arg);
	    MPI_Abort(MPI_COMM_WORLD, -1);
	}
    }
    else {
	result_fp = stdout;
    }

    if (myrank == 0) {
	print_args(result_fp, &args_info, "%");
    }

    MPI_Barrier(MPI_COMM_WORLD);

    for (i=0; i<args_info.num_trials_arg; i++) {
	double t_total;
	double t_open;

	if (myrank == 0) {
	    fprintf(stderr, "trial %d\n", i);
	}

	/* async test? */
	if (args_info.async_flag) {
#ifdef HAVE_AIO_TRUE
	    nton_aio_write(MPI_COMM_WORLD, args.scratch_file, 
		    args.bytes_per_op, args.ops_per_trial,
		    args.remove_file, &t_total, &t_open);
#else
	    fprintf(stderr, "async not supported\n");
	    MPI_Abort(MPI_COMM_WORLD, -1);
#endif
	}

	else {
	    /* stdio write */
	    nton_write(MPI_COMM_WORLD, args_info.scratch_file_arg, 
		    args_info.bytes_per_op_arg, args_info.ops_per_trial_arg,
		    args_info.remove_file_flag, &t_total, &t_open);
	}

	/* Gather and print stats (collective call) */
	print_stats(result_fp, &args_info, t_total, t_open);
	if (myrank == 0) { 
	    fflush(result_fp);
	}
    }

    /* print statistics */
    if (myrank == 0) {
	fclose(result_fp);
    }

    MPI_Finalize();

    return 0; 
}
