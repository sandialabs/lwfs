/**
    @file mds_stress_dir.c
    @brief Stress creation and removal of directory tree structures

    This program randomly creats and deletes directories and objects.
    The probability for creation is somewhat higher than deletion,
    so the directory tree will keep growing until a certain number
    of objects is reached.

    Command line options specify whether the creted objects should
    be removed again, top leave the MDS database in the state it
    was before, or to leave it so the MDS database can be inspected.

    Another command line option specifies how many entries should
    be created.

    @author Rolf Riesen (rolf@cs.sandia.gov)
    @version $Revision: 1073 $
    @date $Date: 2007-01-22 22:06:53 -0700 (Mon, 22 Jan 2007) $
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <getopt.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "logger/logger.h"
#include "lwfs.h"
#include "lwfs_xdr.h"


/* Some globals */
#define DEFAULT_CNT	(1000)
static log_level debug_level = LOG_OFF;
typedef struct entry_t   {
    lwfs_did_t objID;
    lwfs_did_t parentID;
    int parentIdx;
    int subdirs;
    char name[LWFS_NAME_LEN];
} entry_t;
static int count;
static entry_t *dir_list;
static int dir_list_cnt;


/* Local functions */
static void usage(char *pname);
static int do_create(int verbose);
static int do_remove(int verbose);
static char *create_name(void);
static int test_mkdir(char *dirname, lwfs_did_t *parentID, lwfs_did_t *objID);
static int test_rmdir(char *objname, lwfs_did_t *objID);



/**
    Stress tests for an MDS client.

    @return	0 if successful
    		1 if usage error
		2 if out of memory
		3 internal error
		4 mds error
*/
int
main(int argc, char *argv[])
{

extern char *optarg;
extern int optind;
int ch;
static struct option long_options[] = {
    {"help", no_argument, 0, 6},
    {"clean", no_argument, 0, 7},
    {"nid", required_argument, 0, 8},
    {"count", required_argument, 0, 9},
    {"verbose", required_argument, 0, 10},
    {0, 0, 0, 0}
};
uint64_t nid; 
int verbose;
int clean;

lwfs_did_t rootID= {0, 0, 0, 0};
int current_cnt;
int operations;
long int rnd;
double p1;



    /* Set some defaults */
    verbose= 0;
    count= DEFAULT_CNT;
    clean= 1;
    nid= 0;


    /* Process command line options */
    while (1)   {
	ch = getopt_long_only(argc, argv, "", long_options, NULL);
	if (ch == -1)   {
	    /* Done proecssing command line options */
	    break;
	}

	switch (ch) {
	    case 6: /* help */
		    usage(argv[0]);
		    exit(0);

	    case 7: /* do not clean */
		    clean= 0;
		    break;

	    case 8: /* nid */
		    nid = strtoull(optarg, NULL, 10);
		    break;

	    case 9: /* count */
		    count= atoi(optarg);
		    if (count <= 0)   {
			fprintf(stderr, "count (%d) <= 0!\n", count);
			usage(argv[0]);
			exit(1);
		    }
		    break;

	    case 10: /* verbose */
		    verbose= atoi(optarg);
		    break;

	    default:
		    usage(argv[0]);
		    exit(1);
	}
    }


    /* initialize the logger */
    logger_set_file(stdout); 
    if (verbose == 0)   {
	logger_set_default_level(LOG_OFF); 
    } else if (verbose > 5)   {
	logger_set_default_level(LOG_ALL); 
    } else   {
	logger_set_default_level(LOG_OFF - verbose); 
    }
    if (nid == 0) { 
	fprintf(stderr, "No nid specified\n");
	usage(argv[0]);
	exit(1);
    }


    log_debug(debug_level, "initialize lwfs");
    lwfs_init(nid); 


    dir_list= (entry_t *)malloc((count + 1) * sizeof(entry_t));
    if (dir_list == NULL)   {
	fprintf(stderr, "Out of memory!\n");
	exit(2);
    }
    memset(dir_list, 0, (count + 1) * sizeof(entry_t));
    dir_list_cnt= 0; /* don't count the root directory */

    /* Insert root */
    dir_list[0].subdirs= 0;	/* So far it is empty */
    dir_list[0].parentIdx= -1;	/* We don't have a parent */
    dir_list[0].objID= rootID;
    dir_list[0].parentID= rootID;
    strcpy(dir_list[0].name, "/");


    srandom((unsigned int)time(NULL));
    current_cnt= 0;
    operations= 0;
    p1= 0.25;
    while (current_cnt < count)   {
	rnd= random();
	if ((double)rnd > ((double)RAND_MAX * p1))   {
	    /* create */
	    if (do_create(verbose))    {
		current_cnt++;
	    }
	} else   {
	    if (current_cnt > 0)   {
		/* delete */
		if (do_remove(verbose))    {
		    current_cnt--;
		}
	    }
	}
	operations++;
	if (verbose)   {
	    printf("dir_list_cnt = %6d,       current_cnt = %6d\n",
		dir_list_cnt, current_cnt);
	}
    }

    printf("Performed %d create and remove operations\n", operations);

    if (clean)   {
	/* Remove everything we have created */
    }

    return 0;

}  /* end of main() */


