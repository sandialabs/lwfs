/* cmdline_default.h */

/* File autogenerated by gengetopt version 2.17  */

#ifndef CMDLINE_DEFAULT_H
#define CMDLINE_DEFAULT_H

/* If we use autoconf.  */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef CMDLINE_PARSER_PACKAGE
#define CMDLINE_PARSER_PACKAGE "authr-server"
#endif

#ifndef CMDLINE_PARSER_VERSION
#define CMDLINE_PARSER_VERSION "0.1"
#endif

struct gengetopt_args_info
{
  const char *help_help; /* Print help and exit help description.  */
  const char *version_help; /* Print version and exit help description.  */
  int num_reqs_arg;	/* Number of requests before exit (default='-1').  */
  char * num_reqs_orig;	/* Number of requests before exit original value given at command line.  */
  const char *num_reqs_help; /* Number of requests before exit help description.  */
  int use_threads_flag;	/* Flag to use threads for the server (default=off).  */
  const char *use_threads_help; /* Flag to use threads for the server help description.  */
  int daemon_flag;	/* Flag to run server as a daemon (default=off).  */
  const char *daemon_help; /* Flag to run server as a daemon help description.  */
  char * lwfs_config_file_arg;	/* Path to the lwfs config file.  */
  char * lwfs_config_file_orig;	/* Path to the lwfs config file original value given at command line.  */
  const char *lwfs_config_file_help; /* Path to the lwfs config file help description.  */
  int ss_pid_arg;	/* The process ID to use for the server (default='122').  */
  char * ss_pid_orig;	/* The process ID to use for the server original value given at command line.  */
  const char *ss_pid_help; /* The process ID to use for the server help description.  */
  int ss_numbufs_arg;	/* The number of I/O buffers to use on the server (default='10').  */
  char * ss_numbufs_orig;	/* The number of I/O buffers to use on the server original value given at command line.  */
  const char *ss_numbufs_help; /* The number of I/O buffers to use on the server help description.  */
  long ss_bufsize_arg;	/* The size (in bytes) of an I/O buffer (default='1048576').  */
  char * ss_bufsize_orig;	/* The size (in bytes) of an I/O buffer original value given at command line.  */
  const char *ss_bufsize_help; /* The size (in bytes) of an I/O buffer help description.  */
  char * ss_root_arg;	/* Path to root (default='ss-root').  */
  char * ss_root_orig;	/* Path to root original value given at command line.  */
  const char *ss_root_help; /* Path to root help description.  */
  char * ss_iolib_arg;	/* Which I/O library to use (default='sysio').  */
  char * ss_iolib_orig;	/* Which I/O library to use original value given at command line.  */
  const char *ss_iolib_help; /* Which I/O library to use help description.  */
  int ss_xfer_only_flag;	/* A flag to only transfer data (no disk I/O) (default=off).  */
  const char *ss_xfer_only_help; /* A flag to only transfer data (no disk I/O) help description.  */
  char * ss_db_path_arg;	/* Path to the object attribute database (default='ss-attr.db').  */
  char * ss_db_path_orig;	/* Path to the object attribute database original value given at command line.  */
  const char *ss_db_path_help; /* Path to the object attribute database help description.  */
  int ss_db_clear_flag;	/* Clear the object attribute database before use (default=off).  */
  const char *ss_db_clear_help; /* Clear the object attribute database before use help description.  */
  int ss_db_recover_flag;	/* Recover the object attribute database after a crash (default=off).  */
  const char *ss_db_recover_help; /* Recover the object attribute database after a crash help description.  */
  int ss_trace_flag;	/* Enable tracing (default=off).  */
  const char *ss_trace_help; /* Enable tracing help description.  */
  char * ss_tracefile_arg;	/* Path to the trace file (default='ss-trace.sddf').  */
  char * ss_tracefile_orig;	/* Path to the trace file original value given at command line.  */
  const char *ss_tracefile_help; /* Path to the trace file help description.  */
  int ss_traceftype_arg;	/* Type of tracefile (0=binary,1=ASCII) (default='0').  */
  char * ss_traceftype_orig;	/* Type of tracefile (0=binary,1=ASCII) original value given at command line.  */
  const char *ss_traceftype_help; /* Type of tracefile (0=binary,1=ASCII) help description.  */
  int verbose_arg;	/* Debug level of logger [0-5] (default='5').  */
  char * verbose_orig;	/* Debug level of logger [0-5] original value given at command line.  */
  const char *verbose_help; /* Debug level of logger [0-5] help description.  */
  char * logfile_arg;	/* Path to logfile.  */
  char * logfile_orig;	/* Path to logfile original value given at command line.  */
  const char *logfile_help; /* Path to logfile help description.  */
  int tp_init_thread_count_arg;	/* Initial number of thread in the pool (default='1').  */
  char * tp_init_thread_count_orig;	/* Initial number of thread in the pool original value given at command line.  */
  const char *tp_init_thread_count_help; /* Initial number of thread in the pool help description.  */
  int tp_min_thread_count_arg;	/* Minimum number of thread in the pool (default='1').  */
  char * tp_min_thread_count_orig;	/* Minimum number of thread in the pool original value given at command line.  */
  const char *tp_min_thread_count_help; /* Minimum number of thread in the pool help description.  */
  int tp_max_thread_count_arg;	/* Maximum number of thread in the pool (default='1').  */
  char * tp_max_thread_count_orig;	/* Maximum number of thread in the pool original value given at command line.  */
  const char *tp_max_thread_count_help; /* Maximum number of thread in the pool help description.  */
  int tp_low_watermark_arg;	/* Request queue size at which threads are removed from the pool (default='1').  */
  char * tp_low_watermark_orig;	/* Request queue size at which threads are removed from the pool original value given at command line.  */
  const char *tp_low_watermark_help; /* Request queue size at which threads are removed from the pool help description.  */
  int tp_high_watermark_arg;	/* Request queue size at which threads are added to the pool (default='1').  */
  char * tp_high_watermark_orig;	/* Request queue size at which threads are added to the pool original value given at command line.  */
  const char *tp_high_watermark_help; /* Request queue size at which threads are added to the pool help description.  */
  long authr_pid_arg;	/* PID of the authr server (default='124').  */
  char * authr_pid_orig;	/* PID of the authr server original value given at command line.  */
  const char *authr_pid_help; /* PID of the authr server help description.  */
  long authr_nid_arg;	/* NID of the authr server (default='0').  */
  char * authr_nid_orig;	/* NID of the authr server original value given at command line.  */
  const char *authr_nid_help; /* NID of the authr server help description.  */
  int authr_cache_caps_flag;	/* Cache caps on the client (default=off).  */
  const char *authr_cache_caps_help; /* Cache caps on the client help description.  */
  
