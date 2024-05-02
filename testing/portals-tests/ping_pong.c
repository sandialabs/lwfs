/* hello world server */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <time.h>

#include "config.h"

#include PORTALS_HEADER
#include PORTALS_NAL_HEADER
#include PORTALS_RT_HEADER


#include "ptl_opts.h"

#ifndef PTL_EQ_HANDLER_NONE
#define PTL_EQ_HANDLER_NONE NULL
#endif

#ifndef PtlErrorStr
#define PtlErrorStr(a) ""
#endif

/* Special stuff to handle Cray extensions to Portals MD */
#ifndef PTL_MD_EVENT_AUTO_UNLINK_ENABLE
#define PTL_MD_EVENT_AUTO_UNLINK_ENABLE 0
#endif

#ifndef PTL_MD_EVENT_MANUAL_UNLINK_ENABLE
#define PTL_MD_EVENT_MANUAL_UNLINK_ENABLE 0
#endif

#ifndef PTL_IFACE_DUP
#define PTL_IFACE_DUP 999999
#endif


/* The UTCP NAL requires that the application defines where the Portals
 * API and library should send any output.
 */
FILE *utcp_api_out;
FILE *utcp_lib_out; 

/* global variables */
#define BUFSIZE 256

static int debug = 0;

/* wait for a request to complete */
static int wait(ptl_handle_eq_t eq_h, int req_type, ptl_event_t *event)
{
    int rc = PTL_OK;

    int done = 0; 
    int got_put_start = 0;
    int got_put_end = 0;
    int got_send_start = 0;
    int got_send_end = 0;
    int got_unlink = 0;
    int got_ack = 0;

    while (!done) {
	/* wait for a */
	/*fprintf(stderr, "waiting for event\n");*/
	rc = PtlEQWait(eq_h, event);
	if (rc != PTL_OK) {
	    fprintf(stderr, "PtlEQWait failed. rc = %d\n", rc);
	    abort();
	}

	/*fprintf(stderr, "received event\n");*/
	switch (event->type) {
	    case PTL_EVENT_SEND_START:
		if (debug) fprintf(stderr, "\tgot send start\n");
		if (req_type == 0) {
		    got_send_start = 1;
		}
		else {
		    fprintf(stderr, "unexpected send_start event");
		    abort();
		}
		break;
	    case PTL_EVENT_SEND_END:
		if (debug) fprintf(stderr, "\tgot send_end\n");
		if (req_type == 0) {
		    got_send_end = 1;
		}
		else {
		    fprintf(stderr, "unexpected send_end event");
		    abort();
		}
		break;


	    case PTL_EVENT_PUT_START: 
		if (debug) fprintf(stderr, "\tgot put_start\n");
		if (req_type == 1) {
		    got_put_start = 1;
		}
		else {
		    fprintf(stderr, "unexpected put_start event");
		    abort();
		}
		break;

	    case PTL_EVENT_PUT_END:
		if (debug) fprintf(stderr, "\tgot put_end\n");
		if (req_type == 1) {
		    got_put_end = 1;
		}
		else {
		    fprintf(stderr, "unexpected put_start event");
		    abort();
		}
		break;

	    case PTL_EVENT_ACK:
		if (debug) fprintf(stderr, "\tgot ack\n");
		got_ack = 1;
		break;

	    case PTL_EVENT_UNLINK:
		if (debug) fprintf(stderr, "\tgot unlink\n");
		got_unlink = 1;
		break;

	    default:
		fprintf(stderr, "\tunexpected event");
		abort();
	}

	/* the case for sends */
	if (got_send_start 
		&& got_send_end 
		&& got_ack 
		&& got_unlink) {
	    done = 1; 
	    if (debug) fprintf(stderr, "\tdone with send!\n");
	}

	/* the case for receives */
	if (got_put_start 
		&& got_put_end 
		&& got_unlink) {
	    done = 1; 
	    if (debug) fprintf(stderr, "\tdone with recv!\n");
	}
    }


    return rc;
}



