/*
  File autogenerated by gengetopt version 2.17
  generated with the following command:
  /home/raoldfi/software/i386/bin/gengetopt -F portals-xfer-opts -S --set-package=lwfs-tests --set-version=0.1 

  The developers of gengetopt consider the fixed text that goes in all
  gengetopt output files to be in the public domain:
  we make no copyright claims on it.
*/

/* If we use autoconf.  */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "getopt.h"

#include "portals-xfer-opts.h"

const char *gengetopt_args_info_purpose = "";

const char *gengetopt_args_info_usage = "Usage: lwfs-tests [OPTIONS]...";

const char *gengetopt_args_info_help[] = {
  "  -h, --help                    Print help and exit",
  "  -V, --version                 Print version and exit",
  "      --len=INT                 The size of an input buffer  (default=`1')",
  "      --debug                   Debug flag  (default=off)",
  "      --count=INT               The number of experiments  (default=`1')",
  "      --num-reqs=INT            The number of reqs/trial  (default=`1')",
  "      --result-file=STRING      Where to store results",
  "      --result-file-mode=STRING Write mode for the result  (possible \n                                  values=\"w\", \"a\" default=`a')",
  "      --server                  Flag that identifies the server process  \n                                  (default=off)",
  "      --server-nid=LONGLONG     The network ID of the server  (default=`0')",
  "      --server-pid=INT          The process ID of the server  (default=`128')",
    0
};

static
void clear_given (struct gengetopt_args_info *args_info);
static
void clear_args (struct gengetopt_args_info *args_info);

static int
cmdline_parser_internal (int argc, char * const *argv, struct gengetopt_args_info *args_info, int override, int initialize, int check_required, const char *additional_error);

struct line_list
{
  char * string_arg;
  struct line_list * next;
};

static struct line_list *cmd_line_list = 0;
static struct line_list *cmd_line_list_tmp = 0;

static void
free_cmd_list(void)
{
  /* free the list of a previous call */
  if (cmd_line_list)
    {
      while (cmd_line_list) {
        cmd_line_list_tmp = cmd_line_list;
        cmd_line_list = cmd_line_list->next;
        free (cmd_line_list_tmp->string_arg);
        free (cmd_line_list_tmp);
      }
    }
}


char *cmdline_parser_result_file_mode_values[] = {"w", "a", 0} ;	/* Possible values for result-file-mode.  */

static char *
gengetopt_strdup (const char *s);

static
void clear_given (struct gengetopt_args_info *args_info)
{
  args_info->help_given = 0 ;
  args_info->version_given = 0 ;
  args_info->len_given = 0 ;
  args_info->debug_given = 0 ;
  args_info->count_given = 0 ;
  args_info->num_reqs_given = 0 ;
  args_info->result_file_given = 0 ;
  args_info->result_file_mode_given = 0 ;
  args_info->server_given = 0 ;
  args_info->server_nid_given = 0 ;
  args_info->server_pid_given = 0 ;
}

static
void clear_args (struct gengetopt_args_info *args_info)
{
  args_info->len_arg = 1;
  args_info->len_orig = NULL;
  args_info->debug_flag = 0;
  args_info->count_arg = 1;
  args_info->count_orig = NULL;
  args_info->num_reqs_arg = 1;
  args_info->num_reqs_orig = NULL;
  args_info->result_file_arg = NULL;
  args_info->result_file_orig = NULL;
  args_info->result_file_mode_arg = gengetopt_strdup ("a");
  args_info->result_file_mode_orig = NULL;
  args_info->server_flag = 0;
  args_info->server_nid_arg = 0;
  args_info->server_nid_orig = NULL;
  args_info->server_pid_arg = 128;
  args_info->server_pid_orig = NULL;
  
}

