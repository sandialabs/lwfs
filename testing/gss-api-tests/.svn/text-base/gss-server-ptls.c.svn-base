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

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <ctype.h>

#include <gssapi/gssapi_generic.h>
#include "gss-misc-ptls.h"
#include "lwfs/lwfs.h"
#include "lwfs/logger.h"
#include "lwfs_ptls.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

static log_level debug_level = LOG_DEBUG; 

static void usage()
{
     fprintf(stderr, "Usage: gss-server [-port port] [-verbose] [-once]\n");
     fprintf(stderr, "       [-inetd] [-export] [-logfile file] service_name\n");
     exit(1);
}


int verbose = 0;

/*
 * Function: server_acquire_creds
 *
 * Purpose: imports a service name and acquires credentials for it
 *
 * Arguments:
 *
 * 	service_name	(r) the ASCII service name
 * 	server_creds	(w) the GSS-API service credentials
 *
 * Returns: 0 on success, -1 on failure
 *
 * Effects:
 *
 * The service name is imported with gss_import_name, and service
 * credentials are acquired with gss_acquire_cred.  If either opertion
 * fails, an error message is displayed and -1 is returned; otherwise,
 * 0 is returned.
 */
static int server_acquire_creds(service_name, server_creds)
     char *service_name;
     gss_cred_id_t *server_creds;
{
     gss_buffer_desc name_buf;
     gss_name_t server_name;
     OM_uint32 maj_stat, min_stat;

     name_buf.value = service_name;
     name_buf.length = strlen(name_buf.value) + 1;
     maj_stat = gss_import_name(&min_stat, &name_buf, 
				(gss_OID) gss_nt_service_name, &server_name);
     if (maj_stat != GSS_S_COMPLETE) {
	  display_status("importing name", maj_stat, min_stat);
	  return -1;
     }

     gss_OID name_type; 
     gss_buffer_desc srvr_name_buf;
     /* put the server name back into a readable buffer */
     gss_display_name(&min_stat, server_name, &srvr_name_buf, &name_type);

     printf("server_name = %s\n", (char *)srvr_name_buf.value);

     maj_stat = gss_acquire_cred(&min_stat, server_name, 0,
				 GSS_C_NULL_OID_SET, GSS_C_ACCEPT,
				 server_creds, NULL, NULL);
     if (maj_stat != GSS_S_COMPLETE) {
	  display_status("acquiring credentials", maj_stat, min_stat);
	  return -1;
     }

     (void) gss_release_name(&min_stat, &server_name);

     return 0;
}

/*
 * Function: server_establish_context
 *
 * Purpose: establishses a GSS-API context as a specified service with
 * an incoming client, and returns the context handle and associated
 * client name
 *
 * Arguments:
 *
 * 	s		(r) an established TCP connection to the client
 * 	service_creds	(r) server credentials, from gss_acquire_cred
 * 	context		(w) the established GSS-API context
 * 	client_name	(w) the client's ASCII name
 *
 * Returns: 0 on success, -1 on failure
 *
 * Effects:
 *
 * Any valid client request is accepted.  If a context is established,
 * its handle is returned in context and the client name is returned
 * in client_name and 0 is returned.  If unsuccessful, an error
 * message is displayed and -1 is returned.
 */
