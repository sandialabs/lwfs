/* hello world server */

#include <unistd.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include PORTALS_HEADER
#include PORTALS_NAL_HEADER
#include PORTALS_RT_HEADER

#include "support/timer/timer.h"
#include "portals-xfer-opts.h"

static int debug=0; 

/* begin Cray extensions */
#ifndef PTL_IFACE_DUP
#define PTL_IFACE_DUP PTL_OK
#endif

#ifndef PTL_MD_EVENT_AUTO_UNLINK_ENABLE
#define PTL_MD_EVENT_AUTO_UNLINK_ENABLE 0
#endif

#ifndef PTL_MD_EVENT_MANUAL_UNLINK_ENABLE
#define PTL_MD_EVENT_MANUAL_UNLINK_ENABLE 0
#endif

#ifndef HAVE_PTLERRORSTR
#define PtlErrorStr(a) ""
#endif

#ifndef HAVE_PTLNIFAILSTR
#define PtlNIFailStr(a,b) ""
#endif

#ifndef HAVE_PTLEVENTKINDSTR
#define PtlEventKindStr(a) ""
#endif

#ifndef HAVE_PTL_EQ_HANDLER_T
typedef void (*ptl_eq_handler_t)(ptl_event_t *event);
#endif 

#ifndef PTL_EQ_HANDLER_NONE
#define PTL_EQ_HANDLER_NONE (ptl_eq_handler_t)NULL
#endif
/* begin Cray extensions */


/* The UTCP NAL requires that the application defines where the Portals
 * API and library should send any output.
 */
FILE *utcp_api_out;
FILE *utcp_lib_out; 

typedef struct {
	int int_val;
	float float_val;
	double double_val;
} data_t; 



static int print_args(FILE *fp, const char *prefix, struct gengetopt_args_info *args_info) 
{
    time_t now; 

    /* get the current time */
    now = time(NULL);

    fprintf(fp, "%s -----------------------------------\n", prefix);
    fprintf(fp, "%s \tMeasure Portals throughput (%s)\n", prefix,
	    args_info->server_flag? "server": "client");
    fprintf(fp, "%s \t%s", prefix, asctime(localtime(&now)));
    fprintf(fp, "%s ------------  ARGUMENTS -----------\n", prefix);
    if (args_info->debug_flag) {
	fprintf(fp, "%s \t--debug\n", prefix);
    }
    fprintf(fp, "%s \t--server-nid = %lld\n", 
	    prefix, (long long)args_info->server_nid_arg);
    fprintf(fp, "%s \t--server-pid = %d\n", 
	    prefix, args_info->server_pid_arg);
    if (!args_info->server_flag) {
	fprintf(fp, "%s \t--len = %d\n", prefix, args_info->len_arg);
    }
    fprintf(fp, "%s \t--count = %d\n", prefix, args_info->count_arg);
    fprintf(fp, "%s \t--num-reqs = %d\n", prefix, args_info->num_reqs_arg);
    fprintf(fp, "%s \t--result-file = \"%s\"\n", prefix, args_info->result_file_arg);
    fprintf(fp, "%s \t--result-file-mode = \"%s\"\n", prefix, args_info->result_file_mode_arg);
    fprintf(fp, "%s -----------------------------------\n", prefix);

    return 0;
}


