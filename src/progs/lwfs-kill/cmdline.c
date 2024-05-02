/*
  File autogenerated by gengetopt version 2.17
  generated with the following command:
  gengetopt -S --set-package=lwfs --set-version=0.1 

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

#include "cmdline.h"

const char *gengetopt_args_info_purpose = "";

const char *gengetopt_args_info_usage = "Usage: lwfs [OPTIONS]...";

const char *gengetopt_args_info_help[] = {
  "  -h, --help            Print help and exit",
  "  -V, --version         Print version and exit",
  "      --server-nid=INT  NID of the service to kill  (default=`0')",
  "      --server-pid=INT  PID of the service to kill  (default=`0')",
  "      --verbose=INT     Debug level of logger [0-5]  (default=`5')",
  "      --logfile=STRING  Path to logfile",
    0
};

static
void clear_given (struct gengetopt_args_info *args_info);
static
void clear_args (struct gengetopt_args_info *args_info);

static int
cmdline_parser_internal (int argc, char * const *argv, struct gengetopt_args_info *args_info, int override, int initialize, int check_required, const char *additional_error);

static int
cmdline_parser_required2 (struct gengetopt_args_info *args_info, const char *prog_name, const char *additional_error);
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


static char *
gengetopt_strdup (const char *s);

static
void clear_given (struct gengetopt_args_info *args_info)
{
  args_info->help_given = 0 ;
  args_info->version_given = 0 ;
  args_info->server_nid_given = 0 ;
  args_info->server_pid_given = 0 ;
  args_info->verbose_given = 0 ;
  args_info->logfile_given = 0 ;
}

static
void clear_args (struct gengetopt_args_info *args_info)
{
  args_info->server_nid_arg = 0;
  args_info->server_nid_orig = NULL;
  args_info->server_pid_arg = 0;
  args_info->server_pid_orig = NULL;
  args_info->verbose_arg = 5;
  args_info->verbose_orig = NULL;
  args_info->logfile_arg = NULL;
  args_info->logfile_orig = NULL;
  
}

static
void init_args_info(struct gengetopt_args_info *args_info)
{
  args_info->help_help = gengetopt_args_info_help[0] ;
  args_info->version_help = gengetopt_args_info_help[1] ;
  args_info->server_nid_help = gengetopt_args_info_help[2] ;
  args_info->server_pid_help = gengetopt_args_info_help[3] ;
  args_info->verbose_help = gengetopt_args_info_help[4] ;
  args_info->logfile_help = gengetopt_args_info_help[5] ;
  
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
  if (args_info->verbose_orig)
    {
      free (args_info->verbose_orig); /* free previous argument */
      args_info->verbose_orig = 0;
    }
  if (args_info->logfile_arg)
    {
      free (args_info->logfile_arg); /* free previous argument */
      args_info->logfile_arg = 0;
    }
  if (args_info->logfile_orig)
    {
      free (args_info->logfile_orig); /* free previous argument */
      args_info->logfile_orig = 0;
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
  if (args_info->verbose_given) {
    if (args_info->verbose_orig) {
      fprintf(outfile, "%s=\"%s\"\n", "verbose", args_info->verbose_orig);
    } else {
      fprintf(outfile, "%s\n", "verbose");
    }
  }
  if (args_info->logfile_given) {
    if (args_info->logfile_orig) {
      fprintf(outfile, "%s=\"%s\"\n", "logfile", args_info->logfile_orig);
    } else {
      fprintf(outfile, "%s\n", "logfile");
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
  int result = EXIT_SUCCESS;

  if (cmdline_parser_required2(args_info, prog_name, NULL) > 0)
    result = EXIT_FAILURE;

  if (result == EXIT_FAILURE)
    {
      cmdline_parser_free (args_info);
      exit (EXIT_FAILURE);
    }
  
  return result;
}

int
cmdline_parser_required2 (struct gengetopt_args_info *args_info, const char *prog_name, const char *additional_error)
{
  int error = 0;

  /* checks for required options */
  if (! args_info->server_pid_given)
    {
      fprintf (stderr, "%s: '--server-pid' option required%s\n", prog_name, (additional_error ? additional_error : ""));
      error = 1;
    }
  
  
  /* checks for dependences among options */

  return error;
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
      int option_index = 0;
      char *stop_char;

      static struct option long_options[] = {
        { "help",	0, NULL, 'h' },
        { "version",	0, NULL, 'V' },
        { "server-nid",	1, NULL, 0 },
        { "server-pid",	1, NULL, 0 },
        { "verbose",	1, NULL, 0 },
        { "logfile",	1, NULL, 0 },
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
          /* NID of the service to kill.  */
          if (strcmp (long_options[option_index].name, "server-nid") == 0)
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
            args_info->server_nid_arg = strtol (optarg, &stop_char, 0);
            if (!(stop_char && *stop_char == '\0')) {
              fprintf(stderr, "%s: invalid numeric value: %s\n", argv[0], optarg);
              goto failure;
            }
            if (args_info->server_nid_orig)
              free (args_info->server_nid_orig); /* free previous string */
            args_info->server_nid_orig = gengetopt_strdup (optarg);
          }
          /* PID of the service to kill.  */
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
          /* Debug level of logger [0-5].  */
          else if (strcmp (long_options[option_index].name, "verbose") == 0)
          {
            if (local_args_info.verbose_given)
              {
                fprintf (stderr, "%s: `--verbose' option given more than once%s\n", argv[0], (additional_error ? additional_error : ""));
                goto failure;
              }
            if (args_info->verbose_given && ! override)
              continue;
            local_args_info.verbose_given = 1;
            args_info->verbose_given = 1;
            args_info->verbose_arg = strtol (optarg, &stop_char, 0);
            if (!(stop_char && *stop_char == '\0')) {
              fprintf(stderr, "%s: invalid numeric value: %s\n", argv[0], optarg);
              goto failure;
            }
            if (args_info->verbose_orig)
              free (args_info->verbose_orig); /* free previous string */
            args_info->verbose_orig = gengetopt_strdup (optarg);
          }
          /* Path to logfile.  */
          else if (strcmp (long_options[option_index].name, "logfile") == 0)
          {
            if (local_args_info.logfile_given)
              {
                fprintf (stderr, "%s: `--logfile' option given more than once%s\n", argv[0], (additional_error ? additional_error : ""));
                goto failure;
              }
            if (args_info->logfile_given && ! override)
              continue;
            local_args_info.logfile_given = 1;
            args_info->logfile_given = 1;
            if (args_info->logfile_arg)
              free (args_info->logfile_arg); /* free previous string */
            args_info->logfile_arg = gengetopt_strdup (optarg);
            if (args_info->logfile_orig)
              free (args_info->logfile_orig); /* free previous string */
            args_info->logfile_orig = gengetopt_strdup (optarg);
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



  if (check_required)
    {
      error += cmdline_parser_required2 (args_info, argv[0], additional_error);
    }

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

