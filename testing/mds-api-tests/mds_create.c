/*-------------------------------------------------------------------------*/
/**  @file mds_client
 *   
 *   @brief Insert short description of file here. 
 *   
 *   Insert more detailed description of file here (or remove this line). 
 *   
 *   @author Ron Oldfield (raoldfi@sandia.gov)
 *   @version $Revision: 1073 $
 *   @date $Date: 2007-01-22 22:06:53 -0700 (Mon, 22 Jan 2007) $
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <getopt.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "logger/logger.h"
#include "lwfs.h"
#include "lwfs_xdr.h"

static log_level debug_level = LOG_OFF;

/* Local functions */
static void usage(char *pname);
static int test_mkdir(char *dirname, char *parentname, lwfs_did_t *parentID,
    lwfs_did_t *objID);
static int test_mkobj(char *objname, char *parentname, lwfs_did_t *parentID,
    lwfs_did_t *objID);
static int test_lookup(char *objname, lwfs_did_t *parentID, lwfs_did_t *objID);
static int test_lookup2(char *objname, lwfs_did_t *objID);
static int test_remove(char *objname, lwfs_did_t *objID);
static int test_rmdir(char *objname, lwfs_did_t *objID);
static int test_rename(char *objname, lwfs_did_t *objID, char *new_name,
    lwfs_did_t *newDir);
static int test_mklink(char *linkname, char *dirname, char *targetname,
    lwfs_did_t *linkID, lwfs_did_t *dirID, lwfs_did_t *targetID);
static int test_readdir(char *dirname, lwfs_did_t *dirID);



/**
 * Tests for an MDS client. 
 */
