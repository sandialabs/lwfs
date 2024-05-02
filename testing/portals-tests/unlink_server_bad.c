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
#define PTL_EQ_HANDLER_NONE NULL
#define PtlErrorStr(a) ""

#else 

/* Need these for Schutt's version */
#include <p3nal_utcp.h>
#include <p3rt/p3rt.h>
#include <p3api/debug.h>

#define PTL_IFACE_SERVER PTL_IFACE_DEFAULT
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
const char *MSG_DATA = "hello world";

int with_bug = 0; 

static int recv_messages(
	ptl_handle_ni_t ni_h,
	ptl_pt_index_t portal_index,
	ptl_match_bits_t match_bits,
	int total_msgs,
	int msgs_per_md)
{
    /* misc variables */
    int i; 
    int rc; 

    /* two incoming queues */
    const int num_queues = 2; 
    char **queue_bufs; 
    int indices[num_queues];
    int msg_counts[num_queues];
    int msg_count = 0; 

    /* Will receive 2 events per message */
    int events_per_md = 2*msgs_per_md;

    /* Need one event queue */
    ptl_handle_eq_t eq_h; 

    /* Need a match ID for process sending reqs */
    ptl_process_id_t match_id; 

    /* Portals stuff for the incoming queues */
    ptl_md_t md[num_queues];
    ptl_handle_me_t me_h[num_queues]; 
    ptl_handle_me_t bug_me_h; 
    ptl_handle_md_t md_h[num_queues]; 

    /* Need an event so we know what was sent and where it was placed */
    ptl_event_t event; 
    int got_put_start = 0; 
    int got_put_end = 0; 
    int got_unlink = 0; 

    /* allocate space for incoming MDs */
    queue_bufs = (char **)malloc(num_queues*sizeof(char *)); 
    for (i=0; i<num_queues; i++) {
	queue_bufs[i] = (char *)malloc(msgs_per_md * BUFSIZE);
    }

    /* We don't care where a request comes from */
    match_id.nid = PTL_NID_ANY;
    match_id.pid = PTL_PID_ANY;


    /* create a circular event queue for incoming messages */
    rc = PtlEQAlloc(ni_h, events_per_md, PTL_EQ_HANDLER_NONE, &eq_h);
    if (rc != PTL_OK) {
	fprintf(stderr, "PtlEQAlloc() failed: %s (%d)\n",
		PtlErrorStr(rc), rc); 
	exit(-1); 
    }


    /* BUG: Create a match-list entry for this queue (UNLINK with MD). */
    rc = PtlMEAttach(ni_h, portal_index, match_id, match_bits, 0,
	    PTL_RETAIN, PTL_INS_AFTER, &bug_me_h); 
    if (rc != PTL_OK) {
	fprintf(stderr, "PtlMEAttach: %s\n", PtlErrorStr(rc));
	exit(-1); 
    }


    /* Initialize and attach MDs and MEs */
    for (i=0; i<num_queues; i++) {

	indices[i] = i; 
	msg_counts[i] = 0;

	/* initialize the md for the incoming buffer */
	memset(&md[i], 0, sizeof(ptl_md_t)); 
	md[i].start = queue_bufs[i]; 
	md[i].length = msgs_per_md * BUFSIZE; 
	md[i].threshold = msgs_per_md; 
	md[i].user_ptr = &indices[i];  /* identifies the queue index */
	md[i].eq_handle = eq_h; 
	md[i].options = PTL_MD_OP_PUT | PTL_MD_TRUNCATE 
	    | PTL_MD_EVENT_AUTO_UNLINK_ENABLE; 


	/* Create a persistent match-list entry for this queue (UNLINK with MD). */
	rc = PtlMEAttach(ni_h, portal_index, match_id, match_bits, 0,
		PTL_RETAIN, PTL_INS_AFTER, &me_h[i]); 
	if (rc != PTL_OK) {
	    fprintf(stderr, "PtlMEAttach: %s\n", PtlErrorStr(rc));
	    exit(-1); 
	}

	if (with_bug) {
	    /* Attach the descriptor to the bug match_entry.
	     * This should cause an error because we should only
	     * be able to attache one MD to a match entry. 
	     */
	    rc = PtlMDAttach(bug_me_h, md[i], PTL_UNLINK, &md_h[i]); 
	    if (rc != PTL_OK) {
		fprintf(stderr, "PtlMDAttach: %s\n", PtlErrorStr(rc)); 
		exit(-1); 
	    }
	}
	else {
	    /* This is how I should have done it the first time */
	    rc = PtlMDAttach(me_h[i], md[i], PTL_UNLINK, &md_h[i]); 
	    if (rc != PTL_OK) {
		fprintf(stderr, "PtlMDAttach: %s\n", PtlErrorStr(rc)); 
		exit(-1); 
	    }
	}
    }



    msg_count = 0; 

    /* begin procesing incoming messages */

    while (msg_count < total_msgs) {

	/* wait for a PTL_EVENT_PUT_START */
	fprintf(stderr, "waiting for event...\n");
	rc = PtlEQWait(eq_h, &event);
	if (rc != PTL_OK) {
	    fprintf(stderr, "Wait failed. rc = %d\n", rc);
	    abort();
	}

	switch(event.type) {
	    case PTL_EVENT_PUT_START: 
		fprintf(stderr, "Got PUT_START\n");
		got_put_start = 1;
		break;
	    case PTL_EVENT_PUT_END:
		fprintf(stderr, "Got PUT_END\n");
		got_put_end = 1;
		break;
	    case PTL_EVENT_UNLINK:
		fprintf(stderr, "Got UNLINK\n");
		got_unlink = 1;
		break;
	    default:
		fprintf(stderr, "recv_message: unexpected event (%d)\n", event.type);
		abort();
	}

	if (got_put_start && got_put_end) {

	    char *msg = event.md.start + event.offset; 
	    char msg_data[BUFSIZE];  

	    memset(msg_data, 0, BUFSIZE);
	    strncpy(msg_data, msg, event.mlength); 

	    /* get the queue index */
	    i = *(int *)event.md.user_ptr; 

	    /* increment the message counts */
	    msg_counts[i]++;
	    msg_count++; 

	    fprintf(stdout, "Received msg[%d]=%s on queue %d, total=%d\n",
		    msg_counts[i], msg_data, i, msg_count); 

	    got_put_start = 0; 
	    got_put_end = 0; 
	}

	if (got_unlink) {

	    /* get the queue index */
	    i = *(int *)event.md.user_ptr; 

	    fprintf(stdout, "Re-attach MD for queue %d after %d msgs\n", 
		    i, msg_counts[i]); 
	    msg_counts[i] = 0; 

	    if (with_bug) {
		/* Re-attach the memory descriptor to the wrong match entry */
		rc = PtlMDAttach(bug_me_h, md[i], PTL_UNLINK, &md_h[i]); 
		if (rc != PTL_OK) {
		    fprintf(stderr, "PtlMDAttach: %s\n", PtlErrorStr(rc)); 
		    exit(-1); 
		}
	    }
	    else {
		/* Re-attach the memory descriptor to the match entry */
		rc = PtlMDAttach(me_h[i], md[i], PTL_UNLINK, &md_h[i]); 
		if (rc != PTL_OK) {
		    fprintf(stderr, "PtlMDAttach: %s\n", PtlErrorStr(rc)); 
		    exit(-1); 
		}
	    }

	    got_unlink = 0; 
	}
    }

    fprintf(stderr, "Finished processing reqs... Unlinking MEs\n");

    /* Unlink the match_entries, should also unlink the MDs */
    for (i=0; i<num_queues; i++) {
	PtlMEUnlink(me_h[i]);
    }


    /* free the event queue */
    rc = PtlEQFree(eq_h);
    if (rc != PTL_OK) {
	fprintf(stderr, "PtlEQFree() failed: %s (%d)\n",
		PtlErrorStr(rc), rc); 
	abort();
    }


    /* free buffers */
    for (i=0; i<num_queues; i++) {
	free(queue_bufs[i]);
    }
    free(queue_bufs); 


    printf("SUCCESS!\n"); 
    printf("received %d messages\n", msg_count); 

    return 0; 
}







