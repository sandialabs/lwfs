/**  @file trace.c
 *   
 *   @brief A simple library for generating timing traces. 
 *   
 *   @author Ron Oldfield (raoldfi@sandia.gov)
 *   @version $Revision: 406 $
 *   @date $Date: 2005-10-07 15:08:29 -0600 (Fri, 07 Oct 2005) $
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_PABLO
#include "pablo_interface.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "trace.h"
#include "support/timer/timer.h"
#include "support/logger/logger.h"


/*------------ SUPPORT FOR INTERVAL EVENTS WITH PABLO ------------*/

#include "support/hashtable/mt_hashtable.h"
#include "support/hashtable/hash_funcs.h"

#ifdef HAVE_PTHREAD





/* --- Hashtable that stores the interval ID --- */


static struct mt_hashtable interval_hash;

static unsigned int interval_hash_func(void *data)
{
	return (RSHash((char *)data, sizeof(int)));
}

static int interval_comp_func(void *a, void *b)
{
	int ida = *(int *)(a);
	int idb = *(int *)(b);

	return (ida == idb);
}


/* --- Hashtable to store counter information --- */

/* stores the current count for a particular event ID */
static struct mt_hashtable count_hash;

/* the key for a counter as an id and pid component */
struct count_key {
    int id;
    int pid;
};

static unsigned int count_hash_func(void *data)
{
	return (RSHash((char *)data, sizeof(struct count_key)));
}

static int count_comp_func(void *a, void *b)
{
    struct count_key *keya = (struct count_key *)a; 
    struct count_key *keyb = (struct count_key *)b; 

    return ((keya->id == keyb->id) && (keya->pid == keyb->pid));
}


/* stores the scope of intervals for a particular pid */
static struct mt_hashtable scope_hash;
static pthread_mutex_t scope_mutex = PTHREAD_MUTEX_INITIALIZER;

static unsigned int scope_hash_func(void *data)
{
	return (RSHash((char *)data, sizeof(int)));
}

static int scope_comp_func(void *a, void *b)
{
	int ida = *(int *)(a);
	int idb = *(int *)(b);

	return (ida == idb);
}


#endif

static char *trace_fname;
static int trace_ftype = 0;  /* default to binary traces */

/* default is to disable tracing */
static volatile int tracing_enabled = FALSE;
static volatile int tracing_active = FALSE; 



/*------------ TRACE OUTPUT FUNCTIONS ------------*/


/**
 * @brief Output an interval event. 
 *
 * We use the generic pablo trace and encode the additional information 
 * we want in the data field.  The new data field will be, "interval:$name:duration".
 *
 * Pablo has its own interval records, but they are inadequate because it is 
 * difficult to measure concurrent intervals (e.g., in threads).  
 * A better (and more efficient) way to do this would be to create our own 
 * Pablo record type, but this is a quick "hack" to address our needs. 
 * 
 * @param eventID @input_type  The ID of the trace. 
 * @param pid     @input_type  Process ID. 
 * @param level   @input_type  
 * @param data    @input_type  User-defined data passed in a character 
 *                             string. 
 *
 * @return non-zero if successfull
 * @return 0 if failure
 */
static int output_interval_event(
		const int eventID,
		const int pid, 
		const int level, 
		const char *data, 
		double duration)
{
    int rc = 1; 

    /*fprintf(stderr, "output interval\n");*/
    if (tracing_enabled) {
	{
#ifdef HAVE_PABLO
	    rc = pablo_output_interval_event(eventID, pid, level, data, duration);
#else
	    static const size_t maxlen=256;
	    char newdata[maxlen]; 

	    /* generate the newdata field with interval:data:duration */
	    snprintf(newdata, maxlen, "interval:%s:%d:%g", data, level, duration); 

	    /* generate a generic Pablo trace event with our encoded data */
	    rc = trace_event(eventID, pid, newdata);
#endif
	}
    }

    return rc; 
}