int
main(int argc, char *argv[])
{

extern char *optarg;
extern int optind;
int ch;
static struct option long_options[] = {
    {"help", no_argument, 0, 6},
    {"step", no_argument, 0, 7},
    {"nid", required_argument, 0, 8},
    {"verbose", required_argument, 0, 10},
    {0, 0, 0, 0}
};
uint64_t nid; 
int verbose;
int step;
int rc; 
lwfs_did_t rootID= {0, 0, 0, 0};
lwfs_did_t ronID, ron2ID, rolfID, anikaID;
lwfs_did_t oldron2ID, leeID, fooID, barID;
lwfs_did_t findID;


    /* Set some defaults */
    verbose= 0;
    nid= 0;
    step= 0;


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

	    case 7: /* step */
		    step= 1;
		    break;

	    case 8: /* nid */
		    nid = strtoull(optarg, NULL, 10);
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
    debug_level= verbose;
    logger_set_default_level(LOG_OFF); 

    if (nid == 0) { 
	fprintf(stderr, "No nid specified\n");
	usage(argv[0]);
	exit(1);
    }

    log_debug(debug_level, "initialize lwfs");
    lwfs_init(nid); 


    /* Is root there? */
    log_debug(LOG_ALL, "TEST:   lookup object \"%s\"", "/");
    rc= test_lookup("/", &rootID, &findID);
    if (rc == LWFS_OK)   {
	log_debug(LOG_ALL, "PASSED: \"%s\" is here", "/");
    } else   {
	log_debug(LOG_ALL, "FAILED: Root directory \"%s\" is not there", "/");
	return rc;
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }



    /* Is root there, if we know the ID? */
    log_debug(LOG_ALL, "TEST:   lookup2 object \"%s\"", "/");
    rc= test_lookup2("/", &rootID);
    if (rc == LWFS_OK)   {
	log_debug(LOG_ALL, "PASSED: \"%s\" is here", "/");
    } else   {
	log_debug(LOG_ALL, "FAILED: Root directory \"%s\" is not there", "/");
	return rc;
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }



    /* Root should never be removed! */
    log_debug(LOG_ALL, "TEST:   Trying to remove directory \"%s\"", "/");
    rc= test_rmdir("/", &rootID);
    if (rc == LWFS_OK)   {
	log_debug(LOG_ALL, "FAILED: Was able to remove directory \"%s\"!!!!", "/");
	return rc;
    } else   {
	log_debug(LOG_ALL, "PASSED: Couldn't remove directory \"%s\".", "/");
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }


    /* Create an object in the root direcotry */
    log_debug(LOG_ALL, "TEST:   create object \"%s\" in directory \"%s\"", "ron", "/");
    rc= test_mkobj("ron", "/", &rootID, &ronID);
    if (rc != LWFS_OK)   {
	log_debug(LOG_ALL, "FAILED: create object \"%s\" in directory \"%s\"", "ron", "/");
	return rc;
    } else   {
	log_debug(LOG_ALL, "PASSED: created object \"%s\" in directory \"%s\"", "ron", "/");
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }



    /* Creating a second object used to fail... */
    log_debug(LOG_ALL, "TEST:   create object \"%s\" in directory \"%s\"", "ron2", "/");
    rc= test_mkobj("ron2", "/", &rootID, &ron2ID);
    oldron2ID= ron2ID;
    if (rc != LWFS_OK)   {
	log_debug(LOG_ALL, "FAILED: create object \"%s\" in directory \"%s\"", "ron2", "/");
	return rc;
    } else   {
	log_debug(LOG_ALL, "PASSED: created object \"%s\" in directory \"%s\"", "ron2", "/");
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }



    /* This should fail! */
    log_debug(LOG_ALL, "TEST:   create object \"%s\" in directory \"%s\"", "ron2", "/");
    rc= test_mkobj("ron2", "/", &rootID, &ron2ID);
    if (rc == LWFS_OK)   {
	log_debug(LOG_ALL, "FAILED: creating the object \"%s\" twice did not fail!", "ron2");
	return LWFS_ERR_MDS_EXIST;
    } else   {
	log_debug(LOG_ALL, "PASSED: creating the object \"%s\" twice was denied!", "ron2");
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }



    /* Rename ron2 to ron3, leave it in / */
    log_debug(LOG_ALL, "TEST:   rename object \"%s\" in directory \"%s\" to \"%s\"", "ron2", "/", "ron3");
    rc= test_rename("ron2", &oldron2ID, "ron3", &rootID);
    if (rc != LWFS_OK)   {
	log_debug(LOG_ALL, "FAILED: rename \"%s\" to \"%s\"", "ron2", "ron3");
	return rc;
    } else   {
	log_debug(LOG_ALL, "PASSED: rename object \"%s\" in directory \"%s\" to \"%s\"", "ron2", "/", "ron3");
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }



    /* It should not be known under "ron2" anymore! */
    log_debug(LOG_ALL, "TEST:   lookup object \"%s\" in directory \"%s\"", "ron2", "/");
    rc= test_lookup("ron2", &rootID, &findID);
    if (rc == LWFS_OK)   {
	log_debug(LOG_ALL, "FAILED: \"%s\" still known", "ron2");
	return rc;
    } else   {
	log_debug(LOG_ALL, "PASSED: object name \"%s\" in directory \"%s\" is not known anymore", "ron2", "/");
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }



    /* Can we find it as "ron3"? */
    log_debug(LOG_ALL, "TEST:   lookup object \"%s\" in directory \"%s\"", "ron3", "/");
    rc= test_lookup("ron3", &rootID, &findID);
    if (rc != LWFS_OK)   {
	log_debug(LOG_ALL, "FAILED: object \"%s\" in directory \"%s\" is not known", "ron3", "/");
	return rc;
    } else   {
	log_debug(LOG_ALL, "PASSED: object \"%s\" in directory \"%s\" found", "ron3", "/");
    }
    log_debug(LOG_ALL, "TEST:   checking object \"%s\" ID", "ron3");
    if ((findID.part1 != oldron2ID.part1) || (findID.part2 != oldron2ID.part2) ||
	    (findID.part3 != oldron2ID.part3) || (findID.part4 != oldron2ID.part4))   {
	log_debug(LOG_ALL, "FAILED: ID for \"%s\" is not the same anymore", "ron3");
	return LWFS_ERR_CONTEXT;
    } else   {
	log_debug(LOG_ALL, "PASSED: ID for object \"%s\" is correct", "ron3");
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }



    /* Create a sub-directory inside / */
    log_debug(LOG_ALL, "TEST:   create directory \"%s\" in directory \"%s\"", "/rolf", "/");
    rc= test_mkdir("/rolf", "/", &rootID, &rolfID);
    if (rc != LWFS_OK)   {
	log_debug(LOG_ALL, "FAILED: could not create directory \"%s\" in \"%s\"", "/rolf", "/");
	return rc;
    } else   {
	log_debug(LOG_ALL, "PASSED: directory \"%s\" created in directory \'%s\"", "/rolf", "/");
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }



    /* Create an object inside the sub-directory */
    log_debug(LOG_ALL, "TEST:   create object \"%s\" in directory \"%s\"", "anika", "/rolf");
    rc= test_mkobj("anika", "/rolf", &rolfID, &anikaID);
    if (rc != LWFS_OK)   {
	log_debug(LOG_ALL, "FAILED: creation of object \"%s\" in directory \"%s\"", "anika", "/rolf");
	return rc;
    } else   {
	log_debug(LOG_ALL, "PASSED: creation of object \"%s\" in directory \"%s\"", "anika", "/rolf");
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }



    /* Can we find it again? */
    log_debug(LOG_ALL, "TEST:   lookup object \"%s\"", "anika");
    rc= test_lookup("anika", &rolfID, &findID);
    if (rc != LWFS_OK)   {
	log_debug(LOG_ALL, "FAILED: cannot find object \"%s\"", "anika");
	return rc;
    } else   {
	log_debug(LOG_ALL, "PASSED: found object \"%s\"", "anika");
    }
    log_debug(LOG_ALL, "TEST:   checking ID for object \"%s\"", "anika");
    if ((findID.part1 != anikaID.part1) || (findID.part2 != anikaID.part2) ||
	    (findID.part3 != anikaID.part3) || (findID.part4 != anikaID.part4))   {
	log_debug(LOG_ALL, "FAILED: ID for object \"%s\" is incorrect", "anika");
	return LWFS_ERR_CONTEXT;
    } else   {
	log_debug(LOG_ALL, "PASSED: ID for object \"%s\" is correct", "anika");
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }



    /* Move ron3 from / to /rolf */
    log_debug(LOG_ALL, "TEST:   move object \"%s\" in directory \"%s\" to \"%s\"", "ron3", "/", "/rolf");
    rc= test_rename("ron3", &oldron2ID, "ron3", &rolfID);
    if (rc != LWFS_OK)   {
	log_debug(LOG_ALL, "FAILED: move \"%s\" to \"%s\"", "ron3", "/rolf");
	return rc;
    } else   {
	log_debug(LOG_ALL, "PASSED: move object \"%s\" into directory \"%s\"", "ron3", "/rolf");
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }



    /* Can we find it again? */
    log_debug(LOG_ALL, "TEST:   lookup object \"%s\"", "ron3");
    rc= test_lookup("ron3", &rolfID, &findID);
    if (rc != LWFS_OK)   {
	log_debug(LOG_ALL, "FAILED: cannot find object \"%s\"", "ron3");
	return rc;
    } else   {
	log_debug(LOG_ALL, "PASSED: found object \"%s\"", "ron3");
    }
    log_debug(LOG_ALL, "TEST:   checking ID for object \"%s\"", "ron3");
    if ((findID.part1 != oldron2ID.part1) || (findID.part2 != oldron2ID.part2) ||
	    (findID.part3 != oldron2ID.part3) || (findID.part4 != oldron2ID.part4))   {
	log_debug(LOG_ALL, "FAILED: ID for object \"%s\" is incorrect", "ron3");
	return LWFS_ERR_CONTEXT;
    } else   {
	log_debug(LOG_ALL, "PASSED: ID for object \"%s\" is correct", "ron3");
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }



    /* Rename /rolf (a dir with files in it) to /newrolf */
    log_debug(LOG_ALL, "TEST:   rename object \"%s\" (a dir with files in it) in directory \"%s\" to \"%s\"", "/rolf", "/", "/newrolf");
    rc= test_rename("/rolf", &rolfID, "newrolf", &rootID);
    if (rc != LWFS_OK)   {
	log_debug(LOG_ALL, "FAILED: rename \"%s\" to \"%s\"", "/rolf", "/newrolf");
	return rc;
    } else   {
	log_debug(LOG_ALL, "PASSED: rename object \"%s\" into directory \"%s\"", "/rolf", "/newrolf");
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }


    /* Create /lee */
    log_debug(LOG_ALL, "TEST:   create directory \"%s\" in directory \"%s\"", "lee", "/");
    rc= test_mkdir("lee", "/", &rootID, &leeID);
    if (rc != LWFS_OK)   {
	log_debug(LOG_ALL, "FAILED: could not create directory \"%s\" in \"%s\"", "lee", "/");
	return rc;
    } else   {
	log_debug(LOG_ALL, "PASSED: directory \"%s\" created in directory \'%s\"", "lee", "/");
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }



    /* Create "foo" inside "lee" */
    log_debug(LOG_ALL, "TEST:   create object \"%s\" in directory \"%s\"", "foo", "/lee");
    rc= test_mkobj("foo", "/lee", &leeID, &fooID);
    if (rc != LWFS_OK)   {
	log_debug(LOG_ALL, "FAILED: creation of object \"%s\" in directory \"%s\"", "foo", "/lee");
	return rc;
    } else   {
	log_debug(LOG_ALL, "PASSED: creation of object \"%s\" in directory \"%s\"", "foo", "/lee");
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }



    /* Create link "bar" inside "lee" to "foo" */
    log_debug(LOG_ALL, "TEST:   create link \"%s\" in directory \"%s\" to \"%s\"", "bar", "/lee", "foo");
    rc= test_mklink("bar", "/lee", "foo", &barID, &leeID, &fooID);
    if (rc != LWFS_OK)   {
	log_debug(LOG_ALL, "FAILED: creation of link \"%s\" in directory \"%s\" to \"%s\"", "bar", "/lee", "foo");
	return rc;
    } else   {
	log_debug(LOG_ALL, "PASSED: creation of link \"%s\" in directory \"%s\" to \"%s\"", "bar", "/lee", "foo");
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }



    log_debug(LOG_ALL, "TEST:   lookup object \"%s\" in \"%s\"", "bar", "/lee");
    rc= test_lookup("bar", &leeID, &findID);
    if (rc == LWFS_OK)   {
	log_debug(LOG_ALL, "PASSED: \"%s\" is here", "bar");
    } else   {
	log_debug(LOG_ALL, "FAILED: \"%s\" is not there", "bar");
	return rc;
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }



    log_debug(LOG_ALL, "TEST:   lookup object \"%s\" in \"%s\"", "foo", "/lee");
    rc= test_lookup("foo", &leeID, &findID);
    if (rc == LWFS_OK)   {
	log_debug(LOG_ALL, "PASSED: \"%s\" is here", "foo");
    } else   {
	log_debug(LOG_ALL, "FAILED: \"%s\" is not there", "foo");
	return rc;
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }



    /* Read directory / */
    log_debug(LOG_ALL, "TEST:   reading directory \"%s\"", "/");
    rc= test_readdir("/", &rootID);
    if (rc == LWFS_OK)   {
	log_debug(LOG_ALL, "PASSED: reading directory \"%s\"", "/");
    } else   {
	log_debug(LOG_ALL, "FAILED: reading directory \"%s\"", "/");
	return rc;
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }



    log_debug(LOG_ALL, "TEST:   reading directory \"%s\"", "/");
    rc= test_readdir("/", &rootID);
    if (rc == LWFS_OK)   {
	log_debug(LOG_ALL, "PASSED: reading directory \"%s\"", "/");
    } else   {
	log_debug(LOG_ALL, "FAILED: reading directory \"%s\"", "/");
	return rc;
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }



    /* Read directory /newrolf */
    log_debug(LOG_ALL, "TEST:   reading directory \"%s\"", "/newrolf");
    rc= test_readdir("/newrolf", &rolfID);
    if (rc == LWFS_OK)   {
	log_debug(LOG_ALL, "PASSED: reading directory \"%s\"", "/newrolf");
    } else   {
	log_debug(LOG_ALL, "FAILED: reading directory \"%s\"", "/newrolf");
	return rc;
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }



    /* Read directory /lee */
    log_debug(LOG_ALL, "TEST:   reading directory \"%s\"", "/lee");
    rc= test_readdir("/lee", &leeID);
    if (rc == LWFS_OK)   {
	log_debug(LOG_ALL, "PASSED: reading directory \"%s\"", "/lee");
    } else   {
	log_debug(LOG_ALL, "FAILED: reading directory \"%s\"", "/lee");
	return rc;
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }



    /* Remove /lee/bar */
    log_debug(LOG_ALL, "TEST:   remove object \"%s\"", "/lee/bar");
    rc= test_remove("bar", &barID);
    if (rc != LWFS_OK)   {
	log_debug(LOG_ALL, "FAILED: object \"%s\" could not be removed", "bar");
	return rc;
    } else   {
	log_debug(LOG_ALL, "PASSED: object \"%s\" has been removed", "bar");
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }



    /* This should fail, bar is gone */
    log_debug(LOG_ALL, "TEST:   lookup object \"%s\" in \"%s\"", "bar", "/lee");
    rc= test_lookup("bar", &leeID, &findID);
    if (rc == LWFS_OK)   {
	log_debug(LOG_ALL, "FAILED: \"%s\" is still there", "bar");
	return rc;
    } else   {
	log_debug(LOG_ALL, "PASSED: \"%s\" is gone", "bar");
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }



    log_debug(LOG_ALL, "TEST:   lookup object \"%s\" in \"%s\"", "foo", "/lee");
    rc= test_lookup("foo", &leeID, &findID);
    if (rc == LWFS_OK)   {
	log_debug(LOG_ALL, "PASSED: \"%s\" is here", "foo");
    } else   {
	log_debug(LOG_ALL, "FAILED: \"%s\" is not there", "foo");
	return rc;
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }



    /* Create the link "bar" inside "lee" to "foo" again */
    log_debug(LOG_ALL, "TEST:   create link \"%s\" in directory \"%s\" to \"%s\"", "bar", "/lee", "foo");
    rc= test_mklink("bar", "/lee", "foo", &barID, &leeID, &fooID);
    if (rc != LWFS_OK)   {
	log_debug(LOG_ALL, "FAILED: creation of link \"%s\" in directory \"%s\" to \"%s\"", "bar", "/lee", "foo");
	return rc;
    } else   {
	log_debug(LOG_ALL, "PASSED: creation of link \"%s\" in directory \"%s\" to \"%s\"", "bar", "/lee", "foo");
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }



    /* Remove /lee/foo */
    log_debug(LOG_ALL, "TEST:   remove object \"%s\"", "/lee/foo");
    rc= test_remove("foo", &fooID);
    if (rc != LWFS_OK)   {
	log_debug(LOG_ALL, "FAILED: object \"%s\" could not be removed", "foo");
	return rc;
    } else   {
	log_debug(LOG_ALL, "PASSED: object \"%s\" has been removed", "foo");
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }


    /* Make sure foo is gone */
    log_debug(LOG_ALL, "TEST:   lookup object \"%s\" in \"%s\"", "foo", "/lee");
    rc= test_lookup("foo", &leeID, &findID);
    if (rc == LWFS_OK)   {
	log_debug(LOG_ALL, "FAILED: \"%s\" is still there", "foo");
	return rc;
    } else   {
	log_debug(LOG_ALL, "PASSED: \"%s\" is gone", "foo");
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }


    /* Make sure bar is still here */
    log_debug(LOG_ALL, "TEST:   lookup object \"%s\" in \"%s\"", "bar", "/lee");
    rc= test_lookup("bar", &leeID, &findID);
    if (rc == LWFS_OK)   {
	log_debug(LOG_ALL, "PASSED: \"%s\" is here", "bar");
    } else   {
	log_debug(LOG_ALL, "FAILED: \"%s\" is not there", "bar");
	return rc;
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }



    /* Read directory /lee */
    log_debug(LOG_ALL, "TEST:   reading directory \"%s\"", "/lee");
    rc= test_readdir("/lee", &leeID);
    if (rc == LWFS_OK)   {
	log_debug(LOG_ALL, "PASSED: reading directory \"%s\"", "/lee");
    } else   {
	log_debug(LOG_ALL, "FAILED: reading directory \"%s\"", "/lee");
	return rc;
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }




    /*
    ** Start tearing everything appart again
    */



    /* Remove /lee/bar */
    log_debug(LOG_ALL, "TEST:   remove object \"%s\"", "/lee/bar");
    rc= test_remove("bar", &barID);
    if (rc != LWFS_OK)   {
	log_debug(LOG_ALL, "FAILED: object \"%s\" could not be removed", "bar");
	return rc;
    } else   {
	log_debug(LOG_ALL, "PASSED: object \"%s\" has been removed", "bar");
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }



    /* Remove directory "lee" */
    log_debug(LOG_ALL, "TEST:   remove directory \"%s\"", "/lee");
    rc= test_rmdir("/lee", &leeID);
    if (rc != LWFS_OK)   {
	log_debug(LOG_ALL, "FAILED: directory \"%s\" still exists", "/lee");
	return rc;
    } else   {
	log_debug(LOG_ALL, "PASSED: removed directory \"%s\"", "/lee");
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }




    /* Now remove anika! */
    log_debug(LOG_ALL, "TEST:   remove object \"%s\"", "anika");
    rc= test_remove("anika", &anikaID);
    if (rc != LWFS_OK)   {
	log_debug(LOG_ALL, "FAILED: object \"%s\" could not be removed", "anika");
	return rc;
    } else   {
	log_debug(LOG_ALL, "PASSED: object \"%s\" has been removed", "anika");
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }



    /* Do it again to make sure it is gone */
    log_debug(LOG_ALL, "TEST:   remove object \"%s\" again", "anika");
    rc= test_remove("anika", &anikaID);
    if (rc == LWFS_OK)   {
	log_debug(LOG_ALL, "FAILED: Removed object \"%s\" twice!", "anika");
	return rc;
    } else   {
	log_debug(LOG_ALL, "PASSED: Couldn't remove object \"%s\" twice!", "anika");
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }



    /* Remove a directory (this should fail) */
    log_debug(LOG_ALL, "TEST:   remove non-empty directory \"%s\"", "/newrolf");
    rc= test_rmdir("/newrolf", &rolfID);
    if (rc == LWFS_OK)   {
	log_debug(LOG_ALL, "FAILED: non-empty directory \"%s\" removed", "/newrolf");
	return rc;
    } else   {
	log_debug(LOG_ALL, "PASSED: non-empty directory \"%s\" not removed", "/newrolf");
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }



    /* Remove ron */
    log_debug(LOG_ALL, "TEST:   remove object \"%s\"", "ron");
    rc= test_remove("ron", &ronID);
    if (rc != LWFS_OK)   {
	log_debug(LOG_ALL, "FAILED: object \"%s\" still exists", "ron");
	return rc;
    } else   {
	log_debug(LOG_ALL, "PASSED: removed object \"%s\"", "ron");
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }



    /* Remove ron2 */
    log_debug(LOG_ALL, "TEST:   remove object \"%s\"", "ron3");
    rc= test_remove("ron3", &oldron2ID);
    if (rc != LWFS_OK)   {
	log_debug(LOG_ALL, "FAILED: object \"%s\" still exists", "ron3");
	return rc;
    } else   {
	log_debug(LOG_ALL, "PASSED: removed object \"%s\"", "ron3");
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }



    /* Remove a directory */
    log_debug(LOG_ALL, "TEST:   remove directory \"%s\"", "/newrolf");
    rc= test_rmdir("/newrolf", &rolfID);
    if (rc != LWFS_OK)   {
	log_debug(LOG_ALL, "FAILED: directory \"%s\" still exists", "/newrolf");
	return rc;
    } else   {
	log_debug(LOG_ALL, "PASSED: removed directory \"%s\"", "/newrolf");
    }
    if (step)   {
	int ch;
	printf("Hit \"Enter\" for the next step:\n");
	ch= getchar();
    }



    return LWFS_OK;

}  /* end of main() */



