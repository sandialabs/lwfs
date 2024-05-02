#include "mpi.h"
#include "util/stats.h"
#include "logger/logger.h"
#include "types/types.h"
#include "types/fprint_types.h"
#include "storage/ss_clnt.h"
#include "storage/ss_srvr.h"
#include "authr/authr_clnt.h"
#include "authr/authr_srvr.h"

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



    /* ---------- LWFS-specific arguments ---------- */

    /** @brief Flag to run the storage service locally. */
    lwfs_bool ss_local; 

    /** @brief The number of storage servers. */
    int num_ss; 

    /** @brief Process IDs of the storage service. */
    lwfs_process_id *ss; 

    /** @brief Storage service pointers */
    lwfs_service *storage_svc;

    /** @brief File that contains NID, PID pairs for available storage servers */
    char *ss_file; 

    /** @brief Flag to run the auth service locally. */
    lwfs_bool authr_local; 

    /** @brief Process ID of the authorization service. */
    lwfs_process_id authr; 

    /** @brief Clear the database. */
    lwfs_bool authr_db_clear; 

    /** @brief Recover from a crash. */
    lwfs_bool authr_db_recover; 

    /** @brief Path to the database file. */
    char *authr_db_path; 

    /** @brief Verify credentials. */
    lwfs_bool authr_verify_creds; 

    /** @brief Verify caps. */
    lwfs_bool authr_verify_caps; 

    /** @brief Cache caps. */
    lwfs_bool authr_cache_caps; 

}; 