/*  Receive a message. */
static int post_recv(
	ptl_process_id_t src,
	char *buf,
	int bufsize,
	ptl_handle_ni_t ni_h,
	ptl_pt_index_t portal_index,
	ptl_match_bits_t match_bits,
	ptl_handle_eq_t eq_h)
{
	int rc; 

	ptl_md_t md; 
	ptl_handle_me_t me_h; 
	ptl_handle_md_t md_h; 

	/* initialize the md for the incoming buffer */
	memset(&md, 0, sizeof(ptl_md_t)); 
	md.start = buf; 
	md.length = bufsize; 
	md.threshold = 1;  /* only expect one operation */
	md.options = PTL_MD_OP_PUT 
	    | PTL_MD_TRUNCATE
	    | PTL_MD_EVENT_AUTO_UNLINK_ENABLE;
	md.user_ptr = NULL;  /* unused */
	md.eq_handle = eq_h; 

	/* Create a match entry for this message. */
	rc = PtlMEAttach(ni_h, portal_index, src, match_bits, 0,
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

	return 0;
}




/* sends a single message to a server */
static int send_message(
	ptl_process_id_t dest, 
	char *buf, 
	int bufsize,
	ptl_handle_ni_t ni_h,
	ptl_pt_index_t portal_index,
	ptl_match_bits_t match_bits,
	ptl_handle_eq_t eq_h)
{
	int rc; 

	ptl_md_t md; 
	ptl_handle_md_t md_h; 

	/* initialize the md for the outgoing buffer */
	memset(&md, 0, sizeof(ptl_md_t)); 
	md.start = buf; 
	md.length = bufsize;
	/*md.threshold = PTL_MD_THRESH_INF;*/
	md.threshold = 2;  /* send + ack */
	/*md.max_size = 0;*/
	md.options = PTL_MD_OP_PUT 
	    | PTL_MD_TRUNCATE 
	    | PTL_MD_EVENT_AUTO_UNLINK_ENABLE;
	md.user_ptr = NULL;  /* unused */

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

	/* An explicit unlink does not generate an event */
	/*rc = PtlMDUnlink(md_h);*/

	return rc; 
}





/*  Receive one message from the client, return the same 
 *  message, then exit.  We're trying to model the behavior 
 *  of LWFS RPC. */
static int ping_pong_server(
	int count,
	ptl_handle_ni_t ni_h,
	ptl_pt_index_t portal_index,
	ptl_match_bits_t match_bits)
{
    int rc; 
    char buf[BUFSIZE];
    int i;
    ptl_handle_eq_t eq_h; 
    ptl_process_id_t client; 
    ptl_event_t event; 

    for (i=0; i<count; i++) {

	/* anybody can send a message to the server */
	client.nid = PTL_NID_ANY;
	client.pid = PTL_PID_ANY;

	/* create an event queue for the incoming buffer */
	rc = PtlEQAlloc(ni_h, 5, PTL_EQ_HANDLER_NONE, &eq_h);
	if (rc != PTL_OK) {
	    fprintf(stderr, "PtlEQAlloc() failed: %s (%d)\n",
		    PtlErrorStr(rc), rc); 
	    exit(-1); 
	}

	/* receive a message from the client */
	fprintf(stderr, "waiting for msg %d (match_bits=%d) "
		"from client\n", i, (int)(match_bits+i));

	memset(buf, 0, BUFSIZE);
	post_recv(client, buf, BUFSIZE, ni_h, portal_index, match_bits+i, eq_h);
	
	/* wait for a response from the client */
	wait(eq_h, 1, &event); 

	/* set the client ID */
	client.nid = event.initiator.nid;
	client.pid = event.initiator.pid;

	fprintf(stderr, "recv'd msg[%d] = \"%s\" from client (nid=%llu, pid=%llu)\n", 
		i, buf, (unsigned long long)client.nid, (unsigned long long)client.pid);

	fprintf(stderr, "sending response[%d] (match_bits=%d)"
		" to (nid=%llu, pid=%llu)\n", i, (int)(match_bits+100+i),
		(unsigned long long)client.nid, (unsigned long long)client.pid);

	/* free the event queue */
	rc = PtlEQFree(eq_h);
	if (rc != PTL_OK) {
	    fprintf(stderr, "PtlEQFree() failed: %s (%d)\n",
		    PtlErrorStr(rc), rc); 
	    abort();
	}


	/* -------------- SEND RESULT (pong) ---------- */


	/* create an event queue for the result buffer */
	rc = PtlEQAlloc(ni_h, 5, PTL_EQ_HANDLER_NONE, &eq_h);
	if (rc != PTL_OK) {
	    fprintf(stderr, "PtlEQAlloc() failed: %s (%d)\n",
		    PtlErrorStr(rc), rc); 
	    exit(-1); 
	}

	/* send the same message back to client */
	send_message(client, buf, BUFSIZE, ni_h, 
		portal_index+1, match_bits+100+i, eq_h); 
	wait(eq_h, 0, &event); 

	fprintf(stderr, "response[%d] sent to client\n", i);


	/* free the event queue */
	rc = PtlEQFree(eq_h);
	if (rc != PTL_OK) {
	    fprintf(stderr, "PtlEQFree() failed: %s (%d)\n",
		    PtlErrorStr(rc), rc); 
	    abort();
	}
    }

    fprintf(stderr, "exiting server func\n");
    return 0; 
}

/* sends a messages to a server */
static int ping_pong_client(
	int count,
	ptl_process_id_t server, 
	ptl_handle_ni_t ni_h,
	ptl_pt_index_t portal_index,
	ptl_match_bits_t match_bits)
{
    int rc; 
    int i;
    char sendbuf[BUFSIZE];
    char recvbuf[BUFSIZE];
    ptl_handle_eq_t *ping_eq_h; 
    ptl_handle_eq_t *pong_eq_h; 
    ptl_event_t event; 

    ping_eq_h = (ptl_handle_eq_t *)malloc(count*sizeof(ptl_handle_eq_t));
    pong_eq_h = (ptl_handle_eq_t *)malloc(count*sizeof(ptl_handle_eq_t));

    /* initialize the buffer */
    memset(sendbuf, 0, sizeof(sendbuf));
    sprintf(sendbuf, "hello world");

    /* async send */
    for (i=0; i<count; i++) {

	fprintf(stderr, "%d\n",i);

	/* ----- RECV MESSAGE (pong) -------- */

	/* create an event queue for the outgoing buffer */
	rc = PtlEQAlloc(ni_h, 5, PTL_EQ_HANDLER_NONE, &pong_eq_h[i]);
	if (rc != PTL_OK) {
	    fprintf(stderr, "PtlEQAlloc() failed: %s (%d)\n",
		    PtlErrorStr(rc), rc); 
	    exit(-1); 
	}

	if (debug) fprintf(stderr, "Posting recv for pong\n");


	/* post a  receive for the result (pong) from the server */
	post_recv(server, recvbuf, BUFSIZE, ni_h, 
		portal_index+1, match_bits+100+i, pong_eq_h[i]);





	/* ----- SEND MESSAGE (ping) -------- */

	/* create an event queue for the outgoing buffer */
	rc = PtlEQAlloc(ni_h, 5, PTL_EQ_HANDLER_NONE, &ping_eq_h[i]);
	if (rc != PTL_OK) {
	    fprintf(stderr, "PtlEQAlloc() failed: %s (%d)\n",
		    PtlErrorStr(rc), rc); 
	    exit(-1); 
	}

	if (debug) fprintf(stderr, "Sending ping\n");

	/* send initial message to server */
	send_message(server, sendbuf, BUFSIZE, ni_h, 
		portal_index, match_bits+i, ping_eq_h[i]); 

	if (debug) fprintf(stderr, "Waiting for ping to complete\n");

	rc = wait(ping_eq_h[i], 0, &event); 
	fprintf(stderr, "sent msg[%d] (match_bits=%d) to server\n",
		i, (int)(match_bits+i));

	/* free the event queue */
	rc = PtlEQFree(ping_eq_h[i]);
	if (rc != PTL_OK) {
	    fprintf(stderr, "PtlEQFree() failed: %s (%d)\n",
		    PtlErrorStr(rc), rc); 
	    abort();
	}



	fprintf(stderr, "waiting for response[%d] (match_bits=%d) "
		"from (nid=%llu, pid=%llu)\n", i, (int)(match_bits+100+i),
		(unsigned long long)server.nid, (unsigned long long)server.pid);

	/* wait for pong from client */
	rc = wait(pong_eq_h[i], 1, &event); 
	fprintf(stderr, "recv'd pong[%d] = \"%s\"\n", i, recvbuf);

	/* free the event queue */
	rc = PtlEQFree(pong_eq_h[i]);
	if (rc != PTL_OK) {
	    fprintf(stderr, "PtlEQFree() failed: %s (%d)\n",
		    PtlErrorStr(rc), rc); 
	    abort();
	}
    }

    free(ping_eq_h);
    free(pong_eq_h);

    fprintf(stderr, "exiting client func\n");
    return 0;
}


/* ----------------- COMMAND-LINE OPTIONS --------------- */

static int print_args(FILE *fp, 
	const struct gengetopt_args_info *args,
	const char *prefix) 
{
	time_t now; 

	/* get the current time */
	now = time(NULL);

	fprintf(fp, "%s -----------------------------------\n", prefix);
	fprintf(fp, "%s \tPing_Pong Portals Test\n", prefix);
	fprintf(fp, "%s \t%s", prefix, asctime(localtime(&now)));
	fprintf(fp, "%s ------------  ARGUMENTS -----------\n", prefix);
	if (args->server_flag) {
		fprintf(fp, "%s Running as a server\n", prefix);
	}
	else {
		fprintf(fp, "%s Running as a client\n", prefix);
	}

	fprintf(fp, "%s \t--count = %d\n", prefix, args->count_arg);

	fprintf(fp, "%s \t--server-nid = %llu\n", prefix, (unsigned long long)args->server_nid_arg);
	fprintf(fp, "%s \t--server-pid = %llu\n", prefix, (unsigned long long)args->server_pid_arg);

	fprintf(fp, "%s -----------------------------------\n", prefix);

	return 0;
}



int main(int argc, char **argv)
{
    int num_if; 
    int count = 1; 
    int rc; 
    struct gengetopt_args_info args; 
    int shutdown = 1; 
    ptl_interface_t iface;
    
    /*
    iface = CRAY_QK_NAL;
    iface = CRAY_KERN_NAL;
    iface = CRAY_USER_NAL;
    */
    iface = PTL_IFACE_DEFAULT;

    /* fixed for all runs */
    ptl_pt_index_t portal_index=4;   
    ptl_match_bits_t match_bits=5; 

    ptl_process_id_t my_id; 
    ptl_process_id_t server_id; 
    ptl_handle_ni_t ni_h;
    ptl_pid_t pid = PTL_PID_ANY; 

    /* parse command line options */
    if (cmdline_parser(argc, argv, &args) != 0) {
	exit(1);
    }

    /* initialize the server ID */
    server_id.nid = (ptl_nid_t)args.server_nid_arg; 
    server_id.pid = (ptl_pid_t)args.server_pid_arg; 


    /* Initialize the library */
    if (PtlInit(&num_if) != PTL_OK) {
	fprintf(stderr, "PtlInit() failed\n");
	exit(1); 
    }

    /* Initialize the interface */
    pid = (args.server_flag)? args.server_pid_arg : PTL_PID_ANY; 
    rc = PtlNIInit(iface, pid, NULL, NULL, &ni_h); 
    switch (rc) {
	case PTL_OK:
	    shutdown = 1; 
	    break;
	case PTL_IFACE_DUP:
	    fprintf(stderr, "PtlNIInit() IFACE already initialized: rc=%d\n",rc); 
	    shutdown = 0;
	    break;
	default:
	    fprintf(stderr, "PtlNIInit() failed: rc=%d\n",rc); 
	    exit(1); 
    }


    /* Get and print the ID of this process */
    if (PtlGetId(ni_h, &my_id) != PTL_OK) {
	printf("PtlGetId() failed.\n");
	abort();
    }

    if (args.server_flag) {
	server_id.nid = my_id.nid;
	server_id.pid = my_id.pid;
	args.server_nid_arg = my_id.nid;
	args.server_pid_arg = my_id.pid;
    }
    else {
	if (server_id.nid == 0) {
	    server_id.nid = my_id.nid;
	    args.server_nid_arg = my_id.nid;
	}
    }


    printf("%s: nid = %llu, pid = %llu\n",
	    argv[0], 
	    (unsigned long long) my_id.nid,
	    (unsigned long long) my_id.pid);


    print_args(stdout, &args, ""); 


    /***************** EVERYTHING ABOVE USED FOR BOTH SENDER AND RECEIVER *******/

    if (args.server_flag) {
	fprintf(stderr, "starting server (BUFSIZE=%d)...\n",BUFSIZE);
	ping_pong_server(count, ni_h, portal_index, match_bits);
    }
    else { 
	fprintf(stderr, "starting client (BUFSIZE=%d)...\n",BUFSIZE);
	fprintf(stderr, "connecting to nid=%ld, pid=%ld.\n",
		args.server_nid_arg, args.server_pid_arg);

	/* copy the message */
	ping_pong_client(count, server_id, ni_h, portal_index, match_bits); 
    }

    /* cleanup */
    if (shutdown) {
	PtlNIFini(ni_h);
	PtlFini();
    }

    return 0;
}
