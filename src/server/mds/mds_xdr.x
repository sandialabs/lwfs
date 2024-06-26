/* -------------------------------------------------------------------------- */
/**  
 *   @file mds_xdr.x
 *   
 *   @brief XDR defintions for the metadata service.
 *
 *   @author Rolf Riesen (rolf\@cs.sandia.gov).
 *   $Revision: 791 $.
 *   $Date: 2006-06-29 21:32:24 -0600 (Thu, 29 Jun 2006) $.
 *
 */

/* Include files for the mds_xdr.h file (ignored by others) */
#ifdef RPC_HDR
%#include "lwfs_xdr.h"
%#include "security/security_xdr.h"
#endif 


/* -------------------------------------------------------------------------- */
/* General definitions
*/

    /**
     *  @short Structure for object type.
     *
     *  The object in an mds is either a leaf object or a directory.
     */
    enum mds_otype_t {
	/** The node references a container.  */
	MDS_REG = 1,

	/** The node is a directory.  */
	MDS_DIR = 2,

	/** The node references is a link.  */
	MDS_LNK = 3
    };


    /** A type for object names */
    typedef lwfs_name_t mds_name_t;


    /**
     * @brief A structure for object attributes
     *
     * It is just an area of 256 bytes which the clients can use in any
     * way they wish. One example would be to store creation and other
     * times here. The MDS does not care what is stored in here.
     */
    struct mds_opaque_data_t {
	opaque data[DB_ATTR_SIZE];
    };


    /**
     *  @brief A structure with a node ID and a capability for it
     */
    struct mds_node_args_t {
	lwfs_obj_ref_t *ID;
	lwfs_cap_t *cap;
    };



/* -------------------------------------------------------------------------- */
/** Object attribute operations
*/

    /**
     *  @brief A structure to set object attributes
     */
    struct mds_setattr_args_t {
	lwfs_obj_ref_t *ID;
	lwfs_cap_t *cap;
	mds_opaque_data_t attrs;
    };



/* -------------------------------------------------------------------------- */
/** Argument structures to be sent to the MDS server
*/

    /**
     *  @brief A structure for one node ID
     */
    struct mds_args_N_t {
	mds_node_args_t node;
    };

    /**
     *  @brief A structure for two node IDs and a name
     */
    struct mds_args_NNn_t {
	mds_node_args_t node1;
	mds_node_args_t node2;
	mds_name_t *name;
    };


    /**
     *  @brief A structure for one node ID and a name
     */
    struct mds_args_Nn_t {
	mds_node_args_t node;
	mds_name_t *name;
    };


/* -------------------------------------------------------------------------- */
/** Results from the MDS server
*/

    /**
     *  @brief A structure for results with a simple return code
     */
    struct mds_res_rc_t {
	lwfs_return_code_t ret;
    };


    /**
     *  @brief A structure for results with a note and a return code
     */
    struct mds_res_note_rc_t {
	mds_opaque_data_t note;
	lwfs_return_code_t ret;
    };


    /**
     *  @brief A structure for results with a node ID and a return code
     */
    struct mds_res_N_rc_t {
	lwfs_obj_ref_t node;
	lwfs_return_code_t ret;
    };



/* -------------------------------------------------------------------------- */
/** Read directory operations
*/
    typedef struct mds_entry_t *dlist;

    struct mds_entry_t {
	lwfs_obj_ref_t oref;
	mds_name_t name;
	mds_entry_t *next;
    };

    struct mds_readdir_res_t {
	dlist start;
	int return_code;
    };



/* -------------------------------------------------------------------------- */
/** Capability operations
*/

/**
 *  @brief A structure for getcaps arguments. 
 *
 *  The getcaps method creates capabilities that allow authorized
 *  users to access a container of objects.  The getcaps method
 *  requires the credential of the user requesting access, 
 *  a container ID, and a list of requested operations on that container.
 */
struct mds_getcaps_args_t {
	lwfs_credential_t cred;    /** Credential of the user */
	lwfs_container_id_t cid;   /** The container ID */
	lwfs_op_id_list_t oplist; /** The list of requested operations */
};


/**
 * @brief A structure for getcaps results.
 *
 * The getcaps method either returns a list of capabilities if 
 * all requested operations are authorized, or it returns null.
 */
union mds_getcaps_res_t switch (lwfs_return_code_t return_code) {
    case LWFS_OK:
	lwfs_cap_list_t caps;
    default:
	void;
    };




/* -------------------------------------------------------------------------- */
/** ACL operations
*/

/**
 *  @brief A structure for getacl arguments. 
 *
 *  The getacl method returns the list of users that are 
 *  allowed to perform a specific operation on a specific 
 *  container of objects. 
 */
struct mds_getacl_args_t {
	lwfs_op_id_t opid;       /** The operation to query on */
	lwfs_container_id_t cid; /** The container to query on */
	lwfs_cap_t cap;  /** The capability that allows this operation */
};

    union mds_acl_guard_t switch (bool check) {
    case TRUE:
	lwfs_time_t ctime;
    default:
	void;
    };


    struct mds_aclstat_res_ok_t {
	lwfs_uid_list_t acls;
	mds_acl_guard_t guard;
    };


/**
 * @brief  A structure for the result of a getacl or the modacl
 *         method. 
 *
 * The getacl and modacl methods either return a list of users or null.
 */
union mds_aclstat_t switch (lwfs_return_code_t return_code) {
case LWFS_OK:
	lwfs_uid_list_t acls;
    default:
	void;
    };

/**
 *  @brief A structure for modacl arguments. 
 */