static int server_establish_context(
		uint32 local_context_portal,
		lwfs_remote_md *client_md,
		gss_cred_id_t server_creds,
		gss_ctx_id_t *context,
		gss_buffer_t client_name,
		OM_uint32 *ret_flags)
{
     gss_buffer_desc send_tok, recv_tok;
     gss_name_t client;
     gss_OID doid;
     OM_uint32 maj_stat, min_stat, acc_sec_min_stat;
     gss_buffer_desc oid_name;
     int token_flags;
     int rc; 
     char token_buf[GSS_TOKEN_SIZE]; 

     lwfs_local_md local_tok_md; 
     lwfs_remote_md remote_tok_md; 

     *context = GSS_C_NO_CONTEXT;

     log_debug(debug_level, "start\n");


     /* print info about the client's md for tokens */
     log_debug(debug_level, "client_md.id.nid = %llu\n", 
		     (unsigned long long)client_md->id.nid);
	
     log_debug(debug_level, "client_md.id.pid = %llu\n", 
	     	     (unsigned long long)client_md->id.pid);
	
     log_debug(debug_level, "client_md.index = %d\n", 
	     	     client_md->index);

     log_debug(debug_level, "client_md.match_bits = %d\n", 
	     	     client_md->match_bits);

     log_debug(debug_level, "client_md.length = %d\n", 
	     	     client_md->len);

     log_debug(debug_level, "client_md.offset = %d\n", 
	     	     client_md->offset);


     /* post a receive for the next token */
     log_debug(debug_level,
		     "posting recv for token (index=%d, match_bits=%d)\n",
		     local_context_portal, client_md->match_bits);
     rc = post_recv(local_context_portal,
		     &token_buf,
		     GSS_TOKEN_SIZE,
		     client_md->id,
		     client_md->match_bits,
		     &remote_tok_md,
		     &local_tok_md);
     if (rc != LWFS_OK) {
	     log_error(debug_level, "unable to post recv for a token\n");
	     return rc;
     }

     /* send a token that holds the remote memory descriptor to 
      * the dest process */
     send_tok.length = sizeof(lwfs_remote_md); 
     send_tok.value = (char *)&remote_tok_md; 

     log_debug(debug_level, "server_establish_context: sending server md to client\n");
     rc = send_token(client_md, TOKEN_DATA, &send_tok); 
     if (rc != LWFS_OK) {
	     log_error(debug_level, "unable to send initial token\n");
	     return rc;
     }

     /* initialize context structures */
     *context = GSS_C_NO_CONTEXT; 

     /* Start the loop to establish a security context.
      * Loop will break if we receive a NOOP token. 
      */
     do {
	     /* wait for a token from the client */
     	     log_debug(debug_level, "server_establish_context: waiting for next token...\n");
	     rc = recv_token(&local_tok_md, &token_flags, &recv_tok); 
	     if (rc != LWFS_OK) {
		     log_fatal(debug_level, "unable to recv token\n");
		     return rc;
	     }

	     log_debug(debug_level, "server_establish_context: recv'd token (size=%d)\n",
		    	     (int) recv_tok.length);
   		
	     print_token(&recv_tok);

	     /* try to accept the security context */
	     log_debug(debug_level, "server_establish_context: accepting context\n");
	     maj_stat = gss_accept_sec_context(&acc_sec_min_stat,
		    	     context,
			     server_creds,
			     &recv_tok,
			     GSS_C_NO_CHANNEL_BINDINGS,
			     &client,
			     &doid,
			     &send_tok,
			     ret_flags,
			     NULL, 	/* ignore time_rec */
			     NULL); /* ignore del_cred_handle */


	     if (maj_stat == GSS_S_CONTINUE_NEEDED) {
		     /* post a receive for the next token */
		     log_debug(debug_level, "server_establish_context: "
				     "posting recv for token "
				     "(index=%d, match_bits=%llu)\n",
				     local_context_portal, client_md->match_bits);
		     rc = post_recv(local_context_portal,
				     &token_buf,
				     GSS_TOKEN_SIZE,
				     client_md->id,
				     client_md->match_bits,
				     &remote_tok_md,
				     &local_tok_md);
		     if (rc != LWFS_OK) {
			     log_error(debug_level, "server_establish_context: "
					     "unable to post recv for a token\n");
			     return rc;
		     }
	     }

	     /* Free recv_tok? */
	     if(recv_tok.value) {
		     free(recv_tok.value);
		     recv_tok.value = NULL;
	     }
	 
	     /* may need to send a response to the client */
	     if (send_tok.length != 0) {
		     log_debug(debug_level, "server_establish_context: "
				     "sending token to client (size=%d)\n",
		    		     (int) send_tok.length);
		     print_token(&send_tok);
	   
		     rc = send_token(client_md, TOKEN_CONTEXT, &send_tok);
		     if (rc < 0) {
			     log_fatal(debug_level, "failure sending token\n");
			     return rc;
		     }

		     (void) gss_release_buffer(&min_stat, &send_tok);
	     }

	     /* handle error cases */
	     if (maj_stat!=GSS_S_COMPLETE && maj_stat!=GSS_S_CONTINUE_NEEDED) {
		     log_error(debug_level, "server_establish_context: "
				     "could not accept context\n");
		     display_status("accepting context", maj_stat,
		     		     acc_sec_min_stat);
		     if (*context != GSS_C_NO_CONTEXT) {
		    	     gss_delete_sec_context(&min_stat, context,
		    			     GSS_C_NO_BUFFER);
		     }

		     return LWFS_ERR_CONTEXT;
	     }
       
     } while (maj_stat == GSS_S_CONTINUE_NEEDED);

     /* display the flags */
     display_ctx_flags(*ret_flags);

     /* handle error case */
     if (maj_stat != GSS_S_COMPLETE) {
	     log_error(debug_level, "server_establish_context: "
			     "could not accept context\n");
    	     display_status("displaying name", maj_stat, min_stat);
	     return LWFS_ERR_CONTEXT; 
     }

     /* Everything beyond this point assumes an accepted context */
     maj_stat = gss_oid_to_str(&min_stat, doid, &oid_name);
     if (maj_stat != GSS_S_COMPLETE) {
    	     display_status("converting oid->string", maj_stat, min_stat);
    	     return -1;
     }
     log_debug(debug_level, "server_establish_context: "
		     "accepted connection using mechanism OID %.*s.\n",
    		     (int) oid_name.length, (char *) oid_name.value);
     (void) gss_release_buffer(&min_stat, &oid_name);

     maj_stat = gss_display_name(&min_stat, client, client_name, &doid);
     if (maj_stat != GSS_S_COMPLETE) {
    	     display_status("displaying name", maj_stat, min_stat);
    	     return LWFS_ERR_CONTEXT;
     }
     maj_stat = gss_release_name(&min_stat, &client);
     if (maj_stat != GSS_S_COMPLETE) {
    	     display_status("releasing name", maj_stat, min_stat);
    	     return LWFS_ERR_CONTEXT;
     }

     log_debug(debug_level, "server_establish_context: finished\n\n");

     return LWFS_OK;
}