static
void init_args_info(struct gengetopt_args_info *args_info)
{
  args_info->help_help = gengetopt_args_info_help[0] ;
  args_info->version_help = gengetopt_args_info_help[1] ;
  args_info->len_help = gengetopt_args_info_help[2] ;
  args_info->debug_help = gengetopt_args_info_help[3] ;
  args_info->count_help = gengetopt_args_info_help[4] ;
  args_info->num_reqs_help = gengetopt_args_info_help[5] ;
  args_info->result_file_help = gengetopt_args_info_help[6] ;
  args_info->result_file_mode_help = gengetopt_args_info_help[7] ;
  args_info->server_help = gengetopt_args_info_help[8] ;
  args_info->server_nid_help = gengetopt_args_info_help[9] ;
  args_info->server_pid_help = gengetopt_args_info_help[10] ;
  
}

void
cmdline_parser_print_version (void)
{
  printf ("%s %s\n", CMDLINE_PARSER_PACKAGE, CMDLINE_PARSER_VERSION);
}

void
cmdline_parser_print_help (void)
{
  int i = 0;
  cmdline_parser_print_version ();

  if (strlen(gengetopt_args_info_purpose) > 0)
    printf("\n%s\n", gengetopt_args_info_purpose);

  printf("\n%s\n\n", gengetopt_args_info_usage);
  while (gengetopt_args_info_help[i])
    printf("%s\n", gengetopt_args_info_help[i++]);
}

void
cmdline_parser_init (struct gengetopt_args_info *args_info)
{
  clear_given (args_info);
  clear_args (args_info);
  init_args_info (args_info);
}

static void
cmdline_parser_release (struct gengetopt_args_info *args_info)
{
  
  if (args_info->len_orig)
    {
      free (args_info->len_orig); /* free previous argument */
      args_info->len_orig = 0;
    }
  if (args_info->count_orig)
    {
      free (args_info->count_orig); /* free previous argument */
      args_info->count_orig = 0;
    }
  if (args_info->num_reqs_orig)
    {
      free (args_info->num_reqs_orig); /* free previous argument */
      args_info->num_reqs_orig = 0;
    }
  if (args_info->result_file_arg)
    {
      free (args_info->result_file_arg); /* free previous argument */
      args_info->result_file_arg = 0;
    }
  if (args_info->result_file_orig)
    {
      free (args_info->result_file_orig); /* free previous argument */
      args_info->result_file_orig = 0;
    }
  if (args_info->result_file_mode_arg)
    {
      free (args_info->result_file_mode_arg); /* free previous argument */
      args_info->result_file_mode_arg = 0;
    }
  if (args_info->result_file_mode_orig)
    {
      free (args_info->result_file_mode_orig); /* free previous argument */
      args_info->result_file_mode_orig = 0;
    }
  if (args_info->server_nid_orig)
    {
      free (args_info->server_nid_orig); /* free previous argument */
      args_info->server_nid_orig = 0;
    }
  if (args_info->server_pid_orig)
    {
      free (args_info->server_pid_orig); /* free previous argument */
      args_info->server_pid_orig = 0;
    }
  
  clear_given (args_info);
}

