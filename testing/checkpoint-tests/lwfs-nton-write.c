#include "mpi.h"
#include "lwfs-opts.h"
#include "support/logger/logger.h"
#include "client/storage_client/storage_client.h"
#include "client/storage_client/storage_client_sync.h"
#include "client/storage_client/storage_client_opts.h"
#include "client/authr_client/authr_client.h"
#include "client/authr_client/authr_client_sync.h"
#include "client/authr_client/authr_client_opts.h"
#include "common/rpc_common/lwfs_ptls.h"
#include "common/config_parser/config_parser.h"

#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>


#define BUFSIZE 1048576

/* ----------------- COMMAND-LINE OPTIONS --------------- */

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


static log_level debug_level = LOG_UNDEFINED; 


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
    rc = lwfs_create_container_sync(authr_svc, txn_id, *cid, 
			&create_cid_cap, &modacl_cap);
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
    rc = lwfs_create_acl_sync(authr_svc, txn_id, *cid, 
			opcodes, &uid_array, &modacl_cap);
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

/**
 * The UNIX "open" command is roughly equivalent to 
 *   - get permissions
 *   - create object
 */
static int lwfs_open(
	const lwfs_service *authr_svc,
    const lwfs_cred *cred,
    const lwfs_opcode opcode, 
    const lwfs_service *svc, 
    lwfs_cid *cid,
    lwfs_cap *cap, 
    lwfs_obj *obj)
{
    int rc = LWFS_OK;
    int myrank; 

    MPI_Comm_rank(MPI_COMM_WORLD, &myrank); 

    /* Create a container and allocate the capabilities necessary 
     * to perform operations. For simplicity, all objs belong to 
     * one container. 
     */
    if (myrank == 0) {
        rc = get_perms(authr_svc, cred, cid, opcode, cap); 
        if (rc != LWFS_OK) {
            log_error(debug_level, "unable to create a container and get caps: %s",
                    lwfs_err_str(rc));
            return rc; 
        }
    }

    log_debug(debug_level, "broadcast cap");

    /* scatter the capability to all clients O(lg n) */
    MPI_Bcast(cap, sizeof(lwfs_cap), MPI_BYTE, 0, MPI_COMM_WORLD); 

	/* initialize the object structure */
	rc = lwfs_init_obj(svc, 0, *cid, LWFS_OID_ANY, obj); 

    /* create the object (each process creates one) */
    rc = lwfs_create_obj_sync(NULL, obj, cap); 
    if (rc != LWFS_OK) {
        log_error(debug_level, "failed to create obj: %s",
                lwfs_err_str(rc));
        return rc; 
    }

    return rc; 
}


static int write_sync(
	void *buf, 
	lwfs_size bufsize, 
	int num_ops, 
	lwfs_obj *obj, 
	lwfs_cap *cap)
{
	int rc = LWFS_OK;
	int i; 

	for (i=0; i<num_ops; i++) {
		rc = lwfs_write_sync(NULL, obj, i*bufsize, buf, bufsize, cap);
		if (rc != LWFS_OK) {
			log_error(debug_level, "unable to write buffer\n");
			goto cleanup;
		}
	}

cleanup:
	return rc; 
}