static void output_stats(
		FILE *result_fp,
		struct gengetopt_args_info *args_info,
		double t_total)
{
    double t_total_max = t_total;
    double t_total_min = t_total;
    double t_total_sum = t_total;
    int total_reqs = args_info->num_reqs_arg;
    int myrank=0; 
    int np=1;
    static int first=1; 

    if (myrank == 0) {
	double t_total_avg = t_total_sum/np;
	double nbytes = total_reqs*(sizeof(int) 
		+ (args_info->len_arg+1)*sizeof(data_t));

	if (first) {
	    time_t rawtime;
	    time(&rawtime);

	    fprintf(result_fp, "%s ----------------------------------------------------\n", "%");
	    fprintf(result_fp, "%s portals-xfer\n", "%");
	    fprintf(result_fp, "%s %s", "%", ctime(&rawtime));
	    fprintf(result_fp, "%s ----------------------------------------------------\n", "%");
	    fprintf(result_fp, "%s column   description\n", "%");
	    fprintf(result_fp, "%s ----------------------------------------------------\n", "%");
	    fprintf(result_fp, "%s   0     number of clients\n", "%");
	    fprintf(result_fp, "%s   1     total ops\n","%");
	    fprintf(result_fp, "%s   2     data structures/op\n", "%");
	    fprintf(result_fp, "%s   3     aggregate bytes\n","%");
	    fprintf(result_fp, "%s   4     min time (sec)\n","%");
	    fprintf(result_fp, "%s   5     max time (sec)\n","%");
	    fprintf(result_fp, "%s   6     avg time (sec)\n","%");
	    fprintf(result_fp, "%s   7     min throughput (MB/sec)\n","%");
	    fprintf(result_fp, "%s   8     max throughput (MB/sec)\n","%");
	    fprintf(result_fp, "%s   9     avg throughput (MB/sec)\n","%");
	    fprintf(result_fp, "%s ----------------------------------------------------\n", "%");
	    first = 0; 
	}

	/* print the row */
	fprintf(result_fp, "%04d   ", np);    
	fprintf(result_fp, "%09d   ", total_reqs); 
	fprintf(result_fp, "%09d   ", args_info->len_arg);    
	fprintf(result_fp, "%1.6e  ", nbytes); 
	fprintf(result_fp, "%1.6e  ", t_total_min); 
	fprintf(result_fp, "%1.6e  ", t_total_max); 
	fprintf(result_fp, "%1.6e  ", t_total_avg); 
	fprintf(result_fp, "%1.6e  ", nbytes/(t_total_max*1024*1024)); 
	fprintf(result_fp, "%1.6e  ", nbytes/(t_total_min*1024*1024)); 
	fprintf(result_fp, "%1.6e\n", nbytes/(t_total_avg*1024*1024)); 
	fprintf(result_fp, "%s ----------------------------------------------------\n", "%");
	fflush(result_fp);
    }
}


/*  prepare for unexpected requests  */
static int post_incoming_buffer(
	ptl_process_id_t match_id,
	void *buf,
	int bufsize,
	int threshold,
	ptl_handle_ni_t ni_h,
	ptl_pt_index_t portal_index,
	ptl_match_bits_t match_bits, 
	ptl_handle_eq_t *eq_h)
{
    int rc = 0; 

    ptl_md_t md; 
    ptl_handle_me_t me_h; 
    ptl_handle_md_t md_h; 

    ptl_process_id_t my_id;
    PtlGetId(ni_h, &my_id); 

    if (debug > 0) {
	fprintf(stderr, "Posting buffer for (nid=%lu, pid=%u) to "
		"(nid=%lu, pid=%d, index=%d, match_bits=%d, size=%d)\n", 
		(unsigned long)match_id.nid, match_id.pid, 
		(unsigned long)my_id.nid, my_id.pid, portal_index, (int)match_bits, bufsize);
    }

    /* initialize the md for the incoming buffer */
    memset(&md, 0, sizeof(ptl_md_t)); 
    md.start = buf; 
    md.length = bufsize; 
    md.threshold = threshold;  
    md.options = PTL_MD_OP_PUT | PTL_MD_OP_GET | PTL_MD_TRUNCATE;
    md.user_ptr = NULL;  /* unused */

    /* create an event queue for this message */
    memset(eq_h, 0, sizeof(ptl_handle_eq_t));
    rc = PtlEQAlloc(ni_h, 5, PTL_EQ_HANDLER_NONE, eq_h);
    if (rc != PTL_OK) {
	fprintf(stderr, "PtlEQAlloc() failed: %s (%d)\n",
		PtlErrorStr(rc), rc); 
	exit(-1); 
    }
    md.eq_handle = *eq_h; 

    /* Create a match entry for this message. */
    rc = PtlMEAttach(ni_h, portal_index, match_id, match_bits, 0,
	    PTL_UNLINK, PTL_INS_AFTER, &me_h); 
    if (rc != PTL_OK) {
	fprintf(stderr, "PtlMEAttach: %s\n", PtlErrorStr(rc));
	exit(-1); 
    }

    /* attach the memory descriptor to the match entry */
    rc = PtlMDAttach(me_h, md, PTL_UNLINK, &md_h); 
    if (rc != PTL_OK) {
	fprintf(stderr, "PtlMDAttach: %s\n", PtlErrorStr(rc)); 
	exit(-1); 
    }

    return rc; 
}

