/* Multi-threaded ping-pong test for portals */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <argp.h>
#include <time.h>
#include <portals3.h>
#include <p3nal_utcp.h>
#include <p3rt/p3rt.h>
#include <pthread.h>

/* The UTCP NAL requires that the application defines where the Portals
 * API and library should send any output.
 */
FILE *utcp_api_out;
FILE *utcp_lib_out; 

/* global variables */
#define BUFSIZE 256


struct message {
	ptl_pt_index_t portal_index; 
	ptl_match_bits_t match_bits;
	char buf[BUFSIZE];
};

struct send_args {
	ptl_process_id_t client;
	ptl_handle_ni_t ni_h;
	ptl_pt_index_t portal_index;
	ptl_match_bits_t match_bits;
	struct message msg; 
};




/* wait for a request to complete */
static int wait(ptl_handle_eq_t eq_h, int req_type, ptl_process_id_t *initiator)
{
	int rc = PTL_OK;
	ptl_event_t event; 

	int done = 0; 
	int got_put_start = 0;
	int got_put_end = 0;
	int got_send_start = 0;
	int got_send_end = 0;
	int got_unlink = 0;
	int got_ack = 0;

	while (!done) {
		/* wait for a */
		//fprintf(stderr, "waiting for event\n");
		rc = PtlEQWait(eq_h, &event);
		if (rc != PTL_OK) {
			fprintf(stderr, "PtlEQWait failed. rc = %d\n", rc);
			abort();
		}

		switch (event.type) {
			case PTL_EVENT_SEND_START:
				fprintf(stderr, "\tgot send start\n");
				if (req_type == 0) {
					got_send_start = 1;
				}
				else {
					fprintf(stderr, "unexpected send_start event");
					abort();
				}
				break;
			case PTL_EVENT_SEND_END:
				fprintf(stderr, "\tgot send_end\n");
				if (req_type == 0) {
					got_send_end = 1;
				}
				else {
					fprintf(stderr, "unexpected send_end event");
					abort();
				}
				break;


			case PTL_EVENT_PUT_START: 
				fprintf(stderr, "\tgot put_start\n");
				if (req_type == 1) {
					got_put_start = 1;
				}
				else {
					fprintf(stderr, "unexpected put_start event");
					abort();
				}
				break;

			case PTL_EVENT_PUT_END:
				fprintf(stderr, "\tgot put_end\n");
				if (req_type == 1) {
					got_put_end = 1;
				}
				else {
					fprintf(stderr, "unexpected put_start event");
					abort();
				}
				break;

			case PTL_EVENT_ACK:
				fprintf(stderr, "\tgot ack\n");
				got_ack = 1;
				break;

			case PTL_EVENT_UNLINK:
				fprintf(stderr, "\tgot unlink\n");
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
			fprintf(stderr, "\tdone with send!\n");
		}

		/* the case for receives */
		if (got_put_start 
				&& got_put_end 
				&& got_unlink) {
			done = 1; 
			(*initiator).nid = event.initiator.nid;
			(*initiator).pid = event.initiator.pid;
			fprintf(stderr, "\tdone with recv from (nid=%llu,pid=%llu)!\n",
					(unsigned long long)(*initiator).nid,
					(unsigned long long)(*initiator).pid);
		}
	}


	return rc;
}



/*  Receive a message. */
static int recv_message(
	ptl_process_id_t src,
	struct message *msg,
	ptl_handle_ni_t ni_h,
	ptl_pt_index_t portal_index,
	ptl_match_bits_t match_bits,
	ptl_handle_eq_t eq_h,
	ptl_process_id_t *sender)
{
	int rc; 

	ptl_md_t md; 
	ptl_handle_me_t me_h; 
	ptl_handle_md_t md_h; 

	/* initialize the md for the incoming buffer */
	memset(&md, 0, sizeof(ptl_md_t)); 
	md.start = msg;
	md.length = sizeof(struct message); 
	md.threshold = 1;  /* only expect one operation */
	md.options = PTL_MD_OP_PUT | PTL_MD_TRUNCATE;
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


	/* wait for a remote Put to complete */
	rc = wait(eq_h, 1, sender); 
	if (rc != PTL_OK) {
		fprintf(stderr, "PtlPut() failed: %s (%d)\n",
				PtlErrorStr(rc), rc);
		exit(-1);
	}

	return 0; 
}


