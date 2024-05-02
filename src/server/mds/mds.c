/*-------------------------------------------------------------------------*/
/**  @file mds.c
 *   
 *   @brief The metadata server for the LWFS. 
 *   
 *   @author Ron Oldfield (raoldfi\@sandia.gov).
 *   $Revision: 791 $.
 *   $Date: 2006-06-29 21:32:24 -0600 (Thu, 29 Jun 2006) $.
 *
 */

#include <getopt.h>
#include <signal.h>
#include <portals/p30.h>
#include <unistd.h>
#include "logger/logger.h"
#include "comm/comm.h"
#include "comm/lwfs_ptls.h"
#include "lwfs.h"
#include "mds/mds_clnt.h"
#include "mds/mds_srvr.h"
#include "mds/mds_xdr.h"
#include "mds/mds_db.h"
#include "mds/mds_debug.h"

/* --------------- Globals --------------------- */
extern int exit_now;


/* ------------ Local defines ------------------ */
#define MDS_DB_DEFAULT_NAME	"/tmp/mds.db"


/* ----------- Local functions ----------------- */
typedef int (*mds_proc_t)(void *, void *); 
static void usage(char *pname);
static void sighandler(int sig);


/* ----------- Private Methods ----------------- */

static lwfs_return_code_t
proc_req(   void *req_buf, 
	    lwfs_request_header_t *header, 
	    mds_proc_t mds_proc,
	    void *decode_args,
	    int sizeofargs,
	    void *encode_res,
	    int sizeofres,
	    void *args,
	    void *result)
{

int rc;  /* return code */


    /* initialize arguments and results */
    memset(args, 0, sizeofargs);
    memset(result, 0, sizeofres);


    /* fetch the operation arguments */
    log_debug(mds_debug_level, "fetching args");
    rc = lwfs_comm_fetch_args(req_buf, header, (xdrproc_t)decode_args, args);
    if (rc != LWFS_OK) {
	log_fatal(mds_debug_level, "unable to fetch args");
	return rc; 
    }


    /*
    ** Process the request (print warning if method fails), but
    ** don't return error, because some operations are meant to fail
    */
    log_debug(mds_debug_level, "calling the server function");
    rc = mds_proc(args, result); 
    if (rc != LWFS_OK) {
	log_warn(mds_debug_level, "User op failed: %s", lwfs_err_str(rc));
    }


    /* send result back to client */
    log_debug(mds_debug_level, "sending results back to client");
    rc = lwfs_comm_send_result(&header->res_md, (xdrproc_t)encode_res, result);
    if (rc != LWFS_OK) {
	log_fatal(mds_debug_level, "unable to send result");
	return rc; 
    }


    /* free data structures created for the args and result */
    log_debug(mds_debug_level, "freeing args and result data structs");
    xdr_free((xdrproc_t)decode_args, (char *)args); 
    xdr_free((xdrproc_t)encode_res, (char *)result); 

    return LWFS_OK;

}  /* end of proc_req() */



/**
 * @brief Process a requests with a node and a name; return a code.
 *
 * @param req_buf  The encoded request buffer.
 * @param header   The request header.
 * @param mds_proc Pointer to the method to execute.
 */
static lwfs_return_code_t proc_Nn_req_N_rc_res(
		void *req_buf, 
		lwfs_request_header_t *header, 
		mds_proc_t mds_proc) 
{

mds_args_Nn_t args;
mds_res_N_rc_t result; 


    args.node.ID = NULL;
    args.node.cap = NULL;
    args.name = NULL;
    
    return proc_req(req_buf, header, mds_proc, &xdr_mds_args_Nn_t,
	sizeof(mds_args_Nn_t), &xdr_mds_res_N_rc_t, sizeof(mds_res_N_rc_t), &args,
	&result);

}  /* end of proc_Nn_req_N_rc_res() */



/**
 * @brief Process a requests with a single node and return a code.
 *
 * @param req_buf  The encoded request buffer.
 * @param header   The request header.
 * @param mds_proc Pointer to the method to execute.
 */