static int xfer_server_fini(ptl_handle_eq_t eq_h)
{
	int rc = 0;

	/* free the event queue */
	rc = PtlEQFree(eq_h);
	if (rc != PTL_OK) {
		fprintf(stderr, "PtlEQFree() failed: %s (%d)\n",
				PtlErrorStr(rc), rc); 
		abort();
	}

	return rc; 
}

static int wait_for_put(
	ptl_handle_ni_t ni_h,
	ptl_handle_eq_t eq_h,
	int unlink_flag,
	ptl_event_t *event)
{
	int rc = 0; 
	int got_put_start = 0;
	int got_put_end = 0;
	int got_unlink = 0;
	int done = 0;

	while (!done) {
		ptl_sr_value_t status; 

		/* check for dropped message */
		PtlNIStatus(ni_h, PTL_SR_DROP_COUNT, &status);

		if ((int)status > 0) {
			fprintf(stderr, "Detected dropped message: wait_for_put, dropped=%d\n",
					(int)status);
			return -1; 
		}

		/* wait for a PTL_EVENT_PUT_START */
		if (debug > 0) {
			fprintf(stderr, "waiting for event, dropped count=%d\n",(int)status);
		}

		rc = PtlEQWait(eq_h, event);
		if (rc != PTL_OK) {
			fprintf(stderr, "Wait failed. rc = %d\n", rc);
			abort();
		}

		switch(event->type) {
			case PTL_EVENT_PUT_START: 
				if (debug > 0) fprintf(stderr, "Got PUT_START\n");
				got_put_start = 1;
				break;
			case PTL_EVENT_PUT_END:
				if (debug > 0) fprintf(stderr, "Got PUT_END\n");
				got_put_end = 1;
				break;
			case PTL_EVENT_UNLINK:
				if (debug > 0) fprintf(stderr, "Got PTL_UNLINK\n");
				got_unlink = 1;
				break;
			default:
				fprintf(stderr, "recv_message: unexpected event (%d)\n", event->type);
				abort();
		}

		if (got_put_start && got_put_end) {
		    if (unlink_flag) {
			done = got_unlink;
		    }
		    else {
			done = 1; 
		    }
		}
	}

	return 0; 
}

static int recv_len(
	ptl_handle_ni_t ni_h,
	ptl_handle_eq_t eq_h,
	ptl_process_id_t *sender)
{
	int rc; 
	ptl_event_t event; 
	int *len_ptr; 
	int unlink_flag = 0; 

	/* wait for a client to put the array-len to the incoming buffer */
	rc = wait_for_put(ni_h, eq_h, unlink_flag, &event); 
	if (rc != 0) {
		fprintf(stderr, "Failed waiting for a PtlPut()\n");
		abort();
	}

	/* set the sender */
	sender->pid = event.initiator.pid;
	sender->nid = event.initiator.nid;

	len_ptr = (int *)(event.md.start + event.offset);
	return *len_ptr; 
}


