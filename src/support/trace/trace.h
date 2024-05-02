/**  @file trace.h
 *   
 *   @brief A simple tracing API. 
 *   
 *   @author Ron Oldfield (raoldfi\@sandia.gov).
 *   $Revision: 406 $.
 *   $Date: 2005-10-07 15:08:29 -0600 (Fri, 07 Oct 2005) $.
 *
 */
 
#ifndef _TRACE_H_
#define _TRACE_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__STDC__) || defined(__cplusplus)

	/* initialization and wrapup functions */
	extern int trace_init(const char *trace_fname, const int trace_ftype);
	extern int trace_reset(const int enable_flag, const char *trace_fname, const int trace_ftype);
	extern int trace_set_buffer_size(const unsigned long int bufsize);
	extern int trace_set_buffer_size(const unsigned long int bufsize);
	extern int trace_set_file_name(const char *fname);
	//extern int trace_set_processor_number(const int pnum);
	extern int trace_fini(void);

	extern int trace_enable();
	extern int trace_disable();

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
	extern int trace_event(
			const int eventID, 
			const int pid,
			const char *data);

	/* interval functions */



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
	 * @param pid @input_type  The ID of process/thread
	 *
	 * @return non-zero if successfull
	 * @return 0 if failure
	 */
	extern int trace_start_interval(const int interval_id, const int pid);

	/**
 * @brief Generate an end-interval event.
 *
 * The \ref trace_end_interval "trace_end_interval()" function generates an
 * interval record that has information about the start, end, and duration of a
 * particular code fragment.
 *
 * @param interval_id @input_type The interval ID (unique for each interval).
 * @param event_id @input_type  The ID of the trace (could be the thread ID).
 * @param pid      @input_type  Process ID.
 * @param data     @input_type  User-defined data passed in a character
 *                             string.
 *
 * @return non-zero if successfull
 * @return 0 if failure
 */
	extern int trace_end_interval(
			const int interval_id,
			const int event_id,
			const int pid, 
			const char *data);


	/**
	 * @brief Generate a throughput interval start event.
	 *
	 * The \ref trace_start_tput_interval "trace_start_tput_interval()" function generates an
	 * throughput interval record that has information about the start, end, duration, and
	 * number of elements processed during the interval. It is useful for measuring 
	 * bandwidth, ops/sec, ...
	 *
	 * We use the generic pablo trace and encode the additional information
	 * we want in the data field.  The new data field will be, "interval:tput:$name:duration".
	 *
	 * Pablo has its own interval records, but they are inadequate because it is
	 * difficult to measure concurrent intervals (e.g., in threads).
	 * A better (and more efficient) way to do this would be to create our own
	 * Pablo record type, but this is a quick "hack" to address our needs.
	 *
	 * @param interval_id @input_type  The ID of the interval (unique for each interval)
	 * @param pid @input_type  The ID of the process/thread
	 *
	 * @return non-zero if successfull
	 * @return 0 if failure
	 */
	extern int trace_start_tput_interval(const int interval_id, const int pid);

	/**
 * @brief Generate an end-interval event.
 *
 * The \ref trace_end_interval "trace_end_interval()" function generates an
 * interval record that has information about the start, end, and duration of a
 * particular code fragment.
 *
 * @param interval_id @input_type The interval ID (unique for each interval).
 * @param event_id @input_type  The ID of the trace (could be the thread ID).
 * @param pid      @input_type  Process ID.
 * @param data     @input_type  User-defined data passed in a character
 *                             string.
 *
 * @return non-zero if successfull
 * @return 0 if failure
 */
	extern int trace_end_tput_interval(
			const int interval_id,
			const int event_id,
			const int pid, 
			const long num_processed, 
			const char *data);


	//extern int trace_reset_interval(const int eventID);

	/* count events */

	extern int trace_get_count(const int id, const int pid);

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
	extern int trace_inc_count(
			const int id, 
			const int pid, 
			const char *data);


	extern int trace_dec_count(
			const int id, 
			const int pid, 
			const char *data);

	extern int trace_set_count(
			const int id, 
			const int pid, 
			const char *data, 
			const int newval);

	extern int trace_reset_count(
			const int id, 
			const int pid, 
			const char *data);

	extern int trace_put_all_counts(void);


#endif


#ifdef __cplusplus
}
#endif

#endif