static void
usage(char *pname)
{

    fprintf(stderr, "Usage: %s -nid <nid> [-help] [-verbose <lvl>] "
	"[-step]\n", pname);
    fprintf(stderr, "    -nid <nid>      Set nid to <nid> (required command line option!)\n");
    fprintf(stderr, "    -help           This usage message\n");
    fprintf(stderr, "    -verbose <lvl>  Set verbosity to level <lvl>\n");
    fprintf(stderr, "    -step           Single step execution\n");

}  /* end of usage() */



static int
test_lookup(char *objname, lwfs_did_t *parentID, lwfs_did_t *objID)
{

lwfs_obj_ref_t dir; 
lwfs_cap_t cap; 
mds_res_N_rc_t result; 
lwfs_request_t request;
char cname[LWFS_NAME_LEN];
mds_name_t name = (mds_name_t)cname; 
int rc;


    log_debug(debug_level, "Looking up object \"%s\"", objname);

    strncpy(name, objname, LWFS_NAME_LEN); 
    memset(&dir, 0, sizeof(dir));
    dir.dbkey= *parentID;

    logger_set_default_level(LOG_OFF);
    rc= mds_lookup(&dir, &name, &cap, &result, &request);
    if (rc != LWFS_OK) {
	logger_set_default_level(debug_level);
	log_error(debug_level, "mds_lookup() error: \"%s\"", lwfs_err_str(rc));
	return rc;
    }

    rc= lwfs_comm_wait(&request); 
    logger_set_default_level(debug_level);

    if (rc != LWFS_OK) {
	log_error(debug_level, "mds_lookup() error: \"%s\"", lwfs_err_str(rc));
	return rc;
    }

    log_debug(debug_level, "obj ID for \"%s\" is %08x%08x%08x%08x",
	name, result.node.dbkey.part1, result.node.dbkey.part2,
	result.node.dbkey.part3, result.node.dbkey.part4
    );

    *objID= result.node.dbkey;

    return result.ret;

}  /* end of test_lookup() */