/*  Fetch a message. */
static int get_message(
	void *buf,
	int bufsize,
	ptl_handle_ni_t ni_h,
	ptl_process_id_t match_id,
	ptl_pt_index_t portal_index,
	ptl_match_bits_t match_bits)
{
	int rc; 
	int got_reply_start = 0;
	int got_reply_end = 0;
	int got_unlink = 0;
	int done = 0;

	ptl_md_t md; 
	ptl_handle_eq_t eq_h; 
	ptl_handle_md_t md_h; 
	ptl_event_t event; 


	/* create an event queue for this message */
	rc = PtlEQAlloc(ni_h, 5, PTL_EQ_HANDLER_NONE, &eq_h);
	if (rc != PTL_OK) {
		fprintf(stderr, "PtlEQAlloc() failed: %s (%d)\n",
				PtlErrorStr(rc), rc); 
		exit(-1); 
	}

	/* initialize the md for the incoming buffer */
	memset(&md, 0, sizeof(ptl_md_t)); 
	md.start = buf; 
	md.length = bufsize; 
	md.threshold = 1;  /* only expect one operation */
	md.options = 0;    /* unimportant */
	md.user_ptr = NULL;  /* unused */
	md.eq_handle = eq_h; 

	/* bind the memory descriptor */
	rc = PtlMDBind(ni_h, md, PTL_UNLINK, &md_h); 
	if (rc != PTL_OK) {
		fprintf(stderr, "PtlMDBind: %s\n", PtlErrorStr(rc));
		exit(-1); 
	}

	if (debug > 0) {
		fprintf(stderr, "Getting buffer from nid=%lu, pid=%d, index=%d, match_bits=%d\n",
				(unsigned long)match_id.nid, match_id.pid, portal_index, (int)match_bits);
	}

	/* get the data */
	rc = PtlGet(md_h, match_id, portal_index, 0, match_bits, 0);
	if (rc != PTL_OK) {
		fprintf(stderr, "PtlGet: %s\n", PtlErrorStr(rc)); 
		exit(-1); 
	}


	/* wait for events that signal completion */
	while (!done) {
		ptl_sr_value_t status; 

		/* check for dropped message */
		PtlNIStatus(ni_h, PTL_SR_DROP_COUNT, &status);

		if ((int)status > 0) {
			fprintf(stderr, "Detected dropped message: get_message, dropped=%d\n",
				(int)status);
			abort();
		}

		/* wait for a PTL_EVENT_PUT_START */
		if (debug > 0) fprintf(stderr, "waiting for event, dropped count=%d\n",(int)status);
		rc = PtlEQWait(eq_h, &event);
		if (rc != PTL_OK) {
			fprintf(stderr, "Wait failed. rc = %d\n", rc);
			abort();
		}

		switch (event.type) {

			case PTL_EVENT_REPLY_START:
				if (debug > 0) fprintf(stderr, "Got REPLY_START\n");
				got_reply_start = 1;
				if (event.ni_fail_type != PTL_NI_OK) {
					fprintf(stderr, "failed on reply start: ni_fail_type=%d\n",
							event.ni_fail_type);
					abort();
				}
				break; 

			case PTL_EVENT_REPLY_END:
				if (debug > 0) fprintf(stderr, "Got REPLY_END\n");
				got_reply_end = 1;
				if (event.ni_fail_type != PTL_NI_OK) {
					fprintf(stderr, "failed on reply end: ni_fail_type=%d\n",
							event.ni_fail_type);
					abort();
				}
				break;

			case PTL_EVENT_UNLINK:
				if (debug > 0) fprintf(stderr, "Got UNLINK\n");
				got_unlink = 1;
				if (event.ni_fail_type != PTL_NI_OK) {
					fprintf(stderr, "failed on unlink: ni_fail_type=%d\n",
							event.ni_fail_type);
					abort();
				}
				break;

			default:
				fprintf(stderr, "unexpected event=%d",
						event.type);
				abort();
		}

		if (got_reply_start && got_reply_end && got_unlink) {
			done = 1;
		}
	}

	/* free the event queue */
	rc = PtlEQFree(eq_h);
	if (rc != PTL_OK) {
		fprintf(stderr, "PtlEQFree() failed: %s (%d)\n",
				PtlErrorStr(rc), rc); 
		abort();
	}

	return 0; 
}



