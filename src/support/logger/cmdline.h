/* cmdline.h */

/* File autogenerated by gengetopt version 2.17  */

#ifndef CMDLINE_H
#define CMDLINE_H

/* If we use autoconf.  */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef CMDLINE_PARSER_PACKAGE
#define CMDLINE_PARSER_PACKAGE PACKAGE
#endif

#ifndef CMDLINE_PARSER_VERSION
#define CMDLINE_PARSER_VERSION VERSION
#endif

struct gengetopt_args_info
{
  const char *help_help; /* Print help and exit help description.  */
  const char *version_help; /* Print version and exit help description.  */
  int verbose_arg;	/* Debug level of logger [0-5] (default='5').  */
  char * verbose_orig;	/* Debug level of logger [0-5] original value given at command line.  */
  const char *verbose_help; /* Debug level of logger [0-5] help description.  */
  char * logfile_arg;	/* Path to logfile.  */
  char * logfile_orig;	/* Path to logfile original value given at command line.  */
  const char *logfile_help; /* Path to logfile help description.  */
  
  int help_given ;	/* Whether help was given.  */
  int version_given ;	/* Whether version was given.  */
  int verbose_given ;	/* Whether verbose was given.  */
  int logfile_given ;	/* Whether logfile was given.  */

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

int cmdline_parser_configfile (char * const filename,
  struct gengetopt_args_info *args_info,
  int override, int initialize, int check_required);

int cmdline_parser_string (const char *cmdline, struct gengetopt_args_info *args_info,
  const char *prog_name);
int cmdline_parser_string2 (const char *cmdline, struct gengetopt_args_info *args_info,
  const char *prog_name,
  int override, int initialize, int check_required);

int cmdline_parser_required (struct gengetopt_args_info *args_info,
  const char *prog_name);


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CMDLINE_H */