/* sends a single message to a server */
static int send_message(
	ptl_process_id_t dest, 
	struct message *msg,
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
	md.start = msg;
	md.length = sizeof(struct message);
	//md.threshold = PTL_MD_THRESH_INF;
	md.threshold = 2;  /* send + ack */
	//md.options = PTL_MD_OP_PUT | PTL_MD_TRUNCATE;
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

	return rc; 
}


/* This is what executes in the server thread */
static void *server_send_message(void *data)
{
	int rc; 
	ptl_handle_eq_t eq_h; 
	static volatile int tnum = 0;
	int thread_rank; 


	struct send_args *send_args = (struct send_args *)data; 

	ptl_process_id_t client = send_args->client;
	ptl_handle_ni_t ni_h = send_args->ni_h;
	ptl_pt_index_t portal_index = send_args->portal_index;
	ptl_match_bits_t match_bits = send_args->match_bits;
	struct message *msg = &send_args->msg; 

	/* increment the thread number */
	tnum++;
	thread_rank = tnum;

	fprintf(stderr, "started thread %d\n", thread_rank);

	/* create an event queue for the result buffer */
	rc = PtlEQAlloc(ni_h, 5, PTL_EQ_HANDLER_NONE, &eq_h);
	if (rc != PTL_OK) {
		fprintf(stderr, "PtlEQAlloc() failed: %s (%d)\n",
				PtlErrorStr(rc), rc); 
		exit(-1); 
	}

	fprintf(stderr, "sending response to (nid=%llu,pid=%llu) index=%d, match_bits=%d\n",
		(unsigned long long)client.nid, (unsigned long long)client.pid, 
		(int)portal_index, (int)match_bits);

	/* send the same message back to client */
	send_message(client, msg, ni_h, 
			portal_index, match_bits, eq_h); 
	wait(eq_h, 0, NULL); 

	fprintf(stderr, "response sent to client\n");

	/* free the event queue */
	rc = PtlEQFree(eq_h);
	if (rc != PTL_OK) {
		fprintf(stderr, "PtlEQFree() failed: %s (%d)\n",
				PtlErrorStr(rc), rc); 
		abort();
	}

	/* free the data */
	free(data);

	return NULL;
}



/*  Receive one message from the client, return the same 
 *  message, then exit.  We're trying to model the behavior 
 *  of LWFS RPC. */
