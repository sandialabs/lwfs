#include "lwfs-opts.h"
#include "client/storage_client/storage_client.h"
#include "client/storage_client/storage_client_sync.h"
#include "client/storage_client/storage_client_opts.h"
#include "client/authr_client/authr_client.h"
#include "client/authr_client/authr_client_sync.h"
#include "client/authr_client/authr_client_opts.h"
#include "common/rpc_common/lwfs_ptls.h"
#include "common/config_parser/config_parser.h"

#include <mpi.h>
#include <time.h>
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


static log_level debug_level = LOG_UNDEFINED; 


static void print_args(
		FILE *fp, 
		const struct gengetopt_args_info *args_info,
		const char *prefix)
{
	time_t rawtime; 

	time(&rawtime); 

	fprintf(fp, "%s %s",prefix, ctime(&rawtime) );
	fprintf(fp, "%s ------------  Options -----------\n",prefix );
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
	fprintf(fp, "%s \tresult-file = %s\n",prefix , 
			((args_info->result_file_arg)? args_info->result_file_arg: "stdout"));
	fprintf(fp, "%s \tresult-file-mode = %s\n",prefix , 
			args_info->result_file_mode_arg);
	fprintf(fp, "%s \tscratch-file = %s\n",prefix , 
			args_info->scratch_file_arg);
	fprintf(fp, "%s \n", prefix);

	print_authr_client_opts(fp, args_info, prefix);
	print_storage_client_opts(fp, args_info, prefix); 

	fprintf(fp, "%s -----------------------------------\n", prefix);
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
		const lwfs_service *authr_svc, 
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
    rc = lwfs_create_container_sync(authr_svc, txn_id, *cid, &create_cid_cap, &modacl_cap);
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
    rc = lwfs_create_acl_sync(authr_svc, txn_id, *cid, opcodes, &uid_array, &modacl_cap);
    if (rc != LWFS_OK) {
        log_error(debug_level, "unable to create write acl: %s",
                lwfs_err_str(rc));
        return rc; 
    }

    /* get the cap */
    rc = lwfs_get_cap_sync(authr_svc, *cid, opcodes, cred, cap);
    if (rc != LWFS_OK) {
        log_error(debug_level, "unable to call getcaps: %s",
                lwfs_err_str(rc));
        return rc;
    }

    return rc; 
}



static int create_objs_sync(
	const lwfs_service *svc,
	const lwfs_txn *txn,
	const lwfs_cid cid,
	const lwfs_cap *cap,
	const int num_ops, 
	lwfs_obj *obj)
{
    int rc = LWFS_OK;
    int i; 

    for (i=0; i<num_ops; i++) {
	rc = lwfs_init_obj(svc, 0, cid, LWFS_OID_ANY, &obj[i]);
	if (rc != LWFS_OK) {
	    log_error(debug_level, "unable to initialize obj structure\n");
	    goto cleanup;
	}

	rc = lwfs_create_obj_sync(NULL, &obj[i], cap);
	if (rc != LWFS_OK) {
	    log_error(debug_level, "unable to write buffer\n");
	    goto cleanup;
	}
    }

cleanup:
    return rc; 
}


