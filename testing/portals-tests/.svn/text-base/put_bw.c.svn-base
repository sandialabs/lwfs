
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

/*  This Portals 3.0 program tests the put bandwidth.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <float.h>
#include <sys/time.h>

#include <portals3.h>
#include P3_NAL
#include <p3rt/p3rt.h>
#include <p3api/debug.h>

#define INFLIGHT_BITS 5
#define MAX_INFLIGHT (1 << INFLIGHT_BITS)

static 
const char usage[] = " \n\
put_bw [-dhv] [-m <min>] [-M <max>] [-i [x]<inc>] [-n <cnt>] [-o <cnt>] \n\
 \n\
	Performs a bandwidth test using PtlPut calls between pairs of \n\
	processes, where rank n/2 processes send data and rank 1+n/2 \n\
	processes receive data, for 0 <= n <= N/2, with N even. \n\
 \n\
	The elapsed time clock for each message sent starts when PtlPut \n\
	is called, and ends when an ACK is received. \n\
 \n\
	-d dbg	Sets the debug mask to <dbg>, which will cause the \n\
		  Portals library and the NAL to emit various debugging \n\
		  output, assuming both were configured with debugging \n\
		  enabled.  See p3api/debug.h for appropriate values.  \n\
		  See also NAL documentation or implementation for \n\
		  appropriate NAL-specific values. \n\
	-h	Prints this message. \n\
	-i inc	Sets the message size increment.  If prepended with  \n\
		  'x', e.g. '-i x2', the message size increase is \n\
		  multiplicative rather than the default additive. \n\
	-m min	Sets the size of the smallest message sent, in bytes. \n\
	-M max	Sets the upper bound on the size of messages sent. \n\
		  Inc, min, and max may be suffixed with one of the \n\
		  following multipliers: \n\
		  k	*1000 \n\
		  M	*1000*1000 \n\
		  G	*1000*1000*1000 \n\
		  Ki	*1024 \n\
		  Mi	*1024*1024 \n\
		  Gi	*1024*1024*1024 \n\
	-n cnt	Sets the number of messages sent for each message size. \n\
	-o cnt	Sets the number of outstanding puts allowed.  Increasing \n\
		  this value from 1 allows more pipelining of messages. \n\
	-v	Causes put_bw to be verbose about the progress of \n\
		  each trial. \n\
";


FILE* utcp_api_out;
FILE* utcp_lib_out;

typedef struct msg {
	char *buf;

	ptl_handle_me_t me_h;
	ptl_handle_md_t md_h;
	ptl_handle_eq_t eq_h;

	ptl_md_t md;
	ptl_md_t md_save;

	struct timeval start;
	struct timeval end;
	unsigned id;
} msg_t;

static inline
void event(msg_t *msg, ptl_event_t *ev,
	   ptl_event_kind_t evt, int rank, ptl_handle_ni_t ni_h)
{
	int rc;

	if ((rc = PtlEQWait(msg->eq_h, ev)) != PTL_OK) {
		fprintf(stderr, "%d:PtlEQWait(): %s\n",
			rank, PtlErrorStr(rc));
		exit(EXIT_FAILURE);
	}
	if (ev->ni_fail_type != PTL_NI_OK) {
		fprintf(stderr, "%d:NI sent %s in event.\n",
			rank, PtlNIFailStr(ni_h, ev->ni_fail_type));
		exit(EXIT_FAILURE);
	}
	if (ev->type != evt) {
		fprintf(stderr,	"%d:expected %s, got %s\n", rank, 
			PtlEventKindStr(evt), PtlEventKindStr(ev->type));
		exit(EXIT_FAILURE);
	}
}

/* returns the difference *tv1 - *tv0 in microseconds.
 */
static inline
double tv_diff(struct timeval *tv0, struct timeval *tv1)
{
	return (double)(tv1->tv_sec - tv0->tv_sec) * 1e6
		+ (tv1->tv_usec - tv0->tv_usec);
}