static int ping_pong_server(
	const int count,
	ptl_handle_ni_t ni_h,
	ptl_pt_index_t portal_index,
	ptl_match_bits_t match_bits)
{
	int i, rc;
	pthread_t thread[count];
	ptl_sr_value_t status; 
	ptl_handle_eq_t eq_h;
	ptl_md_t md;
	ptl_handle_md_t md_h; 
	ptl_process_id_t any_client;
	ptl_handle_me_t me_h;
	const int max_reqs = 10; 
	struct message incoming_buf[max_reqs];

	
	/* create an event queue for incoming messages */
	rc = PtlEQAlloc(ni_h, 5, PTL_EQ_HANDLER_NONE, &eq_h);
	if (rc != PTL_OK) {
		fprintf(stderr, "PtlEQAlloc() failed: %s (%d)\n",
				PtlErrorStr(rc), rc); 
		exit(-1); 
	}

	/* create a memory descriptor for incoming messages */
	memset(&md, 0, sizeof(ptl_md_t));
	md.start = incoming_buf;
	md.length = max_reqs*sizeof(struct message);
	md.threshold = max_reqs;
	md.max_size = sizeof(struct message);
	md.options = PTL_MD_OP_PUT | PTL_MD_MAX_SIZE;
	md.eq_handle = eq_h; 

	/* accept messages from anybody */
	any_client.nid = PTL_NID_ANY;
	any_client.pid = PTL_PID_ANY;

	/* attach the match entry to the portal index */
	rc = PtlMEAttach(ni_h, portal_index, any_client, match_bits, 0, 
		PTL_UNLINK, PTL_INS_AFTER, &me_h);
	if (rc != PTL_OK) {
		fprintf(stderr, "PtlMEAttach() failed: %s (%d)\n",
				PtlErrorStr(rc), rc); 
		exit(-1); 
	}

	/* attach the MD to the match entry */
	rc = PtlMDAttach(me_h, md, PTL_UNLINK, &md_h);
	if (rc != PTL_OK) {
		fprintf(stderr, "PtlMDAttach() failed: %s (%d)\n",
				PtlErrorStr(rc), rc); 
		exit(-1); 
	}


	for (i=0; i<count; i++) {
		pthread_attr_t attr; 
		ptl_process_id_t client;


		struct send_args *send_args = (struct send_args *)malloc(sizeof(struct send_args));

		/* will be freed in the server thread */
		struct message *msg;


		/* initialize the send arguments */
		memset(send_args, 0, sizeof(struct send_args));

		msg = &(send_args->msg);

		/* get the status of the network interface (see if we dropped anything) */
		PtlNIStatus(ni_h, PTL_SR_DROP_COUNT, &status); 
		fprintf(stderr, "PTL_SR_DROP_COUNT = %d\n", (int)status);

		/* receive a message from the client */
		fprintf(stderr, "waiting for msg %d on index=%d match_bits=%d\n",
				i, (int)portal_index, (int)(match_bits));


		/* wait for a remote Put to complete */
		rc = wait(eq_h, 1, &client); 
		if (rc != PTL_OK) {
			fprintf(stderr, "PtlPut() failed: %s (%d)\n",
					PtlErrorStr(rc), rc);
			exit(-1);
		}

		fprintf(stderr, "recv'd msg[%d] = \"%s\" from (nid=%llu,pid=%llu)\n", i, 
			msg->buf, (unsigned long long)client.nid, (unsigned long long)client.pid);

		send_args->client = client;
		send_args->ni_h = ni_h;
		send_args->portal_index = msg->portal_index;
		send_args->match_bits = msg->match_bits;

		/* set the attributes for the thread */
		pthread_attr_init(&attr);
		//pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

		/* spawn a thread to send the result */
		pthread_create(&thread[i], &attr, server_send_message, send_args);
	}

	/* wait for all the threads to complete */
	fprintf(stderr, "waiting for %d threads to complete\n", count);
	for (i=0; i<count; i++) {
		pthread_join(thread[i], NULL);
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
	ptl_handle_eq_t *eq_h; 
	ptl_handle_eq_t recv_eq_h; 
	int rank=-1;

	struct message send_msg[count];
	struct message recv_msg[count];

	//MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	eq_h = (ptl_handle_eq_t *)malloc(count*sizeof(ptl_handle_eq_t));

	/* initialize the buffer */

	/* async send */
	for (i=0; i<count; i++) {

		fprintf(stderr, "%d\n",i);

		/* create an event queue for the outgoing buffer */
		rc = PtlEQAlloc(ni_h, 5, PTL_EQ_HANDLER_NONE, &eq_h[i]);
		if (rc != PTL_OK) {
			fprintf(stderr, "PtlEQAlloc() failed: %s (%d)\n",
					PtlErrorStr(rc), rc); 
			exit(-1); 
		}

		/* initialize the message (tells server where to send response) */
		memset(&send_msg[i], 0, sizeof(struct message));
		send_msg[i].portal_index = portal_index+1;    // server index + 1
		send_msg[i].match_bits = match_bits+100+i; 
		sprintf(send_msg[i].buf, "%d: hello %d", rank, i);

		/* send initial message to server */
		send_message(server, &send_msg[i], ni_h, portal_index, 
				match_bits, eq_h[i]); 
		fprintf(stderr, "sent msg[%d]=\"%s\" to (nid=%llu,pid=%llu) index=%d, match_bits=%d\n", 
				i, send_msg[i].buf,
				(unsigned long long)server.nid, (unsigned long long)server.pid,
				(int)portal_index, (int)match_bits);
	}


	/* create an event queue for incoming message */
	rc = PtlEQAlloc(ni_h, 5, PTL_EQ_HANDLER_NONE, &recv_eq_h);
	if (rc != PTL_OK) {
		fprintf(stderr, "PtlEQAlloc() failed: %s (%d)\n",
				PtlErrorStr(rc), rc); 
		exit(-1); 
	}


	for (i=0; i<count; i++) {
		ptl_process_id_t sender; 

		/* receive return message from server */
		fprintf(stderr, "waiting for response[%d] (index=%d, match_bits=%d) "
				"from server\n", i, (int)send_msg[i].portal_index, (int)send_msg[i].match_bits);

		recv_message(server, &recv_msg[i], ni_h, 
				send_msg[i].portal_index, send_msg[i].match_bits, recv_eq_h, &sender);
		fprintf(stderr, "recv'd response[%d] = \"%s\"\n", i, recv_msg[i].buf);
	}


	/* wait for each send request to complete */
	for (i=0; i<count; i++) {
		rc = wait(eq_h[i], 0, NULL); 

		/* free the event queue */
		rc = PtlEQFree(eq_h[i]);
		if (rc != PTL_OK) {
			fprintf(stderr, "PtlEQFree() failed: %s (%d)\n",
					PtlErrorStr(rc), rc); 
			abort();
		}
	}

	/* free the event queue for incoming reqs */
	rc = PtlEQFree(recv_eq_h);
	if (rc != PTL_OK) {
		fprintf(stderr, "PtlEQFree() failed: %s (%d)\n",
				PtlErrorStr(rc), rc); 
		abort();
	}

	free(eq_h);

	return rc; 
}



/* ----------------- COMMAND-LINE OPTIONS --------------- */

const char *argp_program_version = "$Revision: 388 $"; 
const char *argp_program_bug_address = "<raoldfi@sandia.gov>"; 

static char doc[] = "Test the LWFS storage service";
static char args_doc[] = "HOSTNAME"; 

/**
 * @brief Command line arguments for testing the storage service. 
 */
struct arguments {

	/** @brief The number of requests to send at a time. */
	int count; 

	/** @brief A flag that identifies a server. */
	int server_flag; 

	/** @brief Number of threads on the server. */
	int server_num_threads; 

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
	fprintf(fp, "%s \tPing_Pong Portals Test\n", prefix);
	fprintf(fp, "%s \t%s", prefix, asctime(localtime(&now)));
	fprintf(fp, "%s ------------  ARGUMENTS -----------\n", prefix);
	if (args->server_flag) {
		fprintf(fp, "%s Running as a server\n", prefix);
	}
	else {
		fprintf(fp, "%s Running as a client\n", prefix);
	}

	fprintf(fp, "%s \t--count = %d\n", prefix, args->count);

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
	{"count",      3, "<val>", 0, "number of requests to send"},
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

		case 3: 
			arguments->count = atoi(arg);
			break;

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
	int num_if; 
	struct arguments args; 

	/* fixed for all runs */
	ptl_pt_index_t portal_index=4;   
	ptl_match_bits_t match_bits=5; 

	ptl_process_id_t my_id; 
	ptl_handle_ni_t ni_h;
	ptl_pid_t pid = PTL_PID_ANY; 
	
	args.server_flag = 0;  /* default to client */
	args.count = 1; 

	args.client.nid = PTL_NID_ANY;  /* default client NID */
	args.client.pid = 127;          /* default client PID */

	args.server.nid = PTL_NID_ANY;  /* default server NID */
	args.server.pid = 128;          /* default server PID */

	/* these must be set, otherwisze, PtlNIInit segfaults */
	utcp_api_out = stdout; 
	utcp_lib_out = stdout; 

	/* parse command line options */
	argp_parse(&argp, argc, argv, 0 ,0, &args); 
	

	/* Initialize the library */
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

	/* Get and print the ID of this process */
	if (PtlGetId(ni_h, &my_id) != PTL_OK) {
		printf("PtlGetId() failed.\n");
		abort();
	}

	printf("%s: nid = %llu, pid = %llu\n",
			argv[0], 
			(unsigned long long) my_id.nid,
			(unsigned long long) my_id.pid);

	/* set the default nid of the client/server  */
	if (args.server.nid == PTL_NID_ANY) args.server.nid = my_id.nid; 
	if (args.client.nid == PTL_NID_ANY) args.client.nid = my_id.nid; 

	print_args(stdout, "", &args); 


	/***************** EVERYTHING ABOVE USED FOR BOTH SENDER AND RECEIVER *******/

	if (args.server_flag) {
		fprintf(stderr, "starting server (BUFSIZE=%d)...\n",BUFSIZE);
		ping_pong_server(args.count, ni_h, portal_index, match_bits);
	}
	else { 
		fprintf(stderr, "starting client (BUFSIZE=%d)...\n",BUFSIZE);
		fprintf(stderr, "connecting to nid=%u, pid=%u.\n",
				args.server.nid, args.server.pid);

		/* copy the message */
		ping_pong_client(args.count, args.server, ni_h, portal_index, match_bits); 
	}

	/* cleanup */
	PtlNIFini(ni_h);
	PtlFini();

	return 0;
}
