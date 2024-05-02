/*
  File autogenerated by gengetopt version 2.17
  generated with the following command:
  /home/raoldfi/software/i386/bin/gengetopt -F ../../../../testing/portals-tests/req_opts -S --set-package=lwfs-tests --set-version=0.1 

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

#include "../../../../testing/portals-tests/req_opts.h"

const char *gengetopt_args_info_purpose = "";

const char *gengetopt_args_info_usage = "Usage: lwfs-tests [OPTIONS]...";

const char *gengetopt_args_info_help[] = {
  "  -h, --help                 Print help and exit",
  "  -V, --version              Print version and exit",
  "      --total-msgs=INT       The number of messages to send/receive  \n                               (default=`20')",
  "      --msgs-per-md=INT      The number of messages per MD on the server  \n                               (default=`5')",
  "      --portal-index=INT     Portals index to use  (default=`3')",
  "      --match-bits=INT       Match bits to use  (default=`3')",
  "      --server-pid=LONGLONG  The process ID of the server  (default=`99')",
  "      --server-nid=LONGLONG  The node ID of the server  (default=`-1')",
  "      --msg-data=STRING      The data to send in each mesg  (default=`Hello')",
  "      --with-bug             Run the buggy version  (default=off)",
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


static char *
gengetopt_strdup (const char *s);

static
void clear_given (struct gengetopt_args_info *args_info)
{
  args_info->help_given = 0 ;
  args_info->version_given = 0 ;
  args_info->total_msgs_given = 0 ;
  args_info->msgs_per_md_given = 0 ;
  args_info->portal_index_given = 0 ;
  args_info->match_bits_given = 0 ;
  args_info->server_pid_given = 0 ;
  args_info->server_nid_given = 0 ;
  args_info->msg_data_given = 0 ;
  args_info->with_bug_given = 0 ;
}

static
void clear_args (struct gengetopt_args_info *args_info)
{
  args_info->total_msgs_arg = 20;
  args_info->total_msgs_orig = NULL;
  args_info->msgs_per_md_arg = 5;
  args_info->msgs_per_md_orig = NULL;
  args_info->portal_index_arg = 3;
  args_info->portal_index_orig = NULL;
  args_info->match_bits_arg = 3;
  args_info->match_bits_orig = NULL;
  args_info->server_pid_arg = 99;
  args_info->server_pid_orig = NULL;
  args_info->server_nid_arg = -1;
  args_info->server_nid_orig = NULL;
  args_info->msg_data_arg = gengetopt_strdup ("Hello");
  args_info->msg_data_orig = NULL;
  args_info->with_bug_flag = 0;
  
}

static
void init_args_info(struct gengetopt_args_info *args_info)
{
  args_info->help_help = gengetopt_args_info_help[0] ;
  args_info->version_help = gengetopt_args_info_help[1] ;
  args_info->total_msgs_help = gengetopt_args_info_help[2] ;
  args_info->msgs_per_md_help = gengetopt_args_info_help[3] ;
  args_info->portal_index_help = gengetopt_args_info_help[4] ;
  args_info->match_bits_help = gengetopt_args_info_help[5] ;
  args_info->server_pid_help = gengetopt_args_info_help[6] ;
  args_info->server_nid_help = gengetopt_args_info_help[7] ;
  args_info->msg_data_help = gengetopt_args_info_help[8] ;
  args_info->with_bug_help = gengetopt_args_info_help[9] ;
  
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
  
  if (args_info->total_msgs_orig)
    {
      free (args_info->total_msgs_orig); /* free previous argument */
      args_info->total_msgs_orig = 0;
    }
  if (args_info->msgs_per_md_orig)
    {
      free (args_info->msgs_per_md_orig); /* free previous argument */
      args_info->msgs_per_md_orig = 0;
    }
  if (args_info->portal_index_orig)
    {
      free (args_info->portal_index_orig); /* free previous argument */
      args_info->portal_index_orig = 0;
    }
  if (args_info->match_bits_orig)
    {
      free (args_info->match_bits_orig); /* free previous argument */
      args_info->match_bits_orig = 0;
    }
  if (args_info->server_pid_orig)
    {
      free (args_info->server_pid_orig); /* free previous argument */
      args_info->server_pid_orig = 0;
    }
  if (args_info->server_nid_orig)
    {
      free (args_info->server_nid_orig); /* free previous argument */
      args_info->server_nid_orig = 0;
    }
  if (args_info->msg_data_arg)
    {
      free (args_info->msg_data_arg); /* free previous argument */
      args_info->msg_data_arg = 0;
    }
  if (args_info->msg_data_orig)
    {
      free (args_info->msg_data_orig); /* free previous argument */
      args_info->msg_data_orig = 0;
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
  if (args_info->total_msgs_given) {
    if (args_info->total_msgs_orig) {
      fprintf(outfile, "%s=\"%s\"\n", "total-msgs", args_info->total_msgs_orig);
    } else {
      fprintf(outfile, "%s\n", "total-msgs");
    }
  }
  if (args_info->msgs_per_md_given) {
    if (args_info->msgs_per_md_orig) {
      fprintf(outfile, "%s=\"%s\"\n", "msgs-per-md", args_info->msgs_per_md_orig);
    } else {
      fprintf(outfile, "%s\n", "msgs-per-md");
    }
  }
  if (args_info->portal_index_given) {
    if (args_info->portal_index_orig) {
      fprintf(outfile, "%s=\"%s\"\n", "portal-index", args_info->portal_index_orig);
    } else {
      fprintf(outfile, "%s\n", "portal-index");
    }
  }
  if (args_info->match_bits_given) {
    if (args_info->match_bits_orig) {
      fprintf(outfile, "%s=\"%s\"\n", "match-bits", args_info->match_bits_orig);
    } else {
      fprintf(outfile, "%s\n", "match-bits");
    }
  }
  if (args_info->server_pid_given) {
    if (args_info->server_pid_orig) {
      fprintf(outfile, "%s=\"%s\"\n", "server-pid", args_info->server_pid_orig);
    } else {
      fprintf(outfile, "%s\n", "server-pid");
    }
  }
  if (args_info->server_nid_given) {
    if (args_info->server_nid_orig) {
      fprintf(outfile, "%s=\"%s\"\n", "server-nid", args_info->server_nid_orig);
    } else {
      fprintf(outfile, "%s\n", "server-nid");
    }
  }
  if (args_info->msg_data_given) {
    if (args_info->msg_data_orig) {
      fprintf(outfile, "%s=\"%s\"\n", "msg-data", args_info->msg_data_orig);
    } else {
      fprintf(outfile, "%s\n", "msg-data");
    }
  }
  if (args_info->with_bug_given) {
    fprintf(outfile, "%s\n", "with-bug");
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
      int option_index = 0;
      char *stop_char;

      static struct option long_options[] = {
        { "help",	0, NULL, 'h' },
        { "version",	0, NULL, 'V' },
        { "total-msgs",	1, NULL, 0 },
        { "msgs-per-md",	1, NULL, 0 },
        { "portal-index",	1, NULL, 0 },
        { "match-bits",	1, NULL, 0 },
        { "server-pid",	1, NULL, 0 },
        { "server-nid",	1, NULL, 0 },
        { "msg-data",	1, NULL, 0 },
        { "with-bug",	0, NULL, 0 },
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
          /* The number of messages to send/receive.  */
          if (strcmp (long_options[option_index].name, "total-msgs") == 0)
          {
            if (local_args_info.total_msgs_given)
              {
                fprintf (stderr, "%s: `--total-msgs' option given more than once%s\n", argv[0], (additional_error ? additional_error : ""));
                goto failure;
              }
            if (args_info->total_msgs_given && ! override)
              continue;
            local_args_info.total_msgs_given = 1;
            args_info->total_msgs_given = 1;
            args_info->total_msgs_arg = strtol (optarg, &stop_char, 0);
            if (!(stop_char && *stop_char == '\0')) {
              fprintf(stderr, "%s: invalid numeric value: %s\n", argv[0], optarg);
              goto failure;
            }
            if (args_info->total_msgs_orig)
              free (args_info->total_msgs_orig); /* free previous string */
            args_info->total_msgs_orig = gengetopt_strdup (optarg);
          }
          /* The number of messages per MD on the server.  */
          else if (strcmp (long_options[option_index].name, "msgs-per-md") == 0)
          {
            if (local_args_info.msgs_per_md_given)
              {
                fprintf (stderr, "%s: `--msgs-per-md' option given more than once%s\n", argv[0], (additional_error ? additional_error : ""));
                goto failure;
              }
            if (args_info->msgs_per_md_given && ! override)
              continue;
            local_args_info.msgs_per_md_given = 1;
            args_info->msgs_per_md_given = 1;
            args_info->msgs_per_md_arg = strtol (optarg, &stop_char, 0);
            if (!(stop_char && *stop_char == '\0')) {
              fprintf(stderr, "%s: invalid numeric value: %s\n", argv[0], optarg);
              goto failure;
            }
            if (args_info->msgs_per_md_orig)
              free (args_info->msgs_per_md_orig); /* free previous string */
            args_info->msgs_per_md_orig = gengetopt_strdup (optarg);
          }
          /* Portals index to use.  */
          else if (strcmp (long_options[option_index].name, "portal-index") == 0)
          {
            if (local_args_info.portal_index_given)
              {
                fprintf (stderr, "%s: `--portal-index' option given more than once%s\n", argv[0], (additional_error ? additional_error : ""));
                goto failure;
              }
            if (args_info->portal_index_given && ! override)
              continue;
            local_args_info.portal_index_given = 1;
            args_info->portal_index_given = 1;
            args_info->portal_index_arg = strtol (optarg, &stop_char, 0);
            if (!(stop_char && *stop_char == '\0')) {
              fprintf(stderr, "%s: invalid numeric value: %s\n", argv[0], optarg);
              goto failure;
            }
            if (args_info->portal_index_orig)
              free (args_info->portal_index_orig); /* free previous string */
            args_info->portal_index_orig = gengetopt_strdup (optarg);
          }
          /* Match bits to use.  */
          else if (strcmp (long_options[option_index].name, "match-bits") == 0)
          {
            if (local_args_info.match_bits_given)
              {
                fprintf (stderr, "%s: `--match-bits' option given more than once%s\n", argv[0], (additional_error ? additional_error : ""));
                goto failure;
              }
            if (args_info->match_bits_given && ! override)
              continue;
            local_args_info.match_bits_given = 1;
            args_info->match_bits_given = 1;
            args_info->match_bits_arg = strtol (optarg, &stop_char, 0);
            if (!(stop_char && *stop_char == '\0')) {
              fprintf(stderr, "%s: invalid numeric value: %s\n", argv[0], optarg);
              goto failure;
            }
            if (args_info->match_bits_orig)
              free (args_info->match_bits_orig); /* free previous string */
            args_info->match_bits_orig = gengetopt_strdup (optarg);
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
            #ifdef HAVE_LONG_LONG
            args_info->server_pid_arg = (long long int) strtol (optarg, &stop_char, 0);
            #else
            args_info->server_pid_arg = (long) strtol (optarg, &stop_char, 0);
            #endif
            if (!(stop_char && *stop_char == '\0')) {
              fprintf(stderr, "%s: invalid numeric value: %s\n", argv[0], optarg);
              goto failure;
            }
            if (args_info->server_pid_orig)
              free (args_info->server_pid_orig); /* free previous string */
            args_info->server_pid_orig = gengetopt_strdup (optarg);
          }
          /* The node ID of the server.  */
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
          /* The data to send in each mesg.  */
          else if (strcmp (long_options[option_index].name, "msg-data") == 0)
          {
            if (local_args_info.msg_data_given)
              {
                fprintf (stderr, "%s: `--msg-data' option given more than once%s\n", argv[0], (additional_error ? additional_error : ""));
                goto failure;
              }
            if (args_info->msg_data_given && ! override)
              continue;
            local_args_info.msg_data_given = 1;
            args_info->msg_data_given = 1;
            if (args_info->msg_data_arg)
              free (args_info->msg_data_arg); /* free previous string */
            args_info->msg_data_arg = gengetopt_strdup (optarg);
            if (args_info->msg_data_orig)
              free (args_info->msg_data_orig); /* free previous string */
            args_info->msg_data_orig = gengetopt_strdup (optarg);
          }
          /* Run the buggy version.  */
          else if (strcmp (long_options[option_index].name, "with-bug") == 0)
          {
            if (local_args_info.with_bug_given)
              {
                fprintf (stderr, "%s: `--with-bug' option given more than once%s\n", argv[0], (additional_error ? additional_error : ""));
                goto failure;
              }
            if (args_info->with_bug_given && ! override)
              continue;
            local_args_info.with_bug_given = 1;
            args_info->with_bug_given = 1;
            args_info->with_bug_flag = !(args_info->with_bug_flag);
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