static int create_objs_async(
	const lwfs_service *svc,
	const lwfs_txn *txn,
	const lwfs_cid cid,
	const lwfs_cap *cap,
	const int num_ops, 
	const int in_transit,
	lwfs_obj *obj)
{
	int rc = LWFS_OK;
	int i; 
	int pending; 
	int count; 
	lwfs_oid oid; 
	lwfs_request reqs[in_transit];

	/* submit each request (we may want to limit the number of pending reqs */
	count=0; 
	lwfs_clear_oid(&oid);


	/* submit the first batch of requests */
	pending = (in_transit<num_ops)? in_transit:num_ops; 
	for (i=0; i<pending; i++) {

		memcpy(&oid, &count, sizeof(count));
		
		rc = lwfs_init_obj(svc, 0, cid, LWFS_OID_ANY, &obj[count]);
		if (rc != LWFS_OK) {
			log_error(debug_level, "unable to write buffer");
			goto cleanup;
		}

		rc = lwfs_create_obj(txn, &obj[count], cap, &reqs[i]);
		if (rc != LWFS_OK) {
			log_error(debug_level, "unable to write buffer");
			goto cleanup;
		}
		count++;
	}

	/* As requests complete, submit the remaining until 
	 * all have been sent */
	while (count < num_ops) {
		int which, remote_rc; 

		rc = lwfs_waitany(reqs, pending, -1, &which, &remote_rc);
		if ((rc != LWFS_OK) || (remote_rc != LWFS_OK)) {
			log_error(debug_level, "remote error: %s", lwfs_err_str(rc | remote_rc));
			rc = remote_rc; 
			goto cleanup;
		}

		/* initialize object structure */
		rc = lwfs_init_obj(svc, 0, cid, LWFS_OID_ANY, &obj[count]);
		if (rc != LWFS_OK) {
			log_error(debug_level, "unable to write buffer");
			goto cleanup;
		}

		/* submit another request to replace the completed request */
		rc = lwfs_create_obj(txn, &obj[count], cap, &reqs[which]);
		if (rc != LWFS_OK) {
			log_error(debug_level, "unable to write buffer");
			goto cleanup;
		}

		count++;
	}
	
	/* at this point all requests have been submitted... wait for the rest to complete */
	while (pending > 0) {
		int which, remote_rc;

		rc = lwfs_waitany(reqs, pending, -1, &which, &remote_rc);
		if ((rc != LWFS_OK) || (remote_rc != LWFS_OK)) {
			log_error(debug_level, "remote error: %s", lwfs_err_str(rc | remote_rc));
			rc = remote_rc; 
			goto cleanup;
		}

		pending--; 

		/* replace the completed request with the last request */
		if (pending > 0) {
			memcpy(&reqs[which], &reqs[pending], sizeof(lwfs_request));
		}
	}

cleanup:
	//free(reqs);
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
		struct gengetopt_args_info *args_info, 
		double t_total)
{
	int np = 1; 
	int myrank = 0;

	double t_total_min, t_total_max, t_total_avg;
	int total_ops; 

	MPI_Comm_size(MPI_COMM_WORLD, &np); 
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank); 

	/* gather stats across participating nodes */
	MPI_Reduce(&t_total, &t_total_min, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
	MPI_Reduce(&t_total, &t_total_max, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
	MPI_Reduce(&t_total, &t_total_avg, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

	/* gather the total number of ops processed */
	MPI_Reduce(&args_info->ops_per_trial_arg, &total_ops, 1, 
			MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

	if (myrank == 0) {
		static int first = 1; 

		t_total_avg = t_total_avg/np; 

		/* print the header */
		if (first) {
			time_t rawtime; 

			time(&rawtime); 

			fprintf(result_fp, "%s ----------------------------------------------------\n", "%");
			fprintf(result_fp, "%s lwfs-nton-create test (ops divided among processes)\n", "%");
			fprintf(result_fp, "%s %s", "%", ctime(&rawtime));
			fprintf(result_fp, "%s ----------------------------------------------------\n", "%");
			fprintf(result_fp, "%s column   description\n", "%");
			fprintf(result_fp, "%s ----------------------------------------------------\n", "%");
			fprintf(result_fp, "%s   0     number of clients (np)\n", "%"); 
			fprintf(result_fp, "%s   1     number of storage targets (num_ss)\n","%"); 
			fprintf(result_fp, "%s   2     aggregate number of operations (ops_per_trial)\n","%"); 
			fprintf(result_fp, "%s   3     aggregate throughput  (ops_per_sec) = "
					"ops_per_trial/total_max\n","%"); 
			fprintf(result_fp, "%s   4     total_avg (s)\n","%"); 
			fprintf(result_fp, "%s   5     total min (s)\n","%"); 
			fprintf(result_fp, "%s   6     total_max (s)\n","%"); 
			fprintf(result_fp, "%s ----------------------------------------------------\n", "%");

			first = 0; 
		}

		/* print the row */
		fprintf(result_fp, "%05d   ", np);
		fprintf(result_fp, "%04d   ", args_info->ss_num_servers_arg);
		fprintf(result_fp, "%03d   ", total_ops);  
		fprintf(result_fp, "%1.6e  ", total_ops/t_total_max);    
		fprintf(result_fp, "%1.6e  ", t_total_avg);
		fprintf(result_fp, "%1.6e  ", t_total_min);
		fprintf(result_fp, "%1.6e\n", t_total_max);

		fprintf(result_fp, "%s ----------------------------------------------------\n", "%");

		fflush(result_fp);
	}
}




int main(int argc, char *argv[])
{
    /* local variables */
    int rc = LWFS_OK; 
    int myrank = 0; 
    int np;
    int i; 
    FILE *result_fp; 
    char hostname[256];
    struct gengetopt_args_info args_info;
    lwfs_obj *objs; 
    struct lwfs_core_services lwfs_core_svc; 

    /* Parse command line options to override defaults */
    if (cmdline_parser(argc, argv, &args_info) != 0) {
	exit(1);
    }

    /* initialize the logger */
    debug_level = args_info.verbose_arg; 
    logger_init(debug_level, args_info.logfile_arg);

    myrank = 0;
    np = 1; 

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &np);


    MPI_Barrier(MPI_COMM_WORLD);


    if ((myrank == 0) && (args_info.result_file_arg!=NULL)) {
	result_fp = fopen(args_info.result_file_arg, 
		args_info.result_file_mode_arg);
	if (!result_fp) {
	    fprintf(stderr, "could not open result file \"%s\"\n", 
		    args_info.result_file_arg);
	    MPI_Abort(MPI_COMM_WORLD, -1);
	}
    }
    else {
	result_fp = stdout;
    }

    /* initialize RPC */
    lwfs_ptl_init(PTL_IFACE_DEFAULT, 128+myrank);
    lwfs_rpc_init(LWFS_RPC_PTL, LWFS_RPC_XDR);

    /* initialize the service descriptions on node 0 */
    if (myrank == 0) {
	struct lwfs_config lwfs_cfg; 

	/* parse the lwfs config file */
	rc = parse_lwfs_config_file(args_info.lwfs_config_file_arg, 
		&lwfs_cfg);
	if (rc != LWFS_OK) {
	    log_error(debug_level, "unable to parse config file");
	    return rc; 
	}

	rc = lwfs_load_core_services(&lwfs_cfg, &lwfs_core_svc); 
	if (rc != LWFS_OK) {
	    log_error(debug_level, "unable to parse config file");
	    return rc; 
	}

	print_args(result_fp, &args_info, "%");
	
	/* release the data allocated for the config structure */
	lwfs_config_free(&lwfs_cfg);
    }



    /* distribute the core service descriptions */
    MPI_Bcast(&lwfs_core_svc, 
	    sizeof(struct lwfs_core_services),
	    MPI_BYTE, 
	    0, MPI_COMM_WORLD);

    /* allocate space for the storage services */
    if (myrank != 0) {
	lwfs_core_svc.storage_svc = (lwfs_service *)
	    malloc(lwfs_core_svc.ss_num_servers * sizeof(lwfs_service));
    }

    /* distribute the service descriptions for the storage servers */
    MPI_Bcast(lwfs_core_svc.storage_svc, 
	    lwfs_core_svc.ss_num_servers * sizeof(lwfs_service),
	    MPI_BYTE, 
	    0, MPI_COMM_WORLD);


    /* divide the ops_per_trial among the different processors */
    i = args_info.ops_per_trial_arg / np; 
    if (myrank < (args_info.ops_per_trial_arg % np)) {
	i++;
    }
    args_info.ops_per_trial_arg = i; 


    if (myrank == 0) {
	print_args(result_fp, &args_info, "%");
    }

    MPI_Barrier(MPI_COMM_WORLD);

    gethostname(hostname, sizeof(hostname));
    fprintf(stderr, "%d:%s starting...\n", myrank, hostname);

    MPI_Barrier(MPI_COMM_WORLD);
    if (myrank == 0) {
	fprintf(stderr, "\n");
    }


    /* allocate objects */
    objs = (lwfs_obj *)malloc(args_info.ops_per_trial_arg*sizeof(lwfs_obj));
    if (objs == NULL) {
	log_error(debug_level, "out of memory");
	rc = LWFS_ERR_NOSPACE;
	goto cleanup;
    }

    for (i=0; i<args_info.num_trials_arg; i++) {
	double t_total, t_start; 
	lwfs_cap cap; 
	lwfs_cred cred; 
	lwfs_cid cid = LWFS_CID_ANY; 
	lwfs_service *svc = &lwfs_core_svc.storage_svc[myrank % lwfs_core_svc.ss_num_servers];
	lwfs_txn *txn = NULL;

	if (myrank == 0) {
	    fprintf(stdout, "trial %d\n", i);
	}

	log_debug(debug_level, "creating objects");

	memset(&cred, 0, sizeof(lwfs_cred));
	memset(&cap, 0, sizeof(lwfs_cap));
	cid = LWFS_CID_ANY;

	memset(objs, 0, args_info.ops_per_trial_arg*sizeof(lwfs_obj));


	/* Create a container and allocate the capabilities necessary 
	 * to perform operations. For simplicity, all objs belong to 
	 * one container. 
	 */
	if (myrank == 0) {
	    rc = get_perms(&lwfs_core_svc.authr_svc, &cred, &cid, LWFS_CONTAINER_WRITE, &cap); 
	    if (rc != LWFS_OK) {
		log_error(debug_level, "unable to create a container and get caps: %s",
			lwfs_err_str(rc));
		return rc; 
	    }
	}

	log_debug(debug_level, "broadcast cap");

	/* scatter the capability to all clients O(lg n) */
	MPI_Bcast(&cap, sizeof(lwfs_cap), MPI_BYTE, 0, MPI_COMM_WORLD); 

	log_debug(debug_level, "creating objects");

	MPI_Barrier(MPI_COMM_WORLD); 
	t_start = MPI_Wtime(); 

	if (args_info.async_flag) {
	    /* asynchronous creates */
	    rc = create_objs_async(svc, txn, cid, &cap, 
		    args_info.ops_per_trial_arg, args_info.in_transit_arg, objs);
	    if (rc != LWFS_OK) {
		goto cleanup;
	    }
	}
	else {
	    /* synchronous creates */
	    rc = create_objs_sync(svc, txn, cid, &cap, 
		    args_info.ops_per_trial_arg, objs);
	    if (rc != LWFS_OK) {
		goto cleanup;
	    }
	}

	t_total = MPI_Wtime() - t_start;

	if (args_info.remove_file_flag) {
	    int j; 
	    for (j=0; j<args_info.ops_per_trial_arg; j++) {
		lwfs_remove_obj_sync(NULL, &objs[j], &cap);
	    }
	}

	print_stats(result_fp, &args_info, t_total);
    }

    /* print statistics */
    if (myrank == 0) {
	fclose(result_fp);
    }

cleanup:

    free(objs);
    lwfs_core_services_free(&lwfs_core_svc);

    MPI_Finalize();
    return rc; 
}