/* sends a single message to a server */
static int send_message(
	ptl_process_id_t dest, 
	void *buf, 
	int bufsize,
	ptl_handle_ni_t ni_h,
	ptl_pt_index_t portal_index,
	ptl_match_bits_t match_bits)
{
	int rc; 
	int got_send_start = 0; 
	int got_send_end = 0;
	int got_ack = 0;
	int got_unlink = 0; 
	int done = 0; 

	ptl_md_t md; 
	ptl_handle_eq_t eq_h; 
	ptl_handle_md_t md_h; 
	ptl_event_t event;

	/* initialize the md for the outgoing buffer */
	memset(&md, 0, sizeof(ptl_md_t)); 
	md.start = buf; 
	md.length = bufsize;
	md.threshold = 2;
	md.options = PTL_MD_OP_PUT | PTL_MD_TRUNCATE;
	md.user_ptr = NULL;  /* unused */

	/* create an event queue for the outgoing buffer */
	rc = PtlEQAlloc(ni_h, 5, PTL_EQ_HANDLER_NONE, &eq_h);
	if (rc != PTL_OK) {
		fprintf(stderr, "PtlEQAlloc() failed: %s (%d)\n",
				PtlErrorStr(rc), rc); 
		exit(-1); 
	}
	md.eq_handle = eq_h; 

	/* bind the memory descriptor */ 
	rc = PtlMDBind(ni_h, md, PTL_UNLINK, &md_h); 
	if (rc != PTL_OK) {
		fprintf(stderr, "PtlMDBind() failed: %s (%d)\n",
				PtlErrorStr(rc), rc); 
		exit(-1); 
	}

	/* "put" the message on the server's memory descriptor */
	rc = PtlPut(md_h, PTL_ACK_REQ, dest, portal_index, 0, match_bits, 0, 0); 
	if (rc != PTL_OK) {
		fprintf(stderr, "PtlPut() failed: %s (%d)\n",
				PtlErrorStr(rc), rc);
		exit(-1);
	}

	/* wait for the events to complete the send */
	got_send_start = 0; 
	got_send_end = 0;
	got_ack = 0;
	got_unlink = 0; 

	while (!done) {
		ptl_sr_value_t status; 

		/* check for dropped message */
		PtlNIStatus(ni_h, PTL_SR_DROP_COUNT, &status);

		if ((int)status > 0) {
			fprintf(stderr, "Detected dropped message: send_message, dropped=%d\n",
				(int)status);
			abort();
		}

		/* wait for a */
		if (debug > 0) {
			fprintf(stderr, "waiting for event, dropped count=%d\n",(int)status);
		}
		rc = PtlEQWait(eq_h, &event);
		if (rc != PTL_OK) {
			fprintf(stderr, "PtlEQWait (START) failed. rc = %d\n", rc);
			abort();
		}

		switch (event.type) {
			case PTL_EVENT_SEND_START:
				if (debug > 0) fprintf(stderr, "Got SEND_START\n");
				got_send_start = 1;
				break;
			case PTL_EVENT_SEND_END:
				if (debug > 0) fprintf(stderr, "Got SEND_END\n");
				got_send_end = 1;
				break;
			case PTL_EVENT_ACK:
				if (debug > 0) fprintf(stderr, "Got SEND_ACK\n");
				got_ack = 1;
				break;
			case PTL_EVENT_UNLINK:
				if (debug > 0) fprintf(stderr, "Got SEND_UNLINK\n");
				got_unlink = 1;
				break;
			default:
				fprintf(stderr, "send_message: unexpected event");
				abort();
		}

		if (got_send_start && got_send_end && got_ack && got_unlink) {
			done = 1; 
		}
	}

	if (debug > 0) fprintf(stderr, "message sent\n");

	/* free the event queue */
	rc = PtlEQFree(eq_h);
	if (rc != PTL_OK) {
		fprintf(stderr, "PtlEQFree() failed: %s (%d)\n",
				PtlErrorStr(rc), rc); 
		abort();
	}

	return 0;
}