static int
test_lookup2(char *objname, lwfs_did_t *objID)
{

lwfs_obj_ref_t dir; 
lwfs_cap_t cap; 
mds_res_note_rc_t result; 
lwfs_request_t request;
int rc;


    log_debug(debug_level, "Looking up object \"%s\"", objname);

    memset(&dir, 0, sizeof(dir));
    dir.dbkey= *objID;

    logger_set_default_level(LOG_OFF);
    rc= mds_lookup2(&dir, &cap, &result, &request);
    if (rc != LWFS_OK) {
	logger_set_default_level(debug_level);
	log_error(debug_level, "mds_lookup2() error: \"%s\"", lwfs_err_str(rc));
	return rc;
    }

    rc= lwfs_comm_wait(&request); 
    logger_set_default_level(debug_level);

    if (rc != LWFS_OK) {
	log_error(debug_level, "mds_lookup2() error: \"%s\"", lwfs_err_str(rc));
	return rc;
    }

    return result.ret;

}  /* end of test_lookup2() */



static int
test_mkobj(char *objname, char *parentname, lwfs_did_t *parentID, lwfs_did_t *objID)
{

lwfs_obj_ref_t dir; 
lwfs_cap_t cap; 
mds_res_N_rc_t result; 
lwfs_request_t request;
char cname[LWFS_NAME_LEN];
mds_name_t name = (mds_name_t)cname; 
int rc;


    log_debug(debug_level, "Creating object \"%s\" in \"%s\"",
	objname, parentname);

    strncpy(name, objname, LWFS_NAME_LEN); 
    memset(&dir, 0, sizeof(dir));
    dir.dbkey= *parentID;

    logger_set_default_level(LOG_OFF);
    rc= mds_create(&dir, &name, &cap, &result, &request); 
    if (rc != LWFS_OK) {
	logger_set_default_level(debug_level);
	log_error(debug_level, "mds_create() error: \"%s\"",
	    lwfs_err_str(rc));
	return rc;
    }

    rc= lwfs_comm_wait(&request); 
    logger_set_default_level(debug_level);

    if (rc != LWFS_OK) {
	log_error(debug_level, "mds_create() error: \"%s\"", lwfs_err_str(rc));
	return rc;
    }

    log_debug(debug_level, "obj ID for \"%s\" is %08x%08x%08x%08x",
	name, result.node.dbkey.part1, result.node.dbkey.part2,
	result.node.dbkey.part3, result.node.dbkey.part4
    );

    *objID= result.node.dbkey;

    return result.ret;

}  /* end of test_mkobj() */



