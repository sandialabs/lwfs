#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "thread_pool_options.h"

#ifdef HAVE_ARGP_H
#include <argp.h>
#else
#include <errno.h>
#define ARGP_ERR_UNKNOWN E2BIG
#endif

/**
 * @brief Output the thread pool options to a specified file
 *
 * @param fp @input The file pointer.
 * @param opts @input The options to print.
 */
void print_thread_pool_opts(FILE *fp, struct thread_pool_options *opts, const char *prefix)
{
	fprintf(fp, "%s ------------ Thread Pool Options -----------\n", prefix);
	fprintf(fp, "%s \ttp-init-thread-count = %d\n", prefix, opts->initial_thread_count);
    fprintf(fp, "%s \ttp-min-thread-count  = %d\n", prefix, opts->min_thread_count);
    fprintf(fp, "%s \ttp-max-thread-count  = %d\n", prefix, opts->max_thread_count);
    fprintf(fp, "%s \ttp-low-watermark     = %d\n", prefix, opts->low_watermark);
    fprintf(fp, "%s \ttp-high-watermark    = %d\n", prefix, opts->high_watermark);
}

int parse_thread_pool_opt(
        int key,
        char *arg,
        struct thread_pool_options *tp_opts)
{
	switch (key) {
		case TP_INITIAL_COUNT_OPT:
			tp_opts->initial_thread_count = atoi(arg);
			break;

		case TP_MIN_COUNT_OPT:
			tp_opts->min_thread_count = atoi(arg);
			break;

		case TP_MAX_COUNT_OPT:
			tp_opts->max_thread_count = atoi(arg);
			break;

		case TP_LOW_WATERMARK_OPT:
			tp_opts->low_watermark = atoi(arg);
			break;

		case TP_HIGH_WATERMARK_OPT:
			tp_opts->high_watermark = atoi(arg);
			break;

		default: 
			return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

int load_default_thread_pool_opts(struct thread_pool_options *tp_opts)
{
    memset(tp_opts, 0, sizeof(struct thread_pool_options));
    tp_opts->initial_thread_count = 1; 
    tp_opts->min_thread_count = 1; 
    tp_opts->max_thread_count = 1;
    tp_opts->low_watermark = 1;
    tp_opts->high_watermark = 1;

    return 0;
}

