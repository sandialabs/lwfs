/**
 *   @file types.x
 *
 *   @brief XDR definitions for messaging layer of the LWFS.
 *
 *   The LWFS servers receive rpc requests in the form of xdr-encoded
 *   data structures. This file contains the definitions of the
 *   the data structures that need to be transferred to/from LWFS
 *   servers.
 *
 *   @author Ron Oldfield (raoldfi\@sandia.gov)
 *   $Revision: 1440 $
 *   $Date: 2007-05-21 08:14:50 -0600 (Mon, 21 May 2007) $
 *
 */

#ifdef RPC_HDR

%#include "config.h"
%#include "lwfs_int.h"
%
%#ifdef BROKEN_DARWIN_RPCGEN
%
%extern bool_t xdr_uint32_t (XDR* x, uint32_t* ii);
%extern bool_t xdr_uint64_t (XDR* x, uint64_t* ii);
%
%#endif

#endif

#ifdef RPC_XDR

%#ifdef BROKEN_DARWIN_RPCGEN
%
%bool_t xdr_uint32_t (XDR* x, uint32_t* ii) {return xdr_u_int32_t(x,ii);}
%bool_t xdr_uint64_t (XDR* x, uint64_t* ii) {return xdr_u_int64_t(x,ii);}
%
%#endif

#endif


/* ------------------ GENERIC TYPES ------------------- */

/**
 * @ingroup return_codes
 *
 * @brief Return Codes.
 *
 * The <tt>\ref lwfs_return_code</tt> enumerator defines return codes used
 * by the LWFS-core interfaces.
 */
enum lwfs_return_code {
	/** @brief The function completed successfully.  */
	LWFS_OK=0,

	/** @brief Unspecified error. */
	LWFS_ERR,

	/** @brief The function timed out. */
	LWFS_ERR_TIMEDOUT,

	/** @brief The function is not supported. */
	LWFS_ERR_NOTSUPP,

	/** @brief Out of memory.  */
	LWFS_ERR_NOSPACE,

	/*-------- STORAGE SERVICE RETURN CODES ---------- */

	/**
	 * @brief Unspecified error in the storage service.
	 */
	LWFS_ERR_STORAGE,

	/**
	 * @brief The object does not exist.
	 */
	LWFS_ERR_NO_OBJ,

	/*-------- NAMING SERVICE RETURN CODES ---------- */

	/**
	 * @brief Unspecified error in the naming service.
	 */
	LWFS_ERR_NAMING,

	/**
	 * @brief The operation was not allowed because
	 * the caller is either not a privileged user
	 * or not the owner of the target of the operation.
	 */
	LWFS_ERR_PERM,

	/**
	 * @brief The specified entry does not exist.
	 */
	LWFS_ERR_NOENT,


	/**
	 * @brief  The caller does not have the correct permission
	 * to perform the requested operation.
	 * Contrast this with LWFS_ERR_PERM, which restricts itself
	 * to owner or privileged user permission failures.
	 */
	LWFS_ERR_ACCESS,

	/**
	 * @brief The specified object already exists.
	 */
	LWFS_ERR_EXIST,

	/**
	 * @brief The caller specified a non-directory
	 * in a directory operation.
	 */
	LWFS_ERR_NOTDIR,

	/**
	 * @brief The caller specified a non-file
	 * in a file operation.
	 */
	LWFS_ERR_NOTFILE,

	/**
	 * @brief The caller specified a non-namespace
	 * in a namespace operation.
	 */
	LWFS_ERR_NOTNS,

	/**
	 * @brief The caller specified a directory in
	 * a non-directory operation.
	 */
	LWFS_ERR_ISDIR,

	/* ----------- RPC ERROR CODES -----------*/

	/** @brief Unspecified RPC error. */
	LWFS_ERR_RPC,

	/** @brief Error encoding the data. */
	LWFS_ERR_ENCODE,

	/** @brief Error decoding the data. */
	LWFS_ERR_DECODE,