static int
test_mkdir(char *dirname, char *parentname, lwfs_did_t *parentID, lwfs_did_t *objID)
{

lwfs_obj_ref_t dir; 
lwfs_cap_t cap; 
mds_res_N_rc_t result; 
lwfs_request_t request;
char cname[LWFS_NAME_LEN];
mds_name_t name = (mds_name_t)cname; 
int rc;


    log_debug(debug_level, "Creating Subdirectory \"%s\" in \"%s\"",
	dirname, parentname);

    strncpy(name, dirname, LWFS_NAME_LEN); 
    memset(&dir, 0, sizeof(dir));
    dir.dbkey= *parentID;

    logger_set_default_level(LOG_OFF);
    rc= mds_mkdir(&dir, &name, &cap, &result, &request); 
    if (rc != LWFS_OK) {
	logger_set_default_level(debug_level);
	log_error(debug_level, "mds_mkdir() error: \"%s\"", lwfs_err_str(rc));
	return rc;
    }

    rc= lwfs_comm_wait(&request); 
    logger_set_default_level(debug_level);

    if (rc != LWFS_OK) {
	log_error(debug_level, "mds_mkdir() error: \"%s\"", lwfs_err_str(rc));
	return rc;
    }

    log_debug(debug_level, "obj ID for \"%s\" is %08x%08x%08x%08x",
	name, result.node.dbkey.part1, result.node.dbkey.part2,
	result.node.dbkey.part3, result.node.dbkey.part4
    );

    *objID= result.node.dbkey;

    return result.ret;

}  /* end of test_mkdir() */



