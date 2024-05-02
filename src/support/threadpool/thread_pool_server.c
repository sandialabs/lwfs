#include <stdio.h>             /* standard I/O routines                      */
#include <pthread.h>           /* pthread functions and data structures      */
#include <stdlib.h>            /* rand() and srand() functions               */
#include <unistd.h>            /* sleep()                                    */
#include <assert.h>            /* assert()                                   */

#include "requests_queue.h"         /* requests queue routines/structs       */
#include "handler_thread.h"         /* handler thread functions/structs      */
#include "handler_threads_pool.h"   /* handler thread list functions/structs */


int print_request(struct lwfs_thread_pool_request *a_request, const int thread_id)
{
	int *data = (int *)a_request->client_data;
	
	fprintf(stderr, "thread_id=%d, request=%d\n", thread_id, *data);
	
	return(0);
}

/* like any C program, program's execution begins in main */
int
main(int argc, char* argv[])
{
    int rc=0;
    int i;                  /* loop counter          */
    struct timespec delay;  /* used for wasting time */
    lwfs_thread_pool pool;
    lwfs_thread_pool_args pool_args;

    pool_args.initial_thread_count=5;
    pool_args.min_thread_count=5;
    pool_args.max_thread_count=10;
    pool_args.low_watermark=3;
    pool_args.high_watermark=15;

    rc = lwfs_thread_pool_init(&pool, &pool_args);

    fprintf(stdout, "adding requests\n");
    fflush(stdout);
    
    /* run a loop that generates requests */
    for (i=0; i<600; i++) {
    	int *data = malloc(sizeof(int));
    	*data = i;
	lwfs_thread_pool_add_request(&pool, data, print_request);

	/* pause execution for a little bit, to allow      */
	/* other threads to run and handle some requests.  */
	if (rand() > 3*(RAND_MAX/4)) { /* this is done about 25% of the time */
	    delay.tv_sec = 0;
	    delay.tv_nsec = 1;
	    nanosleep(&delay, NULL);
	}
    }
    
    fprintf(stdout, "done adding requests\n");
    fflush(stdout);
    
    lwfs_thread_pool_fini(&pool);
    
    printf("Glory,  we are done.\n");

    return 0;
}
