
#include <argp.h>

/* ------------- Global variables ------------------ */


/* ----------------- COMMAND-LINE OPTIONS --------------- */

const char *argp_program_version = "$Revision: 830 $"; 
const char *argp_program_bug_address = "<raoldfi@sandia.gov>"; 

static char doc[] = "Test the EBOFS file system API";
static char args_doc[] = ""; 



/**
 * @brief Command line arguments for testing the storage service. 
 */
struct arguments {

	/** @brief Name of the program */
	char *prog;

	/** @brief Debug level to use. */
	log_level debug_level; 

	/** @brief Path to the device file. */
	char *devfs; 

	/** @brief ID of the test */
	enum testid testid; 

	/** @brief the Object ID to use */
	uint64_t oid; 

	/** @brief the offset for writing and reading. */
	uint64_t offset; 

	/** @brief A character string to write to the object. */
	char *data; 

	/** @brief A character string to name of the attribute. */
	char *attr_name; 

	/** @brief A value for the attribute */
	int attr_val; 
}; 

static int print_args(FILE *fp, struct arguments *args) 
{
	int rc = 0; 

	fprintf(fp, "------------ %s ARGUMENTS -----------\n", args->prog);
	fprintf(fp, "\t--verbose=%d\n", args->debug_level);
	fprintf(fp, "\t--devfs=%s\n", args->devfs);
	fprintf(fp, "\t--testid=%d\n", args->testid);

	return rc;
}


static struct argp_option options[] = {
	{"verbose",   101, "<[0-5]>", 0, "Produce verbose output"},
	{"testid",    102, "<val>", 0, "ID of the test"},
	{"devfs",     103, "<FILENAME>", 0, "Path to the block-device file"},
	{"oid",       104, "<val>", 0, "Id of an object"},
	{"offset",    105, "<val>", 0, "Offset into an object"},
	{"data",      106, "<string>", 0, "Character data for an object"},
	{"attr-name", 107, "<string>", 0, "Name of an attribute to set/get/rm"},
	{"attr-val",  108, "<val>", 0, "Value for the attribute"},
	{ 0 }
};

/** 
 * @brief Parse a command-line argument. 
 * 
 * This function is called by the argp library. 
 */
static error_t parse_opt(
		int key, 
		char *arg, 
		struct argp_state *state)
{
	/* get the input arguments from argp_parse, which points to 
	 * our arguments structure */
	struct arguments *arguments = (struct arguments *)state->input; 

	switch (key) {

		case 101: /* verbose */
			arguments->debug_level= (log_level)atoi(arg);
			break;

		case 102: /* testid */
			arguments->testid= atoi(arg);
			break;

		case 103: /* devfs */
			arguments->devfs= arg;
			break;

		case 104: /* oid */
			arguments->oid= atol(arg);
			break;

		case 105: /* offset */
			arguments->offset= atol(arg);
			break;

		case 106: /* data */
			arguments->data= arg; 
			break;

		case 107: /* attr-name */
			arguments->attr_name= arg; 
			break;

		case 108: /* attr-val */
			arguments->attr_val= atoi(arg); 
			break;




		case ARGP_KEY_ARG:
			/* we don't expect any arguments */
			if (state->arg_num >= 0) {
				argp_usage(state);
			}
			// arguments->args[state->arg_num] = arg; 
			break; 

		case ARGP_KEY_END:
			if (state->arg_num < 0)
				argp_usage(state);
			break;

		default:
			return ARGP_ERR_UNKNOWN; 
	}

	return 0;
}

static struct argp argp = {options, parse_opt, args_doc, doc}; 

