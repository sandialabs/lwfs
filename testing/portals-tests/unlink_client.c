/* hello world server */

#include <time.h>
#include <argp.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef REDSTORM

#include <portals/portals3.h>

#define PTL_IFACE_SERVER CRAY_USER_NAL
#define PTL_IFACE_CLIENT CRAY_QK_NAL
#define PTL_EQ_HANDLER_NONE NULL
#define PtlErrorStr(a) ""

#else 

/* Need these for Schutt's version */
#include <portals3.h>
#include <p3nal_utcp.h>
#include <p3rt/p3rt.h>
#include <p3api/debug.h>

#define PTL_IFACE_SERVER PTL_IFACE_DEFAULT
#define PTL_IFACE_CLIENT PTL_IFACE_DEFAULT
#define PTL_IFACE_DUP PTL_OK
#define PTL_MD_EVENT_AUTO_UNLINK_ENABLE 0
#define PTL_MD_EVENT_MANUAL_UNLINK_ENABLE 0

#endif

#include "unlink_opts.h"

/* The UTCP NAL requires that the application defines where the Portals
 * API and library should send any output.
 */
FILE *utcp_api_out;
FILE *utcp_lib_out; 

/* global variables */
const int BUFSIZE=1000;



/* sends a single message to a server */
static int send_message(
	ptl_process_id_t dest, 
	const char *buf, 
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
    md.start = (char *)buf; 
    md.length = bufsize;
    md.threshold = 2;
    md.options = PTL_MD_OP_PUT | PTL_MD_TRUNCATE 
	| PTL_MD_EVENT_AUTO_UNLINK_ENABLE;
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

	/* wait for a */
	//fprintf(stderr, "waiting for event\n");
	rc = PtlEQWait(eq_h, &event);
	if (rc != PTL_OK) {
	    fprintf(stderr, "PtlEQWait (START) failed. rc = %d\n", rc);
	    abort();
	}

	switch (event.type) {
	    case PTL_EVENT_SEND_START:
		fprintf(stderr, "Got SEND_START\n");
		got_send_start = 1;
		break;
	    case PTL_EVENT_SEND_END:
		fprintf(stderr, "Got SEND_END\n");
		got_send_end = 1;
		break;
	    case PTL_EVENT_ACK:
		fprintf(stderr, "Got SEND_ACK\n");
		got_ack = 1;
		break;
	    case PTL_EVENT_UNLINK:
		fprintf(stderr, "Got SEND_UNLINK\n");
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

    fprintf(stderr, "message sent\n");

    /* free the event queue */
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
	int num_if; 
	int i;
	int total_msgs;
	int msgs_per_md; 
	char msg_data[BUFSIZE]; 
	int rc; 

	/* fixed for all runs */
	ptl_pt_index_t portal_index; 
	ptl_match_bits_t match_bits; 

	ptl_process_id_t my_id; 
	ptl_process_id_t server_id; 
	ptl_handle_ni_t ni_h;
	struct gengetopt_args_info args_info; 

	/* these must be set on Schutt's impl, otherwisze, PtlNIInit segfaults */
	utcp_api_out = stdout; 
	utcp_lib_out = stdout; 

	/* parse the command-line arguments */
	if (cmdline_parser(argc, argv, &args_info) != 0)
	    exit(1); 

	/* get values from the command-line */
	server_id.pid = args_info.server_pid_arg; 
	server_id.nid = args_info.server_nid_arg; 
	total_msgs = args_info.total_msgs_arg; 
	msgs_per_md = args_info.msgs_per_md_arg; 
	portal_index = args_info.portal_index_arg; 
	match_bits = args_info.match_bits_arg; 
	strncpy(msg_data, args_info.msg_data_arg, BUFSIZE);

	/* Initialize the Portals library */
	rc = PtlInit(&num_if); 
	if (rc != PTL_OK) {
		fprintf(stderr, "PtlInit() failed\n");
		exit(1); 
	}

	rc = PtlNIInit(PTL_IFACE_CLIENT, PTL_PID_ANY, NULL, NULL, &ni_h);
	if ((rc != PTL_OK) && (rc != PTL_IFACE_DUP)) {
		fprintf(stderr, "PtlNIInit() failed: %d\n", rc); 
		exit(1); 
	}

	/* tell portals to output debug statements */
	PtlNIDebug(ni_h, 0xfffffdff); 

	/* Get and print the ID of this process */
	if (PtlGetId(ni_h, &my_id) != PTL_OK) {
		printf("PtlGetId() failed.\n");
		abort();
	}

	fprintf(stdout, "---------------------------------\n");
	fprintf(stdout, "Client initialized: nid=%ld, pid=%ld\n",
	       (long)my_id.nid, (long)my_id.pid);

	fprintf(stdout, "   server: nid=%ld, pid=%ld\n", 
		(long)server_id.nid, (long)server_id.pid);
	fprintf(stdout, "   total_msgs = %d\n", total_msgs);
	fprintf(stdout, "   msgs_per_md = %d\n", msgs_per_md);
	fprintf(stdout, "   portal_index = %d\n", portal_index);
	fprintf(stdout, "   match_bits = %d\n", (int)match_bits);
	fprintf(stdout, "   msg_data = %s\n", msg_data);
	fprintf(stdout, "---------------------------------\n\n");

	const char *buf = "hello\0";

	/* Send Messages */
	for (i=0; i<total_msgs; i++) {
	    fprintf(stdout, "Sending msg[%d]=%s\n", i+1, msg_data); 
	    /*
	    send_message(server_id, msg_data, strlen(msg_data), ni_h,
		    portal_index, match_bits); 
		    */
	    send_message(server_id, buf, strlen(buf), ni_h,
		    portal_index, match_bits); 
	}

	/***************** EVERYTHING ABOVE USED FOR BOTH SENDER AND RECEIVER *******/

	/* cleanup */
#ifndef REDSTORM
	PtlNIFini(ni_h);
	PtlFini();
#endif

	return 0;
}