  int help_given ;	/* Whether help was given.  */
  int version_given ;	/* Whether version was given.  */
  int num_reqs_given ;	/* Whether num-reqs was given.  */
  int use_threads_given ;	/* Whether use-threads was given.  */
  int daemon_given ;	/* Whether daemon was given.  */
  int lwfs_config_file_given ;	/* Whether lwfs-config-file was given.  */
  int ss_pid_given ;	/* Whether ss-pid was given.  */
  int ss_numbufs_given ;	/* Whether ss-numbufs was given.  */
  int ss_bufsize_given ;	/* Whether ss-bufsize was given.  */
  int ss_root_given ;	/* Whether ss-root was given.  */
  int ss_iolib_given ;	/* Whether ss-iolib was given.  */
  int ss_xfer_only_given ;	/* Whether ss-xfer-only was given.  */
  int ss_db_path_given ;	/* Whether ss-db-path was given.  */
  int ss_db_clear_given ;	/* Whether ss-db-clear was given.  */
  int ss_db_recover_given ;	/* Whether ss-db-recover was given.  */
  int ss_trace_given ;	/* Whether ss-trace was given.  */
  int ss_tracefile_given ;	/* Whether ss-tracefile was given.  */
  int ss_traceftype_given ;	/* Whether ss-traceftype was given.  */
  int verbose_given ;	/* Whether verbose was given.  */
  int logfile_given ;	/* Whether logfile was given.  */
  int tp_init_thread_count_given ;	/* Whether tp-init-thread-count was given.  */
  int tp_min_thread_count_given ;	/* Whether tp-min-thread-count was given.  */
  int tp_max_thread_count_given ;	/* Whether tp-max-thread-count was given.  */
  int tp_low_watermark_given ;	/* Whether tp-low-watermark was given.  */
  int tp_high_watermark_given ;	/* Whether tp-high-watermark was given.  */
  int authr_pid_given ;	/* Whether authr-pid was given.  */
  int authr_nid_given ;	/* Whether authr-nid was given.  */
  int authr_cache_caps_given ;	/* Whether authr-cache-caps was given.  */

} ;

extern const char *gengetopt_args_info_purpose;
extern const char *gengetopt_args_info_usage;
extern const char *gengetopt_args_info_help[];

int cmdline_parser (int argc, char * const *argv,
  struct gengetopt_args_info *args_info);
int cmdline_parser2 (int argc, char * const *argv,
  struct gengetopt_args_info *args_info,
  int override, int initialize, int check_required);
int cmdline_parser_file_save(const char *filename,
  struct gengetopt_args_info *args_info);

void cmdline_parser_print_help(void);
void cmdline_parser_print_version(void);

void cmdline_parser_init (struct gengetopt_args_info *args_info);
void cmdline_parser_free (struct gengetopt_args_info *args_info);

int cmdline_parser_string (const char *cmdline, struct gengetopt_args_info *args_info,
  const char *prog_name);
int cmdline_parser_string2 (const char *cmdline, struct gengetopt_args_info *args_info,
  const char *prog_name,
  int override, int initialize, int check_required);

int cmdline_parser_required (struct gengetopt_args_info *args_info,
  const char *prog_name);

extern char *cmdline_parser_ss_iolib_values[] ;	/* Possible values for ss-iolib.  */


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CMDLINE_DEFAULT_H */