static int
test_remove(char *objname, lwfs_did_t *objID)
{

lwfs_obj_ref_t obj; 
lwfs_cap_t cap; 
mds_res_note_rc_t result; 
lwfs_request_t request;
int rc;


    log_debug(debug_level, "Removing object \"%s\"", objname);

    memset(&obj, 0, sizeof(obj));
    obj.dbkey= *objID;

    logger_set_default_level(LOG_OFF);
    rc= mds_remove(&obj, &cap, &result, &request);
    if (rc != LWFS_OK) {
	logger_set_default_level(debug_level);
	log_error(debug_level, "mds_remove() error: \"%s\"", lwfs_err_str(rc));
	return rc;
    }

    rc= lwfs_comm_wait(&request); 
    logger_set_default_level(debug_level);

    if (rc != LWFS_OK) {
	log_error(debug_level, "mds_remove() error: \"%s\"", lwfs_err_str(rc));
	return rc;
    }

    return result.ret;

}  /* end of test_remove() */



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
    if (rc != LWFS_OK) {
	logger_set_default_level(debug_level);
	log_error(debug_level, "mds_rmdir() error: \"%s\"", lwfs_err_str(rc));
	return rc;
    }

    rc= lwfs_comm_wait(&request); 
    logger_set_default_level(debug_level);

    if (rc != LWFS_OK) {
	log_error(debug_level, "mds_rmdir() error: \"%s\"", lwfs_err_str(rc));
	return rc;
    }

    return result.ret;

}  /* end of test_rmdir() */



