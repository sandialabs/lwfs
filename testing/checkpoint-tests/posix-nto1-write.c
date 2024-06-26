#include "mpi.h"
#include "support/logger/logger.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <stdio.h>
#include <argp.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>


#define BUFSIZE 1048576

/* ----------------- COMMAND-LINE OPTIONS --------------- */

const char *argp_program_version = "$Revision: 1073 $"; 
const char *argp_program_bug_address = "<raoldfi@sandia.gov>"; 

static char doc[] = "Test MPI-I/O writing";
static char args_doc[] = ""; 

/**
 * @brief Command line arguments for testing the storage service. 
 */
struct arguments {

    /** @brief Debug level to use. */
    log_level debug_level; 

    /** @brief The type of experiment. */
    int type; 

    /** @brief The number of trials per experiment. */
    int num_trials; 

    /** @brief The number of operations in a trial. */
    int ops_per_trial; 

    /** @brief the size of the input/output buffer to use for each op. */
    ssize_t bytes_per_op; 

    /** @brief Where to output results. */
    char *result_file; 

    /** @brief Mode to use when opening file. */
    char *result_file_mode; 

    /** @brief Where to output results. */
    char *scratch_file; 

    /** @brief Remove the file after the experiment */
    int remove_file; 

    /** @brief Number of storage targets */
    int num_ss; 

}; 

static void print_args(FILE *fp, struct arguments *args) 
{
    fprintf(fp, "%s------------  ARGUMENTS -----------\n","%");
    fprintf(fp, "%s\ttype = %d\n","%", args->type);
    fprintf(fp, "%s\tverbose = %d\n","%", args->debug_level);
    fprintf(fp, "%s\tnum-ss = %d\n","%", args->num_ss);
    fprintf(fp, "%s\tbytes-per-op = %lu\n","%", (unsigned long)args->bytes_per_op);
    fprintf(fp, "%s\tnum-trials = %d\n","%", args->num_trials);
    fprintf(fp, "%s\tops-per-trial = %d\n","%", args->ops_per_trial);
    fprintf(fp, "%s\tresult-file = %s\n","%", ((args->result_file)? args->result_file: "stdout"));
    fprintf(fp, "%s\tresult-file-mode = %s\n","%", args->result_file_mode);
    fprintf(fp, "%s\tscratch-file = %s\n","%", args->scratch_file);
    fprintf(fp, "%s\tremove-file = %s\n","%", ((args->remove_file)? "true":"false"));
    fprintf(fp, "%s-----------------------------------\n","%");
}


static struct argp_option options[] = {
    {"verbose",    1, "<0=none,...,6=all>", 0, "Produce verbose output"},
    {"bytes-per-op",    2, "<val>", 0, "Size of buffer to use per operation"},
    {"num-trials", 3, "<val>", 0, "Number of trials to run"},
    {"ops-per-trial",    4, "<val>", 0, "Number of ops per trial"},
    {"result-file",5, "<FILE>", 0, "Results file"},
    {"result-file-mode",6, "<val>", 0, "Results file mode"},
    {"scratch-file",7, "<val>", 0, "Scratch file"},
    {"type",8, "<val>", 0, "Type of experiment"},
    {"remove-file", 9, 0, 0, "Remove the file after the experiment"},
    {"num-ss",10, "<val>", 0, "Number of storage targets"},
    { 0 }
};

/** 
 * @brief Parse a command-line argument. 
 * 
 * This function is called by the argp library. 
 */
static error_t parse_opt(
        int key, 
        char *arg, 
        struct argp_state *state)
{
    /* get the input arguments from argp_parse, which points to 
     * our arguments structure */
    struct arguments *arguments = state->input; 

    switch (key) {

        case 1: /* verbose */
            arguments->debug_level= atoi(arg);
            break;

        case 2: /* bytes_per_op */
            arguments->bytes_per_op = atol(arg);
            break;

        case 3: /* num_trials */
            arguments->num_trials= atoi(arg);
            break;

        case 4: /* ops_per_trial */
            arguments->ops_per_trial= atoi(arg);
            break;

        case 5: /* result_file */
            arguments->result_file = arg;
            break;

        case 6: /* result_file_mode */
            arguments->result_file_mode = arg;
            break;

        case 7: /* scratch_file */
            arguments->scratch_file = arg;
            break;

        case 8: /* type */
            arguments->type = atoi(arg);
            break;

        case 9: /* remove-file */
            arguments->remove_file = TRUE;
            break;

        case 10: /* num-ss */
            arguments->num_ss = atoi(arg);
            break;


        case ARGP_KEY_ARG:
            /* we don't expect any arguments */
            if (state->arg_num >= 0) {
                argp_usage(state);
            }
            // arguments->args[state->arg_num] = arg; 
            break; 

        case ARGP_KEY_END:
            if (state->arg_num < 0)
                argp_usage(state);
            break;

        default:
            return ARGP_ERR_UNKNOWN; 
    }

    return 0;
}

static struct argp argp = {options, parse_opt, args_doc, doc}; 



