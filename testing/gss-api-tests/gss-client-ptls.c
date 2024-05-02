
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <gssapi/gssapi_generic.h>
#include <gssapi/gssapi.h>
#include "gss-misc-ptls.h"

#include "config.h"
#include "lwfs/lwfs.h"
#include "lwfs/logger.h"
#include "lwfs/lwfs_ptls.h"

#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

static int verbose = 1;
static log_level debug_level = LOG_DEBUG;

static void usage()
{
    fprintf(stderr,
	    "Usage: gss-client [-port port] [-mech mechanism] [-d]\n");
    fprintf(stderr, "       [-f] [-q] [-ccount count] [-mcount count]\n");
    fprintf(stderr,
	    "       [-v1] [-na] [-nw] [-nx] [-nm] host service msg\n");
    exit(1);
}


/*
 * send msg
 */
int client_send_message(lwfs_process_id *server,
				    gss_ctx_id_t context,
				    char *msg) 
{
	gss_buffer_desc in_buf, out_buf;
	int encrypt_flag = FALSE;
	OM_uint32 maj_stat, min_stat; 
	gss_request req; 
	int rc; 
	int portal_index = 2;
	int match_bits = 13;
	char token_buf[GSS_TOKEN_SIZE];
	lwfs_local_md local_md; 
	lwfs_remote_md server_md; 
	int flags; 
	int state; 
	gss_qop_t qop_state;

	in_buf.value = msg; 
	in_buf.length = strlen(msg); 

	/* wrap the message using the context */
	log_debug(debug_level,"client_send_message: wrapping message\n"); 
	maj_stat = gss_wrap(&min_stat, 
			context, 
			encrypt_flag,
		       	GSS_C_QOP_DEFAULT,
			&in_buf, 
			&state, 
			&out_buf); 

	if (maj_stat != GSS_S_COMPLETE) {
		display_status("wrapping message", maj_stat, min_stat); 
		return LWFS_ERR_ENCODE; 
	}
	else if (encrypt_flag && !state) {
		log_warn(debug_level, "message not encrypted\n");
	}

	/* create a request */
	req.op = PRINT_DATA; 
	req.msg_len = out_buf.length; 
	memcpy(req.msg, out_buf.value, out_buf.length); 

	/* post a recv for the result */
	rc = post_recv(portal_index,
	   		&token_buf,
	   		GSS_TOKEN_SIZE,
	   		*server,
	   		match_bits,
	   		&(req.result_md),
	   		&local_md);
	if (rc != LWFS_OK) {
   		log_error(debug_level, "unable to post recv for a token\n");
   		return -1;
     	}

	/* initialize the server md for requests */
	server_md.id.nid = server->nid; 
	server_md.id.pid = server->pid; 
	server_md.index  = GSS_SERVER_REQUEST_INDEX;
	server_md.match_bits = 0; 
	server_md.len = sizeof(gss_request);
	server_md.offset = 0; 

	/* send the request to the server */
     	log_debug(debug_level, "client_send_message: "
			"sending \"PRINT_DATA\" request "
			"to server (len=%d, msg=%s)\n", 
			sizeof(gss_request), msg);
	rc = send_request(&server_md, &req); 
	if (rc != LWFS_OK) {
		log_error(debug_level, "client_establish_context: "
				"unable to send request to server\n");
		return rc; 
	}

	/* wait for result (server returns a signed block of the input message) */
     	log_debug(debug_level,"client_send_message: "
			"waiting for response from server...\n");
	rc = recv_token(&local_md, &flags, &out_buf); 
	if (rc != LWFS_OK) {
		log_error(debug_level, "client_establish_context: "
				"unable to send request to server\n");
		return rc; 
	}

	/* verify signature */
	maj_stat = gss_verify_mic(&min_stat, context, &in_buf, &out_buf, &qop_state);
	if (maj_stat != GSS_S_COMPLETE) {
		display_status("verifying signature", maj_stat, min_stat);
		return LWFS_ERR_DECODE;
	}

	log_debug(debug_level,"client_send_message: signature verified "
			"size=%d\n", (int)out_buf.length); 
	return LWFS_OK;
}


/*
 * Function: client_establish_context
 *
 * Purpose: establishes a GSS-API context with a specified service and
 * returns the context handle
 *
 * Arguments:
 *
 * 	s		(r) an established TCP connection to the service
 * 	service_name	(r) the ASCII service name of the service
 *	deleg_flag	(r) GSS-API delegation flag (if any)
 *	auth_flag	(r) whether to actually do authentication
 *	oid		(r) OID of the mechanism to use
 * 	context		(w) the established GSS-API context
 *	ret_flags	(w) the returned flags from init_sec_context
 *
 * Returns: 0 on success, -1 on failure
 *
 * Effects:
 * 
 * service_name is imported as a GSS-API name and a GSS-API context is
 * established with the corresponding service; the service should be
 * listening on the TCP connection s.  The default GSS-API mechanism
 * is used, and mutual authentication and replay detection are
 * requested.
 * 
 * If successful, the context handle is returned in context.  If
 * unsuccessful, the GSS-API error messages are displayed on stderr
 * and -1 is returned.
 */
