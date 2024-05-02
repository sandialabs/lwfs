
#include <portals/p30.h>
#include <gssapi/gssapi_generic.h>
#include "gss-misc-ptls.h"
#include "lwfs/logger.h"
#include "lwfs_ptls.h"
#include "gss_xdr.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#else
extern char *malloc();
#endif

#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

FILE *display_file;
static log_level debug_level = LOG_DEBUG; 

gss_buffer_desc empty_token_buf = { 0, (void *) "" };
gss_buffer_t empty_token = &empty_token_buf;

static void display_status_1
	(char *m, OM_uint32 code, int type);

/**
 * @brief Post a receive request to a portal.  
 *
 * The application calls this method when it expects a remote 
 * process to "send" us data in the near future. 
 *
 * @param index      The index of the local portal for this buffer.
 * @param buf        The memory for the buffer.
 * @param length     The size of the buffer.
 * @param match_id   Process ID's that we receive data from.
 * @param match_bits Bits that must match for data to be received. 
 * @param remote_md  Used by remote processes to references this buffer.
 * @param local_md   Used by the local process to check for completion of request.
 */
lwfs_return_code post_recv(uint32 ptl_index,
		void *buf, 
		uint32 length,
		lwfs_process_id match_id,
		uint32 match_bits,
		lwfs_remote_md *remote_md,
		lwfs_local_md *local_md)
{
	lwfs_return_code rc;  /* return code */

	ptl_process_id_t rcv_match_id = {
		match_id.nid,
		match_id.pid
	}; 

	ptl_match_bits_t rcv_match_bits = (ptl_match_bits_t)match_bits;

	rc = lwfs_ptl_post_md(
			(ptl_pt_index_t)ptl_index, 
			buf, 
			length, 
			PTL_MD_OP_PUT,
			rcv_match_id,
			rcv_match_bits, 
			remote_md,
			local_md);
	if (rc != LWFS_OK) {
		log_fatal(debug_level, "post_recv: could not post memory descriptor\n");
		return rc; 
	}

	return rc; 
}

/**
 * @brief Wait for the next request.
 */
int recv_request(lwfs_local_md *local_md,
		gss_request *req) 
{
	lwfs_return_code rc; 

	log_debug(debug_level, "recv_request: waiting for request...\n");

	rc  = lwfs_ptl_wait(local_md); 
	if (rc != LWFS_OK) {
		log_error(debug_level, "recv_request: failed waiting for request\n");
		return rc; 
	}

	/* in this case, the application should have passed *req to the 
	 * post_recv method. So local_md->buf == req. 
	 */
	if (local_md->ptl_md.start != req) {
		log_error(debug_level, "recv_request: unexpected req buffer\n");
		return LWFS_ERR_COMM; 
	}

	return LWFS_OK; 
}

/**
 * @brief Send a request to the server.
 */
int send_request( lwfs_remote_md *dest_md, gss_request *req)
{
	int rc; 

	log_debug(debug_level, "send_request: sending request to "
			"(dest=%llu, index=%d, match_bits=%d)\n",
			dest_md->id.nid, 
			dest_md->index, 
			dest_md->match_bits);
	rc = lwfs_ptl_put(req, sizeof(gss_request), dest_md); 
	if (rc != LWFS_OK) {
		log_error(debug_level, "send request: unable to put request "
				"to remote portal\n");
	}

	return rc; 
}

/**
 * @brief Send a token to a remote memory descriptor.
 *
 * Send_token writes the token flags (a single byte, even though
 * they're passed in in an integer). Next, the token length (as 
 * a network long) and then the token data are written to the 
 * file descriptor s.  It returns 0 on success, and -1
 * if an error occurs or if it could not write all the data.
 *
 * @param dest_md  The destination memory descriptor.
 * @param flags    The token flags.
 * @param tok      The token to send.
 */
int send_token( 
		lwfs_remote_md *dest_md, 
		int flags, 
		gss_buffer_desc *tok)
{
     int rc;
     void *encoded_buf; 
     lwfs_token send_tok; 
     XDR xdrs; 

     /* convert gss_token into an lwfs_token */
     send_tok.buf.buf_len = tok->length; 
     send_tok.buf.buf_val = tok->value; 
     send_tok.flags = flags; 

     int size = xdr_sizeof((xdrproc_t)&xdr_lwfs_token, &send_tok); 

     /* allocate memory */
     encoded_buf = malloc(size); 

     /* encode the token */
     xdrmem_create(&xdrs, encoded_buf, size, XDR_ENCODE); 
     if (! xdr_lwfs_token(&xdrs, &send_tok)) {
	     log_error(debug_level, "send_token: failed to encode token\n");
	     return LWFS_ERR_ENCODE; 
     }

     /* send the encoded token to the remote memory descriptor */
     rc = lwfs_ptl_put(encoded_buf, size, dest_md); 

     /* free the memory */
     free(encoded_buf);

     return rc;
}