static int
test_rename(char *objname, lwfs_did_t *objID, char *new_name, lwfs_did_t *newDir)
{

lwfs_obj_ref_t obj; 
lwfs_obj_ref_t dir; 
lwfs_cap_t srccap, destcap; 
mds_res_N_rc_t result; 
lwfs_request_t request;
char cname[LWFS_NAME_LEN];
mds_name_t name = (mds_name_t)cname; 
int rc;


    log_debug(debug_level, "Renaming object \"%s\" to \"%s\"", objname, new_name);

    strncpy(name, new_name, LWFS_NAME_LEN); 
    memset(&obj, 0, sizeof(obj));
    obj.dbkey= *objID;
    memset(&dir, 0, sizeof(dir));
    dir.dbkey= *newDir;

    logger_set_default_level(LOG_OFF);
    rc= mds_rename(&obj, &srccap, &dir, &name, &destcap, &result, &request);
    if (rc != LWFS_OK) {
	logger_set_default_level(debug_level);
	log_error(debug_level, "mds_rename() error: \"%s\"", lwfs_err_str(rc));
	return rc;
    }

    rc= lwfs_comm_wait(&request); 
    logger_set_default_level(debug_level);

    if (rc != LWFS_OK) {
	log_error(debug_level, "mds_rename() error: \"%s\"", lwfs_err_str(rc));
	return rc;
    }

    return result.ret;

}  /* end of test_rename() */