static
ptl_size_t suffix(const char *str)
{
	ptl_size_t s = 1;

	switch (*str) {
	case 'k':
		s *= 1000;
		break;
	case 'K':
		if (*(str+1) == 'i')
			s *= 1024;
		break;
	case 'M':
		if (*(str+1) == 'i')
			s *= 1024*1024;
		else
			s *= 1000*1000;
		break;
	case 'G':
		if (*(str+1) == 'i')
			s *= 1024*1024*1024;
		else
			s *= 1000*1000*1000;
		break;
	}

	return s;
}

int main(int argc, char *argv[])
{
	unsigned int rank, size;
	int src;
	int num_if, rc;

	ptl_pt_index_t ptl = 4;
	ptl_process_id_t src_id, dst_id;
	ptl_handle_ni_t ni_h;
	ptl_event_t ev;
	ptl_size_t len, inc = 16, start_len = 0, end_len = 64;
	unsigned sender, trials = 1000;
	unsigned inflight = 1, add_inc = 1;
	unsigned i, m, dbg = 0, done = 0, verbose = 0;

	msg_t *msg, *cmsg;

	struct timeval pgm_start, tv0, tv1;
	double et, ave_et, total_et;
	double min_et, max_et;

	utcp_api_out = stdout;
	utcp_lib_out = stdout;

	while (1) {
		char *next_char;
		int c = getopt(argc, argv, "d:hi:m:M:n:o:v");
		if (c == -1) break;

		switch (c) {
		case 'd':
			dbg = strtoul(optarg, NULL, 0);
			break;
		case 'h':
			printf("%s", usage);
			exit(EXIT_SUCCESS);
		case 'i':
			if (*optarg == 'x') {
				optarg++;
				add_inc = 0;
			}
			inc = strtoul(optarg, &next_char, 0);
			inc *= suffix(next_char);
			break;
		case 'm':
			start_len = strtoul(optarg, &next_char, 0);
			start_len *= suffix(next_char);
			break;
		case 'M':
			end_len = strtoul(optarg, &next_char, 0);
			end_len *= suffix(next_char);
			break;
		case 'n':
			trials = strtoul(optarg, NULL, 0);
			break;
		case 'o':
			inflight = strtoul(optarg, NULL, 0);
			break;
		case 'v':
			verbose = 1;
			break;
		}
	}
	gettimeofday(&pgm_start, 0);

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

	/* Get my rank and group size 
	 */
	if (PtlGetRank(ni_h, &rank, &size) != PTL_OK) {
		fprintf(stderr, "PtlGetRank() failed\n");
		exit(EXIT_FAILURE);
	}
	if (size & 1) {
		if (!rank)
			fprintf(stderr,
				"Put_bw requires an even number of tasks.\n");
		exit(EXIT_FAILURE);
	}
	sender = !(rank & 1);

	if (end_len < start_len) {
		if (rank == 0) 
			fprintf(stderr, "Error: end_len < start_len.\n");
		exit(EXIT_FAILURE);
	}
	if (inflight > MAX_INFLIGHT) {
		if (rank == 0) 
			fprintf(stderr, "Error: use inflight <= %d.\n",
				MAX_INFLIGHT);
		exit(EXIT_FAILURE);
	}

	/* Get source (initiator) and destination (target) ids
	 */
	src = rank >> 1;
	src <<= 1;
	if (PtlGetRankId(ni_h, src, &src_id) != PTL_OK) {
		fprintf(stderr, "%d:PtlGetRankId() failed\n", rank);
		exit(EXIT_FAILURE);
	}
	if (PtlGetRankId(ni_h, src+1, &dst_id) != PTL_OK) {
		fprintf(stderr, "%d:PtlGetRankId() failed\n", rank);
		exit(EXIT_FAILURE);
	}

	/* Allocate the buffers and memory descriptors used for the messages.
	 */
	msg = malloc(inflight*sizeof(*msg));
	if (!msg) {
		perror("buffer malloc");
		exit(EXIT_FAILURE);
	}
	for (i=0; i<inflight; i++) {

		msg[i].buf = malloc(end_len);
		if (!msg[i].buf) {
			perror("buffer malloc");
			exit(EXIT_FAILURE);
		}
		memset(&msg[i].md, 0, sizeof(msg[i].md));
		msg[i].md.start = msg[i].buf;
		msg[i].md.length = end_len;
		msg[i].md.threshold = PTL_MD_THRESH_INF;
		msg[i].md.options = PTL_MD_OP_PUT | PTL_MD_EVENT_START_DISABLE;
		msg[i].md.user_ptr = msg + i;

		/* create an event queue for this message.
		 */
		rc = PtlEQAlloc(ni_h, 5, PTL_EQ_HANDLER_NONE, &msg[i].eq_h);
		if (rc != PTL_OK) {
			fprintf(stderr, "%d:PtlEQAlloc() failed: %s (%d)\n",
				rank, PtlErrorStr(rc), rc);
			exit(EXIT_FAILURE);
		}
		msg[i].md.eq_handle = msg[i].eq_h;

		/* If we want to reuse our memory descriptors, we need a
		 * way to reset the library memory descriptor's local
		 * offset, which gets updated on a successful operation.
		 * We'll use PtlMDUpdate to do that, so we'll save our
		 * values here.
		 */
		msg[i].md_save = msg[i].md;

		if (sender) {
			/* Bind the memory descriptor we use for sending.
			 */
			rc = PtlMDBind(ni_h, msg[i].md,
				       PTL_RETAIN, &msg[i].md_h);
			if (rc != PTL_OK) {
				fprintf(stderr, "%d:PtlMDBind: %s\n",
					rank, PtlErrorStr(rc));
				exit(EXIT_FAILURE);
			}
		}
		else {
			/* Create a match entry to receive this buffer.
			 * Use the source rank as the match bits, and
			 * don't ignore any bits.
			 */
			ptl_match_bits_t mb = 
				(ptl_match_bits_t )src << INFLIGHT_BITS | i;
			rc = PtlMEAttach(ni_h, ptl, src_id, mb, 0,
					 PTL_RETAIN, PTL_INS_AFTER,
					 &msg[i].me_h);
			if (rc != PTL_OK) {
				fprintf(stderr, "%d:PtlMEAttach: %s\n",
					rank, PtlErrorStr(rc));
				exit(EXIT_FAILURE);
			}
			/* Attach the memory descriptor to the match entry.
			 */
			rc = PtlMDAttach(msg[i].me_h, msg[i].md,
					 PTL_RETAIN, &msg[i].md_h);
			if (rc != PTL_OK) {
				fprintf(stderr, "%d:PtlMDAttach: %s\n",
					rank, PtlErrorStr(rc));
				exit(EXIT_FAILURE);
			}
		}
	}
	/* UTCP NAL, at least, brings up communications on demand.
	 * Send a message we don't care about, wait for reply so that 
	 * startup overhead isn't present in first timed message.
	 */
	if (sender) {
		rc = PtlPutRegion(msg[0].md_h, 0, 0, PTL_ACK_REQ,
				  dst_id, ptl, 0, src, 0, 0);
		if (rc != PTL_OK) {
			fprintf(stderr, "%d:PtlPut: %s\n",
				rank, PtlErrorStr(rc)); 
			exit(EXIT_FAILURE);
		}
		event(msg, &ev, PTL_EVENT_SEND_END, rank, ni_h);
		event(msg, &ev, PTL_EVENT_ACK, rank, ni_h);
	}
	else event(msg, &ev, PTL_EVENT_PUT_END, rank, ni_h);

	if (rank == 0) {
		printf("\nResults for %d trial%s, ",
		       trials, (trials > 1 ? "s" : ""));
		printf("with %d message%s in flight:\n", 
		       inflight, (inflight > 1 ? "s" : ""));
		printf("  Times in microseconds; ");
		printf("bandwidth in 10^6 bytes/second.\n");
		printf("\n\
  Message    minimum    average    maximum    aggregate \n\
   Length       ET         ET         ET          BW    \n\
\n\
");
	}

	len = start_len;

next_size:
	total_et = 0;
	max_et = 0;
	min_et = DBL_MAX;

	gettimeofday(&tv0, NULL);

	i = m = 0;
	while (sender && !done) {

		if (i < trials) {

			ptl_match_bits_t mb =
				(ptl_match_bits_t )src << INFLIGHT_BITS | m;

			/* Restore local offset in bound memory descriptor;
			 * keep track of which trial the message buffer is
			 * used for.
			 */
			msg[m].id = i;
			if (verbose) 
				printf("%d:   PtlMDUpdate: len %d trial %d, "
				       "msg %d\n", rank, (int)len, i, m);
			rc = PtlMDUpdate(msg[m].md_h, NULL,
					 &msg[m].md_save, msg[m].eq_h);
			if (rc != PTL_OK) {
				fprintf(stderr, "%d:PtlMDUpdate: msg %d: %s\n",
					rank, m, PtlErrorStr(rc));
				exit(EXIT_FAILURE);
			}

			gettimeofday(&msg[m].start, 0);

			rc = PtlPutRegion(msg[m].md_h, 0, len, PTL_ACK_REQ,
					  dst_id, ptl, 0, mb, 0, 0);
			if (rc != PTL_OK) {
				fprintf(stderr, "%d:PtlPut: %s\n",
					rank, PtlErrorStr(rc));
				exit(EXIT_FAILURE);
			}
			if (verbose) 
				printf("%d:   PtlPut: len %d trial %d, "
				       "msg %d\n", rank, (int)len, i, m);
		}
		m += 1;
		m %= inflight;

		if (++i < inflight) continue;

		event(msg+m, &ev, PTL_EVENT_SEND_END, rank, ni_h);
		event(msg+m, &ev, PTL_EVENT_ACK, rank, ni_h);

		cmsg = ev.md.user_ptr;
		gettimeofday(&cmsg->end, 0);
		et = tv_diff(&cmsg->start, &cmsg->end);

		total_et += et;
		min_et = min_et < et ? min_et : et;
		max_et = max_et > et ? max_et : et;

		if (verbose)
			printf("%d:   ACK: len %d trial %d msg %d "
			       "start %f end %f et %f bw %f\n",
			       rank, (int)len, cmsg->id, (int)(cmsg-msg), 
			       tv_diff(&pgm_start, &cmsg->start),
			       tv_diff(&pgm_start, &cmsg->end),
			       et, len/et);

		if (cmsg->id == trials-1) break;
	}

	for (m=0; !sender && m<inflight; m++) {

		/* Restore local offset in memory descriptor.
		 *
		 * Note that using PtlMDUpdate this way here is inherently
		 * racy when inflight = 1.  We are counting on being able
		 * notice the ACK event, processes it, and update the
		 * memory descriptor quicker than our ACK can reach our
		 * partner, and new data arrive.  It's a good bet, but it's
		 * still just a bet, not a sure thing.
		 */
		rc = PtlMDUpdate(msg[m].md_h, NULL,
				 &msg[m].md_save, msg[m].eq_h);
		if (rc != PTL_OK) {
			fprintf(stderr, "%d:PtlMDUpdate: %s\n",
				rank, PtlErrorStr(rc));
			exit(EXIT_FAILURE);
		}
	}
	i = 0;
	while (!sender) {

		m %= inflight;

		event(msg+m, &ev, PTL_EVENT_PUT_END, rank, ni_h);

		if (verbose)
			printf("%d:   Received len %d trial %d msg %d\n",
			       rank, (int)ev.mlength, i, m);

		if (++i == trials) break;

		/* Restore local offset in memory descriptor.
		 */
		rc = PtlMDUpdate(msg[m].md_h, NULL,
				 &msg[m].md_save, msg[m].eq_h);
		if (rc != PTL_OK) {
			fprintf(stderr, "%d:PtlMDUpdate: %s\n",
				rank, PtlErrorStr(rc));
			exit(EXIT_FAILURE);
		}
		m++;
	}

	ave_et = total_et / trials;
	gettimeofday(&tv1, NULL);

	if (rank == 0)
		printf("%9d%11.2f%11.2f%11.2f%11.4f\n",
		       (int)len, min_et, ave_et, max_et,
		       len*trials/tv_diff(&tv0, &tv1));

	if (add_inc)
		len += inc;
	else 
		if (len) len *= inc;
		else len = 1;

	if (len <= end_len) goto next_size;

		
	/* close down the network interface 
	 */
	PtlNIFini(ni_h);

	/* finalize library 
	 */
	PtlFini();

	for (i=0; i<inflight; i++) free(msg[i].buf);
	free(msg);

	return 0;
}

