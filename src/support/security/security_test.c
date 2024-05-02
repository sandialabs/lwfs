
#include <sys/time.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "logger/logger.h"
#include "security/security.h"
#include "lwfs/types.h"

/**
 * A function to measure how fast we can generate keys. 
 * It returns the time in seconds. 
 */
int generate_keys(int numkeys, double *time) {

	unsigned long starttime, endtime, usec; 
	struct timeval tv; 
	int status; 
	int i;

	lwfs_key key; 

	/* try to generate a key */
	status = generate_key(&key); 
	if (status != LWFS_OK) {
		log_error(LOG_WARN, "could not generate key"); 
		return -1; 
	}

	/* the initial time */
	status = gettimeofday(&tv, NULL); 
	if (status != 0) {
		log_error(LOG_WARN, strerror(status)); 
	}
	starttime = tv.tv_usec; 

	/* start the computations */
	for (i=0; i<numkeys; i++) {

		/* generate a key */
		generate_key(&key); 
	}

	status = gettimeofday(&tv, NULL); 
	if (status != 0) {
		log_error(LOG_WARN, strerror(status)); 
	}
	endtime = tv.tv_usec; 

	*time = (double)(endtime-starttime)/1e6; 

	/*
	fprintf(stderr, "msec = %lu, sec=%e\n", (endtime-starttime), *time);
	*/
	return LWFS_OK;
}