int
cmdline_parser_file_save(const char *filename, struct gengetopt_args_info *args_info)
{
  FILE *outfile;
  int i = 0;

  outfile = fopen(filename, "w");

  if (!outfile)
    {
      fprintf (stderr, "%s: cannot open file for writing: %s\n", CMDLINE_PARSER_PACKAGE, filename);
      return EXIT_FAILURE;
    }

  if (args_info->help_given) {
    fprintf(outfile, "%s\n", "help");
  }
  if (args_info->version_given) {
    fprintf(outfile, "%s\n", "version");
  }
  if (args_info->len_given) {
    if (args_info->len_orig) {
      fprintf(outfile, "%s=\"%s\"\n", "len", args_info->len_orig);
    } else {
      fprintf(outfile, "%s\n", "len");
    }
  }
  if (args_info->debug_given) {
    fprintf(outfile, "%s\n", "debug");
  }
  if (args_info->count_given) {
    if (args_info->count_orig) {
      fprintf(outfile, "%s=\"%s\"\n", "count", args_info->count_orig);
    } else {
      fprintf(outfile, "%s\n", "count");
    }
  }
  if (args_info->num_reqs_given) {
    if (args_info->num_reqs_orig) {
      fprintf(outfile, "%s=\"%s\"\n", "num-reqs", args_info->num_reqs_orig);
    } else {
      fprintf(outfile, "%s\n", "num-reqs");
    }
  }
  if (args_info->result_file_given) {
    if (args_info->result_file_orig) {
      fprintf(outfile, "%s=\"%s\"\n", "result-file", args_info->result_file_orig);
    } else {
      fprintf(outfile, "%s\n", "result-file");
    }
  }
  if (args_info->result_file_mode_given) {
    if (args_info->result_file_mode_orig) {
      fprintf(outfile, "%s=\"%s\"\n", "result-file-mode", args_info->result_file_mode_orig);
    } else {
      fprintf(outfile, "%s\n", "result-file-mode");
    }
  }
  if (args_info->server_given) {
    fprintf(outfile, "%s\n", "server");
  }
  if (args_info->server_nid_given) {
    if (args_info->server_nid_orig) {
      fprintf(outfile, "%s=\"%s\"\n", "server-nid", args_info->server_nid_orig);
    } else {
      fprintf(outfile, "%s\n", "server-nid");
    }
  }
  if (args_info->server_pid_given) {
    if (args_info->server_pid_orig) {
      fprintf(outfile, "%s=\"%s\"\n", "server-pid", args_info->server_pid_orig);
    } else {
      fprintf(outfile, "%s\n", "server-pid");
    }
  }
  
  fclose (outfile);

  i = EXIT_SUCCESS;
  return i;
}

void
cmdline_parser_free (struct gengetopt_args_info *args_info)
{
  cmdline_parser_release (args_info);
}

/*
 * Returns:
 * - the index of the matched value
 * - -1 if no argument has been specified
 * - -2 if more than one value has matched
 */
static int
check_possible_values(const char *val, char *values[])
{
  int i, found, last;
  size_t len;

  if (!val)   /* otherwise strlen() crashes below */
    return -1; /* -1 means no argument for the option */

  found = last = 0;

  for (i = 0, len = strlen(val); values[i]; ++i)
    {
      if (strncmp(val, values[i], len) == 0)
        {
          ++found;
          last = i;
          if (strlen(values[i]) == len)
            return i; /* exact macth no need to check more */
        }
    }

  if (found == 1) /* one match: OK */
    return last;

  return (found ? -2 : -1); /* return many values are matched */
}


/* gengetopt_strdup() */
/* strdup.c replacement of strdup, which is not standard */
char *
gengetopt_strdup (const char *s)
{
  char *result = NULL;
  if (!s)
    return result;

  result = (char*)malloc(strlen(s) + 1);
  if (result == (char*)0)
    return (char*)0;
  strcpy(result, s);
  return result;
}

int
cmdline_parser (int argc, char * const *argv, struct gengetopt_args_info *args_info)
{
  return cmdline_parser2 (argc, argv, args_info, 0, 1, 1);
}

int
cmdline_parser2 (int argc, char * const *argv, struct gengetopt_args_info *args_info, int override, int initialize, int check_required)
{
  int result;

  result = cmdline_parser_internal (argc, argv, args_info, override, initialize, check_required, NULL);

  if (result == EXIT_FAILURE)
    {
      cmdline_parser_free (args_info);
      exit (EXIT_FAILURE);
    }
  
  return result;
}

int
cmdline_parser_required (struct gengetopt_args_info *args_info, const char *prog_name)
{
  return EXIT_SUCCESS;
}