static void print_args(FILE *fp, struct arguments *args) 
{
    fprintf(fp, "------------  ARGUMENTS -----------\n");
    fprintf(fp, "\ttype = %d\n", args->type);
    fprintf(fp, "\tverbose = %d\n", args->debug_level);
    fprintf(fp, "\tbytes-per-op = %lu\n", (unsigned long)args->bytes_per_op);
    fprintf(fp, "\tnum-trials = %d\n", args->num_trials);
    fprintf(fp, "\tops-per-trial = %d\n", args->ops_per_trial);
    fprintf(fp, "\tresult-file = %s\n", ((args->result_file)? args->result_file: "stdout"));
    fprintf(fp, "\tresult-file-mode = %s\n", args->result_file_mode);
    fprintf(fp, "\tscratch-file = %s\n", args->scratch_file);
    fprintf(fp, "\tremove-file = %s\n", ((args->remove_file)? "true":"false"));
    fprintf(fp, "\n");

    fprintf(fp, "\tauthr-verify-creds = %s\n", ((args->authr_verify_creds)? "true" : "false"));
    fprintf(fp, "\tauthr-verify-caps = %s\n", ((args->authr_verify_caps)? "true" : "false"));
    fprintf(fp, "\tauthr-cache-caps = %s\n", ((args->authr_cache_caps)? "true" : "false"));
    fprintf(fp, "\tauthr-local = %s\n", ((args->authr_local)? "true" : "false"));
    if (!args->authr_local) {
        fprintf(fp, "\tauthr-nid = %u\n", args->authr.nid);
        fprintf(fp, "\tauthr-pid = %u\n", args->authr.pid);
    }
    else {
        fprintf(fp, "\tauthr-db-path = %s\n", args->authr_db_path);
        fprintf(fp, "\tauthr-db-clear = %s\n", 
                ((args->authr_db_clear)? "true" : "false"));
        fprintf(fp, "\tauthr-db-recover = %s\n", 
                ((args->authr_db_recover)? "true" : "false"));
    }
    fprintf(fp, "\tss-local = %s\n", ((args->ss_local)? "true" : "false"));
    fprintf(fp, "\tnum-ss = %d\n", args->num_ss); 
    if (!args->ss_local) {
        int i; 
        for (i=0; i<args->num_ss; i++) {
            fprintf(fp, "\tss-nid[%d] = %u\n", i, args->ss[i].nid);
            fprintf(fp, "\tss-pid[%d] = %u\n", i, args->ss[i].pid);
        }
    }

    fprintf(fp, "-----------------------------------\n");
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

    {"authr-local",     20, 0, 0, "Run the authr svc locally [yes,no]"},
    {"authr-nid",       21, "<val>", 0, "Portals NID of the authr server"},
    {"authr-pid",       22, "<val>", 0, "Portals PID of the authr server" },
    {"authr-db-clear",  23, 0, 0, "clear the authr db (local only)"},
    {"authr-db-recover",24, 0, 0, "recover db from crash (local only)"},
    {"authr-db-path",   25, "<FILE>", 0, "Path to authr database (local only)" },
    {"authr-verify-creds",26, 0, 0, "Verify credentials"},
    {"authr-verify-caps",27, 0, 0, "Verify capabilities"},
    {"authr-cache-caps",28, 0, 0, "Cache capabilities"},

    {"ss-local",   30, 0, 0, "Run the storage svc locally [yes,no]"},
    {"num-ss",     31, "<val>", 0, "number of storage servers"},
    {"ss-file",    32, "<val>", 0, "File that lists the Portals NID "
        "of available storage servers"},
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



        case 20: /* authr_local */
            arguments->authr_local = TRUE;
            break;

        case 21: /* authr.nid */
            arguments->authr.nid = (lwfs_ptl_nid)atoll(arg);
            break;

        case 22: /* authr.pid */
            arguments->authr.pid = (lwfs_ptl_pid)atoll(arg);
            break;

        case 23: /* authr_db_clear */
            arguments->authr_db_clear = TRUE;
            break;

        case 24: /* authr_db_recover */
            arguments->authr_db_recover = TRUE;
            break;

        case 25: /* authr_db_recover */
            arguments->authr_db_path = arg;
            break;

        case 26: /* authr_verify_creds */
            arguments->authr_verify_creds = TRUE;
            break;

        case 27: /* authr_verify_caps */
            arguments->authr_verify_caps = TRUE;
            break;

        case 28: /* authr_cache_caps */
            arguments->authr_cache_caps = TRUE;
            break;




        case 30: /* ss_local */
            arguments->ss_local = TRUE;
            break;

        case 31: /* num-ss */
            arguments->num_ss = atoi(arg);
            break;

        case 32: /* ss-file  */
            arguments->ss_file = arg; 
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


static log_level debug_level = LOG_UNDEFINED; 

/** 
 * @brief Read the storage server input file.
 */
static int read_ss_file(
        const char *fname,
        int num_ss,
        lwfs_process_id *ss)
{
    int i; 
    int rc = LWFS_OK; 
    FILE *fp; 

    fp = fopen(fname, "r");
    if (!fp) {
        log_error(debug_level, "unable to open ss_file \"%s\": %s", fname, strerror(errno));
        return LWFS_ERR; 
    }

    for (i=0; i<num_ss; i++) {
        rc = fscanf(fp, "%u %u", &ss[i].nid, &ss[i].pid);
        if (rc != 2) {
            log_error(debug_level, "errors reading ss_file: line %d", i);
            rc = LWFS_ERR; 
            goto cleanup;
        }
        rc = LWFS_OK;
    }

cleanup: 
    fclose(fp); 
    return rc; 
}


/**
 * @brief Create a container and acls for testing the storage service.
 *
 * To properly test the storage service, we need a container for the 
 * objects we wish to create, write to, and read from.  So, we need to
 * create a container, modify the acl to allow the user to perform the 
 * operations.  In this case, we really only need capabilities to write 
 * to the container. 
 *
 * @param cred @input the user's credential. 
 * @param cid @output the generated container ID. 
 * @param modacl_cap @output the cap that allows the user to modify acls for the container.
 * @param write_cap @output the cap that allows the user to 
 */
static int get_perms(
        const lwfs_cred *cred,
        lwfs_cid *cid, 
        const lwfs_opcode opcodes,
        lwfs_cap *cap)
{
    int rc = LWFS_OK;  /* return code */

    lwfs_txn *txn_id = NULL;  /* not used */
    lwfs_cap create_cid_cap;  
    lwfs_cap modacl_cap;
    lwfs_uid_array uid_array; 


    /* get the capability to create the container (for now anyone can create cids) */
    memset(&create_cid_cap, 0, sizeof(lwfs_cap)); 


    /* create the container */
    rc = lwfs_bcreate_container(txn_id, &create_cid_cap, &modacl_cap);
    if (rc != LWFS_OK) {
        log_error(debug_level, "unable to create a container: %s",
                lwfs_err_str(rc));
        return rc; 
    }

    /* initialize the cid */
    *cid = modacl_cap.data.cid; 

    /* initialize the uid array */
    uid_array.lwfs_uid_array_len = 1; 
    uid_array.lwfs_uid_array_val = (lwfs_uid *)&cred->data.uid;

    /* create the acls */
    rc = lwfs_bcreate_acl(txn_id, *cid, opcodes, &uid_array, &modacl_cap);
    if (rc != LWFS_OK) {
        log_error(debug_level, "unable to create write acl: %s",
                lwfs_err_str(rc));
        return rc; 
    }

    /* get the cap */
    rc = lwfs_bget_cap(*cid, opcodes, cred, cap);
    if (rc != LWFS_OK) {
        log_error(debug_level, "unable to call getcaps: %s",
                lwfs_err_str(rc));
        return rc;
    }

    return rc; 
}



static int nton_write(
        lwfs_service *storage_svc, 
        int num_stripes,
        MPI_Comm comm, 
        const char *fname,
        const ssize_t bufsize, 
        const int num_ops, 
        const int remove_file,
        double *t_total)
{
    int rc = LWFS_OK;
    int myrank, i;
    long bytes_per_stripe; 
    double t_start; 
    char *buf = NULL;

    /* data structures for the LWFS security model */
    lwfs_cred cred; 
    lwfs_cap cap; 
    lwfs_cid cid; 
    lwfs_obj *obj = NULL; 
    lwfs_request *reqs = NULL; 

    MPI_Comm_rank(comm, &myrank);

    reqs = (lwfs_request *)malloc(num_stripes*sizeof(lwfs_request));
    if (!reqs) {
        log_error(debug_level, "unable to allocate objects\n");
        goto cleanup; 
    }

    obj = (lwfs_obj *)malloc(num_stripes*sizeof(lwfs_obj));
    if (!obj) {
        log_error(debug_level, "unable to allocate objects\n");
        goto cleanup; 
    }

    buf = (char *)malloc(num_ops*bufsize*sizeof(char));
    if (!buf) {
        log_error(debug_level, "unable to allocate buffer\n");
        goto cleanup; 
    }

    bytes_per_stripe = num_ops*bufsize/num_stripes; 

    for (i=0; i<bufsize; i++) {
        buf[i] = myrank*bufsize + i; 
    }

    /* The UNIX "open" command is equivalent to 
     *    - get permissions
     *    - create object.
     */
    MPI_Barrier(comm); 
    t_start = MPI_Wtime(); 

	/* Create a container and allocate the capabilities necessary 
	 * to perform operations. For simplicity, all objs belong to 
	 * one container. 
	 */
    if (myrank == 0) {
        memset(&cred, 0, sizeof(lwfs_cred));  // cred is not used... yet
        rc = get_perms(&cred, &cid, LWFS_CONTAINER_WRITE, &cap); 
        if (rc != LWFS_OK) {
            log_error(debug_level, "unable to create a container and get caps: %s",
                    lwfs_err_str(rc));
            goto cleanup;
        }
    }

    /* scatter the capability to all clients O(lg n) */
    MPI_Scatter(&cap, sizeof(cap), MPI_BYTE, 
            &cap, sizeof(cap), MPI_BYTE, 0, comm);

    /* create the objects */
    for (i=0; i<num_stripes; i++) {
        rc = lwfs_create_obj(&(storage_svc[i]), NULL, cid, &cap, &(obj[i]), &reqs[i]); 
        if (rc != LWFS_OK) {
            log_error(debug_level, "failed to create obj: %s",
                    lwfs_err_str(rc));
            goto cleanup;
        }
    }

    /* write data */
    for (i=0; i<num_stripes; i++) {

        /* wait for object create to finish */
        lwfs_wait(&reqs[i], &rc);
        if (rc != LWFS_OK) {
            log_error(debug_level, "%d: unable to write buffer\n", myrank);
            goto cleanup;
        }

        /* write the data */
        rc = lwfs_write(NULL, &(obj[i]), i*bytes_per_stripe, buf, bufsize, &cap, &(reqs[i]));
        if (rc != LWFS_OK) {
            log_error(debug_level, "%d: unable to write buffer\n", myrank);
            goto cleanup;
        }
    }

    /* wait for results */
    for (i=0; i<num_stripes; i++) {
        lwfs_wait(&reqs[i], &rc);
        if (rc != LWFS_OK) {
            log_error(debug_level, "%d: unable to write buffer\n", myrank);
            goto cleanup;
        }
    }

    *t_total = MPI_Wtime() - t_start; 


    /* LWFS does not have a "close" operation */
    if (remove_file) {
        for (i=0; i<num_stripes; i++) {
            lwfs_bremove_obj(NULL, &obj[i], &cap);
        }
    }

cleanup:

    free(reqs); 
    free(obj); 
    free(buf); 

    return rc;
}

typedef struct {
    double val;
    double index;
} pair_t;


void vmax(pair_t *in, pair_t *inout, int *len, MPI_Datatype *dptr) {
    int i;

    for (i=0; i<*len; ++i) {

        if ((*in).val > (*inout).val) {
            (*inout).val = (*in).val;
            (*inout).index = (*in).index;
        }
        in++;
        inout++;
    }
}

void vmin(pair_t *in, pair_t *inout, int *len, MPI_Datatype *dptr) {
    int i;

    for (i=0; i<*len; ++i) {

        if ((*in).val < (*inout).val) {
            (*inout).val = (*in).val;
            (*inout).index = (*in).index;
        }
        in++;
        inout++;
    }
}

void vsum(pair_t *in, pair_t *inout, int *len, MPI_Datatype *dptr) {
    int i;

    for (i=0; i<*len; ++i) {
        (*inout).val += (*in).val;
        (*inout).index += (*in).index;
        in++;
        inout++;
    }
}


void print_stats(
        FILE *result_fp, 
        struct arguments *args, 
        double t_total)
{
    int np = 1; 
    int myrank = 0;
    pair_t src; 
    pair_t t_total_min, t_total_max, t_total_avg;
    MPI_Op min_op, max_op, sum_op;
    MPI_Datatype ctype; 

    MPI_Comm_size(MPI_COMM_WORLD, &np); 
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank); 

    MPI_Type_contiguous(2, MPI_DOUBLE, &ctype);
    MPI_Type_commit(&ctype);
    MPI_Op_create((MPI_User_function *)vmax, 1, &max_op);
    MPI_Op_create((MPI_User_function *)vmin, 1, &min_op);
    MPI_Op_create((MPI_User_function *)vsum, 1, &sum_op);

    src.val = t_total; 
    src.index = myrank;

    /* gather stats across participating nodes */
    MPI_Reduce(&src, &t_total_min, 1, ctype, min_op, 0, MPI_COMM_WORLD);
    MPI_Reduce(&src, &t_total_max, 1, ctype, max_op, 0, MPI_COMM_WORLD);
    MPI_Reduce(&src, &t_total_avg, 1, ctype, sum_op, 0, MPI_COMM_WORLD);

    if (myrank == 0) {
        static int first = 1; 

        t_total_avg.val = t_total_avg.val/np; 

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
        fprintf(result_fp, "%1.6e  ", (mb*args->ops_per_trial)/t_total_max.val);    
        fprintf(result_fp, "%1.6e  ", t_total_avg.val);
        fprintf(result_fp, "%1.6e  ", t_total_min.val);
        fprintf(result_fp, "%1.6e  ", t_total_max.val);
        fprintf(result_fp, "%05d\n", (int)t_total_max.index);

        fprintf(result_fp, "%s ----------------------------------------------------\n", "%");

        fflush(result_fp);
    }

}