/*
 * Function: recv_token
 *
 * Purpose: Reads a token from a file descriptor.
 *
 * Arguments:
 *
 * 	s		(r) an open file descriptor
 *	flags		(w) the read flags
 * 	tok		(w) the read token
 *
 * Returns: 0 on success, -1 on failure
 *
 * Effects:
 * 
 * recv_token reads the token flags (a single byte, even though
 * they're stored into an integer, then reads the token length (as a
 * network long), allocates memory to hold the data, and then reads
 * the token data from the file descriptor s.  It blocks to read the
 * length and data, if necessary.  On a successful return, the token
 * should be freed with gss_release_buffer.  It returns 0 on success,
 * and -1 if an error occurs or if it could not read all the data.
 */
int recv_token(
		lwfs_local_md *local_md,
		int *flags, 
		gss_buffer_desc *tok)
{
     int rc;
     lwfs_token recv_token; 
     XDR xdrs; 

     /* wait for the token buffer  */
     rc = lwfs_ptl_wait(local_md);
     if (rc != LWFS_OK) {
	     log_error(debug_level, "recv_token: failed waiting on token\n");
	     return rc; 
     }

     /* initialize the lwfs_token */
     memset(&recv_token, 0, sizeof(lwfs_token)); 

     /* create a memory stream that points to the local memory descriptor */
     xdrmem_create(&xdrs, 
		     local_md->ptl_md.start, 
		     local_md->ptl_md.length, 
		     XDR_DECODE);
     
     /* decode the result */
     if (! xdr_lwfs_token(&xdrs, &recv_token)) {
	     log_error(debug_level, "recv_token: could not decode token\n");
	     return LWFS_ERR_DECODE;
     }

     /* convert the lwfs_token to the gss_token */
     tok->length = recv_token.buf.buf_len; 
     tok->value  = recv_token.buf.buf_val; 
     *flags = recv_token.flags; 

     return LWFS_OK;
}



/* 
 * inquire context 
 */
int inquire_context(gss_ctx_id_t context, log_level level)
{
	 int i;
	 gss_name_t src_name, targ_name; 
	 OM_uint32 lifetime; 
	 gss_OID mechanism, name_type;
	 OM_uint32 context_flags; 
	 gss_OID_set mech_names; 
	 int is_open, is_local;
	 OM_uint32 maj_stat, min_stat; 
	 gss_buffer_desc sname, tname; 
	 gss_buffer_desc oid_name;

	 FILE *fp = logger_get_file(); 
	 
	 fprintf(fp, "\n"); 

	 if (level >= LOG_DEBUG){ 
		 log_debug(debug_level, "------------ Begin Context Information ---------\n");

	 
		 /* Get context information */
	 
		 maj_stat = gss_inquire_context(&min_stat, context,
					&src_name, &targ_name, &lifetime,
					&mechanism, &context_flags,
					&is_local,
					&is_open);
	 
		 if (maj_stat != GSS_S_COMPLETE) {
			 display_status("inquiring context", maj_stat, min_stat);
			 return -1;
		 }

		 maj_stat = gss_display_name(&min_stat, src_name, &sname,
				     &name_type);
	 	
		 if (maj_stat != GSS_S_COMPLETE) {
			 display_status("displaying source name", maj_stat, min_stat);
			 return LWFS_ERR_CONTEXT;
		 }
	 
		 maj_stat = gss_display_name(&min_stat, targ_name, &tname,
				 (gss_OID *) NULL);
		 if (maj_stat != GSS_S_COMPLETE) {
			 display_status("displaying target name", maj_stat, min_stat);
			 return LWFS_ERR_CONTEXT;
		 }
		 fprintf(fp, "\"%.*s\" to \"%.*s\", lifetime %d, flags %x, %s, %s\n",
				 (int) sname.length, (char *) sname.value,
				 (int) tname.length, (char *) tname.value, lifetime,
				 context_flags,
				 (is_local) ? "locally initiated" : "remotely initiated",
				 (is_open) ? "open" : "closed");

		 (void) gss_release_name(&min_stat, &src_name);
		 (void) gss_release_name(&min_stat, &targ_name);
		 (void) gss_release_buffer(&min_stat, &sname);
		 (void) gss_release_buffer(&min_stat, &tname);

	 
		 maj_stat = gss_oid_to_str(&min_stat,
				 name_type,
				 &oid_name);
		 if (maj_stat != GSS_S_COMPLETE) {
			 display_status("converting oid->string", maj_stat, min_stat);
			 return LWFS_ERR_CONTEXT;
		 }
		 fprintf(fp, "Name type of source name is %.*s.\n",
				 (int) oid_name.length, (char *) oid_name.value);

		 (void) gss_release_buffer(&min_stat, &oid_name);

	 
		 /* Now get the names supported by the mechanism */
	 
		 maj_stat = gss_inquire_names_for_mech(&min_stat,
				 mechanism,
				 &mech_names);
		 if (maj_stat != GSS_S_COMPLETE) {
			 display_status("inquiring mech names", maj_stat, min_stat);
			 return LWFS_ERR_CONTEXT;
		 }

		 maj_stat = gss_oid_to_str(&min_stat,
				 mechanism,
				 &oid_name);
	 
		 if (maj_stat != GSS_S_COMPLETE) {
			 display_status("converting oid->string", maj_stat, min_stat);
			 return -1;
		 }
		 fprintf(fp, "Mechanism %.*s supports %d names\n",
				 (int) oid_name.length, (char *) oid_name.value,
				 (int) mech_names->count);
	 
		 (void) gss_release_buffer(&min_stat, &oid_name);

		 for (i=0; i<mech_names->count; i++) {
			 maj_stat = gss_oid_to_str(&min_stat,
					 &mech_names->elements[i],
					 &oid_name);
			 if (maj_stat != GSS_S_COMPLETE) {
				 display_status("converting oid->string", maj_stat, min_stat);
				 return LWFS_ERR_CONTEXT;
			 }
	   
			 fprintf(fp, "  %d: %.*s\n", (int) i,
					 (int) oid_name.length, 
					 (char *) oid_name.value);

			 (void) gss_release_buffer(&min_stat, &oid_name);
		 }
	 
		 (void) gss_release_oid_set(&min_stat, &mech_names);

	 }
	
	 return LWFS_OK;
}