static int client_establish_context(lwfs_process_id *server,
				    char *service_name,
				    gss_OID oid,
				    OM_uint32 deleg_flag,
				    int auth_flag,
				    int v1_format,
				    gss_ctx_id_t * gss_context,
				    OM_uint32 * ret_flags)
{
	int rc; 
	uint32 portal_index = 5;
	/*
	 * uint32 match_bits = rand(); 
	 */
	uint32 match_bits = 99; 

	int flags; 
	gss_request req; 
	lwfs_remote_md server_md; 
	lwfs_local_md local_md; 
	char token_buf[GSS_TOKEN_SIZE];

	gss_buffer_desc send_tok, recv_tok, *token_ptr;
	gss_name_t target_name;
	OM_uint32 maj_stat, min_stat, init_sec_min_stat;

	log_debug(debug_level,"est_ctx: start\n");

	/* Import the name into target_name.  Use send_tok to
	 * save local variable space.  
	 */
	send_tok.value = service_name;
	send_tok.length = strlen(service_name);
	maj_stat = gss_import_name(
			&min_stat, 
			&send_tok,
			(gss_OID) gss_nt_service_name, 
			&target_name);
	if (maj_stat != GSS_S_COMPLETE) {
	    display_status("parsing name", maj_stat, min_stat);
	    return LWFS_ERR_CONTEXT;
	}

	/* output the service name */
	gss_OID name_type;
	gss_buffer_desc service_name_buf;
	gss_display_name(&min_stat, target_name, &service_name_buf,
   			&name_type);
	log_debug(debug_level,"client_establish_context: service_name = %s\n", 
			(char *)service_name_buf.value);

	/* initialize the "establish context" request */
	req.op = ESTABLISH_CONTEXT; 

	/* post recv for initial token from server */
     	log_debug(debug_level, "client_establish_context: posting recv for token "
			"(index=%d, match_bits=%d)\n",
			portal_index, match_bits);
	rc = post_recv(portal_index,
	   		&token_buf,
	   		GSS_TOKEN_SIZE,
	   		*server,
	   		match_bits,
	   		&(req.result_md),
	   		&local_md);
	if (rc != LWFS_OK) {
   		log_error(debug_level, "unable to post recv for a token\n");
   		return -1;
     	}

	/* initialize the server md for requests */
	server_md.id.nid = server->nid; 
	server_md.id.pid = server->pid; 
	server_md.index  = GSS_SERVER_REQUEST_INDEX;
	server_md.match_bits = 0; 
	server_md.len = sizeof(gss_request);
	server_md.offset = 0; 

	/* send the request to the server */
     	log_debug(debug_level, "client_establish_context: "
			"sending \"ESTABLISH_CONTEXT\" request "
			"to server (len=%d)\n", sizeof(gss_request));
	rc = send_request(&server_md, &req); 
	if (rc != LWFS_OK) {
		log_error(debug_level, "client_establish_context: "
				"unable to send request to server\n");
		return rc; 
	}

	/* wait for initial token from server (contains server_md for tokens) */
     	log_debug(debug_level, "client_establish_context: "
			"waiting for server-side md from for tokens...\n");
	rc = recv_token(&local_md, &flags, &recv_tok); 
	if (rc != LWFS_OK) {
		log_error(debug_level, "client_establish_context: "
				"unable to send request to server\n");
		return rc; 
	}
	
	/* extract the server_md for tokens */
	if (recv_tok.length != sizeof(lwfs_remote_md)) {
		log_error(debug_level, "client_establish_context: "
				"expected lwfs_remote_md\n");
		return LWFS_ERR_COMM; 
	}
	memcpy(&server_md, recv_tok.value, sizeof(lwfs_remote_md)); 

	/* print info about the server's md for tokens */
	log_debug(debug_level, "client_establish_context: server_md.id.nid = %llu\n", 
			server_md.id.nid);
	log_debug(debug_level, "client_establish_context: server_md.id.pid = %llu\n", 
			server_md.id.pid);
	log_debug(debug_level, "client_establish_context: server_md.index = %d\n", 
			server_md.index);
	log_debug(debug_level, "client_establish_context: server_md.match_bits = %d\n", 
			server_md.match_bits);
	log_debug(debug_level, "client_establish_context: server_md.length = %d\n", 
			server_md.len);
	log_debug(debug_level, "client_establish_context: server_md.offset = %d\n", 
			server_md.offset);

	/* 
	 * Perform the context-establishement loop.
	 *
	 * On each pass through the loop, token_ptr points to the token
	 * to send to the server (or GSS_C_NO_BUFFER on the first pass).
	 * Every generated token is stored in send_tok which is then
	 * transmitted to the server; every received token is stored in
	 * recv_tok, which token_ptr is then set to, to be processed by
	 * the next call to gss_init_sec_context.
	 * 
	 * GSS-API guarantees that send_tok's length will be non-zero
	 * if and only if the server is expecting another token from us,
	 * and that gss_init_sec_context returns GSS_S_CONTINUE_NEEDED if
	 * and only if the server has another token to send us.
	 */

	token_ptr = GSS_C_NO_BUFFER;
	*gss_context = GSS_C_NO_CONTEXT;

	do {
	    char token_buf[GSS_TOKEN_SIZE];
	    lwfs_remote_md remote_md; 
	    lwfs_local_md local_md; 

	    /* initialize the security context */
	    log_debug(debug_level, "client_establish_context: calling gss_init_sec_context\n");
	    maj_stat = gss_init_sec_context(
			    &init_sec_min_stat, 
			    GSS_C_NO_CREDENTIAL,  /* uses default */
			    gss_context, 
			    target_name, 
			    oid, 
			    GSS_C_MUTUAL_FLAG 
			    | GSS_C_REPLAY_FLAG 
			    | deleg_flag, 
			    0, 
			    NULL,	/* no channel bindings */
			    token_ptr, 
			    NULL,	/* ignore mech type */
			    &send_tok, 
			    ret_flags, 
			    NULL);	/* ignore time_rec */
	
	    log_debug(debug_level, "client_establish_context: "
			    "returned from gss_init_sec_context"
			    " (stat=%d)\n", maj_stat); 


	    /* free the recv_tok if necessary */
	    if (token_ptr != GSS_C_NO_BUFFER)
		free(recv_tok.value);

	    /* post a recv if we need to continue */
	    if (maj_stat == GSS_S_CONTINUE_NEEDED) {
		    /* post recv for initial token from server */
		    log_debug(debug_level, "client_establish_context: posting recv for token "
			    	    "(index=%d, match_bits=%d)\n",
				    portal_index, match_bits);
		    rc = post_recv(portal_index,
				    &token_buf,
			    	    GSS_TOKEN_SIZE,
				    server_md.id,
				    match_bits,
		    		    &remote_md,
			    	    &local_md);
	    	    if (rc != LWFS_OK) {
		    	    log_error(debug_level, "client_establish_context: "
					    "unable to post recv for a token\n");
			    return rc;
	    	    }
	    }

	    /* may need to send context info to the server */
	    if (send_tok.length != 0) {
		    /* send the token to the server */
		    log_debug(debug_level, "client_establish_context: "
		    		    "sending token to server (size=%d)...\n",
		    		    (int) send_tok.length);
		    rc = send_token(&server_md, TOKEN_CONTEXT, &send_tok); 
		    if (rc != LWFS_OK) {
			    log_error(debug_level, "client_establish_contex: "
					    "failed sending token to server\n");
		    	    (void) gss_release_buffer(&min_stat, &send_tok);
		    	    (void) gss_release_name(&min_stat, &target_name);
		    	    return rc;
		    }
	    }

	    /* free memory in the send token */
	    (void) gss_release_buffer(&min_stat, &send_tok);

	    /* handle error case */
	    if ((maj_stat != GSS_S_COMPLETE) 
			    && (maj_stat != GSS_S_CONTINUE_NEEDED)) {
		    log_debug(debug_level, "client_establish_context: problem here\n");
		    display_status("initializing context", maj_stat,
	     			    init_sec_min_stat);
	    	    (void) gss_release_name(&min_stat, &target_name);
	    	    if (*gss_context != GSS_C_NO_CONTEXT)
			    gss_delete_sec_context(&min_stat, gss_context,
	 				    GSS_C_NO_BUFFER);
	    	    return LWFS_ERR_COMM;
	    }

	    /* recv response from server */
	    if (maj_stat == GSS_S_CONTINUE_NEEDED) {
		    log_debug(debug_level, "client_establish_context: "
				    "waiting for token from server\n");
		    /* wait for token buffer */
		    rc = recv_token(&local_md, &flags, &recv_tok); 
		    if (rc != LWFS_OK) {
			    log_error(debug_level, "client_establish_context: "
				    	    "unable to send request to server\n");
			    (void) gss_release_name(&min_stat, &target_name);
			    return rc; 
		    }
	     
		    token_ptr = &recv_tok;
	    }

	} while (maj_stat == GSS_S_CONTINUE_NEEDED);

	(void) gss_release_name(&min_stat, &target_name);

	log_debug(debug_level, "client_establish_context: context established\n"); 

	return LWFS_OK;
}