struct mds_modacl_args_t {
	lwfs_container_id_t cid; /** the container id */
	/* do we need this? 
	 * mds_acl_guard guard; */
	lwfs_op_id_t op;         /** the operation */
	lwfs_uid_list_t set;     /** the list of users to add */
	lwfs_uid_list_t unset;   /** the list of users to remove */
	lwfs_cap_t cap;          /** the capability that allows the modacl */
};

/**
 *  @brief A structure for register args.
 *
 */
struct mds_rgstr_args_t {
	int fillvar;
};



/* -------------------------------------------------------------------------- */
/**
 *  The commands we support and what kind of arguments they take
 */
program MDS_PROGRAM {
	version MDS_VERSION {
		void
		MDSPROC_NULL(void) = 0;

		lwfs_return_code_t
		MDS_GETATTR(mds_args_N_t) = 1;

		lwfs_return_code_t
		MDS_SETATTR() = 2;

		lwfs_return_code_t
		MDS_LOOKUP(mds_args_Nn_t) = 3;

		lwfs_return_code_t
		MDS_CREAT(mds_args_Nn_t) = 4;

		lwfs_return_code_t
		MDS_REMOVE(mds_args_N_t) = 5;

		lwfs_return_code_t
		MDS_LINK(mds_args_NNn_t) = 6;

		lwfs_return_code_t
		MDS_MKDIR(mds_args_Nn_t) = 7;

		lwfs_return_code_t
		MDS_RMDIR(mds_args_N_t) = 8;

		lwfs_return_code_t
		MDS_READDIR(mds_args_N_t) = 9;

		lwfs_return_code_t
		MDS_RENAME(mds_args_NNn_t) = 10;

		lwfs_return_code_t
		MDS_GETCAPS(mds_getcaps_args_t) = 11;

		lwfs_return_code_t
		MDS_GETACL(mds_getacl_args_t) = 12;

		lwfs_return_code_t
		MDS_MODACL(mds_modacl_args_t) = 13;

		lwfs_return_code_t
		MDS_REGISTER(mds_rgstr_args_t) = 14;

		lwfs_return_code_t
		MDS_LOOKUP2(mds_args_N_t) = 15;
	} = 1;
} = 57001;


/***************** Log messages ********************************************
 * $Log$
 * Revision 1.10  2004/09/14 17:36:28  raoldfi
 * Added appropriate #include lines to the xdr files using checking the
 * pre-defined environment variable RPC_HDR (see man rpcgen).  Previously,
 * I had to use "sed" to add the include files.
 *
 * Revision 1.9  2004/09/13 15:18:07  raoldfi
 * added a "_t" to all lwfs defined types.
 *
 * Revision 1.8  2004/08/06 23:07:40  rolf
 *
 *     - Added lookup2(). May go away again in the near future.
 *     - mds_lookup_db() used wrong arguments.
 *     - Return # of links and linksto in db_find.
 *     - Bug in delobj.
 *     - Make sure lists are NULL terminated even in case of error.
 *     - Simplified the mds_readdir_res struct.
 *
 * Revision 1.7  2004/07/29 16:58:00  rolf
 *
 *     Added readdir capability (we need that for ls ;-)
 *
 * Revision 1.6  2004/07/26 21:35:57  rolf
 *
 *         Some clean-up and code for hard links. Links seem to work,
 * 	but more debugging and clean-up is required.
 *
 * Revision 1.5  2004/07/25 23:07:01  rolf
 *
 *     - major reorganization and simplification of code in mds_clnt.c
 * 	and mds.c
 *
 *     - renamed some of the arguments and results passed between
 * 	client and MDS server to more closely reflect recent
 * 	nomenclature definitions.
 *
 * Revision 1.4  2004/07/23 23:03:08  rolf
 *
 *     Fixed a bunch of bugs. mds_create still doesn't run all teh
 *     way trough, though.
 *
 * Revision 1.3  2004/06/23 23:41:42  raoldfi
 * Moved definitions of lwfs_cap and lwfs_credential to the ../security. Also
 * made changes to reflect the association between container IDs and ACLs in
 * the MDS.  Previously, an ACL was associated with an object.
 *
 * Revision 1.2  2004/06/23 22:43:48  rolf
 *
 *     Added code for mds_getattr() and changed the data that
 *     is passed between the client and the server. This is a
 *     major change and it doesn't work yet. (It compile, though ;-)
 *     Everything before this check-in is tagged BEFORE_MDS_XDR_CLEANUP
 *
 * Revision 1.1  2004/02/03 18:07:38  raoldfi
 * xdr files for the MDS
 *
 * Revision 1.5  2004/01/07 19:12:39  raoldfi
 * added code to print out the capid of the arguments for mds_create.
 *
 * Revision 1.4  2004/01/07 17:57:35  raoldfi
 * server "mds" correctly decodes the header and identifies the opcode of the
 * request.
 *
 * Revision 1.3  2003/12/20 00:07:43  raoldfi
 * added logging and finished partial implementation of the mds_api. The
 * create() function now correctly sends an xdr-encoded request to the server and
 * the server correctly decodes the request header and prints out the operation.
 *
 * Revision 1.2  2003/12/17 23:00:28  raoldfi
 * fixed some comment bugs.
 *
 * Revision 1.1  2003/12/17 21:21:38  raoldfi
 * initial commit of xdr files.
 *
 * Revision 1.1  2003/12/11 22:23:45  raoldfi
 * initial addition.
 *
 * Revision 1.1  2003/12/07 04:37:59  raoldfi
 * *** empty log message ***
 *
 *
 */

