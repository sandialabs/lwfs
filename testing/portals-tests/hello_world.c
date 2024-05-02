/* hello world server */

#include <time.h>
#include <argp.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <portals3.h>
#include <p3nal_utcp.h>
#include <p3rt/p3rt.h>
#include <p3api/debug.h>

/* The UTCP NAL requires that the application defines where the Portals
 * API and library should send any output.
 */
FILE *utcp_api_out;
FILE *utcp_lib_out; 

/* global variables */
const int BUFSIZE=1000;


/*  Receive a message. */
static int recv_message(
	ptl_process_id_t src,
	char *buf,
	int bufsize,
	ptl_handle_ni_t ni_h,
	ptl_pt_index_t portal_index,
	ptl_match_bits_t match_bits)
{
	int rc; 
	int got_put_start = 0;
	int got_put_end = 0;
	int got_unlink = 0;
	int done = 0;

	ptl_md_t md; 
	ptl_handle_eq_t eq_h; 
	ptl_handle_me_t me_h; 
	ptl_handle_md_t md_h; 
	ptl_event_t event; 

	ptl_process_id_t match_id; 
	match_id.nid = PTL_NID_ANY;
	match_id.pid = PTL_PID_ANY;

	/* initialize the md for the incoming buffer */
	memset(&md, 0, sizeof(ptl_md_t)); 
	md.start = buf; 
	md.length = bufsize; 
	md.threshold = 1;  /* only expect one operation */
	md.options = PTL_MD_OP_PUT | PTL_MD_TRUNCATE;
	md.user_ptr = NULL;  /* unused */

	/* create an event queue for this message */
	rc = PtlEQAlloc(ni_h, 5, PTL_EQ_HANDLER_NONE, &eq_h);
	if (rc != PTL_OK) {
		fprintf(stderr, "PtlEQAlloc() failed: %s (%d)\n",
				PtlErrorStr(rc), rc); 
		exit(-1); 
	}
	md.eq_handle = eq_h; 

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

	while (!done) {

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
				fprintf(stderr, "Got PUT_UNLINK\n");
				got_unlink = 1;
				break;
			default:
				fprintf(stderr, "recv_message: unexpected event (%d)\n", event.type);
				abort();
		}

		if (got_put_start && got_put_end && got_unlink) {
			done = 1;
		}
	}

	printf("received message: %s\n", buf); 

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
	char *buf, 
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




/*  Receive one message from the client, return the same 
 *  message, then exit.  We're trying to model the behavior 
 *  of LWFS RPC. */
static int hello_world_server(
	ptl_process_id_t client,
	ptl_handle_ni_t ni_h,
	ptl_pt_index_t portal_index,
	ptl_match_bits_t match_bits)
{
	char buf[BUFSIZE];

	/* receive a message from the client */
	recv_message(client, buf, BUFSIZE, ni_h, portal_index, match_bits);

	/* send the same message back to client */
	//send_message(client, buf, BUFSIZE, ni_h, portal_index, match_bits); 

	return 0; 
}

/* sends a single message to a server */
static int hello_world_client(
	ptl_process_id_t server, 
	ptl_handle_ni_t ni_h,
	ptl_pt_index_t portal_index,
	ptl_match_bits_t match_bits)
{
	char sendbuf[BUFSIZE];
	//char recvbuf[BUFSIZE];

	/* initialize the buffer */
	memset(sendbuf, 0, BUFSIZE);
	sprintf(sendbuf, "hello world\n");
	fprintf(stdout, "sending \"%s\" to server\n", sendbuf);
	fflush(stdout);

	/* send initial message to server */
	send_message(server, sendbuf, BUFSIZE, ni_h, portal_index, match_bits); 

	/* receive return message to server */
	//recv_message(server, recvbuf, BUFSIZE, ni_h, portal_index, match_bits);

	return 0;
}


/* ----------------- COMMAND-LINE OPTIONS --------------- */

const char *argp_program_version = "$Revision: 1073 $"; 
const char *argp_program_bug_address = "<raoldfi@sandia.gov>"; 

static char doc[] = "Test the LWFS storage service";
static char args_doc[] = "HOSTNAME"; 

/**
 * @brief Command line arguments for testing the storage service. 
 */
struct arguments {

	/** @brief A flag that identifies a server. */
	int server_flag; 

	/** @brief process ID of the server. */
	ptl_process_id_t server; 

	/** @brief process ID of the client. */
	ptl_process_id_t client; 
}; 