/*  Receive one message from the client, return the same 
 *  message, then exit.  We're trying to model the behavior 
 *  of LWFS RPC. */
static int xfer_server(
	ptl_handle_ni_t ni_h,
	ptl_pt_index_t portal_index,
	ptl_match_bits_t match_bits,
	ptl_handle_eq_t eq_h)
{
	int len; 
	data_t *buf = NULL;
	ssize_t nbytes; 
	ptl_process_id_t client; 

	/* get the length of the buffer from a client */
	len = recv_len(ni_h, eq_h, &client); 


	if (debug > 0) {
		fprintf(stderr, "Server Received Length=%d\n",len);
	}

	/* allocate space for the incoming buffer */
	nbytes = len*sizeof(data_t);
	buf = (data_t *)malloc(nbytes);

	/* fetch the buffer from the client */
	if (debug > 0) {
		fprintf(stderr, "Fetching message from nid=%lu, pid=%d\n",
				(unsigned long)client.nid, client.pid);
	}

	get_message(buf, nbytes, ni_h, client, 
			portal_index, match_bits);

	if (debug > 0) {
		fprintf(stderr, "Server Got buffer, sending result (int=%d, float=%g, double=%g)\n",
				buf[len-1].int_val, buf[len-1].float_val, buf[len-1].double_val);
	}

	/* send the result */
	send_message(client, &buf[len-1], sizeof(data_t), ni_h, 
			portal_index, match_bits+1); 

	if (debug > 0) fprintf(stderr, "Server Sent result\n");

	/* free the buffer */
	free(buf);

	return 0; 
}

/* sends a single message to a server */
static int xfer_client(
	data_t *buf, 
	int len, 
	ptl_process_id_t server, 
	ptl_handle_ni_t ni_h,
	ptl_pt_index_t portal_index,
	ptl_match_bits_t match_bits)
{
	int rc = 0;
	ssize_t nbytes; 
	data_t result; 
	ptl_handle_eq_t eq_h, result_eq_h;
	ptl_event_t event; 
	int unlink_flag = 1; 


	/* post a buffer so the server can GET the data (expect one operation) */
	nbytes = len*sizeof(data_t); 
	post_incoming_buffer(server, buf, nbytes, 1, ni_h, portal_index, match_bits, &eq_h);

	/* post a buffer for the result */
	post_incoming_buffer(server, &result, sizeof(data_t), 1, ni_h, portal_index, match_bits+1, &result_eq_h);

	if (debug > 0) {
		fprintf(stderr, "Client: data-buffer is ready (int=%d, float=%g, double=%g)\n",
				buf[len-1].int_val, buf[len-1].float_val, buf[len-1].double_val);
	}


	/* send the message length to the server (it should have a buffer waiting) */
	send_message(server, &len, sizeof(int), ni_h, 
			portal_index, match_bits); 

	if (debug > 0) {
		fprintf(stderr, "Client: sent length=%d\n",len);


		fprintf(stderr, "Client waiting for result from (nid=%lu, pid=%d), "
				"index=%d, match_bits=%d \n",
				(unsigned long)server.nid, server.pid,
				portal_index, (int)(match_bits+1));
	}

	
	/* wait for result */

	rc = wait_for_put(ni_h, result_eq_h, unlink_flag, &event); 
	if (rc != 0) {
		fprintf(stderr, "Failed waiting for a PtlPut()\n");
		abort();
	}
			
	if (debug > 0) {
		fprintf(stderr, "Client received result from (nid=%lu, pid=%d), "
				"data_t=(int=%d, float=%f, double=%g)\n",
				(unsigned long)event.initiator.nid, event.initiator.pid,
				result.int_val, result.float_val, result.double_val);
	}


	/* need to free the result event queue allocated for the incoming buffer */
	rc = PtlEQFree(result_eq_h); 
	if (rc != PTL_OK) {
		fprintf(stderr, "PtlEQFree() failed: %s (%d)\n",
				PtlErrorStr(rc), rc); 
		abort();
	}

	/* need to free the event queue allocated for the incoming buffer */
	rc = PtlEQFree(eq_h); 
	if (rc != PTL_OK) {
		fprintf(stderr, "PtlEQFree() failed: %s (%d)\n",
				PtlErrorStr(rc), rc); 
		abort();
	}

	return 0;
}






