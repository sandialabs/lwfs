/*
 * Copyright 1994 by OpenVision Technologies, Inc.
 * 
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appears in all copies and
 * that both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of OpenVision not be used
 * in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission. OpenVision makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 * 
 * OPENVISION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL OPENVISION BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * $Id: gss-misc-ptls.h 1073 2007-01-23 05:06:53Z raoldfi $
 */

#ifndef _GSSMISC_H_
#define _GSSMISC_H_

#include <gssapi/gssapi_generic.h>
#include <stdio.h>
#include <lwfs.h>
#include <logger/logger.h>

#define GSS_TOKEN_SIZE  5120
#define GSS_SERVER_PID 5

/* portal indices for the server */
#define GSS_SERVER_REQUEST_INDEX 6
#define GSS_SERVER_CONTEXT_INDEX 7

enum gss_ops {
	ESTABLISH_CONTEXT = 1,
	PRINT_DATA = 2,
};
typedef enum gss_ops gss_ops; 

struct gss_request {
	int op; 
	lwfs_remote_md request_md; /* where to get the request */
	lwfs_remote_md result_md;  /* where to put the result */
	int msg_len; 
	char msg[256]; 
}; 
typedef struct gss_request gss_request; 


extern FILE *display_file;

extern int inquire_context(gss_ctx_id_t context, log_level level); 

extern lwfs_return_code post_recv(uint32 ptl_index,
		void *buf, 
		uint32 length,
		lwfs_process_id match_id,
		uint32 match_bits,
		lwfs_remote_md *remote_md,
		lwfs_local_md *local_md);

extern int send_request(lwfs_remote_md *dest_md, 
		gss_request *req);

extern int recv_request(lwfs_local_md *md,
		gss_request *req);

extern int send_token(lwfs_remote_md *dest_md, 
		int flags, 
		gss_buffer_desc *tok);

extern int recv_token(lwfs_local_md *md,
		int *flags, 
		gss_buffer_desc *tok);

extern void display_status(char *msg, 
		OM_uint32 maj_stat, 
		OM_uint32 min_stat);

extern void display_ctx_flags(OM_uint32 flags);

extern void print_token(gss_buffer_t tok);

/* Token types */
#define TOKEN_NOOP		(1<<0)
#define TOKEN_CONTEXT		(1<<1)
#define TOKEN_DATA		(1<<2)
#define TOKEN_MIC		(1<<3)

/* Token flags */
#define TOKEN_CONTEXT_NEXT	(1<<4)
#define TOKEN_WRAPPED		(1<<5)
#define TOKEN_ENCRYPTED		(1<<6)
#define TOKEN_SEND_MIC		(1<<7)

extern gss_buffer_t empty_token;

#endif