static int output_tput_interval_event(
		const int eventID,
		const int pid, 
		const int level, 
		const char *data, 
		double duration,
		const long num_processed)
{
    int rc = 1; 

    /*fprintf(stderr, "output tput interval\n");*/

    if (tracing_enabled) {
#ifdef HAVE_PABLO
	    rc = pablo_output_tput_event(eventID, pid, level, data, duration, num_processed);
#else
	static const size_t maxlen=256;
	char newdata[maxlen]; 

	/* generate the newdata field with interval:data:duration */
	snprintf(newdata, maxlen, "interval:tput:%s:%ld:%d:%g", data, num_processed, level, duration); 

	/* generate a generic Pablo trace event with our encoded data */
	rc = trace_event(eventID, pid, newdata);
#endif
    }

    return rc; 
}


/**
 * @brief Output a count event. 
 *
 * We use the generic pablo trace and encode the additional information 
 * we want in the data field.  The new data field will be, "interval:$name:duration".
 *
 * Pablo has its own count records, but they are inadequate because they only
 * increment values.  We want to increment, decrement, and set count events. 
 * A better (and more efficient) way to do this would be to create our own 
 * Pablo record type, but this is a quick "hack" that will still work. 
 * 
 * @param intervalID @input_type The interval ID (unique for each interval).
 * @param eventID @input_type  The ID of the trace. 
 * @param data    @input_type  User-defined data passed in a character 
 *                             string. 
 *
 * @return non-zero if successfull
 * @return 0 if failure
 */
static int output_count_event(
		const int eventID,
		const int pid, 
		const char *data, 
		const int count)
{
    int rc = 1; 

    /*fprintf(stderr, "output count\n");*/
    if (tracing_enabled) {
	{
#ifdef HAVE_PABLO
	    rc = pablo_output_count_event(eventID, pid, data, count);
#else
	    static const size_t maxlen=256;
	    char newdata[maxlen]; 

	    /* generate the newdata field with interval:data:duration */
	    snprintf(newdata, maxlen, "count:%s:%d", data, count); 

	    /* generate a generic Pablo trace event with our encoded data */
	    rc = trace_event(eventID, pid, newdata);
#endif
	}
    }

    return rc;
}






/*------------ Initialization and wrapup functions ------------*/


int trace_set_buffer_size(const unsigned long int bufsize)
{
	int rc = 1; 

	if (tracing_enabled) {
#ifdef HAVE_PABLOTRACE
		rc = setTraceBufferSize(bufsize);
#endif
	}
	return rc; 
}

int trace_set_file_name(const char *fname)
{
	int rc = 1;

	if (tracing_enabled) {
	    if (fname == NULL) {
		if (trace_fname) {
		    free(trace_fname); 
		    trace_fname = NULL;
		}
	    }
	    else {
		trace_fname = (char *)malloc(strlen(fname));
		strcpy(trace_fname, fname);
	    }
	}

	return rc; 
}


int trace_set_processor_number(const int pnum)
{
	int rc = 1;

	if (tracing_enabled) {
#ifdef HAVE_PABLOTRACE
		rc = setTraceProcessorNumber(pnum);
#endif
	}

	return rc; 
}


int trace_enable()
{
    int rc = 1; 
#ifdef HAVE_PABLO
    tracing_enabled = TRUE; 
#else
    fprintf(stderr, "Don't have Pablo, can't enable tracing\n");
#endif

    return rc; 
}

int trace_disable()
{
    int rc = 1; 
    /*fprintf(stderr, "Enabling tracing\n");*/

    /* end any tracing that is active */
    trace_fini(); 

    /* disable any future calls to the trace library */
    tracing_enabled = FALSE;

    return rc; 
}

int trace_reset(const int enable_flag, const char *fname, const int ftype)
{
    int rc=0;
    trace_fini();

    if (enable_flag == 1) {
	trace_enable(); 
	trace_init(fname, ftype);
    }

    if (enable_flag == 0) {
	fprintf(stderr, "disabling tracing\n");
	trace_disable(); 
    }

    return rc; 
}


int trace_init(const char *fname, const int ftype)
{
    int rc = 1;

    if (!tracing_active) {

	if (tracing_enabled) {
	    fprintf(stderr, "trace_init, fname=%s, ftype=%d\n",
		    fname, ftype);
	    trace_set_file_name(fname);
#ifdef HAVE_PTHREAD
	    /* create a hashtable for interval events */
	    rc = create_mt_hashtable(10, &interval_hash_func, 
		    interval_comp_func, &interval_hash);

	    /* create a hashtable for count events */
	    rc = create_mt_hashtable(10, &count_hash_func, 
		    count_comp_func, &count_hash);

	    /* create a hashtable for scole events */
	    rc = create_mt_hashtable(10, &scope_hash_func, 
		    scope_comp_func, &scope_hash);
#endif

#ifdef HAVE_PABLO
	    pablo_interface_init(fname, ftype);
#endif

	    tracing_active = 1; 
	}

    }

    return rc;
}


