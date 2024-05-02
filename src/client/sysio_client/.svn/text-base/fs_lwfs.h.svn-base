
/*
 * LWFS file system driver support.
 */

#include "common/types/types.h"
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

	/**
	 * @brief Structures required for an LWFS
	 * file system implementation. 
	 */
	typedef struct {

		/** @brief The authorization service. */
		lwfs_service authr_svc;

		/** @brief The naming service. */
		lwfs_service naming_svc; 

		/** @brief The number of storage servers. */
		int num_servers; 

		/** @brief The storage service. */
		lwfs_service *storage_svc;
		
		/** @brief The default chunk size written to a storage server */
		int default_chunk_size;
		
		/** @brief The number of fake i/o patterns */
		int num_fake_io_patterns;
		
		/** @brief The fake i/o pattern list */
		char **fake_io_patterns;
		
		/** @brief namespace this fs resides in */
		lwfs_namespace namespace;

		/** @brief credentials for this fs */
		lwfs_cred cred;

		/** @brief caps used for access to this fs */
		lwfs_cap cap; 
		lwfs_cap modacl_cap; 

		/** @brief one container used for all objects */
		lwfs_cid cid; 

		/** @brief single txn used for all methods */
		lwfs_txn txn; 

		/** @brief attr timeout (sec) */
		time_t atimo;
	} lwfs_filesystem;

#if defined(__STDC__) || defined(__cplusplus)

	extern int _sysio_lwfs_init(void);

#else /* K&R C */

#endif

#ifdef __cplusplus
}
#endif


