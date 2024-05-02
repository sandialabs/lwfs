/* -------------------------------------------------------------------------- */
/**  
 *   @file simple_svc_xdr.x
 *   
 *   @brief Type definitions for a simple rpc service. 
 *
 *   @author Ron Oldfield (raoldfi\@cs.sandia.gov).
 *   $Revision: 1073 $.
 *   $Date: 2007-01-22 22:06:53 -0700 (Mon, 22 Jan 2007) $.
 *
 */

/* Include files for the mds_xdr.h file (ignored by others) */
#ifdef RPC_HDR
#endif 

struct data_t {
	int int_val;
	float float_val;
	double double_val; 
};

typedef data_t data_array_t<>;

typedef opaque buf_t[16]; 

typedef struct buf_t buf_array_t<>; 

program XFER_PROG {
	version XFER_VERS {
		data_t data_xfer(data_array_t) = 1;
		buf_t buf_xfer(buf_array_t) = 2;
	} = 1; 
} = 0x23451111; 