int
cmdline_parser_internal (int argc, char * const *argv, struct gengetopt_args_info *args_info, int override, int initialize, int check_required, const char *additional_error)
{
  int c;	/* Character of the parsed option.  */

  int error = 0;
  struct gengetopt_args_info local_args_info;

  if (initialize)
    cmdline_parser_init (args_info);

  cmdline_parser_init (&local_args_info);

  optarg = 0;
  optind = 0;
  opterr = 1;
  optopt = '?';

  while (1)
    {
      int found = 0;
      int option_index = 0;
      char *stop_char;

      static struct option long_options[] = {
        { "help",	0, NULL, 'h' },
        { "version",	0, NULL, 'V' },
        { "len",	1, NULL, 0 },
        { "debug",	0, NULL, 0 },
        { "count",	1, NULL, 0 },
        { "num-reqs",	1, NULL, 0 },
        { "result-file",	1, NULL, 0 },
        { "result-file-mode",	1, NULL, 0 },
        { "server",	0, NULL, 0 },
        { "server-nid",	1, NULL, 0 },
        { "server-pid",	1, NULL, 0 },
        { NULL,	0, NULL, 0 }
      };

      stop_char = 0;
      c = getopt_long (argc, argv, "hV", long_options, &option_index);

      if (c == -1) break;	/* Exit from `while (1)' loop.  */

      switch (c)
        {
        case 'h':	/* Print help and exit.  */
          cmdline_parser_print_help ();
          cmdline_parser_free (&local_args_info);
          exit (EXIT_SUCCESS);

        case 'V':	/* Print version and exit.  */
          cmdline_parser_print_version ();
          cmdline_parser_free (&local_args_info);
          exit (EXIT_SUCCESS);


        case 0:	/* Long option with no short option */
          /* The size of an input buffer.  */
          if (strcmp (long_options[option_index].name, "len") == 0)
          {
            if (local_args_info.len_given)
              {
                fprintf (stderr, "%s: `--len' option given more than once%s\n", argv[0], (additional_error ? additional_error : ""));
                goto failure;
              }
            if (args_info->len_given && ! override)
              continue;
            local_args_info.len_given = 1;
            args_info->len_given = 1;
            args_info->len_arg = strtol (optarg, &stop_char, 0);
            if (!(stop_char && *stop_char == '\0')) {
              fprintf(stderr, "%s: invalid numeric value: %s\n", argv[0], optarg);
              goto failure;
            }
            if (args_info->len_orig)
              free (args_info->len_orig); /* free previous string */
            args_info->len_orig = gengetopt_strdup (optarg);
          }
          /* Debug flag.  */
          else if (strcmp (long_options[option_index].name, "debug") == 0)
          {
            if (local_args_info.debug_given)
              {
                fprintf (stderr, "%s: `--debug' option given more than once%s\n", argv[0], (additional_error ? additional_error : ""));
                goto failure;
              }
            if (args_info->debug_given && ! override)
              continue;
            local_args_info.debug_given = 1;
            args_info->debug_given = 1;
            args_info->debug_flag = !(args_info->debug_flag);
          }
          /* The number of experiments.  */
          else if (strcmp (long_options[option_index].name, "count") == 0)
          {
            if (local_args_info.count_given)
              {
                fprintf (stderr, "%s: `--count' option given more than once%s\n", argv[0], (additional_error ? additional_error : ""));
                goto failure;
              }
            if (args_info->count_given && ! override)
              continue;
            local_args_info.count_given = 1;
            args_info->count_given = 1;
            args_info->count_arg = strtol (optarg, &stop_char, 0);
            if (!(stop_char && *stop_char == '\0')) {
              fprintf(stderr, "%s: invalid numeric value: %s\n", argv[0], optarg);
              goto failure;
            }
            if (args_info->count_orig)
              free (args_info->count_orig); /* free previous string */
            args_info->count_orig = gengetopt_strdup (optarg);
          }
          /* The number of reqs/trial.  */
          else if (strcmp (long_options[option_index].name, "num-reqs") == 0)
          {
            if (local_args_info.num_reqs_given)
              {
                fprintf (stderr, "%s: `--num-reqs' option given more than once%s\n", argv[0], (additional_error ? additional_error : ""));
                goto failure;
              }
            if (args_info->num_reqs_given && ! override)
              continue;
            local_args_info.num_reqs_given = 1;
            args_info->num_reqs_given = 1;
            args_info->num_reqs_arg = strtol (optarg, &stop_char, 0);
            if (!(stop_char && *stop_char == '\0')) {
              fprintf(stderr, "%s: invalid numeric value: %s\n", argv[0], optarg);
              goto failure;
            }
            if (args_info->num_reqs_orig)
              free (args_info->num_reqs_orig); /* free previous string */
            args_info->num_reqs_orig = gengetopt_strdup (optarg);
          }
          /* Where to store results.  */
          else if (strcmp (long_options[option_index].name, "result-file") == 0)
          {
            if (local_args_info.result_file_given)
              {
                fprintf (stderr, "%s: `--result-file' option given more than once%s\n", argv[0], (additional_error ? additional_error : ""));
                goto failure;
              }
            if (args_info->result_file_given && ! override)
              continue;
            local_args_info.result_file_given = 1;
            args_info->result_file_given = 1;
            if (args_info->result_file_arg)
              free (args_info->result_file_arg); /* free previous string */
            args_info->result_file_arg = gengetopt_strdup (optarg);
            if (args_info->result_file_orig)
              free (args_info->result_file_orig); /* free previous string */
            args_info->result_file_orig = gengetopt_strdup (optarg);
          }
          /* Write mode for the result.  */
          else if (strcmp (long_options[option_index].name, "result-file-mode") == 0)
          {
            if (local_args_info.result_file_mode_given)
              {
                fprintf (stderr, "%s: `--result-file-mode' option given more than once%s\n", argv[0], (additional_error ? additional_error : ""));
                goto failure;
              }
            if ((found = check_possible_values(optarg, cmdline_parser_result_file_mode_values)) < 0)
              {
                fprintf (stderr, "%s: %s argument, \"%s\", for option `--result-file-mode'%s\n", argv[0], (found == -2) ? "ambiguous" : "invalid", optarg, (additional_error ? additional_error : ""));
                goto failure;
              }
            if (args_info->result_file_mode_given && ! override)
              continue;
            local_args_info.result_file_mode_given = 1;
            args_info->result_file_mode_given = 1;
            if (args_info->result_file_mode_arg)
              free (args_info->result_file_mode_arg); /* free previous string */
            args_info->result_file_mode_arg = gengetopt_strdup (cmdline_parser_result_file_mode_values[found]);
            if (args_info->result_file_mode_orig)
              free (args_info->result_file_mode_orig); /* free previous string */
            args_info->result_file_mode_orig = gengetopt_strdup (optarg);
          }
          /* Flag that identifies the server process.  */
          else if (strcmp (long_options[option_index].name, "server") == 0)
          {
            if (local_args_info.server_given)
              {
                fprintf (stderr, "%s: `--server' option given more than once%s\n", argv[0], (additional_error ? additional_error : ""));
                goto failure;
              }
            if (args_info->server_given && ! override)
              continue;
            local_args_info.server_given = 1;
            args_info->server_given = 1;
            args_info->server_flag = !(args_info->server_flag);
          }
          /* The network ID of the server.  */
          else if (strcmp (long_options[option_index].name, "server-nid") == 0)
          {
            if (local_args_info.server_nid_given)
              {
                fprintf (stderr, "%s: `--server-nid' option given more than once%s\n", argv[0], (additional_error ? additional_error : ""));
                goto failure;
              }
            if (args_info->server_nid_given && ! override)
              continue;
            local_args_info.server_nid_given = 1;
            args_info->server_nid_given = 1;
            #ifdef HAVE_LONG_LONG
            args_info->server_nid_arg = (long long int) strtol (optarg, &stop_char, 0);
            #else
            args_info->server_nid_arg = (long) strtol (optarg, &stop_char, 0);
            #endif
            if (!(stop_char && *stop_char == '\0')) {
              fprintf(stderr, "%s: invalid numeric value: %s\n", argv[0], optarg);
              goto failure;
            }
            if (args_info->server_nid_orig)
              free (args_info->server_nid_orig); /* free previous string */
            args_info->server_nid_orig = gengetopt_strdup (optarg);
          }
          /* The process ID of the server.  */
          else if (strcmp (long_options[option_index].name, "server-pid") == 0)
          {
            if (local_args_info.server_pid_given)
              {
                fprintf (stderr, "%s: `--server-pid' option given more than once%s\n", argv[0], (additional_error ? additional_error : ""));
                goto failure;
              }
            if (args_info->server_pid_given && ! override)
              continue;
            local_args_info.server_pid_given = 1;
            args_info->server_pid_given = 1;
            args_info->server_pid_arg = strtol (optarg, &stop_char, 0);
            if (!(stop_char && *stop_char == '\0')) {
              fprintf(stderr, "%s: invalid numeric value: %s\n", argv[0], optarg);
              goto failure;
            }
            if (args_info->server_pid_orig)
              free (args_info->server_pid_orig); /* free previous string */
            args_info->server_pid_orig = gengetopt_strdup (optarg);
          }
          
          break;
        case '?':	/* Invalid option.  */
          /* `getopt_long' already printed an error message.  */
          goto failure;

        default:	/* bug: option not considered.  */
          fprintf (stderr, "%s: option unknown: %c%s\n", CMDLINE_PARSER_PACKAGE, c, (additional_error ? additional_error : ""));
          abort ();
        } /* switch */
    } /* while */




  cmdline_parser_release (&local_args_info);

  if ( error )
    return (EXIT_FAILURE);

  return 0;

failure:
  
  cmdline_parser_release (&local_args_info);
  return (EXIT_FAILURE);
}

