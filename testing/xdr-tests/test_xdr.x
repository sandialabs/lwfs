/*-------------------------------------------------------------------------*/
/**  
 *   @file test_xdr.x
 *   
 *   @brief XDR defintions for the metadata service.
 *
 *   @author Ron Oldfield (raoldfi\@sandia.gov).
 *   $Revision: 1073 $.
 *   $Date: 2007-01-22 22:06:53 -0700 (Mon, 22 Jan 2007) $.
 *
 */

const NAME_LEN = 256;

struct name_list_t {
	string name<NAME_LEN>; 
	name_list_t *next;
};

const LWFS_UUIDSIZE=16; 
typedef opaque lwfs_uuid[LWFS_UUIDSIZE];
typedef lwfs_uuid lwfs_ssid; 

typedef lwfs_uuid lwfs_vid;

typedef unsigned long uint32; 
typedef unsigned hyper uint64; 
typedef uint64 lwfs_oid; 

struct lwfs_did {
	uint32 part1; 
	uint32 part2; 
	uint32 part3; 
	uint32 part4; 
};

struct lwfs_obj_ref {
	lwfs_ssid ssid; 
	lwfs_vid vid;
	lwfs_oid oid; 
	struct lwfs_did dbkey;
};

const MDS_NAME_LEN = 64;
typedef string mds_name<MDS_NAME_LEN>; 

struct mds_dirop_args {
	lwfs_obj_ref *dir;
	mds_name *name;
	/*lwfs_cap *cap;*/
};

struct data_t {
	int int_val; 
	/*
	float float_val; 
	double double_val; 
	*/
};

typedef data_t data_array_t<>; 


const OPAQUE_ARRAY_LEN = 16;
typedef opaque opaque_array[OPAQUE_ARRAY_LEN]; 