/**
    create an object
*/
static int
do_create(int verbose)
{

int rc;
long int rnd;
long int parent;
long int new;


    if (dir_list_cnt > count)   {
	fprintf(stderr, "Internal error: Too many directories!\n");
	exit(3);
    }

    /* Find a directory to attach to */
    rnd= random();
    parent= (long int)((((double)dir_list_cnt + 1) * (double)rnd) / (double)RAND_MAX);
    new= dir_list_cnt + 1;
    if (verbose > 3)   {
	printf("parent = %ld/%d\n", parent, dir_list_cnt);
    }


    /* Create the new object */
    strcpy(dir_list[new].name, create_name());
    dir_list[new].parentID= dir_list[parent].objID;
    dir_list[new].subdirs= 0;
    dir_list[new].parentIdx= parent;

    if (verbose > 2)   {
	printf("Creating [%ld] %s in obj [%ld] 0x%08x%08x%08x%08x\n", new,
	    dir_list[new].name, parent,
	    (unsigned int)dir_list[new].parentID.part1,
	    (unsigned int)dir_list[new].parentID.part2,
	    (unsigned int)dir_list[new].parentID.part3,
	    (unsigned int)dir_list[new].parentID.part4);
    }
    rc= test_mkdir(dir_list[new].name, &dir_list[new].parentID, &dir_list[new].objID);
    if (rc != LWFS_OK)   {
	fprintf(stderr, "Creation of directory \"%s\" failed\n", dir_list[new].name);
	exit(4);
    }

    dir_list[parent].subdirs++;
    dir_list_cnt++;
    return 1;

}  /* end of do_create() */




/**
    remove an object
*/
static int
do_remove(int verbose)
{

int rc;
int i;
long int rnd;
long int idx;


    if (dir_list_cnt < 1)   {
	fprintf(stderr, "Internal error: Too few directories!\n");
	exit(3);
    }

    /* Find a directory to remove */
    rnd= random();
    idx= (long int)((((double)dir_list_cnt + 1) * (double)rnd) / (double)RAND_MAX);

    /* Find one that has no subdirs */
    while (dir_list[idx].subdirs)   {
	if (verbose > 3)   {
	    printf("Entry %ld has subdirs. Trying next...\n", idx);
	}
	idx++;
	if (idx > dir_list_cnt)   {
	    idx= 1; /* Don't remove root! */
	}
    }

    if (verbose > 2)   {
	printf("removing idx = %ld/%d\n", idx, dir_list_cnt);
    }


    rc= test_rmdir(dir_list[idx].name, &dir_list[idx].objID);
    if (rc != LWFS_OK)   {
	fprintf(stderr, "Removal of directory \"%s\" failed", dir_list[idx].name);
	exit(4);
    }


    /* Reduce count in parent directory */
    dir_list[dir_list[idx].parentIdx].subdirs--;


    /* Copy the last entry over the deleted one, so we always have a compact list */
    dir_list[idx]= dir_list[dir_list_cnt];

    /* Fix all the parent pointers */
    for (i= 0; i < dir_list_cnt; i++)   {
	if (dir_list[i].parentIdx == dir_list_cnt)   {
	    dir_list[i].parentIdx= idx;
	}
    }

    dir_list_cnt--;

    return 1;

}  /* end of do_remove() */