static lwfs_return_code_t
proc_N_req_note_rc_res( void *req_buf, 
			lwfs_request_header_t *header, 
			mds_proc_t mds_proc) 
{

mds_args_N_t args;
mds_res_note_rc_t result; 


    args.node.ID = NULL;
    args.node.cap = NULL;
    
    return proc_req(req_buf, header, mds_proc, &xdr_mds_args_N_t,
	sizeof(mds_args_N_t), &xdr_mds_res_note_rc_t, sizeof(mds_res_note_rc_t),
	&args, &result);

}  /* end of proc_N_req_note_rc_res() */



/**
 * @brief Process a requests with a single node and directory return.
 *
 * @param req_buf  The encoded request buffer.
 * @param header   The request header.
 * @param mds_proc Pointer to the method to execute.
 */
static lwfs_return_code_t
proc_N_req_dir_rc_res(  void *req_buf, 
			lwfs_request_header_t *header, 
			mds_proc_t mds_proc) 
{

mds_args_N_t args;
mds_readdir_res_t result;


    args.node.ID = NULL;
    args.node.cap = NULL;
    
    return proc_req(req_buf, header, mds_proc, &xdr_mds_args_N_t,
	sizeof(mds_args_N_t), &xdr_mds_readdir_res_t, sizeof(mds_readdir_res_t),
	&args, &result);

}  /* end of proc_N_req_dir_rc_res() */



/**
 * @brief Process a request with two nodes and a name; return a node and a 
return code. *
 * @param req_buf  The encoded request buffer.
 * @param header   The request header.
 * @param mds_proc Pointer to the method to execute.
 */
static lwfs_return_code_t
proc_NNn_req_N_rc_res(void *req_buf, 
		    lwfs_request_header_t *header, 
		    mds_proc_t mds_proc) 
{

mds_args_NNn_t args;
mds_res_N_rc_t result; 


    args.node1.ID = NULL;
    args.node1.cap = NULL;
    args.node2.ID = NULL;
    args.node2.cap = NULL;
    args.name = NULL;
	
    return proc_req(req_buf, header, mds_proc, &xdr_mds_args_NNn_t,
	sizeof(mds_args_NNn_t), &xdr_mds_res_N_rc_t, sizeof(mds_res_N_rc_t), &args,
	&result);

}  /* end of proc_NNn_req_N_rc_res() */



/** 
 * @brief Process a received requests.
 * 
 * Each incoming request arrives as a chunk of xdr data. This 
 * method decodes the mds_request_header, decodes the arguments, 
 * and calls the appropriate method to process the request. 
 *
 * @param req_buf        The encoded short-request buffer. 
 * @param short_req_len  The number of bytes in a short request. 
 */ 
