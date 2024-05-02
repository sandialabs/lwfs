/**
*/
#include "config.h"

#include <time.h>

#if STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif

/*#include "portals3.h"*/

#include "client/rpc_client/rpc_client.h"
#include "common/types/types.h"
#include "common/types/fprint_types.h"
#include "common/rpc_common/rpc_debug.h"
#include "common/rpc_common/lwfs_ptls.h"
#include "common/rpc_common/rpc_common.h"

/* ----------------- COMMAND-LINE OPTIONS --------------- */


int
main (int argc, char *argv[])
{

	lwfs_remote_pid myid; 

	/* initialize LWFS RPC */
	lwfs_ptl_init(PTL_IFACE_SERVER, PTL_PID_ANY);
	lwfs_rpc_init(LWFS_RPC_PTL, LWFS_RPC_XDR);
	
	lwfs_get_id(&myid);

	printf("%llu\n", (unsigned long long)myid.nid);

	lwfs_rpc_fini();

	return 0; 
}