int trace_fini()
{
	int rc = 1;

	/*trace_init(trace_fname, trace_ftype);*/
	trace_set_file_name(NULL);

	if (tracing_active) {
	    /* deactivate tracing */
	    tracing_active = 0; 

	    if (tracing_enabled) {
		fprintf(stderr, "calling trace_fini()\n");
#ifdef HAVE_PTHREAD
		mt_hashtable_destroy(&interval_hash, free);
		mt_hashtable_destroy(&count_hash, free);
		mt_hashtable_destroy(&scope_hash, free);
#endif
#ifdef HAVE_PABLO
		fprintf(stderr, "calling pablo_interface_fini()\n");
		pablo_interface_fini();
#endif
#ifdef HAVE_PABLOTRACE
		endTracing();
#endif
	    }

	}

	return rc; 
}





/* generic tracing */

int trace_set_event_level(
		const int eventID, 
		const int traceLevel)
{
	int rc = 1; 
	trace_init(trace_fname, trace_ftype);

	if (tracing_enabled) {
#ifdef HAVE_PABLOTRACE
		//rc = setEventLevel(eventID, traceLevel); 
#endif
	}
	return rc; 
}


/**
 * @brief Generate a generic trace event. 
 *
 * @param eventID @input_type  The ID of the trace.
 * @param pid     @input_type  Process ID. 
 * @param data    @input_type  User-defined data passed in a character string. 
 *
 * @return non-zero if successfull
 * @return 0 if failure
 */
int trace_event(
		const int eventID, 
		const int pid, 
		const char *data)
{
    int rc = 1;

    trace_init(trace_fname, trace_ftype);

    /*fprintf(stderr, "output trace event\n");*/

    if (tracing_enabled) {
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; 

	pthread_mutex_lock(&mutex); 
#ifdef HAVE_PABLO
	pablo_output_generic_event(eventID, pid, data);
#else
	fprintf(stderr, "I don't have PABLO... now what\n");
#endif

#ifdef HAVE_PABLOTRACE
	setTraceProcessorNumber(pid);
	rc = traceEvent(eventID, (char *)data, strlen(data));
#endif
	pthread_mutex_unlock(&mutex); 
    }

    return rc; 
}




/**
 * @brief Generate an interval start event. 
 *
 * The \ref trace_start_interval "trace_start_interval()" function generates an 
 * interval record that has information about the start, end, and duration of a 
 * particular code fragment. 
 *
 * We use the generic pablo trace and encode the additional information 
 * we want in the data field.  The new data field will be, "interval:$name:duration".
 *
 * Pablo has its own interval records, but they are inadequate because it is 
 * difficult to measure concurrent intervals (e.g., in threads).  
 * A better (and more efficient) way to do this would be to create our own 
 * Pablo record type, but this is a quick "hack" to address our needs. 
 * 
 * @param interval_id @input_type  The ID of the interval (unique for each interval)
 * @param pid @input_type  The process ID/rank of the calling process.
 *
 * @return non-zero if successfull
 * @return 0 if failure
 */
int trace_start_interval(
		const int interval_id,
		const int pid)
{
    int rc = 1; 
    trace_init(trace_fname, trace_ftype);

    if (tracing_enabled) {
	{
#ifdef HAVE_PTHREAD
	    int *key = (int *)malloc(1*sizeof(int));
	    double *data = (double *)malloc(1*sizeof(double));


	    *key = interval_id; 
	    *data = lwfs_get_time(); 

	    /* Get the current scope for this pid ... is this a sub-interval */
	    {
		int *ppid = (int *)malloc(sizeof(int));
		int *scope = NULL;

		*ppid = pid; 

		/* This should be inside a critical section */
		pthread_mutex_lock(&scope_mutex); 

		scope = (int *)mt_hashtable_remove(&scope_hash, ppid);
		if (scope == NULL) {

		    /* allocate space for value */
		    scope = (int *)malloc(sizeof(int));

		    *scope = 0; 
		    *ppid = pid; 
		}
		else {
		    (*scope)++; 
		}

		/* insert back into the hashtable */
		rc = mt_hashtable_insert(&scope_hash, ppid, scope);

		/* release the mutex */
		pthread_mutex_unlock(&scope_mutex); 
	    }

	    /* register the interval in the hash table */
	    rc = mt_hashtable_insert(&interval_hash, key, data);
#endif
	}
    }

    return rc; 
}