static int process_request(char *req_buf, uint32_t short_req_len) {

	XDR xdrs; 
	int rc; 
	
	lwfs_request_header_t header; 

	/* create an xdr memory stream from the request buffer */
	xdrmem_create(&xdrs, 
			req_buf, 
			short_req_len, 
			XDR_DECODE);

	/* decode the request header */
	log_debug(mds_debug_level, "decoding header..."); 
	rc = xdr_lwfs_request_header_t(&xdrs, &header); 
	if (!rc) {
		log_fatal(mds_debug_level, "failed to decode header");
		abort();
	}

	/* figure out what type of request we have and process accordingly */
	rc = LWFS_OK;
	switch (header.op) {
		case MDSPROC_NULL: 
			log_debug(mds_debug_level, "received null request"); 
			break; 

		case MDS_GETATTR:
			log_debug(mds_debug_level, "received getattr request"); 
			rc = proc_N_req_note_rc_res(req_buf, &header, (mds_proc_t)&mds_getattr_srvr); 
			break; 

		case MDS_SETATTR:
			log_debug(mds_debug_level, "received setattr request"); 
			break; 

		case MDS_LINK:
			log_debug(mds_debug_level, "received link request"); 
			rc = proc_NNn_req_N_rc_res(req_buf, &header, (mds_proc_t)&mds_link_srvr); 
			break; 

		case MDS_LOOKUP2:
			log_debug(mds_debug_level, "received lookup2 request"); 
			rc = proc_N_req_note_rc_res(req_buf, &header, (mds_proc_t)&mds_lookup2_srvr); 
			break; 

		case MDS_LOOKUP: 
			log_debug(mds_debug_level, "received lookup request"); 
			rc = proc_Nn_req_N_rc_res(req_buf, &header, (mds_proc_t)&mds_lookup_srvr); 
			break; 

		case MDS_READDIR:
			log_debug(mds_debug_level, "received readdir request"); 
			rc = proc_N_req_dir_rc_res(req_buf, &header, (mds_proc_t)&mds_readdir_srvr); 
			break; 

		case MDS_RENAME:
			log_debug(mds_debug_level, "received rename request"); 
			rc = proc_NNn_req_N_rc_res(req_buf, &header, (mds_proc_t)&mds_rename_srvr); 
			break; 

		case MDS_GETCAPS:
			log_debug(mds_debug_level, "received getcaps request"); 
			break; 

		case MDS_GETACL:
			log_debug(mds_debug_level, "received getacl request"); 
			break; 

		case MDS_REGISTER:
			log_debug(mds_debug_level, "received register request"); 
			break; 

		case MDS_CREAT: 
			log_debug(mds_debug_level, "received create request"); 
			rc = proc_Nn_req_N_rc_res(req_buf, &header, (mds_proc_t)&mds_create_srvr); 
			break;

		case MDS_REMOVE:
			log_debug(mds_debug_level, "received remove request"); 
			rc = proc_N_req_note_rc_res(req_buf, &header, (mds_proc_t)&mds_remove_srvr); 
			break; 

		case MDS_MKDIR:
			log_debug(mds_debug_level, "received mkdir request"); 
			rc = proc_Nn_req_N_rc_res(req_buf, &header, (mds_proc_t)&mds_mkdir_srvr); 
			break; 

		case MDS_RMDIR:
			log_debug(mds_debug_level, "received rmdir request"); 
			rc = proc_N_req_note_rc_res(req_buf, &header, (mds_proc_t)&mds_rmdir_srvr); 
			break; 
		
		default:
			log_fatal(mds_debug_level, "unrecognized request");
			abort(); 
	}

	return rc;
}



/**
 * @brief The LWFS metadata server.
 */