static int lwfs_init(struct arguments *args, lwfs_service *authr_svc, lwfs_service **storage_svc)
{
    int rc=LWFS_OK;
    int myproc; 

    MPI_Comm_rank(MPI_COMM_WORLD, &myproc);

    /* initialize the logger */
    logger_set_file(stdout); 
    if (args->debug_level == 0)   {
        logger_set_default_level(LOG_OFF); 
    } else if (args->debug_level > 5)   {
        logger_set_default_level(LOG_ALL); 
    } else   {
        logger_set_default_level(args->debug_level - LOG_OFF); 
    }
    debug_level = args->debug_level;

    /* initialize RPC */
    //lwfs_rpc_init(128-myproc);
    lwfs_rpc_init(129+myproc);

    {
        lwfs_process_id my_id;
        lwfs_get_my_pid(&my_id);
        fprintf(stderr, "%d: Portals ID=%d\n", myproc, my_id.pid);
    }

    /* initialize the authr client */
    if (args->authr_local) {
        /* initialize a local client (no remote calls) */
        rc = lwfs_authr_clnt_init_local(args->authr_verify_caps, 
                args->authr_verify_creds, authr_svc);
        if (rc != LWFS_OK) {
            log_error(args->debug_level, "unable to initialize auth clnt: %s",
                    lwfs_err_str(rc));
            return rc; 
        }
    }
    else {
        /* if the user didn't specify a nid for the storage server, assume one */
        if (args->authr.nid == 0) {
            lwfs_process_id my_id; 
            log_warn(args->debug_level, "nid not set: assuming server is on local node");

            lwfs_get_my_pid(&my_id);
            args->authr.nid = my_id.nid; 
        }

        /* Ping the authr process ID to get the authr svc */
        rc = lwfs_rpc_ping(args->authr, authr_svc); 
        if (rc != LWFS_OK) {
            log_error(args->debug_level, "unable to get authr svc");
            return rc; 
        }

        /* initialize a remote client */
        rc = lwfs_authr_clnt_init(authr_svc, args->authr_cache_caps);
        if (rc != LWFS_OK) {
            log_error(args->debug_level, "unable to initialize auth clnt: %s",
                    lwfs_err_str(rc));
            return rc; 
        }
    }


    /* initialize the storage service client */
    if (args->ss_local) {
        lwfs_process_id my_id; 
        lwfs_get_my_pid(&my_id); 

        args->num_ss = 1; 

        args->ss = (lwfs_process_id *)malloc(args->num_ss*sizeof(lwfs_process_id));
        *storage_svc = (lwfs_service *)malloc(sizeof(lwfs_service)); 

        /* set the arguments  */
        args->ss[0].nid = my_id.nid; 
        args->ss[0].pid = my_id.pid; 



        /* initialize a storage svc local client (local calls to storage svc) */
        rc = lwfs_ss_clnt_init_local(args->ss[0].nid, args->ss[0].pid, 
                authr_svc, args->authr_verify_caps, 
                args->authr_cache_caps, 
                "ss_root_dir", *storage_svc);
        if (rc != LWFS_OK) {
            log_error(args->debug_level, "unable to initialize storage server: %s",
                    lwfs_err_str(rc));
            return rc; 
        }
    }
    else {
        int i; 
        lwfs_service *ss_svc = NULL;
        
        args->ss = malloc(args->num_ss * sizeof(lwfs_process_id));
        *storage_svc = malloc(args->num_ss * sizeof(lwfs_service));

        ss_svc = *storage_svc; 

        rc = read_ss_file(args->ss_file, args->num_ss, args->ss);
        if (rc != LWFS_OK) {
            log_error(debug_level, "%s", lwfs_err_str);
            return rc; 
        }

        for (i=0; i<args->num_ss; i++) {
            /* Ping the storage server to get the service information */
            rc = lwfs_rpc_ping(args->ss[i], &ss_svc[i]); 
            if (rc != LWFS_OK) {
                log_error(args->debug_level, "unable to get authr svc");
                return rc; 
            }

            /* initialize a storage svc remote client */
            rc = lwfs_ss_clnt_init();
            if (rc != LWFS_OK) {
                log_error(args->debug_level, "failed to initialize storage client: %s",
                        lwfs_err_str(rc));
                return rc; 
            }
        }
    }

    return rc; 
}