	/* ----------- SECURITY ERROR CODES ----------- */
	/** @brief Unspecified security error. */
	LWFS_ERR_SEC,

	/** @brief The function could not generate a cryptographic key. */
	LWFS_ERR_GENKEY,

	/** @brief The function could not verify a credential. */
	LWFS_ERR_VERIFYCRED,

	/** @brief The function could not verify a capability. */
	LWFS_ERR_VERIFYCAP,

	/* ----------- Lock error codes ---- */

	/** @brief The entry is not locked. */
	LWFS_ERR_LOCK_EXISTS,

	/** @brief The entry is not locked. */
	LWFS_ERR_NO_LOCK,

	/* ----------- Transaction error codes ---- */

	/** @brief The transaction is invalid. */
	LWFS_ERR_TXN

};



/**
 * @brief Number of bytes in an lwfs_uuid.
 */
const LWFS_UUIDSIZE = 16;

/**
 * @ingroup base_types
 *
 * @brief Boolean type.
 *
 * The <tt>\ref lwfs_bool</tt> type represents boolean variables.
 */
typedef int lwfs_bool;

/**
 * @ingroup base_types
 *
 * @brief Size type.
 *
 * The <tt>\ref lwfs_size</tt> type is used to for ``size'' variables.
 */
typedef uint64_t lwfs_size;
typedef uint64_t lwfs_ssize;


/**
 * @brief The <tt>\ref lwfs_thread</tt> type is an identifier
 *        for threads (used by the service structure).
 */
typedef uint64_t lwfs_thread;

/**
 * @ingroup base_types
 * @brief A data structure for time.
 *
 * The <tt>\ref lwfs_time</tt> structure is used to
 * represent a particular moment in time.
 */
struct lwfs_time {
	/** @brief Seconds. */
	uint32_t seconds;
	/** @brief Nanoseconds. */
	uint32_t nseconds;
};

/**
 * @ingroup base_types
 * @brief A definition for globally unique IDs.
 *
 * The uuid type is a fixed-sized array of \ref LWFS_UUIDSIZE bytes.
 */
typedef opaque lwfs_uuid[LWFS_UUIDSIZE];

/**
 * @ingroup authn_api
 * @brief A definition for user IDs.
 *
 * The <tt>\ref lwfs_uid</tt> type represents a user ID.
 */
typedef lwfs_uuid lwfs_uid;


/**
 * @ingroup authn_api
 * @brief A structure for a list of lwfs_uids.
 *
 * The <tt>\ref lwfs_uid_list</tt> structure represents a list of user IDs.
 */
struct lwfs_uid_list {
	lwfs_uid uid;
	lwfs_uid_list *next;
};

/**
 * @brief A structure for an array (max=2**32) of user ids.
 */
typedef lwfs_uid lwfs_uid_array<>;

/**
 * @brief Type definition for a pointer to a list.
 */
typedef lwfs_uid_list *lwfs_uid_list_ptr;


/**
 * @brief The opstring should be less than 15 chars.
 */
const LWFS_OP_LEN = 16;

/**
 * @brief A definition for operation strings.
 */
typedef opaque lwfs_opstr[LWFS_OP_LEN];


const NUM_CONTAINER_OPS = 6;

/**
 * @brief Operation codes for operations on containers.
 */
enum lwfs_container_op {

	/** @brief Create a container */
	LWFS_CONTAINER_CREATE = 1,

	/** @brief Modify an ACL of a container */
	LWFS_CONTAINER_MODACL = 2,

	/** @brief Remove a container */
	LWFS_CONTAINER_REMOVE = 4,

	/** @brief Read from the objects in a container. */
	LWFS_CONTAINER_READ = 8,

	/** @brief Write to the objects in a container. */
	LWFS_CONTAINER_WRITE = 16,

	/** @brief Write to the objects in a container. */
	LWFS_CONTAINER_EXEC = 32
};


/**
 * @ingroup authr_api
 * @brief A type for cid opcodes
 *
 * An <tt>\ref lwfs_opcode</tt> is an integer value used
 * to identify the operation on a particular container.
 */
