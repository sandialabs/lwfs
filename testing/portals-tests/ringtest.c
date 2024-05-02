
/* Copyright (C) 2004 Sandia National Laboratories.  All Rights Reserved.
 *
 * This file is part of the Portals3 Reference Implementation.
 *
 * Portals3 is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License,
 * as published by the Free Software Foundation.
 *
 * Portals3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Portals3; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */ 

/*  This Portals 3 program sends a message around a ring.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <portals3.h>
#include P3_NAL
#include <p3rt/p3rt.h>
#include <p3api/debug.h>

/* The UTCP NAL requires that the application define where the Portals
 * API and library should send any output.
 */
FILE* utcp_api_out;
FILE* utcp_lib_out;

static 
const char usage[] = "\n\
ringtest [-hv] [-d <dbg>] [-t <cnt>] \n\
\n\
	Performs a simple communication test by passing a token from \n\
	process rank n to rank mod(n+1,N), where 0 <= n < N. \n\
\n\
	-d dbg	Sets the debug mask to <dbg>, which will cause the \n\
		  Portals library and the NAL to emit various debugging \n\
		  output, assuming both were configured with debugging \n\
		  enabled.  See p3api/debug.h for appropriate values.  \n\
		  See also NAL documentation or implementation for \n\
		  appropriate NAL-specific values. \n\
	-h	Prints this message. \n\
	-t cnt	Sets the number of trips the token takes around the ring \n\
		  to <cnt>. \n\
	-v	Causes ringtest to be verbose about the progress of \n\
		  the token. \n\
";


static void test_fail(int rank, const char *str, int rc)
{
	fprintf(stderr, "%d: %s: %s (%d)\n", rank, str, PtlErrorStr(rc), rc);
}