/**
 * @brief Generate an end-interval event. 
 *
 * The \ref trace_interval_event "trace_interval_event()" function generates an 
 * interval record that has information about the start, end, and duration of a 
 * particular code fragment. 
 *
 * @param intervalID @input_type The interval ID (unique for each interval).
 * @param eventID @input_type  The ID of the trace. 
 * @param pid     @input_type  Process ID. 
 * @param level   @input_type  The depth of the interval (0=root, 1=sub-interval, 2=sub-sub,...).
 * @param data    @input_type  User-defined data passed in a character 
 *                             string. 
 *
 * @return non-zero if successfull
 * @return 0 if failure
 */
int trace_end_interval(
		const int intervalID, 
		const int eventID,
		const int pid,
		const char *data)
{
    int level; 
    int rc = 1; 
    trace_init(trace_fname, trace_ftype);

    if (tracing_enabled) {
	double endtime = lwfs_get_time(); 
	int id = (int)intervalID; 
	double duration=0; 
	double *starttime=NULL;

#ifdef HAVE_PTHREAD
	/* fetch and remove the starttime from the hashtable */
	starttime = (double *)mt_hashtable_remove(&interval_hash, &id);
	if (starttime == NULL) {
	    return 0;
	}

	duration = endtime - (*starttime); 

	/* Decrement the scope for this pid ... is this a sub-interval */
	{
	    int *ppid = (int *)malloc(sizeof(int));
	    int *scope = NULL;

	    *ppid = pid; 

	    /* This should be inside a critical section */
	    pthread_mutex_lock(&scope_mutex); 

	    scope = (int *)mt_hashtable_remove(&scope_hash, ppid);
	    if (scope == NULL) {

		fprintf(stderr, "Error: Scope not defined\n");
		return -1;
	    }

	    else {
		level = *scope; 
		(*scope)--; 
	    }

	    /* insert back into the hashtable */
	    rc = mt_hashtable_insert(&scope_hash, ppid, scope);

	    /* release the mutex */
	    pthread_mutex_unlock(&scope_mutex); 
	}
#endif

	/*
	   fprintf(stderr, "OUTPUT INTERVAL: end=%g, start=%g, duration=%g\n",
	   endtime, *starttime, duration);
	 */

	/* output the interval event */
	output_interval_event(eventID, pid, level, data, duration); 

	/* free the result stored in the hashtable */
	free(starttime); 
    }

    return rc; 
}

/**
 * @brief Generate an interval start event. 
 *
 * The \ref trace_start_interval "trace_start_interval()" function generates an 
 * interval record that has information about the start, end, and duration of a 
 * particular code fragment. 
 *
 * We use the generic pablo trace and encode the additional information 
 * we want in the data field.  The new data field will be, "interval:$name:duration".
 *
 * Pablo has its own interval records, but they are inadequate because it is 
 * difficult to measure concurrent intervals (e.g., in threads).  
 * A better (and more efficient) way to do this would be to create our own 
 * Pablo record type, but this is a quick "hack" to address our needs. 
 * 
 * @param interval_id @input_type  The ID of the interval (unique for each interval)
 *
 * @return non-zero if successfull
 * @return 0 if failure
 */
int trace_start_tput_interval(
		const int interval_id,
		const int pid)
{
	return trace_start_interval(interval_id, pid);
}


/**
 * @brief Generate an end-interval event. 
 *
 * The \ref trace_interval_event "trace_interval_event()" function generates an 
 * interval record that has information about the start, end, and duration of a 
 * particular code fragment. 
 *
 * @param intervalID @input_type The interval ID (unique for each interval).
 * @param eventID @input_type  The ID of the trace. 
 * @param pid     @input_type  Process ID. 
 * @param level   @input_type  The depth of the interval (0=root, 1=sub-interval, 2=sub-sub,...).
 * @param data    @input_type  User-defined data passed in a character 
 *                             string. 
 *
 * @return non-zero if successfull
 * @return 0 if failure
 */