typedef uint32_t lwfs_opcode;

const LWFS_OP_NULL = 0;


/**
 * @brief Length of an lwfs_name.
 *
 * We decided a resonable length for names is 64 characters.
 */
const LWFS_NAME_LEN = 64;

/**
 * @brief Definition for the name type.
 *
 * We define a "name" as a character string with a
 * max length of \ref LWFS_NAME_LEN.
 */
typedef string lwfs_name<LWFS_NAME_LEN>;

/**
 * @brief An array of lwfs_name
 */
typedef lwfs_name lwfs_name_array<>;


/**
 * @brief A structure for an access-control list.
 */
struct lwfs_acl {
	lwfs_opcode opcode;
	lwfs_uid_list *users;
};


/**
 * @brief A structure for an opcode list entry.
 */
struct lwfs_opcode_list {
	lwfs_opcode opcode;
	lwfs_opcode_list *next;
};

/**
 * @brief A structure for an opcode array.
 */
typedef lwfs_opcode lwfs_opcode_array<>;

/**
 * @brief Type definition for a pointer to a list.
 */
typedef lwfs_opcode_list *lwfs_opcode_list_ptr;


/**
 * @ingroup authr_api
 * @brief A definition for container IDs.
 *
 * The <tt>\ref lwfs_cid</tt> type identifies a unique
 * ``container'' in an LWFS storage system.
 */
typedef uint64_t lwfs_cid;

/**
 * @brief A constant that represents any container ID.
 */
const LWFS_CID_ANY = -1;


/**
 * @brief Information about an object.

 * The <tt>\ref lwfs_stat_data</tt> structure contains
 * information about an LWFS object. It is acquired
 * by calling the \ref lwfs_stat "lwfs_stat()" function.
 */
struct lwfs_stat_data {

	/** @brief The last accessed time for the object. */
	lwfs_time atime;

	/** @brief The last modified time for the object. */
	lwfs_time mtime;

	/** @brief The last status change time for the object. */
	lwfs_time ctime;

	/** @brief The size of the object. */
	lwfs_size size;
};




/**
 * @ingroup ss_api
 * @brief Definition for a storage server object ID.
 *
 * The <tt>\ref lwfs_oid</tt> type identifies a unique
 * \ref lwfs_obj "object" in an LWFS storage server.
 */
typedef lwfs_uuid lwfs_oid;


/**
 * @brief Definition for a storage server volume ID.
 */
typedef uint32_t lwfs_vid;


/**
 * @ingroup rpc_client_api
 * @brief Definition for a network node ID (needed for process IDs).
 *
 * The <tt>\ref lwfs_nid</tt> type identifies a remote node (i.e., a processor).
 */
typedef uint32_t lwfs_nid;

/**
 * @ingroup rpc_client_api
 * @brief Definition for a process ID (needed for process IDs).
 *
 * The <tt>\ref lwfs_pid</tt> type identifies a process.
 */
typedef uint32_t lwfs_pid;

/**
 * @ingroup rpc_client_api
 * @brief A structure to represent remote processes.
 *
 * The <tt>lwfs_remote_pid</tt> structure contains the
 * <tt>\ref lwfs_nid</tt> and <tt>\ref lwfs_pid</tt> pair needed
 * to identify a process running on a remote node.
 */
struct lwfs_remote_pid {
	/** @brief Node ID. */
	lwfs_nid nid;
	/** @brief Process identifier. */
	lwfs_pid pid;
};


/**
 * @ingroup lock_api
 * @brief Definition for lock IDs.
 */
typedef uint32_t lwfs_lock_id;

/**
 * @ingroup lock_api
 * @brief LWFS lock types.
 *
 * The <tt>\ref lwfs_lock_type</tt> enumerator identifies and assigns values
 * to the different types of supported LWFS locks.  We use a
 * <em>``readers/writers''</em> scheme to synchronize access to storage
 * server objects in a critical section of a transaction.  Either one writer
 * (a client that changes the state of an object) or many readers (do not change
 * state of an object) are allowed in the critical section at a time.
 */