int main(int argc, char **argv)
{
    struct gengetopt_args_info args_info; 
    int num_if; 
    FILE *result_fp = stdout; 

    /* fixed for all runs */
    ptl_pt_index_t portal_index=4;   
    ptl_match_bits_t match_bits=5; 

    ptl_process_id_t my_id; 
    ptl_handle_ni_t ni_h;
    ptl_pid_t pid; 

    int myrank=0; 

    /* these must be set, otherwisze, PtlNIInit segfaults */
    utcp_api_out = stdout; 
    utcp_lib_out = stdout; 

    /* parse command line options */
    if (cmdline_parser(argc, argv, &args_info) != 0) {
	exit(1);
    }

    debug = args_info.debug_flag;

    /* initialize result file */
    if (args_info.result_file_arg != NULL) {
	result_fp = fopen(args_info.result_file_arg, args_info.result_file_mode_arg); 
	if (result_fp == NULL) {
	    fprintf(stderr, "could not open \"%s\": using stdout\n", 
		    args_info.result_file_arg);
	    result_fp = stdout; 
	}
    }

    /* Initialize the Portals library */
    if (PtlInit(&num_if) != PTL_OK) {
	fprintf(stderr, "PtlInit() failed\n");
	exit(1); 
    }

    /* Initialize the interface (PIDS client=98, server=99) */
    pid = (args_info.server_flag)? 
	args_info.server_pid_arg : args_info.server_pid_arg-(myrank+1); 
    if (PtlNIInit(PTL_IFACE_DEFAULT, pid, 
		NULL, NULL, &ni_h) != PTL_OK) {
	fprintf(stderr, "PtlNIInit() failed\n"); 
	exit(1); 
    }

    /* tell portals to output debug statements */
    PtlNIDebug(ni_h, 0xfffffdff); 


    /* Get and print the ID of this process */
    if (PtlGetId(ni_h, &my_id) != PTL_OK) {
	printf("PtlGetId() failed.\n");
	abort();
    }

    /* set server and client nid's (if unset) */
    if (args_info.server_nid_arg == 0) {
	args_info.server_nid_arg = my_id.nid; 
	args_info.server_pid_arg = my_id.pid; 
    }


    /***************** EVERYTHING ABOVE USED FOR BOTH SENDER AND RECEIVER *******/

    /* server code */
    if (args_info.server_flag) {
	int np = 1; 
	int numreqs = 0; 
	args_info.server_nid_arg = my_id.nid; 
	args_info.server_pid_arg = my_id.pid; 
	ptl_handle_eq_t eq_h; 
	int *len_buf = (int *)malloc(args_info.count_arg*sizeof(int));
	int threshold; 

	/* maxreqs is the number of reqs generated by the client */
	int maxreqs = args_info.num_reqs_arg*(args_info.count_arg+1); 

	if (maxreqs == 0) {
	    threshold = PTL_MD_THRESH_INF;
	}
	else {
	    threshold = maxreqs; 
	}


	ptl_process_id_t match_id; 
	match_id.nid = PTL_NID_ANY;
	match_id.pid = PTL_PID_ANY;

	print_args(stdout, "%", &args_info); 

	if (debug > 0) {
	    fprintf(stderr, "starting server nid=%lu, pid=%d\n",
		    (unsigned long)my_id.nid, my_id.pid);
	}

	/* prepare portals stuff for unexpected requests */
	post_incoming_buffer(match_id, len_buf, 
		np*args_info.num_reqs_arg*args_info.count_arg*sizeof(int),
		threshold, ni_h, portal_index, match_bits, &eq_h);


	while (1) {

	    if (debug > 0) {
		fprintf(stderr, "\n============= %d ==============\n",numreqs);
		fprintf(stderr, "Calling xfer_server()\n");
	    }
	    xfer_server(ni_h, portal_index, match_bits, eq_h);
	    numreqs++; 

	    if ((args_info.count_arg > 0) && (numreqs >= maxreqs)) {
		break; 
	    }

	}

	xfer_server_fini(eq_h);
	free(len_buf);
    }

    /* client code */
    else { 
	int i, numreqs=0;
	int rc; 
	ssize_t nbytes = args_info.len_arg*sizeof(data_t);
	data_t *data = (data_t *)malloc(nbytes); 
	ptl_process_id_t server_id; 

	server_id.nid = args_info.server_nid_arg; 
	server_id.pid = args_info.server_pid_arg; 

	if (debug > 0) {
	    fprintf(stderr, "starting client nid=%lu, pid=%d\n",
		    (unsigned long)my_id.nid, my_id.pid);
	}

	print_args(result_fp, "%", &args_info); 


	/* initialize the data */
	for (i=0; i<args_info.len_arg; i++) {
	    data[i].int_val = (int)(i+1);
	    data[i].float_val = (float)(i+1);
	    data[i].double_val = (double)(i+1);
	}
	if (debug > 0) {
	    fprintf(stderr, "data-buffer initialized (int=%d, float=%g, double=%g)\n",
		    data[args_info.len_arg-1].int_val, 
		    data[args_info.len_arg-1].float_val, 
		    data[args_info.len_arg-1].double_val);
	}

	/* set nbytes to the total data tranferred back and forth */
	nbytes = sizeof(int) + (args_info.len_arg+1)*sizeof(data_t);

	/* run it once to warm caches, ... */
	for (i=0; i<args_info.num_reqs_arg; i++) {
	    if (debug > 0) {
		fprintf(stderr, "\n============= %d ==============\n",numreqs);
		fprintf(stderr, "Calling xfer_client()\n");
	    }
	    rc = xfer_client(data, args_info.len_arg, server_id, ni_h, 
		    portal_index, match_bits); 
	    numreqs++;
	}

	/* this time, we time the experiment */
	for (i=0; i<args_info.count_arg; i++) {
	    int j; 

	    double time, start_time = lwfs_get_time(); 

	    for (j=0; j<args_info.num_reqs_arg; j++) {

		if (debug > 0) {
		    fprintf(stderr, "\n============= %d ==============\n",numreqs);
		    fprintf(stderr, "Calling xfer_client()\n");
		}

		rc = xfer_client(data, args_info.len_arg, server_id, 
			ni_h, portal_index, match_bits); 
		if (rc == -1) {
		    fprintf(stderr, "client failed\n"); 
		    exit(-1);
		}
		numreqs++;
	    }

	    time = lwfs_get_time() - start_time; 

	    output_stats(result_fp, &args_info, time); 
	}

	free(data);
    }

    /* cleanup */
    PtlNIFini(ni_h);
    PtlFini();

    if (debug > 0) {
	fprintf(stderr, "finished!\n");
    }

    return 0;
}