int trace_end_tput_interval(
		const int intervalID, 
		const int eventID,
		const int pid,
		const long num_processed, 
		const char *data)
{
    int rc = 1; 
    int level; 
    trace_init(trace_fname, trace_ftype);

    if (tracing_enabled) {
	{
	    double endtime = lwfs_get_time(); 
	    int id = (int)intervalID; 
	    double duration=0; 
	    double *starttime=NULL;

#ifdef HAVE_PTHREAD
	    /* fetch and remove the starttime from the hashtable */
	    starttime = (double *)mt_hashtable_remove(&interval_hash, &id);
	    if (starttime == NULL) {
		return 0;
	    }

	    duration = endtime - (*starttime); 

	    /* free the result stored in the hashtable */
	    free(starttime); 

	    /* Decrement the scope for this pid ... is this a sub-interval */
	    {
		int *ppid = (int *)malloc(1*sizeof(int));
		int *scope = NULL;

		*ppid = pid; 

		/* This should be inside a critical section */
		pthread_mutex_lock(&scope_mutex); 

		scope = (int *)mt_hashtable_remove(&scope_hash, ppid);
		if (scope == NULL) {

		    fprintf(stderr, "Error: Scope not defined\n");
		    return -1;
		}

		else {
		    level = *scope; 
		    (*scope)--; 
		}

		/* insert back into the hashtable */
		rc = mt_hashtable_insert(&scope_hash, ppid, scope);

		/* release the mutex */
		pthread_mutex_unlock(&scope_mutex); 
	    }
#endif

	    /*
	       fprintf(stderr, "OUTPUT INTERVAL: end=%g, start=%g, duration=%g\n",
	       endtime, *starttime, duration);
	     */

	    /* output the interval event */
	    output_tput_interval_event(eventID, pid, level, data, duration, num_processed); 
	}
    }

    return rc; 
}

int trace_get_count(
	const int id,
	const int pid)
{
	int result = 0; 

	if (tracing_enabled) {
		{
#ifdef HAVE_PTHREAD
			int *count; 
			struct count_key key; 

			key.id = id;
			key.pid = pid;

			/* allocate a key */
			/*
			key = (int *)malloc(sizeof(int));

			*key = (int)id;
			*/

			/* lookup the key in the count hash table */
			count = mt_hashtable_remove(&count_hash, &key);
			if (count == NULL) {
				result = 0;
			}
			else {
				result = *count;
			}
#endif
		}
	}

	return result; 
}

/**
 * @brief Generate an increment-count event. 
 *
 * The \ref trace_inc_count_event "trace_inc_count_event()" function
 * increments a counter associated with the ID of counter and outputs
 * a count event to the tracefile. 
 *
 * @param id @input_type The ID of the counter. 
 * @param pid @input_type Process ID. 
 * @param data @input_type User-defined data in a character string.
 *
 * @returns non-zero if successful. 
 * @returns 0 if failure. 
 */
int trace_inc_count(
		const int id,
		const int pid, 
		const char *data)
{
    int rc = 1; 
    trace_init(trace_fname, trace_ftype);

    if (tracing_enabled) {
#ifdef HAVE_PTHREAD
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	int *count; 
	struct count_key *key;

	/* allocate a key */
	key = (struct count_key *)malloc(sizeof(struct count_key));

	key->id = id;
	key->pid = pid;

	pthread_mutex_lock(&mutex);

	/* lookup the key in the count hash table */
	count = mt_hashtable_remove(&count_hash, key);
	if (count == NULL) {

	    /* allocate the count variable */
	    count = (int *)malloc(sizeof(int));

	    /* assign a value of 0 */
	    *count = 0; 
	}

	/* increment count */
	(*count)++; 

	/* store the value back in the hash table */
	rc = mt_hashtable_insert(&count_hash, key, count);
	if (!rc) {
	    return rc; 
	}

	pthread_mutex_unlock(&mutex);

	/* output a count event */
	rc = output_count_event(id, pid, data, *count);
#else
	fprintf(stderr, "trace_inc_count: don't have PTHREADs... now what!\n");
#endif
    }

    return rc; 
}

