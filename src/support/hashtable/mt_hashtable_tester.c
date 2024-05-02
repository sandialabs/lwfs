/* Copyright (C) 2002, 2004 Christopher Clark <firstname.lastname@cl.cam.ac.uk> */

/*
 * Copyright (c) 2002, 2004, Christopher Clark
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 	* Redistributions of source code must retain the above copyright 
 *        notice, this list of conditions and the following disclaimer.
 *
 *	* Redistributions in binary form must reproduce the above 
 *        copyright notice, this list of conditions and the 
 *        following disclaimer in the documentation and/or other 
 *        materials provided with the distribution.
 *
 *	* Neither the name of the original author; nor the names of 
 *	  any contributors may be used to endorse or promote products derived
 *	  from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "mt_hashtable.h"
#include "hash_funcs.h"
#include "hashtable.h"
#include "hashtable_itr.h"
#include <stdlib.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h> /* for memcmp */

#define NUM_THREADS 9


typedef void *(*tproc_t)(void *);

/**
 * @brief Hash a char *. 
 * 
 * This hash was created by Dan Bernstein and reported in comp.lang.c. 
 */
static unsigned int
my_hash(void *ky)
{
	char *key = (char *)ky;

	return RSHash(key, strlen(key));
}

static int
equalkeys(void *k1, void *k2)
{
	return (0 == strcmp(k1,k2));
}

struct args {
	int index; 
	struct mt_hashtable *table; 
	int count; 
	struct timeval start; 
};

static int add_vals(struct args *args) 
{
	struct timeval tv; 

	struct mt_hashtable *table = args->table; 
	int count = args->count;
	int i; 
	double time; 


	for (i = 0; i<count; i++) {
		char *key = (char *)calloc(25, sizeof(char));
		char *data = (char *)calloc(25, sizeof(char));

		gettimeofday(&tv, NULL); 

		time = (double)(tv.tv_sec - args->start.tv_sec) * 1e6
			+ (tv.tv_usec - args->start.tv_usec);

		sprintf(key, "%g", time);
		sprintf(data, "%d: %g sec", args->index, time);

		fprintf(stderr, "inserting ""%s""\n", data);

		if (!mt_hashtable_insert(table, key, data)) {
			fprintf(stderr, "%s\n", data); 
		}

		//sched_yield();
	}

	pthread_exit(NULL);
}

void myfree(void *data)
{
	static int count=0; 

	fprintf(stderr,"freeing item %d\n",++count);
	free(data);
}

/**
 * 
 */
int
main(int argc, char **argv)
{
	/* a thread-safe hash table */
	struct mt_hashtable mt_table; 

	int tablesize = 16; 
	int i; 
	struct args args[NUM_THREADS];
	pthread_t thread[NUM_THREADS];

	struct timeval start; 

	if (!create_mt_hashtable(tablesize, my_hash, equalkeys, &mt_table)) { 
		fprintf(stderr, "could not create hash table");
		return -1; 
	}

	gettimeofday(&start, NULL); 

	for (i=0; i<NUM_THREADS; i++) {
		args[i].index = i; 
		args[i].table = &mt_table; 
		args[i].count = 16;
		args[i].start = start; 
		pthread_create(&thread[i], NULL, (tproc_t)add_vals, (void *)&args[i]); 
	}

	for (i=0; i<NUM_THREADS; i++) {
		pthread_join(thread[i], NULL);
	}


	fprintf(stderr,"destroying hashtable (items=%d)\n",mt_hashtable_count(&mt_table));
	mt_hashtable_destroy(&mt_table, myfree);

	pthread_exit(NULL);
}
