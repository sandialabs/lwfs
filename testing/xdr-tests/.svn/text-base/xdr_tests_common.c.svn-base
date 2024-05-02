#include <stdio.h>
#include <rpc/rpc.h>
#include "support/logger/logger.h"
#include "xdr_tests_common.h"
#include "test_xdr.h"

int num_entries = 10; 

name_list_t global_list; 
mds_dirop_args args; 
mds_name dirname; 
data_array_t data_array; 

log_level xdr_debug_level = LOG_DEBUG;

/**
 * @brief Initialize global data structures.
 */
int xdr_test_init() 
{
	data_t *data; 

	/* create a list of 10 names */
	name_list_t *curr = &global_list;  /* first entry */

	int i; 
	for (i=0; i<num_entries; i++) {
		curr->name = (char *)malloc(NAME_LEN);
		snprintf(curr->name, NAME_LEN, "entry %d", i);

		if (i < (num_entries-1)) {
			curr->next = (name_list_t *)malloc(1*sizeof(name_list_t));
		}
		else {
			curr->next = NULL;
		}

		/* increment the current pointer */
		if (curr->next != NULL) {
			curr = curr->next; 
		}
	}

	/* initialize an mds_dirop_args structure */
	args.dir = (lwfs_obj_ref *)malloc(1*sizeof(lwfs_obj_ref)); 
	args.name = (mds_name *)malloc(1*sizeof(mds_name));
	*(args.name) = (char *)malloc(NAME_LEN); 
	snprintf(*(args.name), NAME_LEN, "rondir"); 

	/* initialize the data array */
	data = (data_t *)malloc(10*sizeof(data_t)); 
	for (i=0; i<num_entries; i++) {
		data->int_val= i; 
	}
	data_array.data_array_t_len = num_entries;
	data_array.data_array_t_val = data; 

	return 0;
}


int xdr_test_fini() 
{
	xdr_free((xdrproc_t)xdr_name_list_t, (char *)&global_list); 
	xdr_free((xdrproc_t)xdr_mds_dirop_args, (char *)&args); 
	xdr_free((xdrproc_t)xdr_data_array_t, (char *)&data_array);
	return 0;
}

/**
 * @brief Return the size of the buffer needed for the xdrs.
 */
int xdr_test_size() {

	long i; 
	int size = 0; 

	/* add sizes for the long ints */
	size += num_entries*xdr_sizeof((xdrproc_t)&xdr_long, &i); 

	/* add size for entry list */
	size += xdr_sizeof((xdrproc_t)&xdr_name_list_t, &global_list); 

	/* add size for dirops list */
	size += xdr_sizeof((xdrproc_t)&xdr_mds_dirop_args, &args); 

	/* add size for data array */

	int data_array_size = xdr_sizeof((xdrproc_t)&xdr_data_array_t, &data_array); 
	size += data_array_size; 

	int actual_size = 0;
	actual_size += sizeof(u_int); 
	actual_size += num_entries*sizeof(data_t); 

	return size; 
}

/**
 * @brief Decode a set of data structures
 */
int xdr_test_decode(XDR *xdrs) 
{
	long i,j; 

	/*----  read num_entries long integers ----*/
	log_info(xdr_debug_level, "decoding integers...\n");
	for (j = 0; j < num_entries; j++) {
		if (! xdr_long(xdrs, &i)) {
			log_fatal(xdr_debug_level, "failed to decode long\n");
			return -1;
		}

		if (i != j) {
			log_fatal(xdr_debug_level, "incorrect long value: "
					"correct=%d, actual=%d\n",j, i);
			return -1; 
		}
	}

	name_list_t entry; 
	memset(&entry, 0, sizeof(name_list_t)); 

	log_info(xdr_debug_level, "decoding entry list...\n\n"); 
	if (! xdr_name_list_t(xdrs, &entry)) {
		log_fatal(xdr_debug_level, "failed to decode name list\n");
		return -1; 
	}

	/*----------  check the entry list -------------*/
	name_list_t *actual = &entry;
	name_list_t *correct = &global_list;

	while ((actual != NULL) && (correct != NULL)) {
		/* compare the entry names */
		if (strcmp((char *)actual->name, (char *)correct->name) != 0) {
			log_fatal(xdr_debug_level, "incorrect entry name: "
					"correct=%s, actual=%s\n",
					correct->name, actual->name);
			xdr_free((xdrproc_t)xdr_name_list_t, (char *)&entry); 
			return -1;
		}
		/* increment pointers */
		actual = actual->next; 
		correct = correct->next; 
	}

	if ((correct != NULL) || (actual != NULL)) {
		log_fatal(xdr_debug_level, "failed to decode all entries\n");
		xdr_free((xdrproc_t)xdr_name_list_t, (char *)&entry); 
		return -1; 
	}

	/* free the data structures */
	xdr_free((xdrproc_t)xdr_name_list_t, (char *)&entry); 


	/*----------- Decode the dirop_args ---------------*/
	mds_dirop_args decoded_args; 
	memset(&decoded_args, 0, sizeof(mds_dirop_args)); 

	/*
	   decoded_args.dir = NULL;
	   decoded_args.name = NULL;
	 */
	log_info(xdr_debug_level, "decoding mds_dirop_args ...\n\n"); 
	if (! xdr_mds_dirop_args(xdrs, &decoded_args)) {
		log_fatal(xdr_debug_level, "failed to decode mds_dirop_args \n");
		return -1; 
	}

	/* free the data structure */
	xdr_free((xdrproc_t)xdr_mds_dirop_args, (char *)(&decoded_args)); 


	/*----------- Decode the data_array ---------------*/
	data_array_t d_array; 
	memset(&d_array, 0, sizeof(data_array_t)); 

	log_info(xdr_debug_level, "decoding data_array ...\n\n"); 
	if (! xdr_data_array_t(xdrs, &d_array)) {
		log_fatal(xdr_debug_level, "failed to decode data_array \n");
		return -1; 
	}

	/* verify the result */
	for (i=0; i<num_entries; i++) {
		if (d_array.data_array_t_val[i].int_val != data_array.data_array_t_val[i].int_val) {
			log_fatal(xdr_debug_level, "failed to verify data_array[%d]",i);
		}
	}

	/* free the data structure */
	xdr_free((xdrproc_t)xdr_data_array_t, (char *)(&d_array)); 

	return 0; 
}

/**
 * @brief Encode the global data structures.
 */
int xdr_test_encode(XDR *xdrs) 
{
	long j; 

	/* write num_entries long integers */
	log_info(xdr_debug_level, "encoding longs");
	for (j = 0; j < num_entries; j++) {
		if (! xdr_long(xdrs, &j)) {
			log_fatal(xdr_debug_level, "failed to encode long\n");
			return -1;
		}
	}

	log_info(xdr_debug_level, "encoding name list...\n"); 
	if (! xdr_name_list_t(xdrs, &global_list)) {
		log_fatal(xdr_debug_level, "failed to encode name list");
		return -1; 
	}

	log_info(xdr_debug_level, "encoding dirop_args...\n"); 
	if (! xdr_mds_dirop_args(xdrs, &args)) {
		log_fatal(xdr_debug_level, "failed to encode dirop_args");
		return -1; 
	}

	log_info(xdr_debug_level, "encoding data array...\n"); 
	if (! xdr_data_array_t(xdrs, &data_array)) {
		log_fatal(xdr_debug_level, "failed to encode data_array");
		return -1; 
	}
	

	return 0; 
}


