#ifndef TP_OPTIONS_H_
# define TP_OPTIONS_H_

#include <pthread.h>           /* pthread functions and data structures      */
#include <stdio.h>

#define THREAD_POOL_OPTIONS \
	{"tp-init-thread-count",  TP_INITIAL_COUNT_OPT, "<val>", 0, \
		"Initial number of threads in the pool." }, \
	{"tp-min-thread-count",  TP_MIN_COUNT_OPT, "<val>", 0, \
		"Minimum number of threads in the pool." }, \
	{"tp-max-thread-count",  TP_MAX_COUNT_OPT, "<val>", 0, \
		"Maximum number of threads in the pool." }, \
	{"tp-low-watermark",  TP_LOW_WATERMARK_OPT, "<val>", 0, \
		"Request queue size at which threads are removed from the pool." }, \
	{"tp-high-watermark",  TP_HIGH_WATERMARK_OPT, "<val>", 0, \
		"Request queue size at which threads are added to the pool." }

#ifdef __cplusplus
extern "C" {
#endif

	struct thread_pool_options {
		int initial_thread_count;
		int min_thread_count;
		int max_thread_count;
		int low_watermark;
		int high_watermark;
	};

	typedef struct thread_pool_options lwfs_thread_pool_args;

	enum thread_pool_opt_ids {
		TP_INITIAL_COUNT_OPT = 1000,
		TP_MIN_COUNT_OPT = 1001,
		TP_MAX_COUNT_OPT = 1002,
		TP_LOW_WATERMARK_OPT = 1003,
		TP_HIGH_WATERMARK_OPT = 1004
	};

#if defined(__STDC__) || defined(__cplusplus)

	extern void print_thread_pool_opts(FILE *fp, struct thread_pool_options *opts, const char *prefix);

	extern int parse_thread_pool_opt(
        int key,
        char *arg,
        struct thread_pool_options *tp_opts);

    extern int load_default_thread_pool_opts(
            struct thread_pool_options *tp_opts);
#else 

#endif


#ifdef __cplusplus
}
#endif

#endif /* THREAD_POOL_H */