static int print_args(FILE *fp, const char *prefix, struct arguments *args) 
{
	time_t now; 

	/* get the current time */
	now = time(NULL);

	fprintf(fp, "%s -----------------------------------\n", prefix);
	fprintf(fp, "%s \tHello World Portals Test\n", prefix);
	fprintf(fp, "%s \t%s", prefix, asctime(localtime(&now)));
	fprintf(fp, "%s ------------  ARGUMENTS -----------\n", prefix);
	if (args->server_flag) {
		fprintf(fp, "%s Running as a server\n", prefix);
	}
	else {
		fprintf(fp, "%s Running as a client\n", prefix);
	}


	fprintf(fp, "%s \t--client-nid = ", prefix);
	if (args->client.nid == PTL_NID_ANY)
		fprintf(fp, "ANY\n");

	else 
		fprintf(fp, "%u\n", args->client.nid);

	fprintf(fp, "%s \t--client-pid = ", prefix);
	if (args->client.pid == PTL_PID_ANY)
		fprintf(fp, "ANY\n");

	else 
		fprintf(fp, "%u\n", args->client.pid);

	fprintf(fp, "%s \t--server-nid = %u\n", prefix, args->server.nid);
	fprintf(fp, "%s \t--server-pid = %u\n", prefix, args->server.pid);

	fprintf(fp, "%s -----------------------------------\n", prefix);

	return 0;
}


static struct argp_option options[] = {
	{"server",      4, 0, 0, "Run as a server"},
	{"server-nid",  5, "<val>", 0, "NID of the server"},
	{"server-pid",  6, "<val>", 0, "PID of the server"},
	{"client-nid",  7, "<val>", 0, "NID of the client"},
	{"client-pid",  8, "<val>", 0, "PID of the client"},
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

		case 4: /* server flag */
			arguments->server_flag = 1;
			break;

		case 5: /* NID of the server */
			arguments->server.nid = atoll(arg);
			break;

		case 6: /* PID of the server */
			arguments->server.pid = atoll(arg);
			break;

		case 7: /* NID of the client */
			arguments->client.nid = atoll(arg);
			break;

		case 8: /* PID of the server */
			arguments->client.pid = atoll(arg);
			break;



		case ARGP_KEY_ARG:
			/* we expect only one argument */
			if (state->arg_num >= 0) {
				argp_usage(state);
			}
			//arguments->args[state->arg_num] = arg; 
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





int main(int argc, char **argv)
{
	struct arguments args; 
	int num_if; 

	/* fixed for all runs */
	ptl_pt_index_t portal_index=4;   
	ptl_match_bits_t match_bits=5; 

	ptl_process_id_t my_id; 
	ptl_handle_ni_t ni_h;
	ptl_pid_t pid; 

	/* default values for arguments */
	args.server_flag = 0;  /* default to a client */
	args.client.pid = PTL_PID_ANY;  /* default client PID */
	args.client.nid = PTL_NID_ANY;  /* default client NID */
	args.server.pid = 99;  /* default server NID */
	args.server.nid = PTL_NID_ANY;  /* default server PID */

	/* these must be set, otherwisze, PtlNIInit segfaults */
	utcp_api_out = stdout; 
	utcp_lib_out = stdout; 

	/* parse the command-line arguments */
	argp_parse(&argp, argc, argv, 0 ,0, &args); 


	/* Initialize the Portals library */
	if (PtlInit(&num_if) != PTL_OK) {
		fprintf(stderr, "PtlInit() failed\n");
		exit(1); 
	}

	/* Initialize the interface (PIDS client=98, server=99) */
	pid = (args.server_flag)? args.server.pid : args.client.pid; 
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
	if (args.server.nid == PTL_NID_ANY) {
		args.server.nid = my_id.nid; 
	}

	if (!args.server_flag && (args.client.nid == PTL_NID_ANY)) {
		args.client.nid = my_id.nid; 
		args.client.pid = my_id.pid; 
	}

	print_args(stdout, "", &args); 


	/***************** EVERYTHING ABOVE USED FOR BOTH SENDER AND RECEIVER *******/

	if (args.server_flag) {
		/* set the match ID */
		fprintf(stderr, "starting server ...\n");
		hello_world_server(args.client, ni_h, portal_index, match_bits);
	}
	else { 

		fprintf(stderr, "starting client ...\n");

		/* copy the message */
		hello_world_client(args.server, ni_h, portal_index, match_bits); 
	}

	/* cleanup */
	PtlNIFini(ni_h);
	PtlFini();

	return 0;
}