enum lwfs_lock_type {
	/**
	 * A Null lock. This type of lock may be used when a lock type parameter
	 * is required, but the client does not want to request a lock. For example,
	 * in the \ref lwfs_lookup method, the client may not want to lock the
	 * parent directory before looking for named entry.
	 */
	LWFS_LOCK_NULL = 0,

	/**
	 * @brief Read lock. Multiple clients may hold a read lock.
	 */
	LWFS_READ_LOCK,

	/**
	 * @brief Write lock.
	 * More than one client may hold a shared lock,
	 * allowing concurrent access to the object or file entry.
	 */
	LWFS_WRITE_LOCK
};

/**
 * @ingroup rpc_client_api
 * @brief Definition for a match bits used in the
 * <tt>\ref lwfs_rma</tt> data structure.
 */
typedef uint64_t lwfs_match_bits;

/**
 * @ingroup rpc_client_api
 * @brief Definition for a remote buffer id.
 */
typedef uint32_t lwfs_buffer_id;


/**
 * @ingroup rpc_client_api
 * @brief LWFS Remote Memory Address
 *
 * The <tt>\ref lwfs_rma</tt> data structure contains
 * the information need to access remote memory using
 * a one-sided data-movement model.  In particular,
 * a one-sided model requires (at minimum) a process
 * ID, a memory buffer ID, and an offset.  We also include
 * the match bits and index required by the Portals
 * message passing interface.
 */
struct lwfs_rma {
	/** @brief The ID of the remote process. */
	lwfs_remote_pid match_id;

	/** @brief The memory buffer ID. */
	lwfs_buffer_id buffer_id;

	/** @brief The offset into the remote buffer. */
	lwfs_size offset;

	/** @brief The match bits (required by Portals). */
	lwfs_match_bits match_bits;

	/** @brief The length of the remote buffer. */
	lwfs_size len;

	/** @brief A local buffer pointer (NULL if not used). */
	uint64_t local_buf;
};


/**
 * @brief The size of a service's thread pool.
 */
const MAX_SVC_THREADS = 1;

/**
 * @brief Enumerator for the different type of transport mechanisms.
 *
 * The <tt>\ref lwfs_rpc_transport</tt> enumerator provides integer values
 * to represent the different types of supported transport mechanisms.
 * Initially, the only supported transport mechanism is Portals.
 */
enum lwfs_rpc_transport {
	/** @brief Use Portals to transfer rpc requests. */
    LWFS_RPC_PTL,

	/** @brief Use a local buffer (not a remote operation). */
	LWFS_RPC_LOCAL
};

const LWFS_TRANSPORT_DEFAULT = LWFS_RPC_PTL;

/**
 * @brief Enumerator for the different type of encoding mechanisms.
 *
 * The <tt>\ref lwfs_rpc_encode</tt> enumerator provides integer values
 * to represent the different types of supported mechanisms for encoding
 * control messages transferred to/from LWFS servers.
 * Initially, the only supported transport mechanism is XDR.
 */
enum lwfs_rpc_encode {
	/** @brief Use XDR to encode/decode rpc requests and results. */
    LWFS_RPC_XDR
};

const LWFS_ENCODE_DEFAULT = LWFS_RPC_XDR;


/**
 * @brief A descriptor for remote services.
 *
 * The <tt>\ref lwfs_service</tt> data
 * structure contains information needed by the client
 * to send operation requests to a remote service, including the process ID
 * of the remote host, the encoding mechanism to use for control messages,
 * and the memory location reserved for incoming requests.
 *
 * There are two ways for a client to obtain the
 * <tt>\ref lwfs_service</tt> structure for a particular service:
 * it can get the service descriptor from a known service registry
 * by calling the <tt>\ref lwfs_lookup_service "lwfs_lookup_service()"</tt>
 * function @latexonly described in Section~\ref{func:lwfs_lookup_service}@endlatexonly
 * , or it can get the descriptor directly from the server by calling the
 * <tt>\ref lwfs_get_service "lwfs_get_service()"</tt> function
 * @latexonly described in Section~\ref{func:lwfs_get_service}@endlatexonly .
 */