static int write_async(
	void *buf, 
	const lwfs_size bufsize, 
	const int num_ops, 
	const int in_transit,
	lwfs_obj *obj, 
	lwfs_cap *cap)
{
	int rc = LWFS_OK;
	int i; 
	int pending; 
	int count; 
	lwfs_request reqs[in_transit];

	//lwfs_request *reqs = (lwfs_request *)malloc(num_ops*sizeof(lwfs_request));

	/* submit each write request (we may want to limit the number of pending reqs */
	
	count=0; 


	/* submit the first batch of requests */
	pending = (in_transit<num_ops)? in_transit:num_ops; 
	for (i=0; i<pending; i++) {
		rc = lwfs_write(NULL, obj, i*bufsize, buf, bufsize, cap, &reqs[i]);
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

		/* submit another request to replace the completed request */
		rc = lwfs_write(NULL, obj, count*bufsize, buf, bufsize, cap, &reqs[which]);
		if (rc != LWFS_OK) {
			log_error(debug_level, "unable to write buffer");
			goto cleanup;
		}

		count++;
	}
	
	/* at this point all requests have been submitted... wait for the rest */
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


/* include the source code for printing statistics */
#include "print-stats.c"


int main(int argc, char *argv[])
{
	/* local variables */
	int rc = LWFS_OK;
	int myrank = 0; 
	int num_servers=0; 
	FILE *result_fp = stdout; 
	lwfs_service authr_svc; 
	lwfs_service *storage_svc = NULL;
	struct gengetopt_args_info args_info; 
	char *buf = NULL; 
	int i; 


	/* Parse command line options to override defaults */
	if (cmdline_parser(argc, argv, &args_info) != 0) {
		exit(1);
	}

	/* initialize the logger */
	debug_level = args_info.verbose_arg; 
	logger_init(debug_level, args_info.logfile_arg);


	/* initialize MPI */
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

	MPI_Barrier(MPI_COMM_WORLD);

	/* initialize RPC */
	lwfs_ptl_init(PTL_IFACE_DEFAULT, 128+myrank);
	lwfs_rpc_init(LWFS_RPC_PTL, LWFS_RPC_XDR);


	/* get the service descriptions from the config file */
	if (myrank == 0) {
	    struct lwfs_config lwfs_cfg;
	    memset(&lwfs_cfg, 0, sizeof(struct lwfs_config));

	    log_debug(debug_level, "Parsing config_file \"%s\"",args_info.lwfs_config_file_arg);
	    rc = parse_lwfs_config_file(args_info.lwfs_config_file_arg, &lwfs_cfg);
	    if (rc != LWFS_OK) {
		log_error(debug_level, "unable to parse lwfs config file");
		MPI_Abort(MPI_COMM_WORLD,-EINVAL);
	    }


	    /* get the service description for the authr svc */
	    rc = lwfs_get_service(lwfs_cfg.authr_id, &authr_svc);
	    if (rc != LWFS_OK) {
		log_error(debug_level, "could not get authr svc: %s",
			lwfs_err_str(rc));
		return -EINVAL;
	    }

	    /* get the service description for the storage services */
	    /* allocate the storage service descriptors */
	    num_servers = lwfs_cfg.ss_num_servers; 

	    storage_svc = calloc(num_servers, sizeof(lwfs_service));
	    if (!storage_svc) {
		log_error(debug_level, "unable to alloc storage services");
		return LWFS_ERR_NOSPACE;

	    }

	    rc = lwfs_get_services(lwfs_cfg.ss_server_ids, num_servers, storage_svc);
	    if (rc != LWFS_OK) {
		log_error(debug_level, "unable to get storage service descriptions");
		return -EINVAL; 
	    }

	    lwfs_config_free(&lwfs_cfg);
	    print_args(result_fp, &args_info, "%");
	}

	/* distribute the services to the other processes */
	MPI_Bcast(&authr_svc, sizeof(lwfs_service), MPI_BYTE, 0, MPI_COMM_WORLD);
	MPI_Bcast(&num_servers, 1, MPI_INT, 0, MPI_COMM_WORLD);

	if (myrank != 0) {
	    storage_svc = calloc(num_servers, sizeof(lwfs_service));
	    if (!storage_svc) {
		log_error(debug_level, "unable to alloc storage services");
		return LWFS_ERR_NOSPACE;

	    }
	}

	MPI_Bcast(storage_svc, num_servers*sizeof(lwfs_service), 
		MPI_BYTE, 0, MPI_COMM_WORLD);


	MPI_Barrier(MPI_COMM_WORLD);

	log_debug(debug_level, "%d:starting...\n", myrank);

	MPI_Barrier(MPI_COMM_WORLD);
	if (myrank == 0) {
		fprintf(stderr, "\n");
	}

	buf = calloc(args_info.bytes_per_op_arg, 1);
	if (!buf) {
		fprintf(stderr, "%d could not allocate buffer\n", myrank);
		goto cleanup;
	}

	for (i=0; i<args_info.num_trials_arg; i++) {
		double t_total, t_open, t_start; 
		lwfs_cap cap; 
		lwfs_obj obj; 
		lwfs_cred cred; 
		lwfs_cid cid = LWFS_CID_ANY; 
		lwfs_service *svc = &storage_svc[myrank % num_servers];
		long bufsize = args_info.bytes_per_op_arg; 

		if (myrank == 0) {
			fprintf(stdout, "trial %d\n", i);
		}

		log_debug(debug_level, "opening");

		memset(&cred, 0, sizeof(lwfs_cred));
		memset(&cap, 0, sizeof(lwfs_cap));
		memset(&obj, 0, sizeof(lwfs_obj));
		cid = LWFS_CID_ANY;

		MPI_Barrier(MPI_COMM_WORLD); 
		t_start = MPI_Wtime();

		rc = lwfs_open(&authr_svc, &cred, 
				LWFS_CONTAINER_WRITE, svc, &cid, &cap, &obj); 
		if (rc != LWFS_OK) {
			log_error(debug_level, "could not open object");
			goto cleanup;
		}

		t_open = MPI_Wtime()-t_start; 

		log_debug(debug_level, "writing data");

		if (args_info.async_flag) {
			/* write the data using asynchronous writes */
			rc = write_async(buf, bufsize, 
					args_info.ops_per_trial_arg, 
					args_info.in_transit_arg, 
					&obj, &cap);
			if (rc != LWFS_OK) {
				goto cleanup;
			}
		}
		else {
			/* write the data using synchronous writes */
			rc = write_sync(buf, bufsize,  
				args_info.ops_per_trial_arg, &obj, &cap);
			if (rc != LWFS_OK) {
				goto cleanup;
			}
		}

		/* fsync */
		rc = lwfs_fsync_sync(NULL, &obj, &cap); 
		if (rc != LWFS_OK) {
			log_error(debug_level, "%d: unable to sync object");
			goto cleanup; 
		}

		t_total = MPI_Wtime() - t_start;

		if (args_info.remove_file_flag) {
			lwfs_remove_obj_sync(NULL, &obj, &cap);
		}

		print_stats(result_fp, &args_info, t_total, t_open);
	}

	/* print statistics */
	if (myrank == 0) {
		fclose(result_fp);
	}

cleanup:

	free(buf);
	free(storage_svc);

	MPI_Finalize();
	return rc; 
}