int
main(argc, argv)
     int argc;
     char **argv;
{
     char *service_name;
     gss_cred_id_t server_creds;
     OM_uint32 min_stat;
     u_short port = 4444;
     int rc; 
     int once = 0;
     int do_inetd = 0;
     int export = 0;
     int done = FALSE; 
     int count; 
     
     lwfs_remote_md remote_md; 
     lwfs_local_md local_md; 

     gss_request req[2]; 
     lwfs_process_id match_id = {
	     PTL_NID_ANY,
	     PTL_PID_ANY,
     };
     uint32 match_bits = 0;

     logger_init(stdout, LOG_DEBUG); 
     display_file = stdout;
     argc--; argv++;
     while (argc) {
	  if (strcmp(*argv, "-port") == 0) {
	       argc--; argv++;
	       if (!argc) usage();
	       port = atoi(*argv);
	  } else if (strcmp(*argv, "-verbose") == 0) {
	      verbose = 1;
	  } else if (strcmp(*argv, "-once") == 0) {
	      once = 1;
	  } else if (strcmp(*argv, "-inetd") == 0) {
	      do_inetd = 1;
	  } else if (strcmp(*argv, "-export") == 0) {
	      export = 1;
	  } else if (strcmp(*argv, "-logfile") == 0) {
	      argc--; argv++;
	      if (!argc) usage();
	      /* Gross hack, but it makes it unnecessary to add an
                 extra argument to disable logging, and makes the code
                 more efficient because it doesn't actually write data
                 to /dev/null. */
	      if (! strcmp(*argv, "/dev/null")) {
		display_file = NULL;
		logger_set_file(NULL);
	      }
	      else {
		FILE *logfile = fopen(*argv, "a");
		display_file = logfile;
		logger_set_file(logfile);
		if (!logfile) {
		  perror(*argv);
		  exit(1);
		}
	      }
	  } else
	       break;
	  argc--; argv++;
     }
     if (argc != 1)
	  usage();

     if ((*argv)[0] == '-')
	  usage();

     log_debug(debug_level, "main: acquiring credentials of server\n");
     service_name = *argv;
     if (server_acquire_creds(service_name, &server_creds) < 0)
	 return -1;
     

     /* initialize portals */
     log_debug(debug_level, "main: initializing portals\n");
     rc = lwfs_ptl_init(GSS_SERVER_PID); 
     if (rc != LWFS_OK) {
	     log_fatal(debug_level, "unable to initialize portals\n");
	     return -1; 
     }

     count = 0;

     /* post the receive for the request */
     log_debug(debug_level, "main: posting recv for request\n"); 
     rc = post_recv(GSS_SERVER_REQUEST_INDEX,
		     &req[count%2],
		     sizeof(gss_request),
		     match_id,
		     match_bits,
		     &remote_md,
		     &local_md);
     if (rc != LWFS_OK) {
	     log_fatal(debug_level, "error posting recv for request"); 
	     return rc; 
     }

     while (!done) {
	 gss_buffer_desc client_name;
	 OM_uint32 ret_flags; 
	 gss_ctx_id_t context; 

	 /* wait for the request  */
	 log_debug(debug_level, "main: wait for request\n"); 
	 rc = recv_request(&local_md, &req[count%2]); 
	 if (rc != LWFS_OK) {
		 log_fatal(debug_level, "error receiving request"); 
		 break;
	 }
	 log_debug(debug_level, "received request (op=%d)\n", req[count%2].op);
     
	 /* post the receive for the next request */
	 log_debug(debug_level, "main: posting recv for request\n"); 
	 rc = post_recv(GSS_SERVER_REQUEST_INDEX,
		     &(req[(count+1)%2]),
		     sizeof(gss_request),
		     match_id,
		     match_bits,
		     &remote_md,
		     &local_md);
	 if (rc != LWFS_OK) {
		 log_fatal(debug_level, "error posting recv for request"); 
		 return rc; 
	 }

	 switch (req[count%2].op) {

		 case ESTABLISH_CONTEXT:
			 rc = server_establish_context(
					 GSS_SERVER_CONTEXT_INDEX,
					 &(req[count%2].result_md),
					 server_creds, 
					 &context, 
					 &client_name,
					 &ret_flags);

			 log_debug(debug_level, "Accepted connection: \"%.*s\"\n",
					 (int) client_name.length,
					 (char *) client_name.value);
			 (void) gss_release_buffer(&min_stat, &client_name);
			 break; 


		 case PRINT_DATA:
			 {
				 gss_buffer_desc msg_buf, xmit_buf; 
				 OM_uint32 maj_stat, min_stat;
				 int conf_state;

				 /* extract the message buffer from the request */
				 xmit_buf.length = req[count%2].msg_len; 
				 xmit_buf.value = req[count%2].msg; 

				 /* unwrap the message */
				 maj_stat = gss_unwrap(&min_stat,
						 context, 
						 &xmit_buf,
						 &msg_buf,
						 &conf_state, 
						 (gss_qop_t *)NULL);

				 if (maj_stat != GSS_S_COMPLETE) {
					 display_status("unsealing message", maj_stat, min_stat); 
					 done = TRUE;
					 break; 
				 }

				 log_debug(debug_level, "main: received message (%.*s)\n", 
						 (int)msg_buf.length,
						 (char *)msg_buf.value);

				 /* sign the message using the server's context */
				 maj_stat = gss_get_mic(&min_stat, 
						 context, 
						 GSS_C_QOP_DEFAULT,
						 &msg_buf,
						 &xmit_buf); 
				 if (maj_stat != GSS_S_COMPLETE) {
					 display_status("signing message", maj_stat, min_stat); 
					 done = TRUE; 
					 break;
				 }

				 /* send the signature block back to the client */
				 rc = send_token(&(req[count%2].result_md), TOKEN_MIC, &xmit_buf); 
				 if (rc != LWFS_OK) {
					 log_error(debug_level, "unable to send signed message\n");
					 done = TRUE; 
					 break;
				 }

				 log_debug(debug_level, "signed message sent to client\n"); 
			 }
			 break;
	 }

	 count++; 
     }

     (void) gss_release_cred(&min_stat, &server_creds);
     rc = lwfs_ptl_fini(); 

     return 0;
}