/**
    create a unique name
*/
static char *
create_name(void)
{

static char name[LWFS_NAME_LEN];
static unsigned long int num= 0;


    sprintf(name, "Name%08ld", num++);
    return name;

}  /* end of create_name() */



static void
usage(char *pname)
{

    fprintf(stderr, "Usage: %s -nid <nid> [-help] [-verbose <lvl>] "
	"[-count <cnt>] [-clean]\n", pname);
    fprintf(stderr, "    -nid <nid>      Set nid to <nid> (required command line option!)\n");
    fprintf(stderr, "    -help           This usage message\n");
    fprintf(stderr, "    -verbose <lvl>  Set verbosity to level <lvl>\n");
    fprintf(stderr, "    -count <cnt>    Set count to <cnt> (default %d)\n", DEFAULT_CNT);
    fprintf(stderr, "    -clean          Do not remove all created objects at exit\n");

}  /* end of usage() */



static int
test_mkdir(char *dirname, lwfs_did_t *parentID, lwfs_did_t *objID)
{

lwfs_obj_ref_t dir; 
lwfs_cap_t cap; 
mds_res_N_rc_t result; 
lwfs_request_t request;
char cname[LWFS_NAME_LEN];
mds_name_t name = (mds_name_t)cname; 
int rc;


    log_debug(debug_level, "Creating Subdirectory \"%s\"", dirname);

    strncpy(name, dirname, LWFS_NAME_LEN); 
    memset(&dir, 0, sizeof(dir));
    dir.dbkey= *parentID;

    logger_set_default_level(LOG_OFF);
    rc= mds_mkdir(&dir, &name, &cap, &result, &request); 
    if (rc != LWFS_OK)   {
	fprintf(stderr, "mds_mkdir() error: \"%s\"\n", lwfs_err_str(rc));
	return rc;
    }

    rc= lwfs_comm_wait(&request); 
    logger_set_default_level(debug_level);

    if (rc != LWFS_OK) {
	fprintf(stderr, "mds_mkdir() error: \"%s\"\n", lwfs_err_str(rc));
	return rc;
    }

    log_debug(debug_level, "obj ID for \"%s\" is %08x%08x%08x%08x",
	name, result.node.dbkey.part1, result.node.dbkey.part2,
	result.node.dbkey.part3, result.node.dbkey.part4
    );

    *objID= result.node.dbkey;

    return LWFS_OK;

}  /* end of test_mkdir() */



static int
test_rmdir(char *objname, lwfs_did_t *objID)
{

lwfs_obj_ref_t obj; 
lwfs_cap_t cap; 
mds_res_note_rc_t result; 
lwfs_request_t request;
int rc;


    log_debug(debug_level, "Removing directory \"%s\"", objname);

    memset(&obj, 0, sizeof(obj));
    obj.dbkey= *objID;

    logger_set_default_level(LOG_OFF);
    rc= mds_rmdir(&obj, &cap, &result, &request);
    if (rc != LWFS_OK)   {
	fprintf(stderr, "mds_rmdir() error: \"%s\"\n", lwfs_err_str(rc));
	return rc;
    }

    rc= lwfs_comm_wait(&request); 
    logger_set_default_level(debug_level);

    if (rc != LWFS_OK) {
	fprintf(stderr, "mds_rmdir() error: \"%s\"\n", lwfs_err_str(rc));
	return rc;
    }

    return LWFS_OK;

}  /* end of test_rmdir() */