static unsigned int
cmdline_parser_create_argv(const char *cmdline_, char ***argv_ptr, const char *prog_name)
{
  char *cmdline, *p;
  size_t n = 0, j;
  int i;

  if (prog_name) {
    cmd_line_list_tmp = (struct line_list *) malloc (sizeof (struct line_list));
    cmd_line_list_tmp->next = cmd_line_list;
    cmd_line_list = cmd_line_list_tmp;
    cmd_line_list->string_arg = gengetopt_strdup (prog_name);

    ++n;
  }

  cmdline = gengetopt_strdup(cmdline_);
  p = cmdline;

  while (p && strlen(p))
    {
      j = strcspn(p, " \t");
      ++n;
      if (j && j < strlen(p))
        {
          p[j] = '\0';

          cmd_line_list_tmp = (struct line_list *) malloc (sizeof (struct line_list));
          cmd_line_list_tmp->next = cmd_line_list;
          cmd_line_list = cmd_line_list_tmp;
          cmd_line_list->string_arg = gengetopt_strdup (p);

          p += (j+1);
          p += strspn(p, " \t");
        }
      else
        {
          cmd_line_list_tmp = (struct line_list *) malloc (sizeof (struct line_list));
          cmd_line_list_tmp->next = cmd_line_list;
          cmd_line_list = cmd_line_list_tmp;
          cmd_line_list->string_arg = gengetopt_strdup (p);

          break;
        }
    }

  *argv_ptr = (char **) malloc((n + 1) * sizeof(char *));
  cmd_line_list_tmp = cmd_line_list;
  for (i = (n-1); i >= 0; --i)
    {
      (*argv_ptr)[i] = cmd_line_list_tmp->string_arg;
      cmd_line_list_tmp = cmd_line_list_tmp->next;
    }

  (*argv_ptr)[n] = NULL;

  free(cmdline);
  return n;
}

int
cmdline_parser_string(const char *cmdline, struct gengetopt_args_info *args_info, const char *prog_name)
{
  return cmdline_parser_string2(cmdline, args_info, prog_name, 0, 1, 1);
}

int
cmdline_parser_string2(const char *cmdline, struct gengetopt_args_info *args_info, const char *prog_name,
    int override, int initialize, int check_required)
{
  char **argv_ptr = 0;
  int result;
  unsigned int argc;
  
  argc = cmdline_parser_create_argv(cmdline, &argv_ptr, prog_name);
  
  result =
    cmdline_parser_internal (argc, argv_ptr, args_info, override, initialize, check_required, 0);
  
  if (argv_ptr)
    {
      free (argv_ptr);
    }

  free_cmd_list();
  
  if (result == EXIT_FAILURE)
    {
      cmdline_parser_free (args_info);
      exit (EXIT_FAILURE);
    }
  
  return result;
}