int main(int argc, char **argv)
{
    int rc; 
    int num_if; 
    int total_msgs;
    int msgs_per_md; 

    /* fixed for all runs */
    ptl_pt_index_t portal_index; 
    ptl_match_bits_t match_bits; 

    ptl_process_id_t my_id; 
    ptl_handle_ni_t ni_h;
    ptl_pid_t pid; 
    struct gengetopt_args_info args_info; 

    /* these must be set on Schutt's impl, otherwisze, PtlNIInit segfaults */
    utcp_api_out = stdout; 
    utcp_lib_out = stdout; 

    /* parse the command-line arguments */
    if (cmdline_parser(argc, argv, &args_info) != 0)
	exit(1); 

    /* get values from the command-line */
    pid = args_info.server_pid_arg; 
    total_msgs = args_info.total_msgs_arg; 
    msgs_per_md = args_info.msgs_per_md_arg; 
    portal_index = args_info.portal_index_arg; 
    match_bits = args_info.match_bits_arg; 
    with_bug = args_info.with_bug_flag; 

    /* Initialize the Portals library */
    rc = PtlInit(&num_if); 
    if (rc != PTL_OK) {
	fprintf(stderr, "PtlInit() failed\n");
	exit(1); 
    }



    if (PtlNIInit(PTL_IFACE_SERVER, pid, 
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

    fprintf(stdout, "Server initialized: nid=%ld, pid=%ld\n",
	    (long)my_id.nid, (long)my_id.pid);

    fprintf(stdout, "   total_msgs = %d\n", total_msgs);
    fprintf(stdout, "   msgs_per_md = %d\n", msgs_per_md);
    fprintf(stdout, "   portal_index = %d\n", portal_index);
    fprintf(stdout, "   match_bits = %d\n", (int)match_bits);
    fprintf(stdout, "   with_bug = %d\n", (int)with_bug);

    /* start processing incoming messages */
    recv_messages(ni_h, portal_index, match_bits, total_msgs, msgs_per_md); 

    /***************** EVERYTHING ABOVE USED FOR BOTH SENDER AND RECEIVER *******/

    /* cleanup */
    PtlNIFini(ni_h);
    PtlFini();

    return 0;
}