struct lwfs_service {

	/** @brief Identifies the mechanism to use for encoding messages. */
	lwfs_rpc_encode rpc_encode;

	/** @brief The remote memory address reserved for incoming requests. */
	lwfs_rma req_addr;

	/** @brief The maximum number of requests to process. */
	int max_reqs;

	/** @brief A thread ID for the request processing thread (used only on the server). */
	lwfs_thread req_thread;

	/** @brief The pool of threads used by the service (used only by the server). */
	lwfs_thread thread_pool[MAX_SVC_THREADS];
};

/**
 * @brief A structure used to reference a storage server object.
 *
 * The <tt>\ref lwfs_obj</tt> structure contains the information
 * required by an LWFS client to access a remote ``blob'' of
 * bytes stored on an LWFS storage server.  At minimum, this structure
 * needs to be able to identify the host storage server and the local
 * id of the object. Additional fields such as the type of object,
 * the ID of the container the object belongs in, and a lock ID for
 * synchronizing access to the object are also provided for convenience.
 */
struct lwfs_obj {

	/** @brief The service descriptor for the storage server hosting the object. */
	lwfs_service svc;

	/** @brief The user-defined type of object. */
	int type;

	/** @brief The container ID of the object (used for access control). */
	lwfs_cid cid;

	/** @brief The object ID (unique on the storage server). */
	lwfs_oid oid;

	/** @brief A lock ID (may be used to synchronize access to the obj). */
	lwfs_lock_id lock_id;
};


/**
 * @brief Definition for transaction.
 *
 * The <tt>\ref lwfs_txn</tt> data structure contains a
 * reference to a remote storage object that contains
 * information about pending operations in a distributed
 * transaction.
 */
struct lwfs_txn {
	/** @brief The remote storage object for the journal. */
	lwfs_obj journal;
};



/* -------------- LWFS NAMING SERVICE TYPES ------------ */


struct lwfs_distributed_obj {
	int chunk_size;
	int ss_obj_count;	/* number of objs the file is distributed across */
	lwfs_obj *ss_obj;	/* array of objs the file is distributed across */
};

/**
 * @brief A structure for entries in the namespace.
 *
 */
struct lwfs_ns_entry {

	/** @brief The name of the object in the namespace. */
	char name[LWFS_NAME_LEN];

	/** @brief The unique id of this dirent */
	lwfs_oid dirent_oid;

	/** @brief The oid of the inode the dirent names. */
	lwfs_oid inode_oid;

	/** @brief The oid for the parent.
	 */
	lwfs_oid parent_oid;

	/** @brief The number of links to this entry.  */
	int link_cnt;

	/** @brief The object reference used to access the entry.
	 *
	 *  For files and links, this points to an object on a
	 *  storage server.
	 */
	lwfs_obj entry_obj;

	/**
	 *  @brief The file object.
	 *
	 *  A file object is a storage server object that
	 *  the file system (that uses the LWFS namespace)
	 *  uses to represent metadata or data about the file.
	 *  For directories, this is set to NULL.
	 */
	lwfs_obj *file_obj;
	
	/** Keeps track of the lwfs_obj's composing a distributed obj */
	lwfs_distributed_obj *d_obj;
};


/**
 * @brief Definition for an array of namespace entries.
 */
typedef lwfs_ns_entry lwfs_ns_entry_array<>;


/**
 * @brief A structure to represent a namespace.
 *
 */
struct lwfs_namespace {

	/** @brief The name of the namespace. */
	char name[LWFS_NAME_LEN];

	/** @brief The object reference used to access the namespace.
	 *
	 */
	lwfs_ns_entry ns_entry;
};


/**
 * @brief Definition for an array of namespaces.
 */
typedef lwfs_namespace lwfs_namespace_array<>;


