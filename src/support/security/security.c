/*-------------------------------------------------------------------------*/
/**  @file security.c
 *   
 *   @brief Implementation of the security API for the lightweight 
 *   file system. 
 *   
 *   This implementation of the security API for the LWFS uses the 
 *   OpenSSL cryptographic library to generate and verify credentials
 *   and capabilities.  
 *
 *   @author Ron Oldfield (raoldfi\@sandia.gov).
 *   $Revision: 791 $.
 *   $Date: 2006-06-29 21:32:24 -0600 (Thu, 29 Jun 2006) $.
 *
 */

#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <stdio.h>

#include "security.h"

static int seeded = 0; 

/*========= EXPORTED API ==============*/

/**
 * @brief generate a cryptographically strong pseudo 
 * random key. 
 *
 * @param key The result.
 */
int generate_key(
		lwfs_key *key)
{
	int status; 

	/* we may have to initialize the random number generator */
	if (seeded == -1) {
		lwfs_key seedkey; 

		/* FIXME: for now, we just seed the RNG with 
		 * the pseudo random generator. This is probably
		 * not good enough though. */
		RAND_pseudo_bytes(seedkey, sizeof(lwfs_key)); 
		
		RAND_seed(seedkey, sizeof(lwfs_key)); 
	}
		

	/* generate a cryptographically strong key of 
	 * length key->length */
	status = RAND_bytes(*key, sizeof(lwfs_key)); 

	/* Rand returns 1 on success, 0 otherwise */ 
	if (status == 0) {
		return LWFS_ERR_GENKEY; 
	}

	return LWFS_OK;
}

/**
 * @brief Generate a credential.
 *
 * A credential consists of a data portion and a 
 * MAC.  In this implementation, we generate a MAC 
 * using the sha1 hash function. 
 *
 * @param key  A cryptographically strong key.
 * @param data The contents of the credential.
 * @param cred The result. 
 */
int generate_cred(
		const lwfs_key *key,
		const lwfs_cred_data *data, 
		lwfs_cred *cred)
{
	unsigned long datasize = sizeof(lwfs_cred_data); 
	lwfs_mac tmpmac; 
	unsigned int macsize; 

	/* copy the data portion to the credential */
	memcpy(&cred->data, &data, datasize); 

	/* zero out the mac */
	memset(cred->mac, 0, sizeof(lwfs_mac)); 
	memset(tmpmac, 0, sizeof(lwfs_mac)); 

	/* generate the MAC using the provided key and 
	 * the sha1 hash function */
	HMAC(EVP_sha1(),(unsigned char *)(*key), sizeof(lwfs_key), 
			(unsigned char *)(&cred->data), datasize, 
			(unsigned char *)(cred->mac), &macsize); 

	HMAC(EVP_sha1(),(unsigned char *)(*key), sizeof(lwfs_key), 
			(unsigned char *)(&cred->data), datasize, 
			(unsigned char *)(tmpmac), &macsize); 

	if (memcmp(cred->mac, tmpmac, sizeof(lwfs_mac)) != 0) {
		fprintf(stderr, "something is very wrong!\n");
	}
	return LWFS_OK; 
}

/**
 * @brief Verify a credential. 
 *
 * We verify a credential by attempting to reproduce 
 * a MAC that matches the MAC inside the credential.
 * If the MACs do not match, either the key is wrong,
 * or the credential is not valid. In either case 
 * return a failure. 
 *
 * @param key  @input A cryptographic key used to check the credential.
 * @param cred @input The credential to verify. 
 */
int verify_cred(
		const lwfs_key *key,
		const lwfs_cred *cred)
{
	int status; 
	lwfs_mac newmac; 

	unsigned int datasize = sizeof(lwfs_cred_data); 
	unsigned int macsize; 

	/* zero out the mac */
	memset((void *)(newmac), 0, sizeof(lwfs_mac)); 
	
	/* generate a MAC for comparison */
	HMAC(EVP_sha1(),(unsigned char *)(*key), sizeof(lwfs_key), 
			(unsigned char *)(&cred->data), datasize, 
			(unsigned char *)(newmac), &macsize); 

	/* compare the original mac to the new mac */
	if (memcmp(cred->mac, newmac, sizeof(lwfs_mac)) != 0) {
		return LWFS_ERR_VERIFYCRED; 
	}

	return LWFS_OK; 
}

/**
 * @brief Generate a capability.
 */
int generate_cap(
		const lwfs_key *key,
		const lwfs_cap_data *data, 
		lwfs_cap *cap)
{
	unsigned long datasize = sizeof(lwfs_cap_data); 
	unsigned int macsize; 

	/* copy the data portion to the credential */
	memcpy(&cap->data, data, sizeof(lwfs_cap_data)); 

	/* zero out the mac */
	memset((void *)(cap->mac), 0, sizeof(lwfs_mac)); 

	/* generate the MAC using the provided key and 
	 * the sha1 hash function */
	HMAC(EVP_sha1(), key, sizeof(lwfs_key), 
			(unsigned char *)data, sizeof(lwfs_cap_data), 
			(unsigned char *)cap->mac, &macsize); 

	return LWFS_OK; 
}

/**
 * @brief Verify a capability. 
 *
 * We verify a capability the same way we verify a credential. 
 * We generate a new MAC with the provided key that be identical
 * to the MAC inside the capability.  If the MACs do not match, 
 * either the key is wrong, or the capability is not valid. In 
 * either case we return a \ref LWFS_ERR_VERIFYCAP error code. 
 *
 * @param key @input The symmetric key used to generate the original cap.
 * @param cap @input The capability to verify. 
 */
int verify_cap(
		const lwfs_key *key,
		const lwfs_cap *cap)
{
	int status; 
	lwfs_mac newmac; 

	unsigned int datasize = sizeof(lwfs_cap_data); 
	unsigned int macsize; 

	/* zero out the mac */
	memset((void *)(newmac), 0, sizeof(lwfs_mac)); 
	
	HMAC(EVP_sha1(),(unsigned char *)(*key), sizeof(lwfs_key), 
			(unsigned char *)(&cap->data), datasize, 
			(unsigned char *)(newmac), &macsize); 

	/* compare the original mac to the new mac */
	if (memcmp(cap->mac, newmac, sizeof(lwfs_mac)) != 0) {
		return LWFS_ERR_VERIFYCAP; 
	}

	return LWFS_OK; 
}

/*============ Support functions ============*/