int main(int argc, char **argv)
{
	extern char *optarg;
	extern int optind;
	int ch;
	static struct option long_options[] = {
		{"help", 0, 0, 6},
		{"dbclear", 0, 0, 7},
		{"dbrecover", 0, 0, 8},
		{"dbfname", 1, 0, 9},
		{"verbose", 1, 0, 10},
		{0, 0, 0, 0}
	};

	char *dbfname;
	int dbclear;
	int dbrecover;
	int verbose;
	int rc;
	char req_buf[2][LWFS_SHORT_REQUEST_SIZE]; 
	int req_count = 0; 
	int num_bufs = 2; 

	lwfs_local_md_t req_local_md; 
	lwfs_remote_md_t req_remote_md; 

	ptl_process_id_t match_id; 
	uint32_t match_bits; 

	/* Set some defaults */
	exit_now= 0;
	dbfname= MDS_DB_DEFAULT_NAME;
	dbclear= 0;
	dbrecover= 0;
	verbose= 0;

	/* Process command line options */
	while (1)   {
		ch = getopt_long_only(argc, argv, "", long_options, NULL);
		if (ch == -1)   {
			/* Done proecssing command line options */
			break;
		}

		switch (ch) {
			case 6: /* help */
				usage(argv[0]);
				return LWFS_OK;

			case 7: /* dbclear */
				dbclear= 1;
				break;

			case 8: /* dbrecover */
				dbrecover= 1;
				break;

			case 9: /* dbfname */
				dbfname= optarg;
				break;

			case 10: /* verbose */
				verbose= atoi(optarg);
				break;

			default:
				usage(argv[0]);
				exit(-1);
		}

	}


	/* initialize the logger */
	logger_set_file(stdout); 
	if (verbose == 0)   {
	    logger_set_default_level(LOG_OFF); 
	} else if (verbose > 5)   {
	    logger_set_default_level(LOG_ALL); 
	} else   {
	    logger_set_default_level(verbose - LOG_OFF); 
	}

        fprintf(stderr, "calling log function\n");
        log_debug(verbose,"initialize MDS server");
        fprintf(stderr, "finished calling log function\n");

	/* initialze the communication layer */
	lwfs_comm_init_server();

        log_debug(verbose, "initialize db");

	/* initialize the database */
	rc= mds_init_db(dbfname, dbclear, dbrecover);
	if (rc != LWFS_OK)   {
	    return rc;
	}

	/* Install the signal handler */
	signal(SIGINT, sighandler);

	req_count = 0;

	/* initialize the match_id (anyone can connect) */
	match_id.nid = PTL_NID_ANY;
	match_id.pid = PTL_PID_ANY; 

	match_bits = 0; 

	/* post memory descriptor for next request */
	rc = lwfs_ptl_post_md(
			SRVR_REQUEST_PORTAL_INDEX,
			req_buf[req_count],
			LWFS_SHORT_REQUEST_SIZE,
			PTL_MD_OP_PUT,
			match_id,
			match_bits,
			&req_remote_md,   /* what others use */
			&req_local_md);   /* used to check status */
	if (rc != LWFS_OK) {
		log_error(mds_debug_level, "unable to post md for next request");
		return rc; 
	}
	

	/* SIGINT (Ctrl-C) will get us out of this loop */
	while (!exit_now) {

		/* wait for the next encoded request buffer */
		log_debug(mds_debug_level, 
				"waiting for next request...");
		rc = lwfs_ptl_wait(&req_local_md); 
		if (exit_now)   {
		    log_debug(mds_debug_level, "Exiting");
		    break;
		}
		if (rc != LWFS_OK) {
			log_fatal(mds_debug_level, 
					"failed waiting for next request");
			return rc; 
		}

		/* post memory descriptor for next request */
		rc = lwfs_ptl_post_md(
				SRVR_REQUEST_PORTAL_INDEX,
				req_buf[(req_count+1) % num_bufs],
				LWFS_SHORT_REQUEST_SIZE,
				PTL_MD_OP_PUT,
				match_id,
				match_bits,
				&req_remote_md,   /* what others use */
				&req_local_md);   /* used to check status */

		if (rc != LWFS_OK) {
			log_error(mds_debug_level, "mds: unable to post md for next request");
			return rc; 
		}

		/* FIXME: spawn a thread to process the request */
		log_debug(mds_debug_level, "mds main: processing request...");

		rc = process_request(req_buf[req_count % num_bufs], 
				LWFS_SHORT_REQUEST_SIZE); 
		if (rc != LWFS_OK) {
			log_fatal(mds_debug_level, "main: unable to process request");
			return rc; 
		}

		req_count++;
	}

	/* Close the database */
	mds_exit_db();

	return LWFS_OK;
}

static void
usage(char *pname)
{

    fprintf(stderr, "Usage: %s [-help] [-dbfname <name>] [-dbclear] [-dbrecover]\n", pname);    
    fprintf(stderr, "           [-verbose <lvl>]\n");    
    fprintf(stderr, "    -help           This usage message\n");    
    fprintf(stderr, "    -dbfname <name> specifies the database name. Default: \"%s\"\n", MDS_DB_DEFAULT_NAME);    
    fprintf(stderr, "    -dbclear        clears (empties) the database before use\n");    
    fprintf(stderr, "                     WARNING: All metadata will be lost!\n");    
    fprintf(stderr, "    -dbrecover      Recovers the database after a crash\n");    
    fprintf(stderr, "    -verbose <lvl>  Set verbosity to level <lvl>\n");
}  /* end of usage() */


static void
sighandler(int sig)
{

    signal(SIGINT, SIG_IGN);
    log_warn(mds_debug_level, "Got exit signal");
    exit_now= 1;

}  /* end of sighandler() */


