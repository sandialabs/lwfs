#!/bin/sh -x

export PTL_IFACE=eth0; 
export PTL_MY_RID=1; 
export PTL_NIDMAP=2264789557:`utcp_nid eth0`; 
export PTL_PIDMAP=98:99; 

# bandwidth tests
#get_bw_utcp
#put_bw_utcp
put_pp_utcp
#valgrind --tool=memcheck --leak-check=yes put_bw_utcp