int main() {
    int level=LOG_ALL; 
    int status; 

    lwfs_key key1, key2; 
    lwfs_cred cred1, cred2; 
    lwfs_cap cap1, cap2; 

    double time =0.0;
    int nkeys = 1000000; 

    logger_init(stderr, LOG_WARN); 

    /*
       log_debug(level, "print number=%d", 10);
       log_info(level,  "print number=%d", 11);
       log_warn(level,  "warn msg");
       log_error(level, "error msg");
       log_fatal(level, "fatal msg");
       */

    fprintf(stdout, "\n\n------------ TESTING CREDENTIALS ---------\n\n");

    /* generate two keys */
    status = generate_key(&key1); 
    fprintf(stdout, "generating key1 (size=%d bits) ... ", sizeof(lwfs_key)*8);
    if (status != LWFS_OK) {
        log_error(level, "could not generate key1"); 
        return -1; 
    }
    fprintf(stdout, "passed\n");

    fprintf(stdout, "generating key2 ... ");
    status = generate_key(&key2); 
    if (status != LWFS_OK) {
        log_error(level,"could not generate key2"); 
        return -1; 
    }
    fprintf(stdout, "passed\n");
    fprintf(stdout, "\n");


    /* generate credentials */
    fprintf(stdout, "generating cred1 with key1 ... ");
    status = generate_cred((const lwfs_key *)(&key1), &cred1.data, &cred1); 
    if (status != LWFS_OK) {
        log_error(level,"could not generate cred1"); 
        return -1; 
    }
    fprintf(stdout, "passed\n");
    fprint_lwfs_cred(stdout, "cred1", " ", &cred1);
    fprintf(stdout, "\n");

    fprintf(stdout, "generating cred2 with key2 ... ");
    status = generate_cred((const lwfs_key *)(&key2), &cred2.data, &cred2); 
    if (status != LWFS_OK) {
        log_error(level,"could not generate cred2"); 
        return -1; 
    }
    fprintf(stdout, "passed\n");
    fprint_lwfs_cred(stdout, "cred2", " ", &cred2);
    fprintf(stdout, "\n");


    /* verify credentials */
    fprintf(stdout, "verifying cred1 with key1 ... ");
    status = verify_cred((const lwfs_key *)(&key1), &cred1); 
    if (status != LWFS_OK) {
        log_error(level,"could not verify cred1 with key1"); 
        return -1; 
    }
    fprintf(stdout, "passed\n");

    fprintf(stdout, "verifying cred2 with key2 ... ");
    status = verify_cred((const lwfs_key *)(&key2), &cred2); 
    if (status != LWFS_OK) {
        log_error(level,"could not verify cred2 with key2"); 
        return -1; 
    }
    fprintf(stdout, "passed\n");

    fprintf(stdout, "verifying cred1 with key2 (expecting LWFS_ERR_VERIFYCRED) ... ");
    status = verify_cred((const lwfs_key *)(&key2), &cred1); 
    if (status != LWFS_ERR_VERIFYCRED) {
        log_error(level,"failed verify cred1 with key2"); 
        return -1; 
    }
    fprintf(stdout, "passed\n");

    fprintf(stdout, "verifying cred2 with key1 (expecting LWFS_ERR_VERIFYCRED) ... ");
    status = verify_cred((const lwfs_key *)(&key1), &cred2); 
    if (status != LWFS_ERR_VERIFYCRED) {
        log_error(level,"failed verify cred2 with key1"); 
        return -1; 
    }
    fprintf(stdout, "passed\n");
    fprintf(stdout, "\n");

    /* ---------------- generate caps -------------------- */
    fprintf(stdout, "\n\n------------ TESTING CAPS ---------\n\n");

    fprintf(stdout, "generating cap1 with key1 ... ");
    lwfs_cap_data cap1_data; 
    cap1_data.cid = 1; 
    cap1_data.opcode = 1; 
    status = generate_cap((const lwfs_key *)(&key1), &cap1_data, &cap1); 
    if (status != LWFS_OK) {
        log_error(level,"could not generate cap1"); 
        return -1; 
    }
    fprintf(stdout, "passed\n");
    fprint_lwfs_cap_data(stdout, "data", " ", &cap1_data);
    fprint_lwfs_cap(stdout, "cap1", " ", &cap1);
    fprintf(stdout, "\n");

    fprintf(stdout, "generating cap2 with key2 ... ");
    lwfs_cap_data cap2_data; 
    cap2_data.cid = 2; 
    cap2_data.opcode = 2; 
    status = generate_cap((const lwfs_key *)(&key2), &cap2_data, &cap2); 
    if (status != LWFS_OK) {
        log_error(level,"could not generate cap2"); 
        return -1; 
    }
    fprintf(stdout, "passed\n");
    fprint_lwfs_cap_data(stdout, "cap2_data", " ", &cap2_data);
    fprint_lwfs_cap(stdout, "cap2", " ", &cap2);
    fprintf(stdout, "\n");


    /* verify credentials */
    fprintf(stdout, "verifying cap1 with key1 ... ");
    status = verify_cap((const lwfs_key *)(&key1), &cap1); 
    if (status != LWFS_OK) {
        log_error(level,"could not verify cap1 with key1"); 
        return -1; 
    }
    fprintf(stdout, "passed\n");

    fprintf(stdout, "verifying cap2 with key2 ... ");
    status = verify_cap((const lwfs_key *)(&key2), &cap2); 
    if (status != LWFS_OK) {
        log_error(level,"could not verify cred2 with key2"); 
        return -1; 
    }
    fprintf(stdout, "passed\n");

    fprintf(stdout, "verifying cap1 with key2 (expecting LWFS_ERR_VERIFYCAP) ... ");
    status = verify_cap((const lwfs_key *)(&key2), &cap1); 
    if (status != LWFS_ERR_VERIFYCAP) {
        log_error(level,"verify did not fail!!!"); 
        return -1; 
    }
    fprintf(stdout, "passed\n");

    fprintf(stdout, "verifying cap2 with key1 (expecting LWFS_ERR_VERIFYCAP) ... ");
    status = verify_cap((const lwfs_key *)(&key1), &cap2); 
    if (status != LWFS_ERR_VERIFYCAP) {
        log_error(level,"verify did not fail!!!"); 
        return -1; 
    }
    fprintf(stdout, "passed\n");
    fprintf(stdout, "\n");


    /*---- How fast can we generate keys -----*/
    generate_keys(nkeys, &time); 
    fprintf(stdout, "generated %d keys in %g secs (%g secs/key)\n",
            nkeys, time, time/(double)nkeys);

    return 0;
}