/**
 * @brief Generate an decrement-count event. 
 *
 * The \ref trace_inc_count_event "trace_inc_count_event()" function
 * increments a counter associated with the ID of counter and outputs
 * a count event to the tracefile. 
 *
 * @param id @input_type The ID of the counter. 
 * @param pid @input_type Process ID.
 * @param data @input_type User-defined data in a character string.
 *
 * @returns non-zero if successful. 
 * @returns 0 if failure. 
 */
int trace_dec_count(
		const int id,
		const int pid,
		const char *data)
{
    int rc = 1; 
    trace_init(trace_fname, trace_ftype);

    if (tracing_enabled) {
#ifdef HAVE_PTHREAD
	static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	int *count; 
	struct count_key *key;

	/* allocate a key */
	key = (struct count_key *)malloc(sizeof(struct count_key));

	key->id = id;
	key->pid = pid;

	/* protect access to this section */
	pthread_mutex_lock(&mutex);

	/* lookup the key in the count hash table */
	count = mt_hashtable_remove(&count_hash, key);
	if (count == NULL) {

	    /* allocate the count variable */
	    count = (int *)malloc(sizeof(int));

	    /* assign a value of 0 */
	    *count = 0; 
	}

	/* decrement count */
	(*count)--; 

	/* store the value back in the hash table */
	rc = mt_hashtable_insert(&count_hash, key, count);
	if (!rc) {
	    return rc; 
	}

	pthread_mutex_unlock(&mutex);

	/* output a count event */
	rc = output_count_event(id, pid, data, *count);
#endif
    }

    return rc; 
}

/**
 * @brief Generate a reset-count event. 
 *
 * The \ref trace_reset_count "trace_reset_count()" function
 * sets the value of a counter associated with the ID to 0
 * and outputs a count event to the tracefile. 
 *
 * @param id @input_type The ID of the counter. 
 * @param pid @input_type Process ID.
 * @param data @input_type User-defined data in a character string.
 *
 * @returns non-zero if successful. 
 * @returns 0 if failure. 
 */
int trace_reset_count(
		const int eventID,
		const int pid,
		const char *data)
{
	return trace_set_count(eventID, pid, data, 0);
}

/**
 * @brief Generate an set-count event. 
 *
 * The \ref trace_reset_count "trace_reset_count()" function
 * sets the value of a counter associated with the ID
 * and outputs a count event to the tracefile. 
 *
 * @param id @input_type The ID of the counter. 
 * @param pid @input_type Process ID.
 * @param data @input_type User-defined data in a character string.
 * @param new_count @input_type New value for the counter.
 *
 * @returns non-zero if successful. 
 * @returns 0 if failure. 
 */
int trace_set_count(
		const int id,
		const int pid,
		const char *data, 
		const int new_count)
{
    int rc = 1;
    trace_init(trace_fname, trace_ftype);

    if (tracing_enabled) {
	{
#ifdef HAVE_PTHREAD
	    int *count; 
	    struct count_key *key;

	    /* allocate a key */
	    key = (struct count_key *)malloc(sizeof(struct count_key));

	    key->id = id;
	    key->pid = pid;

	    /* TODO: the following should be in a critical section */

	    /* lookup the key in the count hash table */
	    count = mt_hashtable_remove(&count_hash, key);
	    if (count == NULL) {

		/* allocate the count variable */
		count = (int *)malloc(sizeof(int));
	    }

	    /* set the count variable */
	    *count = new_count; 

	    /* store the value back in the hash table */
	    rc = mt_hashtable_insert(&count_hash, key, count);
	    if (!rc) {
		return rc; 
	    }

	    /* output a count event */
	    rc = output_count_event(id, pid, data, *count);
#endif
	}
    }

    return rc; 
}


int trace_put_all_counts(void)
{
    int rc = 1;
    trace_init(trace_fname, trace_ftype);

    if (tracing_enabled) {
	/* we need to be able to iterate through all entries in the hashtable */
#ifdef HAVE_PABLOTRACE
	rc = putAllCounts();
#endif
    }

    return rc; 
}