/**
 * @brief Definition for object types.
 */
enum lwfs_entry_type {
	/** @brief Generic object. */
	LWFS_GENERIC_OBJ = 0,

	/** @brief File entry in the namespace. */
	LWFS_FILE_ENTRY,

	/** @brief Directory entry. */
	LWFS_DIR_ENTRY,

	/** @brief Link entry. */
	LWFS_LINK_ENTRY,

	/** @brief File object. */
	LWFS_FILE_OBJ,

	/** @brief File segment object. */
	LWFS_SEG_OBJ,

	/** @brief namespace object. */
	LWFS_NS_OBJ
};


enum lwfs_pt_indices {
	LWFS_REQ_PT_INDEX = 1,   /* index to send requests */
	LWFS_RES_PT_INDEX,       /* where to send results */
	LWFS_DATA_PT_INDEX,      /* where to put/get data */
	LWFS_LONG_ARGS_PT_INDEX, /* where to fetch long args */
	LWFS_LONG_RES_PT_INDEX   /* where to fetch long results */
};

/* Minimum request size is sizeof(lwfs_request_header) == */
const LWFS_SHORT_REQUEST_SIZE = 512;
const LWFS_SHORT_RESULT_SIZE = 512;

const LWFS_SS_PID = 122;
const LWFS_SS_MATCH_BITS = 0;

const LWFS_AUTHR_PID = 124;
const LWFS_AUTHR_MATCH_BITS = 0;

const LWFS_NAMING_PID = 126;
const LWFS_NAMING_MATCH_BITS = 0;


/**
 * @brief The request header.
 *
 * The lwfs_request_header structure contains details needed by an
 * LWFS server to perform a remote operation.
 *
 * The operation arguments may or may not be sent to the server
 * in the original request. If the field \em fetch_args is
 * \em true, the server will fetch arguments from \em args_addr
 * in a separate operation. Otherwise, the client sends the
 * arguments to the server with the request.
 */
struct lwfs_request_header {
	/** @brief ID of the request */
	unsigned long id;

	/** @brief ID of the operation to perform. */
	lwfs_opcode opcode;

	/** @brief A flag that tells the server to fetch args from
      *        <em>\ref args_addr</em>. */
	lwfs_bool fetch_args;

	/** @brief The remote memory address reserved for
      *        long arguments. */
	lwfs_rma args_addr;

	/** @brief The remote memory address reserved for bulk
      *        data transfers. */
	lwfs_rma data_addr;

	/** @brief The remote memory address reserved for the short result. */
	lwfs_rma res_addr;
};


/**
 * @brief The result header.
 *
 * The mds_request_header structure contains the details needed by the
 * MDS server to perform a remote operation.
 *
 * The arguments may or may not be sent to the server in the original request.
 * If the field \em fetchargs is \em true, the arguments will be fetched
 * from \em args_portal in a separate operation by the server. Otherwise,
 * the arguments are sent directly to the server.
 *
 * NO POINTERS ALLOWED IN THIS STRUCTURE!
 *
 */
struct lwfs_result_header {
	/** @brief ID of the result (should be same as request) */
	unsigned long id;

	/** @brief A flag that tells the client to "get" result from the client. */
	lwfs_bool fetch_result;

	/** @brief The remote memory address reserved for long results. */
	lwfs_rma result_addr;

	/** @brief The return code of the function. */
	int rc;
};


/**
 * @brief Size of a cryptographic key.
 *
 * The choice to use a 128-bit key was somewhat arbitrary. We
 * expect the keys to last a long time (days to months), so
 * we need one long enough to be sufficiently difficult to guess;
 * however, a large key will increase the time to generate and
 * verify credentials and capabilities.
 */
const LWFS_KEYSIZE = 16;

/**
 * @brief The LWFS cryptographic key.
 *
 * Our cryptographic key is a fixed-length array
 * of randomly generated bytes.
 */
typedef opaque lwfs_key[LWFS_KEYSIZE];