static int
test_mklink(char *linkname, char *dirname, char *targetname, lwfs_did_t *linkID, lwfs_did_t *dirID, lwfs_did_t *targetID)
{

lwfs_obj_ref_t target;
lwfs_obj_ref_t dir; 
char cname[LWFS_NAME_LEN];
mds_name_t name = (mds_name_t)cname; 
lwfs_cap_t destcap; 
mds_res_N_rc_t result; 
lwfs_request_t request;
int rc;


    memset(&target, 0, sizeof(target));
    target.dbkey= *targetID;
    memset(&dir, 0, sizeof(dir));
    dir.dbkey= *dirID;
    strncpy(name, linkname, LWFS_NAME_LEN); 

    logger_set_default_level(LOG_OFF);
    rc= mds_link(&target, &dir, &name, &destcap, &result, &request);
    if (rc != LWFS_OK) {
	logger_set_default_level(debug_level);
	log_error(debug_level, "mds_link() error: \"%s\"", lwfs_err_str(rc));
	return rc;
    }

    rc= lwfs_comm_wait(&request); 
    logger_set_default_level(debug_level);

    if (rc != LWFS_OK) {
	log_error(debug_level, "mds_link() error: \"%s\"", lwfs_err_str(rc));
	return rc;
    }

    *linkID= result.node.dbkey;
    return result.ret;


}  /* end of test_mklink() */



static int
test_readdir(char *dirname, lwfs_did_t *dirID)
{

lwfs_obj_ref_t dir; 
lwfs_cap_t cap; 
mds_readdir_res_t result; 
lwfs_request_t request;
int rc;
/* struct mds_entry *entry; */
dlist entry;


    log_debug(debug_level, "reading directory \"%s\"", dirname);

    memset(&dir, 0, sizeof(dir));
    dir.dbkey= *dirID;

    logger_set_default_level(LOG_OFF);
    memset(&result, 0, sizeof(result));
    memset(&request, 0, sizeof(request));
    rc= mds_readdir(&dir, &cap, &result, &request);
    if (rc != LWFS_OK) {
	logger_set_default_level(debug_level);
	log_error(debug_level, "mds_readdir() error: \"%s\"", lwfs_err_str(rc));
	return rc;
    }

    rc= lwfs_comm_wait(&request); 
    logger_set_default_level(debug_level);

    if (rc != LWFS_OK) {
	log_error(debug_level, "mds_readdir() error: \"%s\"", lwfs_err_str(rc));
	return rc;
    }

    if (result.return_code == LWFS_OK)   {
	log_debug(LOG_ALL, "Listing for directory \"%s\"", dirname);
	entry= result.start;
	while (entry)   {
	    log_debug(LOG_ALL, "    --- type %d,   ID 0x%08x%08x%08x%08x,   name \"%s\"",
		entry->oref.type,
		entry->oref.dbkey.part1, entry->oref.dbkey.part2,
		entry->oref.dbkey.part3, entry->oref.dbkey.part4,
		entry->name);
	    entry= entry->next;
	}
	log_debug(LOG_ALL, "done with \"%s\"", dirname);
    }

    return result.return_code;

}  /* end of test_readdir() */