int main(int argc, char *argv[])
{
    /* local variables */
    int rc = LWFS_OK, rc2=LWFS_OK;
    int myrank = 0; 
    int i; 
    FILE *result_fp; 
    char hostname[256];
    struct arguments args; 
    lwfs_service authr_svc; 
    lwfs_service *storage_svc = NULL;

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

    args.num_ss = 1; 
    args.ss_local = FALSE; 
    args.ss_file = "ss-list.txt";
    args.ss = NULL; 

    args.authr.nid = 0;
    args.authr.pid = LWFS_AUTHR_PID;
    args.authr_local = FALSE;
    args.authr_db_path = "naming.db";
    args.authr_db_clear = FALSE; 
    args.authr_db_recover = FALSE; 
    args.authr_verify_creds = FALSE;
    args.authr_verify_caps = TRUE;
    args.authr_cache_caps = FALSE;
    /* ----- */



    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);


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

    /*---- SETUP LWFS CLIENTS ----*/
    rc = lwfs_init(&args, &authr_svc, &storage_svc);
    if (rc != LWFS_OK) {
        log_error(args.debug_level, "could not initialize LWFS");
        goto cleanup;
    }


    if (myrank == 0) {
        print_args(stdout, &args);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    gethostname(hostname, sizeof(hostname));
    fprintf(stderr, "%d:%s starting...\n", myrank, hostname);

    MPI_Barrier(MPI_COMM_WORLD);
    if (myrank == 0) {
        fprintf(stderr, "\n");
    }

    for (i=0; i<args.num_trials; i++) {
        double t_total; 

        if (myrank == 0) {
            fprintf(stdout, "trial %d\n", i);
        }

        rc = nton_write(storage_svc, args.num_ss, MPI_COMM_WORLD, args.scratch_file, 
                args.bytes_per_op, args.ops_per_trial,
                args.remove_file, &t_total);
        if (rc != LWFS_OK) {
            log_error(debug_level, "%s", lwfs_err_str(rc));
            goto cleanup; 
        }

        print_stats(result_fp, &args, t_total);
    }


    /* print statistics */
    if (myrank == 0) {
        fclose(result_fp);
    }

cleanup:

    /* close authr svc */
	if (args.authr_local) {
		rc2 = lwfs_authr_srvr_fini(&authr_svc);
	}
	else {
		rc2 = lwfs_authr_clnt_fini();
	}
	if (rc2 != LWFS_OK) {
		log_error(debug_level, "unable to finish authr svc: %s", 
				lwfs_err_str(rc));
	}

	/* close ss */
	if (args.ss_local) {
		lwfs_ss_srvr_fini(&storage_svc[0]);
	}
	else {
		for (i=0; i<args.num_ss; i++) {
			lwfs_ss_clnt_fini();
		}
	}

MPI_Finalize();

    if (storage_svc != NULL) {
        free(storage_svc);
    }

    if (args.ss != NULL) {
        free(args.ss);
    }

    return rc; 
}