static void nto1_write(
        MPI_Comm comm, 
        const char *fname,
        const ssize_t bufsize, 
        const int num_ops, 
        const int remove_file,
        double *t_total)
{
    int myrank, i;
    double t_start;
    char filename[256]; 
    char *buf = NULL;
    int fd; 
    off_t offset = 0; 

    MPI_Comm_rank(comm, &myrank);

    buf = (char *)malloc(sizeof(*buf)*bufsize);
    if (!buf) {
        fprintf(stderr, "unable to allocate buffer\n");
        MPI_Abort(MPI_COMM_WORLD, -1); 
    }

    for (i=0; i<bufsize; i++) {
        buf[i] = myrank*bufsize + i; 
    }
    sprintf(filename, "%s", fname); 

    /* remove the file if it already exists */
    if (myrank == 0) {
        if (access(fname, W_OK) == 0) {
            remove(fname);
        }
    }

    /* Time the open command */
    MPI_Barrier(comm); 
    t_start = MPI_Wtime(); 
    if (myrank == 0) {
        /* create the file on node 0 */
        if ((fd = open(filename, O_WRONLY | O_CREAT, 0666)) == -1) {
            fprintf(stderr, "%d: could not open file: %s\n",
                    myrank, strerror(errno));
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
        MPI_Barrier(comm); 
    }
    else {
        /* have to wait for create to complete before we open */
        MPI_Barrier(comm); 
        if ((fd = open(filename, O_WRONLY)) == -1) {
            fprintf(stderr, "%d: could not open file: %s\n",
                    myrank, strerror(errno));
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
    }


    /* write the data */
    offset = num_ops*bufsize*myrank; 
    if (lseek(fd, offset, SEEK_SET) == -1) {
        fprintf(stderr, "%d: unable to seek to offset %lu: %s\n", myrank, offset, strerror(errno));
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

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


void print_stats(
        FILE *result_fp, 
        struct arguments *args, 
        double t_total)
{
    int np = 1; 
    int myrank = 0;

    double t_total_min, t_total_max, t_total_avg;

    MPI_Comm_size(MPI_COMM_WORLD, &np); 
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank); 

    /* gather stats across participating nodes */
    MPI_Reduce(&t_total, &t_total_min, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
    MPI_Reduce(&t_total, &t_total_max, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&t_total, &t_total_avg, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    if (myrank == 0) {
        static int first = 1; 

        t_total_avg = t_total_avg/np; 

        /* calculate the aggregate megabytes per operation */
        double mb = (double)(args->bytes_per_op*np)/(1024*1024); 

        /* print the header */
        if (first) {
            fprintf(result_fp, "%s ----------------------------------------------------\n", "%");
            fprintf(result_fp, "%s column   description\n", "%");
            fprintf(result_fp, "%s ----------------------------------------------------\n", "%");
            fprintf(result_fp, "%s   0     number of clients (np)\n", "%"); 
            fprintf(result_fp, "%s   1     number of storage targets (num_ss)\n","%"); 
            fprintf(result_fp, "%s   2     number of operations (ops_per_trial)\n","%"); 
            fprintf(result_fp, "%s   3     aggregate MB/operation (MB_per_op) = np*bytes_per_op/(1024*1024)\n","%"); 
            fprintf(result_fp, "%s   4     aggregate MB/trial  (MB_per_trial) = MB_per_op*num_ops\n","%"); 
            fprintf(result_fp, "%s   5     aggregate throughput  (MB_per_sec) = MB_per_trial/total_max\n","%"); 
            fprintf(result_fp, "%s   6     total_avg (s)\n","%"); 
            fprintf(result_fp, "%s   7     total min (s)\n","%"); 
            fprintf(result_fp, "%s   8     total_max (s)\n","%"); 
            fprintf(result_fp, "%s ----------------------------------------------------\n", "%");

            first = 0; 
        }

        /* print the row */
        fprintf(result_fp, "%05d   ", np);
        fprintf(result_fp, "%04d   ", args->num_ss);
        fprintf(result_fp, "%03d   ", args->ops_per_trial);  
        fprintf(result_fp, "%1.6e  ", mb);    
        fprintf(result_fp, "%1.6e  ", mb*args->ops_per_trial);    
        fprintf(result_fp, "%1.6e  ", (mb*args->ops_per_trial)/t_total_max);    
        fprintf(result_fp, "%1.6e  ", t_total_avg);
        fprintf(result_fp, "%1.6e  ", t_total_min);
        fprintf(result_fp, "%1.6e\n", t_total_max);

        fprintf(result_fp, "%s ----------------------------------------------------\n", "%");
    }

}


int main(int argc, char *argv[])
{
    /* local variables */
    char hostname[256];
    int myrank = 0; 
    int i; 
    struct arguments args; 
    FILE *result_fp = NULL;

    /* default command-line arguments */
    args.type = 0; 
    args.debug_level = LOG_ALL; 
    args.ops_per_trial = 10; 
    args.bytes_per_op = 1024; 
    args.num_trials = 1; 
    args.result_file=NULL;
    args.result_file_mode = "w"; 
    args.scratch_file = "/usr/tmp/testfile"; 
    args.remove_file = FALSE;
    args.num_ss = -1;


    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

    gethostname(hostname, sizeof(hostname));
    fprintf(stderr, "%d:%s starting...\n", myrank, hostname);

    MPI_Barrier(MPI_COMM_WORLD); 

    /* parse the command-line arguments */
    argp_parse(&argp, argc, argv, 0, 0, &args); 

    if ((myrank == 0) && (args.result_file!=NULL)) {
        result_fp = fopen(args.result_file, args.result_file_mode);
        if (!result_fp) {
            fprintf(stderr, "could not open result file \"%s\"\n", 
                args.result_file);
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
    }
    else {
        result_fp = stdout;
    }

    if (myrank == 0) {
        print_args(result_fp, &args);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    for (i=0; i<args.num_trials; i++) {
        double t_total;

        if (myrank == 0) {
            fprintf(stderr, "trial %d\n", i);
        }

        nto1_write(MPI_COMM_WORLD, args.scratch_file, 
                args.bytes_per_op, args.ops_per_trial,
                args.remove_file, &t_total); 

        print_stats(result_fp, &args, t_total);
        fflush(result_fp);
    }


    /* print statistics */
    if (myrank == 0) {
        fclose(result_fp);
    }

    MPI_Finalize();

    return 0; 
}
