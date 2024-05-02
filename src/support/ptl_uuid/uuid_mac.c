/*
**  OSSP uuid - Universally Unique Identifier
**  Copyright (c) 2004-2006 Ralf S. Engelschall <rse@engelschall.com>
**  Copyright (c) 2004-2006 The OSSP Project <http://www.ossp.org/>
**
**  This file is part of OSSP uuid, a library for the generation
**  of UUIDs which can found at http://www.ossp.org/pkg/lib/uuid/
**
**  Permission to use, copy, modify, and distribute this software for
**  any purpose with or without fee is hereby granted, provided that
**  the above copyright notice and this permission notice appear in all
**  copies.
**
**  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
**  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
**  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
**  IN NO EVENT SHALL THE AUTHORS AND COPYRIGHT HOLDERS AND THEIR
**  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
**  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
**  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
**  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
**  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
**  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
**  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
**  SUCH DAMAGE.
**
**  uuid_mac.c: Media Access Control (MAC) resolver implementation
*/

#include "config.h"
#include "uuid_mac.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif
#ifdef HAVE_NET_IF_ARP_H
#include <net/if_arp.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_IFADDRS_H
#include <ifaddrs.h>
#endif

#include PORTALS_HEADER
#include PORTALS_NAL_HEADER
#include PORTALS_RT_HEADER
   

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE (/*lint -save -e506*/ !FALSE /*lint -restore*/)
#endif

/* return the Media Access Control (MAC) address of
   the FIRST network interface card (NIC) */
int mac_address(unsigned char *data_ptr, size_t data_len)
{
    /* sanity check arguments */
    if (data_ptr == NULL || data_len < MAC_LEN)
        return FALSE;

#if defined(HAVE_PORTALS)
    /* If we have portals, use the nid for the mac addr */
    {
	static int initialized = FALSE;
	static ptl_handle_ni_t ni_handle; 
	static ptl_process_id_t process_id; 
	static int nid_size = sizeof(ptl_nid_t);


	if (initialized == FALSE) {

	    /* shouldn't this stuff work on catamount as well */
#if !defined(LIB_CATAMOUNT)
#endif
	    int max_interfaces; 
	    ptl_ni_limits_t desired, actual; 

	    /* This should not cause problems if called multiple times */
	    PtlInit(&max_interfaces); 

	    /* The Portals 3.3 reference guide says that multiple calls to 
	     *  PtlNIInit should return a valid ni_handle */
	    PtlNIInit(PTL_IFACE_DEFAULT, PTL_PID_ANY, &desired, &actual, &ni_handle); 

	    /* Get the ID of this process */
	    PtlGetId(ni_handle, &process_id); 

	    initialized = TRUE;
	}

	memset(data_ptr, 0, data_len);
	memcpy(&(data_ptr[2]), &process_id.nid, nid_size); 

	return TRUE;
    }


#elif defined(HAVE_IFADDRS_H) && defined(HAVE_NET_IF_DL_H) && defined(HAVE_GETIFADDRS)
    /* use getifaddrs(3) on BSD class platforms (xxxBSD, MacOS X, etc) */
    {
        struct ifaddrs *ifap;
        struct ifaddrs *ifap_head;
        const struct sockaddr_dl *sdl;
        unsigned char *ucp;
        int i;

        if (getifaddrs(&ifap_head) < 0)
            return FALSE;
        for (ifap = ifap_head; ifap != NULL; ifap = ifap->ifa_next) {
            if (ifap->ifa_addr != NULL && ifap->ifa_addr->sa_family == AF_LINK) {
                sdl = (const struct sockaddr_dl *)(void *)ifap->ifa_addr;
                ucp = (unsigned char *)(sdl->sdl_data + sdl->sdl_nlen);
                if (sdl->sdl_alen > 0) {
                    for (i = 0; i < MAC_LEN && i < sdl->sdl_alen; i++, ucp++)
                        data_ptr[i] = (unsigned char)(*ucp & 0xff);
                    freeifaddrs(ifap_head);
                    return TRUE;
                }
            }
        }
        freeifaddrs(ifap_head);
    }

#elif defined(HAVE_NET_IF_H) && defined(SIOCGIFHWADDR)
    /* use SIOCGIFHWADDR ioctl(2) on Linux class platforms */
    {
        struct ifreq ifr;
        struct sockaddr *sa;
        int s;
        int i;

        if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
            return FALSE;
        sprintf(ifr.ifr_name, "eth0");
        if (ioctl(s, SIOCGIFHWADDR, &ifr) < 0) {
            close(s);
            return FALSE;
        }
        sa = (struct sockaddr *)&ifr.ifr_addr;
        for (i = 0; i < MAC_LEN; i++)
            data_ptr[i] = (unsigned char)(sa->sa_data[i] & 0xff);
        close(s);
        return TRUE;
    }

#elif defined(SIOCGARP)
    /* use SIOCGARP ioctl(2) on SVR4 class platforms (Solaris, etc) */
    {
        char hostname[MAXHOSTNAMELEN];
        struct hostent *he;
        struct arpreq ar;
        struct sockaddr_in *sa;
        int s;
        int i;

        if (gethostname(hostname, sizeof(hostname)) < 0)
            return FALSE;
        if ((he = gethostbyname(hostname)) == NULL)
            return FALSE;
        if ((s = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
            return FALSE;
        memset(&ar, 0, sizeof(ar));
        sa = (struct sockaddr_in *)((void *)&(ar.arp_pa));
        sa->sin_family = AF_INET;
        memcpy(&(sa->sin_addr), *(he->h_addr_list), sizeof(struct in_addr));
        if (ioctl(s, SIOCGARP, &ar) < 0) {
            close(s);
            return FALSE;
        }
        close(s);
        if (!(ar.arp_flags & ATF_COM))
            return FALSE;
        for (i = 0; i < MAC_LEN; i++)
            data_ptr[i] = (unsigned char)(ar.arp_ha.sa_data[i] & 0xff);
        return TRUE;
    }
#endif

    return FALSE;
}