/*-------------------------------------------------------------------------
 * Revision history
 *
 * $Log$
 * Revision 1.23  2004/09/13 15:18:07  raoldfi
 * added a "_t" to all lwfs defined types.
 *
 * Revision 1.22  2004/09/01 22:41:21  raoldfi
 * Fixed dependency problems with Makefile.am files.
 *
 * Revision 1.21  2004/08/11 17:06:34  raoldfi
 * Made lots of minor changes:
 *   - removed the include directory (not needed),
 *   - fixed #include statements that pointed to the include directory,
 *   - fixed paths in Makefile.am to include the full path name.
 *
 * Revision 1.20  2004/08/06 22:54:39  rolf
 *
 *     Added lookup2(). When we already know the node ID, but want to make
 *     sure it is still there or retrieve some information about it; like
 *     the notepad.
 *
 *     This may be replaced with mds_getattr() at some point in the
 *     near future.
 *
 * Revision 1.19  2004/07/29 16:58:00  rolf
 *
 *     Added readdir capability (we need that for ls ;-)
 *
 * Revision 1.18  2004/07/26 21:35:57  rolf
 *
 *         Some clean-up and code for hard links. Links seem to work,
 * 	but more debugging and clean-up is required.
 *
 * Revision 1.17  2004/07/25 23:07:01  rolf
 *
 *     - major reorganization and simplification of code in mds_clnt.c
 * 	and mds.c
 *
 *     - renamed some of the arguments and results passed between
 * 	client and MDS server to more closely reflect recent
 * 	nomenclature definitions.
 *
 * Revision 1.16  2004/07/23 23:03:08  rolf
 *
 *     Fixed a bunch of bugs. mds_create still doesn't run all teh
 *     way trough, though.
 *
 * Revision 1.15  2004/06/23 22:43:48  rolf
 *
 *     Added code for mds_getattr() and changed the data that
 *     is passed between the client and the server. This is a
 *     major change and it doesn't work yet. (It compile, though ;-)
 *     Everything before this check-in is tagged BEFORE_MDS_XDR_CLEANUP
 *
 * Revision 1.14  2004/03/31 18:03:38  rolf
 *
 *     Need to call the correct decode function!
 *     (This took for ever to find :-(
 *
 * Revision 1.13  2004/03/29 17:41:59  rolf
 *
 *     Turned lwfs_err_str from an array into a function. That way we don't
 *     segfault when we call it with an out of bounds value.
 *
 * Revision 1.12  2004/03/08 18:37:16  rolf
 *
 *     - Initial code for rename
 *     - always use long options (gets rid of -- and use - for options)
 *
 * Revision 1.11  2004/02/29 04:58:33  rolf
 *
 *     - Added the rmdir function
 *     - Lots more documentation
 *     - Fixed some typos
 *     - Not all directory operations need "name" as an argument.
 * 	Often we have the object ID, which is enough to find the
 * 	object, so passing along the name is superfluous. This
 * 	needs more work to be implemented properly...
 *
 * Revision 1.10  2004/02/26 19:26:19  rolf
 *
 *     Now we can create objects and directories, look up by name and
 *     object ID, and remove objects.
 *
 *     Expanded the little test program mds_create a little bit to
 *     try out all these functions.
 *
 * Revision 1.9  2004/02/23 17:05:07  raoldfi
 * Added an input log_level variable to the logging interface
 * and added function name, file name, and line number to the
 * output of a log message.
 *
 * Revision 1.8  2004/02/19 22:52:49  raoldfi
 * *** empty log message ***
 *
 * Revision 1.7  2004/02/18 17:16:19  rolf
 *
 *     Added a -v and --verbose option. --verbose <lvl> sets the
 *     logging to level <lvl>. I.e. --verbose 1 logs everything up
 *     to LOG_DEBUG. 0 means no logging at all, and 5 and above logs
 *     everything.
 *
 *     -v increments the logging level each time it is listed on the
 *     command line.
 *
 *     The default is no logging.
 *
 * Revision 1.6  2004/02/13 23:01:32  rolf
 *
 *     The files needed to link into the Berkeley database. This is
 *     just a start. Not much works yet, and I'll probably change how
 *     things are stored in the database.
 *
 *     mds now accepts command line options:
 *
 *     mds [--help] [--dbfname <name>] [--dbclear] [--dbrecover]
 * 	--help              This usage message
 * 	--dbfname <name>    specifies the database name. Default: /tmp/mds.db
 * 	--dbclear           clears (empties) the database before use
 * 			    WARNING: All metadata will be lost!
 * 	--dbrecover         Recovers the database after a crash
 *
 * Revision 1.5  2004/02/06 23:07:03  raoldfi
 * added more debugging information to mds
 *
 * Revision 1.4  2004/02/03 18:22:06  raoldfi
 * *** empty log message ***
 *
 * Revision 1.3  2004/02/03 18:03:25  raoldfi
 * major changes to directory file names.
 *
 *
 *
 */