/**
 * @brief Size of a message authentication code
 *
 * We chose a 160-bit hash to accommodate the most popular
 * implementations of one-way hash functions. Most require
 * 128 bits (e.g., MD2, MD3, MD5, SNEFRU, N-HASH, RIPE-MD),
 * but the SHA (from the NSA) requires 160 bits.
 */
const LWFS_MACSIZE = 20;

/**
 * @ingroup authr_api
 * @brief The LWFS message authentication code.
 *
 * The <tt>\ref lwfs_mac</tt> structure is a fixed-length array of
 * bytes used to store an encrypted one-way hash--also known
 * as a message authentication code.
 */
typedef opaque lwfs_mac[LWFS_MACSIZE];


/**
 * @brief Data for the user credential.
 *
 * The <tt>\ref lwfs_credential_data</tt> structure contains
 * the information needed by the implementation to verify
 * the user ID as well as information about how to verify the
 * authenticity of the user ID (i.e., how to prove that identity
 * of the user).
 */
struct lwfs_cred_data {
	lwfs_uid uid;
};

/**
 * @brief A structure for a credential.
 *
 * The <tt>\ref lwfs_cred</tt> structure represents the
 * identity of a previously authenticated user. The data
 * portion contains information needed to identify the user
 * and how to authenticate the user. The MAC represents
 * verifiable proof of the integrity of the credential.
 * The contents of the data portion and the MAC
 * are implementation-specific and defined in a separate file.
 */
struct lwfs_cred {
	/** @brief Contains implementation-specific data needed to
	*          authenticate the user. */
	lwfs_cred_data data;
	/** @brief A cryptographically secure (implementation-specific)
	*          structure used to verify the credential. */
	lwfs_mac mac;
};




/**
 * @brief A structure that holds the data portion of a capability.
 *
 * The data portion of a capability specifies a container ID,
 * a \ref container_op that identifies the operation(s) that the
 * capablity enables, and the credential of the user that was
 * granted the capability.  We need all of this information so the
 * system can automatically refresh the capability if it expires
 * while the application is still running.
 */
struct lwfs_cap_data {

	/** @brief The container ID. */
	lwfs_cid cid;

	/** @brief The container operations enabled. */
	lwfs_container_op container_op;

	/** @brief The credential of the requesting user. */
	lwfs_cred cred;
};


/**
 * @brief A structure for a capability.
 *
 * A capability is a data structure that enables the holder to
 * perform a set of operations on a container. The capability
 * data structure consists of two parts: the data and the MAC.
 * The data contains the container ID, the operations to enable, and
 * the credential of the user that was granted the capability.  The
 * MAC holds a set of "random" bits generated (and later verified)
 * by the entity that created the capability.  The LWFS uses the MAC
 * to guarantee the integrity of the data portion of the capability.
 *
 */
struct lwfs_cap {
	/** @brief Important information about the capability. */
	lwfs_cap_data data;
	/** @brief The message authentication code used to verify the capability. */
	lwfs_mac mac;
};


/**
 * @brief A structure for an array (max=2**32) of caps.
 */
typedef lwfs_cap lwfs_cap_array<>;

/**
 * @brief A structure for a list of capabilities.
 */
struct lwfs_cap_list {
	lwfs_cap cap;
	lwfs_cap_list *next;
};

/**
 * @brief Type definition for a pointer to a list.
 */
typedef lwfs_cap_list *lwfs_cap_list_ptr;


/* -------------- LWFS EXTENDED ATTRIBUTE TYPES ------------ */


 
/**
 * @brief The maximum size of an extended attribute is 8KB.
 */
const LWFS_ATTR_SIZE = 8192;

/**
 * @brief The extended attribute is a variable-sized blob of data.
 */
typedef opaque lwfs_attr_data<LWFS_ATTR_SIZE>;

/**
 * @brief A structure for an extended attribute
 */
struct lwfs_attr {
	lwfs_name name;
	lwfs_attr_data value;
};

/**
 * @brief An array of lwfs_attr
 */
typedef lwfs_attr lwfs_attr_array<>;