int main(int argc, char *argv[])
{
	int prev, next;
	unsigned int rank, size;
	int num_if;

	ptl_process_id_t my_id, prev_id, next_id;
	ptl_match_bits_t send_mbits, recv_mbits, ibits;
	ptl_handle_ni_t ni_h;
	ptl_handle_me_t me_h;
	ptl_md_t md;
	ptl_handle_md_t md_h;
	ptl_handle_eq_t eq_h;
	ptl_event_t ev;
	int token, have_token, rc, count = 16;
	unsigned dbg = 0;
	int verbose = 0;

	utcp_api_out = stdout;
	utcp_lib_out = stdout;

	while (1) {
		int c = getopt(argc, argv, "d:ht:v");
		if (c == -1) break;

		switch (c) {
		case 'd':
			dbg = strtoul(optarg, NULL, 0);
			break;
		case 'h':
			printf("%s", usage);
			exit(EXIT_SUCCESS);
		case 't':
			count = strtol(optarg, NULL, 0);
			break;
		case 'v':
			verbose = 1;
			break;
		}
	}

	/* Initialize library 
	 */
	if (PtlInit(&num_if) != PTL_OK) {
		fprintf(stderr, "PtlInit() failed\n");
		exit(1);
	}
	/* Initialize the interface 
	 */
	if (PtlNIInit(PTL_IFACE_DEFAULT, PTL_PID_ANY,
		      NULL, NULL, &ni_h) != PTL_OK) {
		fprintf(stderr, "PtlNIInit() failed\n");
		exit(EXIT_FAILURE);
	}
	PtlNIDebug(ni_h, dbg);

	/* Get my id
	 */
	if (PtlGetId(ni_h, &my_id) != PTL_OK) {
		fprintf(stderr, "PtlGetId() failed\n");
		exit(EXIT_FAILURE);
	}
	/* Get my rank and group size 
	 */
	if (PtlGetRank(ni_h, &rank, &size) != PTL_OK) {
		fprintf(stderr, "PtlGetRank() failed\n");
		exit(EXIT_FAILURE);
	}

	/* Figure out my next neighbor's rank and id
	 */
	next = (rank + 1) % size;
	if (PtlGetRankId(ni_h, next, &next_id) != PTL_OK) {
		fprintf(stderr, "PtlGetRankId() failed for next rank\n");
		exit(EXIT_FAILURE);
	}
	/* Figure out my previous neighbor's rank 
	 */
	prev = (rank + size - 1) % size;
	if (PtlGetRankId(ni_h, prev, &prev_id) != PTL_OK) {
		fprintf(stderr, "PtlGetRankId() failed for prev rank\n");
		exit(EXIT_FAILURE);
	}

	/* All match bits are significant 
	 */
	ibits = 0;

	/* Match bits are prev rank 
	 */
	recv_mbits = (ptl_match_bits_t) prev;

	rc = PtlMEAttach(ni_h, 4,	/* portal table index */
			 prev_id,	/* source address */
			 recv_mbits,	/* expected match bits */
			 ibits,		/* ignore bits to mask */
			 PTL_UNLINK,	/* unlink when md is unlinked */
			 PTL_INS_AFTER,
			 &me_h);
	if (rc != PTL_OK) {
		fprintf(stderr, "%d: PtlMEAttach() failed: %s (%d)\n",
			rank, ptl_err_str[rc], rc);
		exit(EXIT_FAILURE);
	}

	/* Create an event queue 
	 */
	rc = PtlEQAlloc(ni_h, 2, PTL_EQ_HANDLER_NONE, &eq_h);
	if (rc != PTL_OK) {
		fprintf(stderr, "%d: PtlEQAlloc() failed: %s (%d)\n",
			rank, PtlErrorStr(rc), rc);
		exit(EXIT_FAILURE);
	}

	/* Create a memory descriptor 
	 */
	md.start = &token;			/* start address */
	md.length = sizeof(token);		/* length of buffer */
	md.threshold = PTL_MD_THRESH_INF;	/* number of expected
						 * operations on md */
	md.options =
		PTL_MD_OP_PUT | PTL_MD_MANAGE_REMOTE | PTL_MD_TRUNCATE |
		PTL_MD_EVENT_START_DISABLE;
	md.max_size = 0;
	md.user_ptr = NULL;	/* nothing to cache */
	md.eq_handle = eq_h;	/* event queue handle */

	/* Attach the memory descriptor to the match entry 
	 */
	rc = PtlMDAttach(me_h, md,	/* md to attach */
			 PTL_UNLINK,	/* unlink when threshold is 0 */
			 &md_h);
	if (rc != PTL_OK) {
		fprintf(stderr, "%d: PtlMDAttach() failed: %s (%d)\n",
			rank, ptl_err_str[rc], rc);
		exit(EXIT_FAILURE);
	}

	/* Rank zero gets the token first 
	 */
	have_token = rank == 0 ? 1 : 0;

	do {
		if (have_token) {
			if (verbose || rank == 0)
				printf("%d: Sending token to %d (round %d)\n",
					rank, next, count);

			send_mbits = (ptl_match_bits_t) rank;

			rc = PtlPut(md_h, PTL_NO_ACK_REQ,
				    next_id, 4, 0, send_mbits, 0, 0);
			if (rc != PTL_OK) {
				test_fail(rank, "PtlPut", rc);
				exit(EXIT_FAILURE);
			}
			/* Wait for the send to complete 
			 */
			rc = PtlEQWait(eq_h, &ev);
			if (rc != PTL_OK && rc != PTL_EQ_DROPPED) {
				test_fail(rank, "PtlEQWait for send", rc);
				exit(EXIT_FAILURE);
			}
			/* Check for NI failure
			 */
			if (ev.ni_fail_type != PTL_NI_OK) {
				fprintf(stderr,	"%d: NI sent %s in event "
					"for send.\n", rank, 
					PtlNIFailStr(ni_h, ev.ni_fail_type));
				exit(EXIT_FAILURE);
			}
			/* Check event type 
			 */
			if (ev.type != PTL_EVENT_SEND_END) {
				fprintf(stderr,	"%d: expected "
					"PTL_EVENT_SEND_END, got %s\n",
					rank, PtlEventKindStr(ev.type));
				exit(EXIT_FAILURE);
			}
			have_token = 0;
			count--;
		}
		if (rank > 0 && count == 0 ) break;
		
		rc = PtlEQWait(eq_h, &ev);
		if (rc != PTL_OK && rc != PTL_EQ_DROPPED) {
			test_fail(rank, "PtlEQWait for recv", rc);
			exit(EXIT_FAILURE);
		}
		/* Check for NI failure
		 */
		if (ev.ni_fail_type != PTL_NI_OK) {
			fprintf(stderr,
				"%d: NI sent %s in event for receive.\n",
				rank, PtlNIFailStr(ni_h, ev.ni_fail_type));
			exit(EXIT_FAILURE);
		}
		/* Check event type 
		 */
		if (ev.type != PTL_EVENT_PUT_END) {
			fprintf(stderr, "%d: expected "
				"PTL_EVENT_PUT_END got %s\n",
				rank, PtlEventKindStr(ev.type));
			exit(EXIT_FAILURE);
		}
		else {
			have_token = 1;
			if (verbose)
				printf("%d: received token from %d"
				       " (round %d)\n",
				       rank, prev, count);
		}
		if (rank == 0 && count == 0) break;
	} while (1);

	/* Close down the network interface 
	 */
	if (verbose || rank == 0) {
		printf("%d: Passed all rounds\n", rank);
	}

	PtlNIFini(ni_h);

	/* Close down library 
	 */
	PtlFini();

	return 0;
}