static void display_status_1(m, code, type)
     char *m;
     OM_uint32 code;
     int type;
{
     OM_uint32 maj_stat, min_stat;
     gss_buffer_desc msg;
     OM_uint32 msg_ctx;
     
     msg_ctx = 0;
     while (1) {
	  maj_stat = gss_display_status(&min_stat, code,
				       type, GSS_C_NULL_OID,
				       &msg_ctx, &msg);

	  log_error(debug_level, "GSS-API error %s: %s\n", m, (char *)msg.value); 
	  (void) gss_release_buffer(&min_stat, &msg);
	  
	  if (!msg_ctx)
	       break;
     }
}

/*
 * Function: display_status
 *
 * Purpose: displays GSS-API messages
 *
 * Arguments:
 *
 * 	msg		a string to be displayed with the message
 * 	maj_stat	the GSS-API major status code
 * 	min_stat	the GSS-API minor status code
 *
 * Effects:
 *
 * The GSS-API messages associated with maj_stat and min_stat are
 * displayed on stderr, each preceeded by "GSS-API error <msg>: " and
 * followed by a newline.
 */
void display_status(msg, maj_stat, min_stat)
     char *msg;
     OM_uint32 maj_stat;
     OM_uint32 min_stat;
{
     display_status_1(msg, maj_stat, GSS_C_GSS_CODE);
     display_status_1(msg, min_stat, GSS_C_MECH_CODE);
}

/*
 * Function: display_ctx_flags
 *
 * Purpose: displays the flags returned by context initation in
 *	    a human-readable form
 *
 * Arguments:
 *
 * 	int		ret_flags
 *
 * Effects:
 *
 * Strings corresponding to the context flags are printed on
 * stdout, preceded by "context flag: " and followed by a newline
 */

void display_ctx_flags(flags)
     OM_uint32 flags;
{
     if (flags & GSS_C_DELEG_FLAG)
	  log_debug(debug_level, "context flag: GSS_C_DELEG_FLAG\n");
     if (flags & GSS_C_MUTUAL_FLAG)
	  log_debug(debug_level, "context flag: GSS_C_MUTUAL_FLAG\n");
     if (flags & GSS_C_REPLAY_FLAG)
	  log_debug(debug_level, "context flag: GSS_C_REPLAY_FLAG\n");
     if (flags & GSS_C_SEQUENCE_FLAG)
	  log_debug(debug_level, "context flag: GSS_C_SEQUENCE_FLAG\n");
     if (flags & GSS_C_CONF_FLAG )
	  log_debug(debug_level, "context flag: GSS_C_CONF_FLAG \n");
     if (flags & GSS_C_INTEG_FLAG )
	  log_info(debug_level, "context flag: GSS_C_INTEG_FLAG \n");
}

void print_token(tok)
     gss_buffer_t tok;
{
    int i;
    unsigned char *p = tok->value;

    for (i=0; i < tok->length; i++, p++) {
	fprintf(logger_get_file(), "%02x ", *p);
	if ((i % 16) == 15) {
	    fprintf(logger_get_file(), "\n");
	}
    }
    fprintf(logger_get_file(), "\n");
    fflush(logger_get_file());
}

#ifdef _WIN32
#include <sys\timeb.h>
#include <time.h>

int gettimeofday (struct timeval *tv, void *ignore_tz)
{
    struct _timeb tb;
    _tzset();
    _ftime(&tb);
    if (tv) {
	tv->tv_sec = tb.time;
	tv->tv_usec = tb.millitm * 1000;
    }
    return 0;
}
#endif /* _WIN32 */