static void parse_oid(char *mechanism, gss_OID *oid)
{
    char	*mechstr = 0, *cp;
    gss_buffer_desc tok;
    OM_uint32 maj_stat, min_stat;
    
    if (isdigit((int) mechanism[0])) {
	mechstr = malloc(strlen(mechanism)+5);
	if (!mechstr) {
	    fprintf(stderr, "Couldn't allocate mechanism scratch!\n");
	    return;
	}
	sprintf(mechstr, "{ %s }", mechanism);
	for (cp = mechstr; *cp; cp++)
	    if (*cp == '.')
		*cp = ' ';
	tok.value = mechstr;
    } else
	tok.value = mechanism;
    tok.length = strlen(tok.value);
    maj_stat = gss_str_to_oid(&min_stat, &tok, oid);
    if (maj_stat != GSS_S_COMPLETE) {
	display_status("str_to_oid", maj_stat, min_stat);
	return;
    }
    if (mechstr)
	free(mechstr);
}



int main(argc, argv)
int argc;
char **argv;
{
    int rc; 
    char *service_name,
    *msg;
    char *mechanism = 0;
    u_short port = 4444;
    int use_file = 0;
    OM_uint32 deleg_flag = 0,
	min_stat;
    gss_OID oid = GSS_C_NULL_OID;
    int mcount = 1,
	ccount = 1;
    int i;
    int auth_flag;
    int wrap_flag; 
    int encrypt_flag; 
    int mic_flag; 
    int v1_format;
    lwfs_process_id server;
    gss_ctx_id_t context; 


    auth_flag = wrap_flag = encrypt_flag = mic_flag = 1;
    v1_format = 0;
    OM_uint32 ret_flags;

    /* Parse arguments. */
    argc--;
    argv++;
    while (argc) {
	if (strcmp(*argv, "-port") == 0) {
	    argc--;
	    argv++;
	    if (!argc)
		usage();
	    port = atoi(*argv);
	} else if (strcmp(*argv, "-mech") == 0) {
	    argc--;
	    argv++;
	    if (!argc)
		usage();
	    mechanism = *argv;
	} else if (strcmp(*argv, "-d") == 0) {
	    deleg_flag = GSS_C_DELEG_FLAG;
	} else if (strcmp(*argv, "-f") == 0) {
	    use_file = 1;
	} else if (strcmp(*argv, "-q") == 0) {
	    verbose = 0;
	} else if (strcmp(*argv, "-ccount") == 0) {
	    argc--;
	    argv++;
	    if (!argc)
		usage();
	    ccount = atoi(*argv);
	    if (ccount <= 0)
		usage();
	} else if (strcmp(*argv, "-mcount") == 0) {
	    argc--;
	    argv++;
	    if (!argc)
		usage();
	    mcount = atoi(*argv);
	    if (mcount < 0)
		usage();
	} else if (strcmp(*argv, "-na") == 0) {
	    auth_flag = wrap_flag = encrypt_flag = mic_flag = 0;
	} else if (strcmp(*argv, "-nw") == 0) {
	    wrap_flag = 0;
	} else if (strcmp(*argv, "-nx") == 0) {
	    encrypt_flag = 0;
	} else if (strcmp(*argv, "-nm") == 0) {
	    mic_flag = 0;
	} else if (strcmp(*argv, "-v1") == 0) {
	    v1_format = 1;
	} else
	    break;
	argc--;
	argv++;
    }
    if (argc != 3)
	usage();

    server.nid = strtoull(*argv++, NULL, 10);
    server.pid = GSS_SERVER_PID;

    service_name = *argv++;
    msg = *argv++;

    sleep(5);

    /* initialize the logger */
    logger_init(stdout, LOG_DEBUG);


    /* initialize portals */
    rc = lwfs_ptl_init(PTL_PID_ANY);

    if (mechanism)
	parse_oid(mechanism, &oid);



    for (i = 0; i < ccount; i++) {
	/* Establish context */
	    display_ctx_flags(ret_flags);
	rc = client_establish_context(&server, 
				service_name, 
				oid, 
				deleg_flag, 
				auth_flag, 
				v1_format,
				&context, 
				&ret_flags); 
	if (rc != LWFS_OK) {
		log_fatal(debug_level, "unable to establish context\n");
		return rc; 
	}
	
	rc = inquire_context(context, logger_get_default_level()); 
	if (rc != LWFS_OK) {
		log_error(debug_level, "unable to inquire context\n");
		return rc; 
	}

	rc = client_send_message(&server, context, msg); 
    }

    if (oid != GSS_C_NULL_OID)
	(void) gss_release_oid(&min_stat, &oid);

    return 0;
}

