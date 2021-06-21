/**\file n_network.c
 *  Network Engine
 *\author Castagnier Mickael
 *\version 1.0
 *\date 10/05/2005
 */

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#ifdef LINUX
#define _GNU_SOURCE
#include <unistd.h>
#include <string.h>
#endif
#include <sys/types.h>

#include "nilorea/n_network.h"
#include "nilorea/n_network_msg.h"
#include "nilorea/n_log.h"
#include "nilorea/n_hash.h"

#ifdef __windows__

/*
 * From https://stackoverflow.com/questions/3019977/convert-wchar-t-to-char/55715607#55715607 by Richard Bamford
 */
char* wchar_to_char(const wchar_t* pwchar)
{
    // get the number of characters in the string.
    int currentCharIndex = 0;
    char currentChar = pwchar[currentCharIndex];
    char *filePathC = NULL ;

    while (currentChar != '\0')
    {
        currentCharIndex++;
        currentChar = pwchar[currentCharIndex];
    }

    const int charCount = currentCharIndex + 1;

    // allocate a new block of memory size char (1 byte) instead of wide char (2 bytes)
    Malloc( filePathC, char, charCount );
    __n_assert( filePathC, return NULL );

    for (int i = 0; i < charCount; i++)
    {
        // convert to char (1 byte)
        char character = pwchar[i];

        *filePathC = character;

        filePathC += sizeof(char);

    }
    filePathC += '\0';

    filePathC -= (sizeof(char) * charCount);

    return filePathC;
}

#define neterrno WSAGetLastError()

#define netstrerror( code )({ \
        wchar_t *s = NULL; \
        FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, \
                NULL, code , \
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), \
                (LPWSTR)&s, 0, NULL); \
                char *netstr = wchar_to_char( s ); \
                LocalFree( s ); \
                netstr ; \
                })


/* if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 5) */
#if __GNUC__ <= 6 && __GNUC_MINOR__ <= 3

/*--------------------------------------------------------------------------------------

  By Marco Ladino - mladinox.. jan/2016


  MinGW 3.45 thru 4.5 versions, don't have the socket functions:

  --> inet_ntop(..)
  --> inet_pton(..)

  But with this adapted code using the original functions from FreeBSD,
  one can to use it in the C/C++ Applications, without problem..!

  This implementation, include tests for IPV4 and IPV6 addresses,
  and is full C/C++ compatible..

  --------------------------------------------------------------------------------------*/


/*	$OpenBSD: strlcpy.c,v 1.11 2006/05/05 15:27:38 millert Exp $	*/

/*-
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

//mladinox:: #include <sys/cdefs.h>
//mladinox:: __FBSDID("$FreeBSD: stable/9/sys/libkern/strlcpy.c 243811 2012-12-03 18:08:44Z delphij $");
//mladinox:: #include <sys/types.h>
//mladinox:: #include <sys/libkern.h>

/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t strlcpy(char *dst, const char *src, size_t siz)
{
    char *d = dst;
    const char *s = src;
    size_t n = siz;

    /* Copy as many bytes as will fit */
    if (n != 0)
    {
        while (--n != 0)
        {
            if ((*d++ = *s++) == '\0')
                break;
        }
    }

    /* Not enough room in dst, add NUL and traverse rest of src */
    if (n == 0)
    {
        if (siz != 0)
            *d = '\0';		/* NUL-terminate dst */
        while (*s++)
            ;
    }

    return(s - src - 1);	/* count does not include NUL */
}


/*
 * Copyright (c) 2004 by Internet Systems Consortium, Inc. ("ISC")
 * Copyright (c) 1996-1999 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */


//mladinox:: #if defined(LIBC_SCCS) && !defined(lint)
//mladinox:: static const char rcsid[] = "$Id: inet_ntop.c,v 1.3.18.2 2005/11/03 23:02:22 marka Exp $";
//mladinox:: #endif /* LIBC_SCCS and not lint */
//mladinox:: #include <sys/cdefs.h>
//mladinox:: __FBSDID("$FreeBSD: stable/9/sys/libkern/inet_ntop.c 213103 2010-09-24 15:01:45Z attilio $");
//mladinox:: #include <sys/param.h>
//mladinox:: #include <sys/socket.h>
//mladinox:: #include <sys/systm.h>
//mladinox:: #include <netinet/in.h>

/*%
 * WARNING: Don't even consider trying to compile this on a system where
 * sizeof(int) < 4.  sizeof(int) > 4 is fine; all the world's not a VAX.
 */

static char	*inet_ntop4(const unsigned char *src, char *dst, socklen_t size);
static char	*inet_ntop6(const unsigned char *src, char *dst, socklen_t size);

/* char *
 * inet_ntop(af, src, dst, size)
 *	convert a network format address to presentation format.
 * return:
 *	pointer to presentation format address (`dst'), or NULL (see errno).
 * author:
 *	Paul Vixie, 1996.
 */
char *inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
    switch (af)
    {
    case AF_INET:
        return (inet_ntop4((const unsigned char*)src, dst, size));
    case AF_INET6:
        return (inet_ntop6((const unsigned char*)src, dst, size));
    default:
        return (NULL);
    }
    /* NOTREACHED */
}

/* const char *
 * inet_ntop4(src, dst, size)
 *	format an IPv4 address
 * return:
 *	`dst' (as a const)
 * notes:
 *	(1) uses no statics
 *	(2) takes a u_char* not an in_addr as input
 * author:
 *	Paul Vixie, 1996.
 */
static char *inet_ntop4(const unsigned char *src, char *dst, socklen_t size)
{
    static const char fmt[] = "%u.%u.%u.%u";
    char tmp[sizeof "255.255.255.255"];
    int l;

    l = snprintf(tmp, sizeof(tmp), fmt, src[0], src[1], src[2], src[3]);
    if (l <= 0 || (socklen_t) l >= size)
    {
        return (NULL);
    }
    strlcpy(dst, tmp, size);
    return (dst);
}

/* const char *
 * inet_ntop6(src, dst, size)
 *	convert IPv6 binary address into presentation (printable) format
 * author:
 *	Paul Vixie, 1996.
 */
static char *inet_ntop6(const unsigned char *src, char *dst, socklen_t size)
{
    /*
     * Note that int32_t and int16_t need only be "at least" large enough
     * to contain a value of the specified size.  On some systems, like
     * Crays, there is no such thing as an integer variable with 16 bits.
     * Keep this in mind if you think this function should have been coded
     * to use pointer overlays.  All the world's not a VAX.
     */
    char tmp[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"], *tp;
    struct
    {
        int base, len;
    } best, cur;
#define NS_IN6ADDRSZ	16
#define NS_INT16SZ	2
    u_int words[NS_IN6ADDRSZ / NS_INT16SZ];
    int i;

    /*
     * Preprocess:
     *	Copy the input (bytewise) array into a wordwise array.
     *	Find the longest run of 0x00's in src[] for :: shorthanding.
     */
    memset(words, '\0', sizeof words);
    for (i = 0; i < NS_IN6ADDRSZ; i++)
        words[i / 2] |= (src[i] << ((1 - (i % 2)) << 3));
    best.base = -1;
    best.len = 0;
    cur.base = -1;
    cur.len = 0;
    for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++)
    {
        if (words[i] == 0)
        {
            if (cur.base == -1)
                cur.base = i, cur.len = 1;
            else
                cur.len++;
        }
        else
        {
            if (cur.base != -1)
            {
                if (best.base == -1 || cur.len > best.len)
                    best = cur;
                cur.base = -1;
            }
        }
    }
    if (cur.base != -1)
    {
        if (best.base == -1 || cur.len > best.len)
            best = cur;
    }
    if (best.base != -1 && best.len < 2)
        best.base = -1;

    /*
     * Format the result.
     */
    tp = tmp;
    for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++)
    {
        /* Are we inside the best run of 0x00's? */
        if (best.base != -1 && i >= best.base &&
                i < (best.base + best.len))
        {
            if (i == best.base)
                *tp++ = ':';
            continue;
        }
        /* Are we following an initial run of 0x00s or any real hex? */
        if (i != 0)
            *tp++ = ':';
        /* Is this address an encapsulated IPv4? */
        if (i == 6 && best.base == 0 && (best.len == 6 ||
                                         (best.len == 7 && words[7] != 0x0001) ||
                                         (best.len == 5 && words[5] == 0xffff)))
        {
            if (!inet_ntop4(src+12, tp, sizeof tmp - (tp - tmp)))
                return (NULL);
            tp += strlen(tp);
            break;
        }
        tp += sprintf(tp, "%x", words[i]);
    }
    /* Was it a trailing run of 0x00's? */
    if (best.base != -1 && (best.base + best.len) ==
            (NS_IN6ADDRSZ / NS_INT16SZ))
        *tp++ = ':';
    *tp++ = '\0';

    /*
     * Check for overflow, copy, and we're done.
     */
    if ((socklen_t)(tp - tmp) > size)
    {
        return (NULL);
    }
    strcpy(dst, tmp);
    return (dst);
}


//----------------
/*
 * Copyright (c) 2004 by Internet Systems Consortium, Inc. ("ISC")
 * Copyright (c) 1996,1999 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

//mladinox:: #if defined(LIBC_SCCS) && !defined(lint)
//mladinox:: static const char rcsid[] = "$Id: inet_pton.c,v 1.3.18.2 2005/07/28 07:38:07 marka Exp $";
//mladinox:: #endif /* LIBC_SCCS and not lint */
//mladinox:: #include <sys/cdefs.h>
//mladinox:: __FBSDID("$FreeBSD: stable/9/sys/libkern/inet_pton.c 213103 2010-09-24 15:01:45Z attilio $");
//mladinox:: #include <sys/param.h>
//mladinox:: #include <sys/socket.h>
//mladinox:: #include <sys/systm.h>
//mladinox:: #include <netinet/in.h>
//mladinox:: #if __FreeBSD_version < 700000
//mladinox:: #define strchr index
//mladinox:: #endif

/*%
 * WARNING: Don't even consider trying to compile this on a system where
 * sizeof(int) < 4.  sizeof(int) > 4 is fine; all the world's not a VAX.
 */

static int	inet_pton4(const char *src, u_char *dst);
static int	inet_pton6(const char *src, u_char *dst);

/* int
 * inet_pton(af, src, dst)
 *	convert from presentation format (which usually means ASCII printable)
 *	to network format (which is usually some kind of binary format).
 * return:
 *	1 if the address was valid for the specified address family
 *	0 if the address wasn't valid (`dst' is untouched in this case)
 *	-1 if some other error occurred (`dst' is untouched in this case, too)
 * author:
 *	Paul Vixie, 1996.
 */
int inet_pton(int af, const char *src, void *dst)
{
    switch (af)
    {
    case AF_INET:
        return (inet_pton4(src, (unsigned char *)dst));
    case AF_INET6:
        return (inet_pton6(src, (unsigned char *)dst));
    default:
        return (-1);
    }
    /* NOTREACHED */
}

/* int
 * inet_pton4(src, dst)
 *	like inet_aton() but without all the hexadecimal and shorthand.
 * return:
 *	1 if `src' is a valid dotted quad, else 0.
 * notice:
 *	does not touch `dst' unless it's returning 1.
 * author:
 *	Paul Vixie, 1996.
 */
static int inet_pton4(const char *src, u_char *dst)
{
    static const char digits[] = "0123456789";
    int saw_digit, octets, ch;
#define NS_INADDRSZ	4
    u_char tmp[NS_INADDRSZ], *tp;

    saw_digit = 0;
    octets = 0;
    *(tp = tmp) = 0;
    while ((ch = *src++) != '\0')
    {
        const char *pch;

        if ((pch = strchr(digits, ch)) != NULL)
        {
            u_int uiNew = *tp * 10 + (pch - digits);

            if (saw_digit && *tp == 0)
                return (0);
            if (uiNew > 255)
                return (0);
            *tp = uiNew;
            if (!saw_digit)
            {
                if (++octets > 4)
                    return (0);
                saw_digit = 1;
            }
        }
        else if (ch == '.' && saw_digit)
        {
            if (octets == 4)
                return (0);
            *++tp = 0;
            saw_digit = 0;
        }
        else
            return (0);
    }
    if (octets < 4)
        return (0);
    memcpy(dst, tmp, NS_INADDRSZ);
    return (1);
}

/* int
 * inet_pton6(src, dst)
 *	convert presentation level address to network order binary form.
 * return:
 *	1 if `src' is a valid [RFC1884 2.2] address, else 0.
 * notice:
 *	(1) does not touch `dst' unless it's returning 1.
 *	(2) :: in a full address is silently ignored.
 * credit:
 *	inspired by Mark Andrews.
 * author:
 *	Paul Vixie, 1996.
 */
static int inet_pton6(const char *src, u_char *dst)
{
    static const char xdigits_l[] = "0123456789abcdef",
                                    xdigits_u[] = "0123456789ABCDEF";
#define NS_IN6ADDRSZ	16
#define NS_INT16SZ	2
    u_char tmp[NS_IN6ADDRSZ], *tp, *endp, *colonp;
    const char *curtok;
    int ch, seen_xdigits;
    u_int val;

    memset((tp = tmp), '\0', NS_IN6ADDRSZ);
    endp = tp + NS_IN6ADDRSZ;
    colonp = NULL;
    /* Leading :: requires some special handling. */
    if (*src == ':')
        if (*++src != ':')
            return (0);
    curtok = src;
    seen_xdigits = 0;
    val = 0;
    while ((ch = *src++) != '\0')
    {
const char *xdigits :
        const char *pch ;

        if ((pch = strchr((xdigits = xdigits_l), ch)) == NULL)
            pch = strchr((xdigits = xdigits_u), ch);
        if (pch != NULL)
        {
            val <<= 4;
            val |= (pch - xdigits);
            if (++seen_xdigits > 4)
                return (0);
            continue;
        }
        if (ch == ':')
        {
            curtok = src;
            if (!seen_xdigits)
            {
                if (colonp)
                    return (0);
                colonp = tp;
                continue;
            }
            else if (*src == '\0')
            {
                return (0);
            }
            if (tp + NS_INT16SZ > endp)
                return (0);
            *tp++ = (u_char) (val >> 8) & 0xff;
            *tp++ = (u_char) val & 0xff;
            seen_xdigits = 0;
            val = 0;
            continue;
        }
        if (ch == '.' && ((tp + NS_INADDRSZ) <= endp) &&
                inet_pton4(curtok, tp) > 0)
        {
            tp += NS_INADDRSZ;
            seen_xdigits = 0;
            break;	/*%< '\\0' was seen by inet_pton4(). */
        }
        return (0);
    }
    if (seen_xdigits)
    {
        if (tp + NS_INT16SZ > endp)
            return (0);
        *tp++ = (u_char) (val >> 8) & 0xff;
        *tp++ = (u_char) val & 0xff;
    }
    if (colonp != NULL)
    {
        /*
         * Since some memmove()'s erroneously fail to handle
         * overlapping regions, we'll do the shift by hand.
         */
        const int n = tp - colonp;
        int i;

        if (tp == endp)
            return (0);
        for (i = 1; i <= n; i++)
        {
            endp[- i] = colonp[n - i];
            colonp[n - i] = 0;
        }
        tp = endp;
    }
    if (tp != endp)
        return (0);
    memcpy(dst, tmp, NS_IN6ADDRSZ);
    return (1);
}

#endif /* if GCC_VERSION <= 4.5 */

#else /* ifdef __windows__ */

#include <sys/types.h>
#include <sys/wait.h>

/*! Keep it compatible with bsd like */
#define neterrno errno
/*! BSD style errno string NO WORKING ON REDHAT */
/* #define netstrerror( code )({ \
   size_t errmsglen = 512 ; // strerrorlen_s( code ) + 1  ; \
   char *errmsg = NULL ; \
   Malloc( errmsg , char , errmsglen ); \
   if( errmsg ) \
   { \
   strerror_s( errmsg , errmsglen , code ); \
   } \
   errmsg ; \
   }) */

#define netstrerror( code )({ \
        char *errmsg = NULL ; \
        errno = 0 ; \
        errmsg = strdup( strerror( code ) ); \
        if( errno == ENOMEM ) \
        { \
        errmsg = NULL ; \
        } \
        errmsg ; \
        })

void netw_sigchld_handler( int sig )
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
    n_log( LOG_DEBUG, "Signal %d", sig );
}

#endif /* ifndef windows */

/*!\fn NETWORK *netw_new( int send_list_limit , int recv_list_limit )
 *\brief Return an empty allocated network ready to be netw_closed
 *\param send_list_limit Thread engine number of tosend message limit
 *\param recv_list_limit Thread engine number of received message limit
 *\return NULL or a new empty network
 */
NETWORK *netw_new( int send_list_limit, int recv_list_limit )
{
    NETWORK *netw = NULL ;

    Malloc( netw, NETWORK, 1 );
    __n_assert( netw, return NULL );

    /* netw itself */
    netw -> nb_pending = -1 ;
    netw -> mode = -1 ;
    netw -> user_id = -1 ;
    netw -> nb_running_threads = 0 ;
    netw -> state = NETW_EXITED ;
    netw -> threaded_engine_status = NETW_THR_ENGINE_STOPPED ;

    /* netw -> link */
    netw -> link . sock = -1 ;
    netw -> link . port =
        netw -> link . ip = NULL ;
    memset( &netw -> link . hints, 0, sizeof( struct addrinfo ) );
    memset( &netw -> link . raddr, 0, sizeof( struct sockaddr_storage ) );

    /*initiliaze mutexs*/
    if ( pthread_mutex_init( &netw -> sendbolt, NULL ) != 0 )
    {
        n_log( LOG_ERR, "Error initializing netw -> sendbolt" );
        Free( netw );
        return NULL ;
    }
    /*initiliaze mutexs*/
    if ( pthread_mutex_init( &netw -> recvbolt, NULL ) != 0 )
    {
        n_log( LOG_ERR, "Error initializing netw -> recvbolt" );
        pthread_mutex_destroy( &netw -> sendbolt );
        Free( netw );
        return NULL ;
    }
    /*initiliaze mutexs*/
    if ( pthread_mutex_init( &netw -> eventbolt, NULL ) != 0 )
    {
        n_log( LOG_ERR, "Error initializing netw -> eventbolt" );
        pthread_mutex_destroy( &netw -> sendbolt );
        pthread_mutex_destroy( &netw -> recvbolt );
        Free( netw );
        return NULL ;
    }
    /* initialize send sem bolt */
    if( sem_init(&netw -> send_blocker, 0, 0 ) != 0 )
    {
        n_log( LOG_ERR, "Error initializing netw -> eventbolt" );
        pthread_mutex_destroy( &netw -> eventbolt );
        pthread_mutex_destroy( &netw -> sendbolt );
        pthread_mutex_destroy( &netw -> recvbolt );
        Free( netw );
        return NULL ;
    }
    /*initialize queues */
    netw -> recv_buf = new_generic_list( recv_list_limit );
    if( ! netw -> recv_buf )
    {
        n_log( LOG_ERR, "Error when creating receive list with %d item limit", recv_list_limit );
        netw_close( &netw );
        return NULL ;
    }
    netw -> send_buf = new_generic_list( send_list_limit );
    if( !netw -> send_buf )
    {
        n_log( LOG_ERR, "Error when creating send list with %d item limit", send_list_limit );
        netw_close( &netw );
        return NULL ;
    }
    netw -> pools = new_generic_list( -1 );
    if( !netw -> pools )
    {
        n_log( LOG_ERR, "Error when creating pools list" );
        netw_close( &netw );
        return NULL ;
    }
    netw -> addr_infos_loaded = 0 ;
    netw -> send_queue_consecutive_wait = 0 ;
    netw -> so_reuseaddr = -1 ;
    netw -> tcpnodelay = -1 ;
    netw -> so_sndbuf = -1 ;
    netw -> so_rcvbuf = -1 ;
    netw -> so_rcvtimeo = -1 ;
    netw -> so_sndtimeo = -1 ;
    netw -> so_linger = -1 ;
    netw -> deplete_timeout = -1 ;
    netw -> crypto_mode = NETW_CRYPTO_NONE ;
    netw -> crypto_algo = NETW_ENCRYPT_NONE ;

    netw -> send_data = &send_data;
    netw -> recv_data = &recv_data;

    /* blocking by default */
    netw -> link . is_blocking = 1 ;
    return netw ;

} /* netw_new() */



/*!\fn void *get_in_addr(struct addrinfo *sa)
 *\brief get sockaddr, IPv4 or IPv6
 *\param sa addrinfo to get
 */
void *get_in_addr(struct sockaddr *sa)
{
    return sa->sa_family == AF_INET
           ? (void *) &(((struct sockaddr_in*)sa)->sin_addr)
           : (void *) &(((struct sockaddr_in6*)sa)->sin6_addr);
}


/*!\fn int netw_init_wsa( int mode , int v1 , int v2 )
 *\brief Do not directly use, internal api. Initialize winsock dll loading on windows if needed.
 *\param mode 1 for opening 0 for close 2 for status
 *\param v1 First digit of version requested
 *\param v2 Second digit of version requested
 *\return TRUE on success FALSE on error
 */
int netw_init_wsa( int mode, int v1, int v2 )
{
    int compiler_warning_suppressor = 0 ;
#if !defined( __linux__ ) && !defined( __sun ) && !defined( _AIX )
    static WSADATA WSAdata; /*WSA world*/
    static int WSA_IS_INITIALIZED = 0; /*status checking*/

    switch ( mode )
    {
    default:
    /*returning WSA status*/
    case 2 :
        return WSA_IS_INITIALIZED;
        break;
    /*loading WSA dll*/
    case 1 :
        if ( WSA_IS_INITIALIZED == 1 )
            return TRUE; /*already loaded*/
        if ( ( WSAStartup( MAKEWORD( v1, v2 ), &WSAdata ) ) != 0 )
        {
            WSA_IS_INITIALIZED = 0;
            return FALSE;
        }
        else
        {
            WSA_IS_INITIALIZED = 1 ;
            return TRUE;
        }
        break;
    /*unloading (closing) WSA */
    case 0 :
        if ( WSA_IS_INITIALIZED == 0 )
            return TRUE; /*already CLEANED or not loaded */
        if ( WSACleanup() == 0 )
        {
            WSA_IS_INITIALIZED = 0;
            return TRUE;
        }
        break;
    } /*switch(...)*/
#endif /*#ifndef __linux__ __sun _AIX */
    compiler_warning_suppressor = mode + v1 + v2 ;
    compiler_warning_suppressor = TRUE ;
    return compiler_warning_suppressor ;
} /*netw_init_wsa(...)*/



/*!\fn int netw_set_blocking( NETWORK *netw , unsigned long int is_blocking )
 *\brief Modify blocking socket mode
 *\param netw The network to configure
 *\param is_blocking 0 NON BLOCk , 1 BLOCK
 *\return TRUE or FALSE
 */
int netw_set_blocking( NETWORK *netw, unsigned long int is_blocking )
{
    __n_assert( netw, return FALSE );

    int error = 0 ;
    char *errmsg = NULL ;

    if( netw -> link . is_blocking == is_blocking )
        return TRUE ;

#if defined(__linux__) || defined(__sun)
    int flags = fcntl( netw -> link . sock, F_GETFL, 0 );
    if ( (flags &O_NONBLOCK) && !is_blocking )
    {
        n_log( LOG_INFO, "socket %d was already in non-blocking mode", netw -> link . sock );
        /* in case we missed it, let's update the link mode */
        netw -> link. is_blocking = 0 ;
        return TRUE;
    }
    if (!(flags &O_NONBLOCK) &&  is_blocking )
    {
        /* in case we missed it, let's update the link mode */
        netw -> link. is_blocking = 1 ;
        n_log( LOG_INFO, "socket %d was already in non-blocking mode", netw -> link . sock );
        return TRUE;
    }
    if( fcntl(netw -> link . sock, F_SETFL, is_blocking ? flags ^ O_NONBLOCK : flags | O_NONBLOCK) == -1 )
    {
        error = neterrno ;
        errmsg = netstrerror( error );
        n_log( LOG_ERR, "couldn't set blocking mode %d on %d: %s", is_blocking, netw -> link . sock, _str( errmsg ) );
        FreeNoLog( errmsg );
        return FALSE ;
    }
#else
    unsigned long int blocking = 1 - is_blocking ;
    int res = ioctlsocket( netw -> link . sock, FIONBIO, &blocking );
    error = neterrno ;
    if( res != NO_ERROR )
    {
        errmsg = netstrerror( error );
        n_log( LOG_ERR, "ioctlsocket failed with error: %ld , neterrno: %s", res, _str( errmsg ) );
        FreeNoLog( errmsg );
        netw_close( &netw );
        return FALSE ;
    }
#endif
    netw -> link. is_blocking = is_blocking ;
    return TRUE ;
} /* netw_set_blocking */



/*!\fn int netw_setsockopt( NETWORK *netw  , int optname , int value )
 *\brief Modify common socket options on the given netw
 *\param netw The socket to configure
 *\param optname NETWORK_DEPLETE_TIMEOUT,NETWORK_CONSECUTIVE_SEND_TIMEOUT ,SO_REUSEADDR,TCP_NODELAY,SO_SNDBUF,SO_RCVBUF,SO_LINGER,SO_RCVTIMEO,SO_SNDTIMEO. Please refer to man setsockopt for details
 *\value The value of the socket parameter
 *\return TRUE or FALSE
 */
int netw_setsockopt( NETWORK *netw, int optname, int value )
{
    __n_assert( netw, return FALSE );

    int error = 0 ;
    char *errmsg = NULL ;

    switch( optname )
    {
    case TCP_NODELAY :
        if( value >= 0 )
        {
            /* disable naggle algorithm */
            if ( setsockopt( netw -> link . sock, IPPROTO_TCP, TCP_NODELAY, ( const char * ) &value, sizeof( value ) ) == -1 )
            {
                error = neterrno ;
                errmsg = netstrerror( error );
                n_log( LOG_ERR, "Error from setsockopt(TCP_NODELAY) on socket %d. neterrno: %s", netw -> link . sock, _str( errmsg ) );
                FreeNoLog( errmsg );
                return FALSE ;
            }
        }
        netw -> tcpnodelay = value ;
        break ;
    case SO_SNDBUF :
        /* socket sending buffer size */
        if( value >= 0 )
        {
            if ( setsockopt ( netw -> link . sock, SOL_SOCKET, SO_SNDBUF, ( const char * ) &value, sizeof( value ) ) == -1 )
            {
                error = neterrno ;
                errmsg = netstrerror( error );
                n_log( LOG_ERR, "Error from setsockopt(SO_SNDBUF) on socket %d. neterrno: %s", netw -> link . sock, _str( errmsg ) );
                return FALSE ;
            }
        }
        netw -> so_sndbuf = value ;
        break ;
    case SO_RCVBUF :
        /* socket receiving buffer */
        if( value >= 0 )
        {
            if ( setsockopt ( netw -> link . sock, SOL_SOCKET, SO_RCVBUF, ( const char * ) &value, sizeof( value ) ) == -1 )
            {
                error = neterrno ;
                errmsg = netstrerror( error );
                n_log( LOG_ERR, "Error from setsockopt(SO_RCVBUF) on socket %d. neterrno: %s", netw -> link . sock, _str( errmsg ) );
                FreeNoLog( errmsg );
                return FALSE ;
            }
        }
        netw -> so_rcvbuf = value ;
        break ;
    case SO_REUSEADDR :
        /* lose the pesky "Address already in use" error message*/
        /* Disabled SO_REUSEPORT because not widely supported */
        if ( setsockopt( netw -> link . sock, SOL_SOCKET, SO_REUSEADDR, (char *)&value, sizeof( value  ) ) == -1 )
        {
            error=neterrno ;
            errmsg = netstrerror( error );
            n_log( LOG_ERR, "Error from setsockopt(SO_REUSEADDR) on socket %d. neterrno: %s", netw -> link . sock, _str( errmsg ) );
            FreeNoLog( errmsg );
            return FALSE ;
        }
        netw -> so_reuseaddr = value ;
        break ;
    case SO_LINGER :
    {
        struct linger ling;
        if( value < 0 )
        {
            ling.l_onoff=0;
            ling.l_linger=0;
        }
        else if( value == 0 )
        {
            ling.l_onoff=1;
            ling.l_linger=0;
        }
        else
        {
            ling.l_onoff=1;
            ling.l_linger=value;
        }
#ifndef __windows__
        if( setsockopt(netw -> link . sock, SOL_SOCKET, SO_LINGER, &ling, sizeof( ling ) ) == -1 )
        {
            error=neterrno ;
            errmsg = netstrerror( error );
            n_log( LOG_ERR, "Error from setsockopt(SO_LINGER) on socket %d. neterrno: %s", netw -> link . sock,_str( errmsg ) );
            FreeNoLog( errmsg );
            return FALSE ;
        }
#else
        if( setsockopt(netw -> link . sock, SOL_SOCKET, SO_LINGER, (const char *)&ling, sizeof( ling ) ) == -1 )
        {
            error=neterrno ;
            errmsg = netstrerror( error );
            n_log( LOG_ERR, "Error from setsockopt(SO_LINGER) on socket %d. neterrno: %s", netw -> link . sock,_str( errmsg ) );
            FreeNoLog( errmsg );
            return FALSE ;
        }
#endif // __windows__
        netw -> so_linger = value ;
    }
    break ;
    case SO_RCVTIMEO :
        if( value >= 0 )
        {
#ifndef __windows__
            {
                struct timeval tv;
                tv.tv_sec = value ;
                tv.tv_usec = 0;
                if( setsockopt(netw -> link . sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv) == -1 )
                {
                    error=neterrno ;
                    errmsg = netstrerror( error );
                    n_log( LOG_ERR, "Error from setsockopt(SO_RCVTIMEO) on socket %d. neterrno: %s", netw -> link . sock, _str( errmsg ) );
                    FreeNoLog( errmsg );
                    return FALSE ;
                }
            }
#else
            if( setsockopt(netw -> link . sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&value, sizeof value ) == -1 )
            {
                error=neterrno ;
                errmsg = netstrerror( error );
                n_log( LOG_ERR, "Error from setsockopt(SO_RCVTIMEO) on socket %d. neterrno: %s", netw -> link . sock, _str( errmsg ) );
                FreeNoLog( errmsg );
                return FALSE ;
            }
#endif
        }
        netw -> so_rcvtimeo = value ;
        break ;
    case SO_SNDTIMEO :
        if( value >= 0 )
        {
#ifndef __windows__
            {
                struct timeval tv;
                tv.tv_sec = value ;
                tv.tv_usec = 0;

                if( setsockopt(netw -> link . sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv) == -1 )
                {
                    error=neterrno ;
                    errmsg = netstrerror( error );
                    n_log( LOG_ERR, "Error from setsockopt(SO_SNDTIMEO) on socket %d. neterrno: %s", netw -> link . sock, _str( errmsg ) );
                    FreeNoLog( errmsg );
                    return FALSE ;
                }
            }
#else
            if( setsockopt(netw -> link . sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&value, sizeof value ) == -1 )
            {
                error=neterrno ;
                errmsg = netstrerror( error );
                n_log( LOG_ERR, "Error from setsockopt(SO_SNDTIMEO) on socket %d. neterrno: %s", netw -> link . sock, _str( errmsg ) );
                FreeNoLog( errmsg );
                return FALSE ;
            }

#endif
        }
        netw -> so_sndtimeo = value ;
        break ;
    case NETWORK_DEPLETE_TIMEOUT:
        netw -> deplete_timeout = value ;
        break;
    case NETWORK_CONSECUTIVE_SEND_TIMEOUT:
        netw -> send_queue_consecutive_wait = value ;
        break ;

    default:
        n_log( LOG_ERR, "%d is not a supported setsockopt", optname );
        return FALSE ;
    }
    return TRUE ;
} /* netw_set_sock_opt */



#ifdef HAVE_OPENSSL
/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2019, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/
/* <DESC>
 * Show the required mutex callback setups for GnuTLS and OpenSSL when using
 * libcurl multi-threaded.
 * </DESC>
 */
/* A multi-threaded example that uses pthreads and fetches 4 remote files at
 * once over HTTPS. The lock callbacks and stuff assume OpenSSL <1.1 or GnuTLS
 * (libgcrypt) so far.
 *
 * OpenSSL docs for this:
 *   https://www.openssl.org/docs/man1.0.2/man3/CRYPTO_num_locks.html
 * gcrypt docs for this:
 *   https://gnupg.org/documentation/manuals/gcrypt/Multi_002dThreading.html
 */

/* we have this global to let the callback get easy access to it */
static pthread_mutex_t *lockarray;

static void lock_callback(int mode, int type, char *file, int line)
{
    (void)file;
    (void)line;
    if(mode & CRYPTO_LOCK)
    {
        pthread_mutex_lock(&(lockarray[type]));
    }
    else
    {
        pthread_mutex_unlock(&(lockarray[type]));
    }
}

static unsigned long thread_id(void)
{
    unsigned long ret;

    ret = (unsigned long)pthread_self();
    return ret;
}

static void init_locks(void)
{
    int i;

    lockarray = (pthread_mutex_t *)OPENSSL_malloc(CRYPTO_num_locks() *
                sizeof(pthread_mutex_t));
    for(i = 0; i<CRYPTO_num_locks(); i++)
    {
        pthread_mutex_init(&(lockarray[i]), NULL);
    }

    CRYPTO_set_id_callback((unsigned long (*)())thread_id);
    CRYPTO_set_locking_callback((void (*)())lock_callback);
}

static void kill_locks(void)
{
    int i;

    CRYPTO_set_locking_callback(NULL);
    for(i = 0; i<CRYPTO_num_locks(); i++)
        pthread_mutex_destroy(&(lockarray[i]));

    OPENSSL_free(lockarray);
}


/*!\fn int netw_init_openssl( void )
 *\brief Do not directly use, internal api. Initialize openssl
 *\return TRUE
 */
int netw_init_openssl( void )
{
    static int OPENSSL_IS_INITIALIZED = 0; /*status checking*/

    if ( OPENSSL_IS_INITIALIZED == 1 )
        return TRUE; /*already loaded*/

#ifdef HAVE_OPENSSL
    SSL_library_init();
    SSL_load_error_strings();
    ERR_load_BIO_strings();
    OpenSSL_add_all_algorithms();
    OpenSSL_add_all_algorithms();
    init_locks();
#endif

    OPENSSL_IS_INITIALIZED = 1 ;

    return TRUE ;
} /*netw_init_openssl(...)*/

/*!\fn int netw_unload_openssl( void )
 *\brief Do not directly use, internal api. Initialize openssl
 *\return TRUE
 */
int netw_init_openssl( void )
{
    static int OPENSSL_IS_INITIALIZED = 0; /*status checking*/

    if ( OPENSSL_IS_INITIALIZED == 1 )
        return TRUE; /*already loaded*/

#ifdef HAVE_OPENSSL
    SSL_load_error_strings();
    SSL_library_init();
    ERR_load_BIO_strings();
    OpenSSL_add_all_algorithms();
    OpenSSL_add_all_algorithms();
    init_locks();
#endif

    OPENSSL_IS_INITIALIZED = 1 ;

    return TRUE ;
} /*netw_init_openssl(...)*/

int netw_set_crypto( NETWORK *netw, char *key, char *certificat, char *vigenere )
{
    __n_assert( netw, return FALSE );

    if( key && strlen( key ) > 0 )
    {
        netw -> key = strdup( key );
    }
    if( certificat && strlen( certificat ) > 0 )
    {
        netw -> certificat = strdup( certificat );
    }
    if( vigenere && strlen( vigenere ) > 0 )
    {
        netw -> vigenere = strdup( vigenere );
    }

    netw_init_openssl();

    netw -> method = TLSv1_2_method();         /* create new server-method instance */
    netw -> ctx    = SSL_CTX_new(method);      /* create new context from method */
    if ( ctx == NULL )
    {
        unsigned long error = 0 ;
        while( error = ERR_get_error() )
        {
            n_log( LOG_ERR, "%s on socket %d", ERR_reason_error_string( ERR_get_error(), NULL ) );
        }

        EVP_CIPHER_CTX_free( netw -> ctx  );
        return FALSE ;
    }





} /* netw_set_crypto */


#endif // HAVE_OPENSSL WIP



/*!\fn int netw_connect_ex(NETWORK **netw , char *host , char *port, int send_list_limit , int recv_list_limit , int ip_version )
 *\brief Use this to connect a NETWORK to any listening one
 *\param netw a NETWORK *object
 *\param host Host or IP to connect to
 *\param port Port to use to connect
 *\param send_list_limit Internal sending list maximum number of item. 0 or negative for unrestricted
 *\param recv_list_limit Internal receiving list maximum number of item. 0 or negative for unrestricted
 *\param ip_version NETWORK_IPALL for both ipv4 and ipv6 , NETWORK_IPV4 or NETWORK_IPV6
 *\return TRUE or FALSE
 */
int netw_connect_ex( NETWORK **netw, char *host, char *port, int send_list_limit, int recv_list_limit, int ip_version )
{
    int error = 0, net_status = 0;
    char *errmsg = NULL ;

    /*do not work over an already used netw*/
    if( (*netw) )
    {
        n_log( LOG_ERR, "Unable to allocate (*netw), already existing. You must use empty NETWORK *structs." );
        return FALSE ;
    }

    /*creating array*/
    (*netw) = netw_new( send_list_limit, recv_list_limit );
    __n_assert( netw&&(*netw), return FALSE );

    /*checking WSA when under windows*/
    if ( netw_init_wsa( 1, 2, 2 ) == FALSE )
    {
        n_log( LOG_ERR, "Unable to load WSA dll's" );
        Free( (*netw) );
        return FALSE ;
    }

    /* choose ip version */
    if( ip_version == NETWORK_IPV4 )
    {
        (*netw) -> link . hints . ai_family   = AF_INET ;     /* Allow IPv4 */
    }
    else if( ip_version == NETWORK_IPV6 )
    {
        (*netw) -> link . hints . ai_family   = AF_INET6 ;     /* Allow IPv6 */
    }
    else
    {
        /* NETWORK_ALL or unknown value */
        (*netw) -> link . hints . ai_family   = AF_UNSPEC;     /* Allow IPv4 or IPv6 */
    }

    (*netw) -> link . hints . ai_socktype = SOCK_STREAM;
    (*netw) -> link . hints . ai_protocol = IPPROTO_TCP;
    (*netw) -> link . hints . ai_flags    = AI_PASSIVE;
    (*netw) -> link . hints . ai_canonname = NULL;
    (*netw) -> link . hints . ai_next = NULL;

    /* Note: on some system, i.e Solaris, it WILL show leak in getaddrinfo.
     * Testing it inside a 1,100 loop showed not effect on the amount of leaked
     * memory */
    error = getaddrinfo( host, port, &(*netw) -> link . hints, &(*netw) -> link . rhost );
    if( error != 0 )
    {
        n_log( LOG_ERR, "Error when resolving %s:%s getaddrinfo: %s", host, port, gai_strerror( error ) );
        netw_close( &(*netw) );
        return FALSE ;
    }
    (*netw) -> addr_infos_loaded = 1 ;
    Malloc( (*netw) -> link . ip, char, 64 );
    __n_assert( (*netw) -> link . ip, netw_close( &(*netw ) ); return FALSE );

    /* getaddrinfo() returns a list of address structures. Try each address until we successfully connect. If socket or connect fails, we close the socket and try the next address. */
    struct addrinfo *rp = NULL ;
    for( rp = (*netw) -> link . rhost ; rp != NULL ; rp = rp -> ai_next )
    {
        int sock = socket( rp -> ai_family, rp -> ai_socktype, rp->ai_protocol );
        if( sock == -1 )
        {
            error = neterrno ;
            errmsg = netstrerror( error );
            n_log( LOG_ERR, "Error while trying to make a socket: %s", _str( errmsg ) );
            FreeNoLog( errmsg );
            continue ;
        }

        (*netw) -> link . sock = sock ;

        net_status = connect( sock, rp -> ai_addr, rp -> ai_addrlen );
        if( net_status == -1 )
        {
            error = neterrno ;
            errmsg = netstrerror( error );
            n_log( LOG_INFO, "connecting to %s:%s : %s", host, port, _str( errmsg ) );
            FreeNoLog( errmsg );
            closesocket( sock );
            (*netw) -> link . sock = -1 ;
            continue ;
        }
        else
        {
            /*storing connected port and ip adress*/
            if(!inet_ntop( rp->ai_family, get_in_addr( rp -> ai_addr ), (*netw) -> link . ip, 64 ) )
            {
                error = neterrno ;
                errmsg = netstrerror( error );
                n_log( LOG_ERR, "inet_ntop: %p , %s", rp, _str( errmsg ) );
                FreeNoLog( errmsg );
            }
            break;                  /* Success */
        }
    }
    if( rp == NULL )
    {
        /* No address succeeded */
        n_log( LOG_ERR, "Couldn't connect to %s:%s : no adress succeeded", host, port );
        netw_close( &(*netw) );
        return FALSE ;
    }

    (*netw)->link . port = strdup( port );
    __n_assert( (*netw) -> link . port, netw_close( &(*netw ) ); return FALSE );

#ifdef HAVE_OPENSSL
    (*netw) -> key = NULL ;
    (*netw) -> certificat = NULL ;
#endif

    (*netw) -> vigenere_key = NULL ;


    n_log( LOG_DEBUG, "Connected to %s:%s", (*netw) -> link . ip, (*netw) -> link . port );

    netw_set( (*netw), NETW_CLIENT|NETW_RUN|NETW_THR_ENGINE_STOPPED );

    return TRUE ;
} /* netw_connect_ex(...)*/



/*!\fn int netw_connect( NETWORK **netw , char *host , char *port , int ip_version )
 *\brief Use this to connect a NETWORK to any listening one, unrestricted send/recv lists
 *\param netw a NETWORK *object
 *\param host Host or IP to connect to
 *\param port Port to use to connect
 *\param ip_version NETWORK_IPALL for both ipv4 and ipv6 , NETWORK_IPV4 or NETWORK_IPV6
 *\return TRUE or FALSE
 */
int netw_connect( NETWORK **netw, char *host, char *port, int ip_version )
{
    n_log( LOG_INFO, "Trying to connect to %s : %s", _str( host ), _str( port ) );
    return netw_connect_ex( &(*netw), host, port, -1, -1, ip_version );
} /* netw_connect() */



/*!\fn int netw_get_state( NETWORK *netw , int *state , int *thr_engine_status )
 *\brief Get the state of a network
 *\param netw The NETWORK *connection to query
 *\param state pointer to network status storage , NETW_RUN , NETW_EXIT_ASKED , NETW_EXITED
 *\param thr_engine_status pointer to network thread engine status storage ,NETW_THR_ENGINE_STARTED , NETW_THR_ENGINE_STOPPED
 *\return TRUE or FALSE
 */
int netw_get_state( NETWORK *netw, int *state, int *thr_engine_status )
{
    if( netw )
    {
        pthread_mutex_lock( &netw -> sendbolt );
        if( state )
            (*state) = netw -> state ;
        if( thr_engine_status )
            (*thr_engine_status) = netw -> threaded_engine_status ;
        pthread_mutex_unlock( &netw -> sendbolt );
        return TRUE ;
    }
    else
    {
        n_log( LOG_ERR, "Can't get status of a NULL network" );
    }
    return FALSE ;
} /*netw_get_state() */



/*!\fn int netw_set( NETWORK *netw , int flag )
 *\brief Restart or reset the specified network ability
 *\param netw The NETWORK *connection to modify
 *\param flag NETW_EMPTY_SENDBUF, NETW_EMPTY_RECVBUF, NETW_RUN , NETW_EXIT_ASKED , NETW_EXITED
 *\return TRUE or FALSE
 */
int netw_set( NETWORK *netw, int flag )
{
    if( flag & NETW_EMPTY_SENDBUF )
    {
        pthread_mutex_lock( &netw -> sendbolt );
        if( netw -> send_buf )
            list_empty( netw -> send_buf );
        pthread_mutex_unlock( &netw -> sendbolt );
    };
    if( flag & NETW_EMPTY_RECVBUF )
    {
        pthread_mutex_lock( &netw -> recvbolt );
        if( netw -> recv_buf )
            list_empty( netw -> recv_buf );
        pthread_mutex_unlock( &netw -> recvbolt );
    }
    if( flag & NETW_DESTROY_SENDBUF )
    {
        pthread_mutex_lock( &netw -> sendbolt );
        if( netw -> send_buf )
            list_destroy( &netw -> send_buf );
        pthread_mutex_unlock( &netw -> sendbolt );
    };
    if( flag & NETW_DESTROY_RECVBUF )
    {
        pthread_mutex_lock( &netw -> recvbolt );
        if( netw -> recv_buf )
            list_destroy( &netw -> recv_buf );
        pthread_mutex_unlock( &netw -> recvbolt );
    }
    pthread_mutex_lock( &netw -> eventbolt );
    if( flag & NETW_CLIENT )
    {
        netw -> mode = NETW_CLIENT ;
    }
    if( flag & NETW_SERVER )
    {
        netw -> mode = NETW_SERVER ;
    }
    if( flag & NETW_RUN )
    {
        netw -> state = NETW_RUN ;
    }
    if( flag & NETW_EXITED )
    {
        netw -> state = NETW_EXITED ;
    }
    if( flag & NETW_ERROR )
    {
        netw -> state = NETW_ERROR ;
    }
    if( flag & NETW_EXIT_ASKED )
    {
        netw -> state = NETW_EXIT_ASKED ;
    }
    if( flag & NETW_THR_ENGINE_STARTED )
    {
        netw -> threaded_engine_status = NETW_THR_ENGINE_STARTED ;
    }
    if( flag & NETW_THR_ENGINE_STOPPED )
    {
        netw -> threaded_engine_status = NETW_THR_ENGINE_STOPPED ;
    }
    pthread_mutex_unlock( &netw -> eventbolt );

    sem_post( &netw -> send_blocker );

    return TRUE;
} /* netw_set(...) */



/*!\fn netw_close( NETWORK **netw )
 *\brief Closing a specified Network, destroy queues, free the structure
 *\param netw A NETWORK *network to close
 *\return TRUE on success , FALSE on failure
 */
int netw_close( NETWORK **netw )
{
    int state = 0, thr_engine_status = 0 ;
    __n_assert( netw&&(*netw), return FALSE );

    list_foreach( node, (*netw) -> pools )
    {
        NETWORK_POOL *pool = (NETWORK_POOL *)node -> ptr ;
        netw_pool_remove( pool, (*netw) );
    }

    list_destroy( &(*netw) -> pools );

    netw_get_state( (*netw), &state, &thr_engine_status );
    if( thr_engine_status == NETW_THR_ENGINE_STARTED )
    {
        netw_stop_thr_engine( (*netw ) );
    }

    /*closing connection*/
    /* Now with end of send fix */
    if( (*netw) -> link . sock != INVALID_SOCKET )
    {
        closesocket( (*netw) -> link . sock );
        n_log( LOG_DEBUG, "socket %d closed", (*netw) -> link . sock );
    }

    FreeNoLog( (*netw) -> link . ip );
    FreeNoLog( (*netw) -> link . port );
#ifdef HAVE_OPENSSL
    FreeNoLog( (*netw) -> key );
    FreeNoLog( (*netw) -> certificat );
#endif
    FreeNoLog( (*netw) -> vigenere_key );

    if( (*netw) -> link . rhost )
    {
        freeaddrinfo( (*netw) -> link . rhost );
    }

    /*list freeing*/
    netw_set( (*netw), NETW_DESTROY_SENDBUF|NETW_DESTROY_RECVBUF );

    pthread_mutex_destroy( &(*netw) -> recvbolt );
    pthread_mutex_destroy( &(*netw) -> sendbolt );
    pthread_mutex_destroy( &(*netw) -> eventbolt );
    sem_destroy( &(*netw) -> send_blocker );

    Free( (*netw) );

    return TRUE;
} /* netw_close(...)*/



/*!\fn int deplete_send_buffer( int fd , long timeout )
 *\brief wait until the socket is empty or timeout, checking each 50 msec
 *\param fd socket descriptor
 *\timeout timeout value in msec , 0 => disabled
 *\return 0 or the amount of remaining datas in bytes
 */
int deplete_send_buffer( int fd, long timeout )
{
#if defined( __linux__ )
    int outstanding = 0 ;
    if( timeout <= 0 )
    {
        return 0 ;
    }
    for( int it = 0 ; it < timeout ; it += 50 )
    {
        outstanding = 0 ;
        ioctl(fd, SIOCOUTQ, &outstanding);
        if(!outstanding)
        {
            break;
        }
        usleep( 50000 );
    }
    return outstanding ;
#else
    (void)fd;
    (void)timeout;
    return 0 ;
#endif
}


/*!\fn netw_wait_close( NETWORK **netw )
 *\brief Wait for peer closing a specified Network, destroy queues, free the structure. Default 30 seconds timeout
 *\warning Do not use on the accept socket itself (the server socket) as it will display false errors
 *\param netw A NETWORK *network to close
 *\return TRUE on success , FALSE on failure
 */
int netw_wait_close( NETWORK **netw )
{
    // default 30 secs timeout
    return netw_wait_close_timed( netw, 30 );
} /* netw_wait_close(...)*/



/*!\fn netw_wait_close_timed( NETWORK **netw , int timeout )
 *\brief Wait for peer closing a specified Network, destroy queues, free the structure
 *\warning Do not use on the accept socket itself (the server socket) as it will display false errors
 *\param netw A NETWORK *network to close
 *\param timeout timeout in seconds before force close engine
 *\return TRUE on success , FALSE on failure
 */
int netw_wait_close_timed( NETWORK **netw, int timeout )
{
    int state = 0, thr_engine_status = 0 ;
    __n_assert( netw&&(*netw), return FALSE );

    int error = 0,
        countdown = 0 ;
    char *errmsg = NULL ;

    countdown = timeout ;

    netw_get_state( (*netw), &state, &thr_engine_status );
    if( thr_engine_status == NETW_THR_ENGINE_STARTED )
    {
        int nb_running = 0 ;
        do
        {
            pthread_mutex_lock( &(*netw) -> eventbolt );
            nb_running = (*netw) -> nb_running_threads ;
            pthread_mutex_unlock( &(*netw) -> eventbolt );
            sleep( 1 );
            countdown -- ;
        }
        while( nb_running > 0 && countdown > 0 );
    }
    if( countdown == 0 )
    {
        n_log( LOG_ERR, "netw %d waited too long (%ds) for peer to close", (*netw) -> link . sock, timeout );
    }

    /* wait for close fix */
    if( (*netw) -> link . sock != INVALID_SOCKET )
    {
        /* inform peer that we have finished */
        shutdown( (*netw) -> link . sock, SHUT_WR );
        int remaining = deplete_send_buffer( (*netw) -> link . sock, (*netw) -> deplete_timeout );
        if( remaining > 0 )
        {
            n_log( LOG_ERR, "socket %d (%s:%s) took more than %d msecs to send %d octets before closing => force close", (*netw) -> link . sock, (*netw) -> link . ip, (*netw) -> link . port, (int)(*netw) -> deplete_timeout, remaining );
        }
        /* wait for fin ack */
        char buffer[ 4096 ] = "" ;
        for( ;; )
        {
            int res = 0 ;
            res = recv( (*netw) -> link . sock, buffer, 4096, NETFLAGS );
            if( !res )
                break;
            if( res < 0 )
            {
                error = neterrno ;
                if( error != ENOTCONN && error != 10057 && error != EINTR && error != ECONNRESET ) // no an error if already closed
                {
                    errmsg = netstrerror( error );
                    n_log( LOG_ERR, "read returned error %d when closing socket %d (%s:%s): %s", error, (*netw) -> link . sock, _str( (*netw) -> link . ip ), (*netw) -> link . port,  _str( errmsg ) );
                    FreeNoLog( errmsg );
                }
                break ;
            }
        }
    }
    return netw_close( &(*netw) );

} /* netw_wait_close_timed(...)*/



/*!\fn int netw_make_listening( NETWORK **netw , char *addr , char *port , int nbpending  , int ip_version )
 *\brief Make a NETWORK be a Listening network
 *\param netw A NETWORK **network to make listening
 *\param addr Adress to bind, NULL for automatic address filling
 *\param port For choosing a PORT to listen to
 *\param nbpending Number of pending connection when listening
 *\param ip_version NETWORK_IPALL for both ipv4 and ipv6 , NETWORK_IPV4 or NETWORK_IPV6
 *\return TRUE on success, FALSE on error
 */
int netw_make_listening( NETWORK **netw, char *addr, char *port, int nbpending, int ip_version )
{
    __n_assert( port, return FALSE );

    int error = 0 ;
    char *errmsg = NULL ;

    if( *netw )
    {
        n_log( LOG_ERR, "Cannot use an allocated network. Please pass a NULL network to modify" );
        return FALSE;
    }

    /*checking WSA when under windows*/
    if ( netw_init_wsa( 1, 2, 2 ) == FALSE )
    {
        n_log( LOG_ERR, "Unable to load WSA dll's" );
        return FALSE ;
    }

    (*netw) = netw_new( -1, -1 );
    (*netw) -> link . port = strdup( port );
    /*creating array*/
    if( ip_version == NETWORK_IPV4 )
    {
        (*netw) -> link . hints . ai_family   = AF_INET ;     /* Allow IPv4 */
    }
    else if( ip_version == NETWORK_IPV6 )
    {
        (*netw) -> link . hints . ai_family   = AF_INET6 ;     /* Allow IPv6 */
    }
    else
    {
        /* NETWORK_ALL or unknown value */
        (*netw) -> link . hints . ai_family   = AF_UNSPEC;     /* Allow IPv4 or IPv6 */
    }
    if( !addr )
    {
        (*netw) -> link . hints . ai_flags    = AI_PASSIVE;    /* For wildcard IP address */
    }
    (*netw) -> link . hints . ai_socktype = SOCK_STREAM;
    (*netw) -> link . hints . ai_protocol = IPPROTO_TCP;
    (*netw) -> link . hints . ai_canonname = NULL;
    (*netw) -> link . hints . ai_next = NULL;

    error = getaddrinfo( addr, port, &(*netw) -> link . hints, &(*netw) -> link . rhost );
    if( error != 0)
    {
        n_log( LOG_ERR, "Error when resolving %s:%s getaddrinfo: %s", _str( addr ),  port, gai_strerror( error ) );
        netw_close( &(*netw) );
        return FALSE ;
    }
    (*netw) -> addr_infos_loaded = 1 ;

    /* getaddrinfo() returns a list of address structures.
       Try each address until we successfully connect(2).
       If socket(2) (or connect(2)) fails, we (close the socket
       and) try the next address. */
    struct addrinfo *rp = NULL ;
    for( rp = (*netw) -> link . rhost ; rp != NULL ; rp = rp -> ai_next )
    {
        (*netw) -> link . sock = socket( rp -> ai_family, rp -> ai_socktype, rp->ai_protocol );
        if( (*netw) -> link . sock == INVALID_SOCKET )
        {
            error = neterrno ;
            errmsg = netstrerror( error );
            n_log( LOG_ERR, "Error while trying to make a socket: %s", _str( errmsg ) );
            FreeNoLog( errmsg );
            continue ;
        }
        netw_setsockopt( (*netw), SO_REUSEADDR, 1 );
        if( bind( (*netw) -> link . sock, rp -> ai_addr, rp -> ai_addrlen ) == 0 )
        {
            char *ip = NULL ;
            Malloc( ip, char, 64 );
            if( !ip )
            {
                n_log( LOG_ERR, "Error allocating 64 bytes for ip" );
                netw_close( &(*netw) );
                return FALSE ;
            }
            if( !inet_ntop( rp -> ai_family, get_in_addr( rp -> ai_addr ), ip, 64 ) )
            {
                error = neterrno ;
                errmsg = netstrerror( error );
                n_log( LOG_ERR, "inet_ntop: %p , %s", (*netw) -> link . raddr, _str( errmsg ) );
                FreeNoLog( errmsg );
            }
            /*n_log( LOG_DEBUG, "Socket %d successfully binded to %s %s",  (*netw) -> link . sock, _str( ip ) , _str( port ) );*/
            (*netw) -> link . ip = ip ;
            break;                  /* Success */
        }
        error = neterrno ;
        errmsg = netstrerror( error );
        n_log( LOG_ERR, "Error from bind() on port %s neterrno: %s", port, netstrerror( neterrno ), error );
        FreeNoLog( errmsg );
        closesocket( (*netw) -> link .sock );
    }
    if( rp == NULL )
    {
        /* No address succeeded */
        n_log( LOG_ERR, "Couldn't get a socket for listening on port %s", port );
        netw_close( &(*netw) );
        return FALSE ;
    }

    /* nb_pending connections*/
    ( *netw ) -> nb_pending = nbpending ;
    listen( ( *netw ) -> link . sock, ( *netw ) -> nb_pending );

    netw_set( (*netw), NETW_SERVER|NETW_RUN|NETW_THR_ENGINE_STOPPED );

    return TRUE;
} /* netw_make_listening(...)*/



/*!\fn NETWORK *netw_accept_from_ex( NETWORK *from , int send_list_limit , int recv_list_limit , int non_blocking , int *retval )
 *\brief make a normal 'accept' . Network 'from' must be allocated with netw_make_listening.
 *\param from the network from where we accept
 *\param send_list_limit Internal sending list maximum number of item. 0 or negative for unrestricted
 *\param recv_list_limit Internal receiving list maximum number of item. 0 or negative for unrestricted
 *\param non_blocking set to -1 to make it non blocking, to 0 for blocking, else it's the select timeout value in msecs.
 *\param retval EAGAIN ou EWOULDBLOCK or neterrno (use netstrerr( retval) to obtain a string describing the code )
 *\return NULL on failure, if not a pointer to the connected network
 */
NETWORK *netw_accept_from_ex( NETWORK *from, int send_list_limit, int recv_list_limit, int non_blocking, int *retval )
{
    int tmp = 0, error = 0;
    char *errmsg = NULL ;

#if defined( __linux__ ) || defined( __sun ) || defined( _AIX )
    socklen_t sin_size = 0 ;
#else
    int sin_size = 0 ;
#endif

    NETWORK *netw = NULL ;

    /*checking WSA when under windows*/
    if( netw_init_wsa( 1, 2, 2 ) == FALSE )
    {
        n_log( LOG_ERR, "Unable to load WSA dll's" );
        return NULL ;
    }

    __n_assert( from, return NULL );

    netw = netw_new( send_list_limit, recv_list_limit );

    sin_size = sizeof( netw -> link . raddr );

    if( non_blocking > 0 )
    {
        fd_set accept_set ;
        FD_ZERO( &accept_set );
        int secs = (non_blocking%1000000) ;
        int usecs = non_blocking - (secs * 1000000);
        struct timeval select_timeout ;

        select_timeout . tv_sec = secs ;
        select_timeout . tv_usec = usecs ;

        FD_SET( from -> link . sock, &accept_set );

        int ret = select( from -> link . sock + 1, &accept_set, NULL, NULL, &select_timeout );
        if( ret == -1 )
        {
            error = neterrno ;
            errmsg = netstrerror( error );
            if( retval != NULL )
                (*retval) = error ;
            n_log( LOG_ERR, "Error on select with %d timeout , neterrno: %s", non_blocking, _str( errmsg ) );
            FreeNoLog( errmsg );
            netw_close( &netw );
            return NULL;
        }
        else if( ret == 0 )
        {
            /* that one produce waaaay too much logs under a lot of cases */
            /* n_log( LOG_DEBUG , "No connection waiting on %d" , from -> link . sock ); */
            netw_close( &netw );
            return NULL;
        }
        if( FD_ISSET(  from -> link . sock, &accept_set ) )
        {
            //n_log( LOG_DEBUG, "select accept call on %d", from -> link . sock );
            tmp = accept( from -> link . sock, (struct sockaddr * )&netw -> link . raddr, &sin_size );
            if ( tmp < 0 )
            {
                error = neterrno ;
                errmsg = netstrerror( error );
                if( retval != NULL )
                    (*retval) = error ;
                n_log( LOG_DEBUG, "error accepting on %d, %s", netw -> link . sock, _str( errmsg ) );
                FreeNoLog( errmsg );
                netw_close( &netw );
                return NULL;
            }
        }
        else
        {
            netw_close( &netw );
            return NULL;
        }
        netw -> link . is_blocking = 1 ;
    }
    else if( non_blocking == -1 )
    {
        if( from -> link . is_blocking  == 1 )
        {
            netw_set_blocking( from, 0 );
            n_log( LOG_DEBUG, "non blocking accept call on %d", from -> link . sock );
        }
        tmp = accept( from -> link . sock, (struct sockaddr *)&netw -> link . raddr, &sin_size );
        error = neterrno ;
        if( retval != NULL )
            (*retval) = error ;

        if ( tmp < 0 )
        {
            netw_close( &netw );
            return NULL;
        }
#ifdef __windows__
        netw -> link . is_blocking = 0 ;
#endif // __windows__
    }
    else
    {
        if( from -> link . is_blocking  == 0 )
        {
            n_log( LOG_DEBUG, "blocking accept call on %d", from -> link . sock );
            netw_set_blocking( from, 1 );
        }
        tmp = accept( from -> link . sock, (struct sockaddr *)&netw -> link . raddr, &sin_size );
        if ( tmp < 0 )
        {
            error = neterrno ;
            errmsg = netstrerror( error );
            n_log( LOG_ERR, "error accepting on %d, %s", netw -> link . sock, _str( errmsg ) );
            FreeNoLog( errmsg );
            netw_close( &netw );
            return NULL;
        }
        netw -> link . is_blocking = 1 ;
    }

    netw -> link . sock = tmp ;

    netw -> link . port = strdup( from -> link . port );
    Malloc( netw -> link . ip, char, 64 );
    if( !netw -> link . ip )
    {
        n_log( LOG_ERR, "Error allocating 64 bytes for ip" );
        netw_close( &netw );
        return FALSE ;
    }

    if( !inet_ntop( netw -> link . raddr . ss_family, get_in_addr( ((struct sockaddr *)&netw -> link . raddr) ), netw -> link . ip, 64 ) )
    {
        error = neterrno ;
        errmsg = netstrerror( error );
        n_log( LOG_ERR, "inet_ntop: %p , %s", netw -> link . raddr, _str( errmsg ) );
        FreeNoLog( errmsg );
    }

    netw_setsockopt( netw, SO_REUSEADDR, 1 );

#ifdef __windows__
    netw_set_blocking( netw, 1 );
#endif // __windows__

    netw_set( netw, NETW_SERVER|NETW_RUN|NETW_THR_ENGINE_STOPPED );
    n_log( LOG_DEBUG, "Connection accepted from %s:%s", netw-> link . ip, netw -> link . port );

    return netw;
} /* netw_accept_from_ex(...) */



/*!\fn NETWORK *netw_accept_from( NETWORK *from )
 *\brief make a normal blocking 'accept' . Network 'from' must be allocated with netw_make_listening.
 *\param from The network from which to obtaion the connection
 *\return NULL
 */
NETWORK *netw_accept_from( NETWORK *from )
{
    return netw_accept_from_ex( from, -1, -1, 0, NULL );
} /* network_accept_from( ... ) */



/*!\fn NETWORK *netw_accept_nonblock_from( NETWORK *from , int blocking )
 *\brief make a normal blocking 'accept' . Network 'from' must be allocated with netw_make_listening.
 *\param from The network from which to obtaion the connection
 *\param blocking set to -1 to make it non blocking, to 0 for blocking, else it's the select timeout value in mseconds.
 *\return NULL
 */
NETWORK *netw_accept_nonblock_from( NETWORK *from, int blocking )
{
    return netw_accept_from_ex( from, -1, -1, blocking, NULL );
} /* network_accept_from( ... ) */


/*!\fn int netw_add_msg( NETWORK *netw , N_STR *msg )
 *\brief Add a message to send in aimed NETWORK
 *\param netw NETWORK where add the message
 *\param msg  the message to add
 *\return TRUE if success FALSE on error
 */
int netw_add_msg( NETWORK *netw, N_STR *msg )
{
    __n_assert( netw, return FALSE );
    __n_assert( msg, return FALSE );
    __n_assert( msg -> data, return FALSE );

    if( msg -> length <= 0 )
    {
        n_log( LOG_ERR, "Empty messages are not supported. msg(%p)->lenght=%d", msg, msg -> length );
        return FALSE;
    }

    pthread_mutex_lock( &netw -> sendbolt );

    if( list_push( netw -> send_buf, msg, free_nstr_ptr ) == FALSE )
    {
        pthread_mutex_unlock( &netw -> sendbolt );
        return FALSE;
    }

    pthread_mutex_unlock( &netw -> sendbolt );

    sem_post( &netw -> send_blocker );

    return TRUE;
} /* netw_add_msg(...) */



/*!\fn int netw_add_msg_ex( NETWORK *netw , char *str , unsigned int length )
 *\brief Add a message to send in aimed NETWORK
 *\param netw NETWORK where add the message
 *\param str  the message to add
 *\param length  the size of the message to add
 *\return TRUE if success FALSE on error
 */
int netw_add_msg_ex( NETWORK *netw, char *str, unsigned int length )
{
    __n_assert( netw, return FALSE );
    __n_assert( str, return FALSE );
    if( length == 0 )
    {
        return FALSE ;
    }

    N_STR *nstr = NULL ;
    Malloc( nstr, N_STR, 1 );
    __n_assert( nstr, return FALSE );

    if( length == 0 )
    {
        n_log( LOG_ERR, "Empty messages are not supported. msg(%p)->lenght=%d", str, length );
        return FALSE;
    }

    nstr -> data = str ;
    nstr -> written = nstr -> length = length ;

    pthread_mutex_lock( &netw -> sendbolt );
    if( list_push( netw -> send_buf, nstr, free_nstr_ptr ) == FALSE )
    {
        pthread_mutex_unlock( &netw -> sendbolt );
        return FALSE;
    }
    pthread_mutex_unlock( &netw -> sendbolt );

    sem_post( &netw -> send_blocker );

    return TRUE;
} /* netw_add_msg_ex(...) */




/*!\fn N_STR *netw_get_msg( NETWORK *netw )
 *\brief Get a message from aimed NETWORK
 *\param netw NETWORK where get the msg
 *\return NULL or a filled N_STR
 */
N_STR *netw_get_msg( NETWORK *netw )
{
    N_STR * ptr = NULL ;

    __n_assert( netw, return NULL );

    pthread_mutex_lock( &netw -> recvbolt );

    ptr = list_shift( netw -> recv_buf, N_STR );

    pthread_mutex_unlock( &netw -> recvbolt );

    return ptr ;
} /* netw_get_msg(...)*/



/*!\fn N_STR *netw_wait_msg( NETWORK *netw , long refresh , long timeout )
 *\brief Wait a message from aimed NETWORK. Recheck each usec until a valid
 *\param netw The link on which we wait a message
 *\param refresh The time in usec between each check until there is a message
 *\param timeout in usecs , maximum amount of time to wait before return. Pass a value <= 0 to disable.
 *\return NULL or a filled N_STR
 */
N_STR *netw_wait_msg( NETWORK *netw, long refresh, long timeout )
{
    N_STR *nstrptr = NULL ;
    int timed = 0 ;
    long secs = 0 ;
    long usecs = 0 ;

    __n_assert( netw, return NULL );

    usecs = refresh ;
    if( refresh > 999999 )
    {
        secs = refresh / 1000000 ;
        usecs = refresh % 1000000 ;
    }

    if( timeout > 0 )
        timed = 1 ;

    int state = NETW_RUN, thr_state = 0 ;

    /* first try without sleep in case there alreay is a message in queue */
    nstrptr = netw_get_msg( netw );
    if( nstrptr )
        return nstrptr ;
    do
    {
        if(secs > 0)
            sleep( secs );
        if( usecs >0 )
            u_sleep( usecs );

        nstrptr = netw_get_msg( netw );
        if( nstrptr )
            return nstrptr ;

        if( timed == 1 )
        {
            timeout -= refresh ;
            if( timeout < 0 )
            {
                n_log( LOG_ERR, "timed out for netw %d (%s)", netw -> link . sock, _str( netw -> link . ip ) );
                break ;
            }
        }
        netw_get_state( netw, &state, &thr_state );
    }
    while( state == NETW_RUN );

    return NULL ;
} /* netw_wait_msg(...) */



/*!\fn netw_start_thr_engine( NETWORK *netw )
 *\brief Start the NETWORK netw Threaded Engine. Create a sending & receiving thread.
 *\param netw The aimed NETWORK connection to start receiving data
 *\return TRUE on success FALSE on failure
 */
int netw_start_thr_engine( NETWORK *netw )
{
    __n_assert( netw, return FALSE );

    pthread_mutex_lock( &netw -> eventbolt );
    if( netw -> threaded_engine_status == NETW_THR_ENGINE_STARTED )
    {
        n_log( LOG_ERR, "THR Engine already started for network %p (%s)", netw, _str( netw -> link . ip ) );
        pthread_mutex_unlock( &netw -> eventbolt );
        return FALSE ;
    }

    if( pthread_create(  &netw -> recv_thr,   NULL,  netw_recv_func,  (void *) netw ) != 0 )
    {
        n_log( LOG_ERR, "Unable to create recv_thread for network %p (%s)", netw, _str( netw -> link . ip ) );
        pthread_mutex_unlock( &netw -> eventbolt );
        return FALSE ;
    }
    netw -> nb_running_threads ++ ;
    if( pthread_create(  &netw -> send_thr,   NULL,  netw_send_func,  (void *) netw ) != 0 )
    {
        n_log( LOG_ERR, "Unable to create send_thread for network %p (%s)", netw, _str( netw -> link . ip ) );
        pthread_mutex_unlock( &netw -> eventbolt );
        return FALSE ;
    }
    netw -> nb_running_threads ++ ;

    netw -> threaded_engine_status = NETW_THR_ENGINE_STARTED ;

    pthread_mutex_unlock( &netw -> eventbolt );

    return TRUE ;
}  /* netw_create_recv_thread(....) */



/*!\fn void *netw_send_func( void *NET )
 *\brief Thread send function
 *\param NET the NETWORK connection to use, casted into (void*)
 */
void *netw_send_func( void *NET )
{
    int DONE = 0,
        state = 0,
        net_status = 0 ;

    NSTRBYTE nboctet = 0 ;

    char nboct[ 5 ] = "" ;

    N_STR *ptr = NULL ;

    NETWORK *netw = (NETWORK *) NET ;

    __n_assert( netw, return NULL );

    do
    {

        /* do not consume cpu for nothing, reduce delay */
        sem_wait( &netw -> send_blocker );
        int message_sent = 0 ;
        while( message_sent == 0 && !DONE )
        {
            netw_get_state( netw, &state, NULL );
            if( state&NETW_ERROR )
            {
                DONE = 666 ;
            }
            else if( state&NETW_EXITED )
            {
                DONE = 100 ;
            }
            else if( state&NETW_EXIT_ASKED )
            {
                DONE = 100 ;
                /* sending state */
                nboctet = htonl( NETW_EXIT_ASKED );
                memcpy( nboct, &nboctet, sizeof( NSTRBYTE ) );
                n_log( LOG_DEBUG, "%d Sending Quit !", netw -> link . sock );
                net_status = netw->send_data( netw -> link . sock, nboct, sizeof( NSTRBYTE ) );
                if( net_status < 0 )
                    DONE = 4 ;
                n_log( LOG_DEBUG, "%d Quit sent!", netw -> link . sock );
            }
            else
            {
                pthread_mutex_lock( &netw -> sendbolt );
                ptr = list_shift( netw -> send_buf, N_STR ) ;
                pthread_mutex_unlock( &netw -> sendbolt );
                if( ptr && ptr -> length > 0 && ptr -> data )
                {
                    n_log( LOG_DEBUG, "Sending ptr size %d written %d", ptr -> length, ptr -> written );

                    /* sending state */
                    nboctet = htonl( state );
                    memcpy( nboct, &nboctet, sizeof( NSTRBYTE ) );
                    /* sending state */
                    if ( !DONE )
                    {
                        net_status = netw->send_data( netw -> link . sock, nboct, sizeof( NSTRBYTE ) );
                        if( net_status < 0 )
                            DONE = 1 ;
                    }
                    if( !DONE )
                    {
                        /* sending number of octet */
                        nboctet = htonl( ptr -> written );
                        memcpy( nboct, &nboctet, sizeof( NSTRBYTE ) );
                        /* sending the number of octet to receive on next message */
                        net_status = netw->send_data( netw -> link . sock, nboct, sizeof( NSTRBYTE ) );
                        if( net_status < 0 )
                            DONE = 2 ;
                    }
                    /* sending the data itself */
                    if ( !DONE )
                    {
                        net_status = netw->send_data( netw -> link . sock, ptr -> data, ptr -> written );
                        if( net_status < 0 )
                            DONE = 3 ;
                    }
                    free_nstr( &ptr );
                    u_sleep(  netw -> send_queue_consecutive_wait );
                    message_sent = 1 ;
                }
            }
        }
    }
    while( !DONE );

    if( DONE == 1 )
        n_log( LOG_ERR,  "Error when sending state %d on socket %d (%s), network: %s", netw -> state, netw -> link . sock, _str( netw -> link . ip ), (net_status==-2)?"disconnected":"socket error" );
    else if( DONE == 2 )
        n_log( LOG_ERR,  "Error when sending number of octet to socket %d (%s), network: %s", netw -> link . sock, _str( netw -> link . ip ), (net_status==-2)?"disconnected":"socket error" );
    else if( DONE == 3 )
        n_log( LOG_DEBUG,  "Error when sending data on socket %d (%s), network: %s", netw -> link . sock, _str( netw -> link . ip ), (net_status==-2)?"disconnected":"socket error" );
    else if( DONE == 4 )
        n_log( LOG_ERR,  "Error when sending state QUIT on socket %d (%s), network: %s", netw -> link . sock, _str( netw -> link . ip ), (net_status==-2)?"disconnected":"socket error" );
    else if( DONE == 5 )
        n_log( LOG_ERR,  "Error when sending state QUIT number of octet (0) on socket %d (%s), network: %s", netw -> link . sock, _str( netw -> link . ip ), (net_status==-2)?"disconnected":"socket error" );

    if( DONE == 100 )
    {
        n_log( LOG_DEBUG, "Socket %d: Sending thread exiting correctly", netw -> link . sock );
        netw_set( netw, NETW_EXIT_ASKED );
    }
    else
    {
        n_log( LOG_ERR, "Socket %d (%s): Sending thread exiting with error %d !", netw -> link . sock, _str( netw -> link . ip ),DONE );
        netw_set( netw, NETW_ERROR );
    }

    pthread_mutex_lock( &netw -> eventbolt );
    netw -> nb_running_threads -- ;
    pthread_mutex_unlock( &netw -> eventbolt );

    pthread_exit( 0 );

    /* suppress compiler warning */
#if !defined( __linux__ ) && !defined( __sun )
    return NULL ;
#endif
} /* netw_send_func(...) */



/*!\fn void *netw_recv_func( void *NET )
 *\brief To Thread Receiving function
 *\param NET the NETWORK connection to use
 */
void *netw_recv_func( void *NET )
{
    int DONE = 0,
        state = 0,
        tmpstate = 0,
        net_status = 0 ;

    NSTRBYTE nboctet = 0 ;

    char nboct[ 5 ]= "" ;

    N_STR *recvdmsg = NULL ;

    NETWORK *netw = (NETWORK *) NET ;

    __n_assert( netw, return NULL );

    do
    {
        netw_get_state( netw, &state, NULL );

        if( state&NETW_EXIT_ASKED || state&NETW_EXITED )
        {
            DONE = 100;
        }
        if( state&NETW_ERROR )
        {
            DONE = 666 ;
        }
        else
        {
            /*if( !DONE && !(state&NETW_EXIT_ASKED) && !(state&NETW_EXITED) ) */
            if( !DONE )
            {
                /* receiving state */
                net_status = netw->recv_data( netw -> link . sock, nboct, sizeof( NSTRBYTE ) );
                if( net_status < 0 )
                {
                    DONE = 1 ;
                }
                else
                {
                    memcpy( &nboctet, nboct, sizeof( NSTRBYTE ) );
                    tmpstate = ntohl( nboctet );
                    nboctet = tmpstate ;
                    if( tmpstate==NETW_EXIT_ASKED )
                    {
                        n_log( LOG_DEBUG,  "%d receiving order to QUIT, nboctet %d NETW_EXIT_ASKED %d !", netw -> link . sock, nboctet, NETW_EXIT_ASKED );
                        DONE = 100 ;
                        netw_set( netw, NETW_EXIT_ASKED );
                    }
                    else
                    {
                        /* receiving nboctet */
                        net_status = netw->recv_data( netw -> link . sock, nboct, sizeof( NSTRBYTE ) );
                        if( net_status < 0 )
                        {
                            DONE = 2 ;
                        }
                        else
                        {
                            memcpy( &nboctet, nboct, sizeof( NSTRBYTE ) );
                            tmpstate = ntohl( nboctet );
                            nboctet = tmpstate ;

                            Malloc( recvdmsg, N_STR, 1 );
                            if( !recvdmsg )
                            {
                                DONE = 3;
                            }
                            else
                            {
                                n_log( LOG_DEBUG,  "%d octets to receive...", nboctet );
                                Malloc( recvdmsg -> data, char, nboctet + 1 );
                                if( !recvdmsg -> data )
                                {
                                    DONE = 4 ;
                                }
                                else
                                {
                                    recvdmsg -> length  = nboctet + 1 ;
                                    recvdmsg -> written = nboctet ;

                                    /* receiving the data itself */
                                    net_status = netw->recv_data( netw -> link . sock, recvdmsg -> data, nboctet );
                                    if( net_status < 0 )
                                    {
                                        DONE = 5 ;
                                    }
                                    else
                                    {
                                        pthread_mutex_lock( &netw -> recvbolt );
                                        if( !DONE && list_push( netw -> recv_buf, recvdmsg, free_nstr_ptr ) == FALSE )
                                            DONE = 6 ;
                                        pthread_mutex_unlock( &netw -> recvbolt );
                                        n_log( LOG_DEBUG,  "%d octets received !", nboctet );
                                    } /* recv data */
                                } /* recv data allocation */
                            } /* recv struct allocation */
                        } /* recv nb octet*/
                    } /* exit asked */
                } /* recv state */
            } /* if( !done) */
        } /* if !exit */
    }
    while( !DONE );

    if( DONE == 1 )
        n_log( LOG_ERR,  "Error when receiving state from socket %d (%s), net_status: %s", netw -> link . sock, _str( netw -> link . ip ), (net_status==-2)?"disconnected":"socket error" );
    else if( DONE == 2 )
        n_log( LOG_ERR,  "Error when receiving nboctet from socket %d (%s), net_status: %s", netw -> link . sock, _str( netw -> link . ip ), (net_status==-2)?"disconnected":"socket error" );
    else if( DONE == 3 )
        n_log( LOG_ERR,  "Error when receiving data from socket %d (%s), net_status: %s", netw -> link . sock, _str( netw -> link . ip ), (net_status==-2)?"disconnected":"socket error" );
    else if( DONE == 4 )
        n_log( LOG_ERR,  "Error allocating received message struct from socket %d (%s), net_status: %s", netw -> link . sock, _str( netw -> link . ip ), (net_status==-2)?"disconnected":"socket error" );
    else if( DONE == 5 )
        n_log( LOG_ERR,  "Error allocating received messages data array from socket %d (%s), net_status: %s", netw -> link . sock, _str( netw -> link . ip ), (net_status==-2)?"disconnected":"socket error" );
    else if( DONE == 6 )
        n_log( LOG_ERR,  "Error adding receved message from socket %d (%s), net_status: %s", netw -> link . sock, _str( netw -> link . ip ), (net_status==-2)?"disconnected":"socket error" );


    if( DONE == 100 )
    {
        n_log( LOG_DEBUG, "Socket %d (%s): Receive thread exiting correctly", netw -> link . sock, _str( netw -> link . ip ) );
        netw_set( netw, NETW_EXIT_ASKED );
    }
    else
    {
        n_log( LOG_ERR, "Socket %d (%s): Receive thread exiting with code %d !", netw -> link . sock, _str( netw -> link . ip ), DONE );
        netw_set( netw, NETW_ERROR );
    }

    pthread_mutex_lock( &netw -> eventbolt );
    netw -> nb_running_threads -- ;
    pthread_mutex_unlock( &netw -> eventbolt );

    pthread_exit( 0 );

    /* suppress compiler warning */
#if !defined( __linux__ ) && !defined( __sun )
    return NULL ;
#endif
} /* netw_recv_func(...) */



/*!\fn netw_stop_thr_engine( NETWORK *netw )
 *\brief Stop a NETWORK connection sending and receing thread
 *\param netw The aimed NETWORK conection to stop
 *\return TRUE on success FALSE on failure
 */
int netw_stop_thr_engine( NETWORK *netw )
{
    __n_assert( netw,  return FALSE );
    int state = 0, thr_engine_status = 0 ;

    n_log( LOG_DEBUG,  "Network %d stop threads event", netw -> link . sock );

    netw_get_state( netw, &state, &thr_engine_status );

    if( thr_engine_status != NETW_THR_ENGINE_STARTED )
    {
        n_log( LOG_ERR, "Thread engine status already stopped for network %p", netw );
        return FALSE ;
    }

    netw_set( netw, NETW_EXIT_ASKED );

    n_log( LOG_DEBUG,  "Network %d waits for send threads to stop", netw -> link . sock );
    for( int it = 0 ; it < 10 ; it ++ )
    {
        sem_post( &netw -> send_blocker );
        usleep( 1000 );
    }
    pthread_join( netw -> send_thr, NULL );
    n_log( LOG_DEBUG,  "Network %d waits for recv threads to stop", netw -> link . sock );
    pthread_join( netw -> recv_thr, NULL );

    netw_set( netw, NETW_EXITED|NETW_THR_ENGINE_STOPPED );

    n_log( LOG_DEBUG,  "Network %d threads Stopped !", netw -> link . sock );

    return TRUE;
} /* Stop_Network( ... ) */



/*!\fn send_data( SOCKET s , char *buf, NSTRBYTE n )
 *\brief send data onto the socket
 *\param s connected socket
 *\param buf pointer to buffer
 *\param n number of characters we want to send
 *\return -1 on error, -2 on disconnection, n on success
 */
int send_data( SOCKET s, char *buf, NSTRBYTE n )
{
    __n_assert( buf, return -1 );

    int bcount = 0 ; /* counts bytes sent */

    int error = 0 ;
    char *errmsg = NULL ;

    if( n == 0 )
    {
        n_log( LOG_ERR, "Send of 0 is unsupported." );
        return -1 ;
    }

    while( (NSTRBYTE)bcount < n )                 /* loop until full buffer */
    {
        int br = 0 ;     /* bytes sent this pass */
        br = send( s, buf, n - bcount, NETFLAGS );
        error = neterrno ;
        if( br > 0 )
        {
            bcount += br;                /* increment byte counter */
            buf += br;                   /* move buffer ptr for next read */
        }
        else
        {
            if( error == ECONNRESET || error == ENOTCONN )
            {
                n_log( LOG_DEBUG, "socket %d disconnected !", s );
                return -2 ;
            }
            /* signal an error to the caller */
            errmsg = netstrerror( error );
            n_log( LOG_ERR,  "Socket %d send Error: %d , %s", s, br, _str( errmsg ) );
            FreeNoLog( errmsg );
            return -1 ;
        }
    }

    return bcount;
} /*send_data(...)*/

/*!\fn recv_data( SOCKET s , char *buf, NSTRBYTE n )
 *\brief recv data from the socket
 *\param s connected socket
 *\param buf pointer to buffer
 *\param n number of characters we want
 *\return -1 on error, -2 on disconnection n on success
 */
int recv_data( SOCKET s, char *buf, NSTRBYTE n )
{
    __n_assert( buf, return -1 );
    int bcount = 0 ; /* counts bytes read */

    int error = 0 ;
    char *errmsg = NULL ;

    if( n == 0 )
    {
        n_log( LOG_ERR, "Recv of 0 is unsupported." );
        return -1 ;
    }

    while( (NSTRBYTE)bcount < n )
    {
        int br = 0 ;     /* bytes read this pass */
        /* loop until full buffer */
        CALL_RETRY( br, recv( s, buf, n - bcount, NETFLAGS ) );
        error = neterrno ;
        if( br > 0 )
        {
            bcount += br;                    /* increment byte counter */
            buf += br;                       /* move buffer ptr for next read */
        }
        else
        {
            if( br == 0 )
            {
                n_log( LOG_DEBUG, "socket %d disconnected !", s );
                return -2 ;
            }

            /* signal an error to the caller */
            errmsg = netstrerror( error );
            n_log( LOG_ERR,  "socket %d recv returned %d, error: %s", s, br, _str( errmsg ) );
            FreeNoLog( errmsg );
            return -1 ;
        }
    }

    return bcount;
} /*recv_data(...)*/



/*!\fn int send_php( SOCKET s, int _code , char *buf, int n )
 *
 *\brief send data onto the socket
 *
 *\param s connected socket
 *\param buf pointer to buffer
 *\param n number of characters we want to send
 *\param _code Code for php decoding rule
 *
 *\return -1 on error, n on success
 */

int send_php( SOCKET s, int _code, char *buf, int n )
{
    int bcount = 0 ;   /* counts bytes read */
    int br = 0 ;       /* bytes read this pass */

    int error = 0 ;
    char *errmsg = NULL ;

    char *ptr = NULL ; /* temp char ptr ;-) */
    char tmpstr[ HEAD_SIZE +1 ] = "";
    char head[ HEAD_SIZE +1 ] = "" ;
    char code[ HEAD_CODE +1 ] = "" ;

    snprintf( tmpstr, HEAD_SIZE + 1, "%%0%dd", HEAD_SIZE );
    snprintf( head, HEAD_SIZE + 1, tmpstr, n );
    snprintf( tmpstr, HEAD_SIZE + 1, "%%0%dd", HEAD_CODE );
    snprintf( code, HEAD_CODE + 1, tmpstr, _code );

    /* sending head */
    bcount = br = 0 ;
    ptr = head ;
    while ( bcount < HEAD_SIZE )                 /* loop until full buffer */
    {
        CALL_RETRY( br, send( s, head, HEAD_SIZE - bcount, NETFLAGS ) );
        error = neterrno ;
        if( br > 0 )
        {
            bcount += br;                /* increment byte counter */
            ptr += br;                   /* move buffer ptr for next read */
        }
        else
        {
            /* signal an error to the caller */
            errmsg = netstrerror( error );
            n_log( LOG_ERR,  "Socket %d sending Error %d when sending head size, neterrno: %s", s, br, _str( errmsg ) );
            FreeNoLog( errmsg );
            return FALSE;
        }
    }

    /* sending code */
    bcount = br = 0 ;
    ptr = code ;
    while ( bcount < HEAD_CODE )                 /* loop until full buffer */
    {
        br = send( s, code, HEAD_CODE - bcount, NETFLAGS );
        error = neterrno ;
        if( br > 0 )
        {
            bcount += br;                /* increment byte counter */
            ptr += br;                   /* move buffer ptr for next read */
        }
        else
        {
            errmsg = netstrerror( error );
            n_log( LOG_ERR,  "Socket %d sending Error %d when sending head code, neterrno: %s", s, br, _str( errmsg ) );
            FreeNoLog( errmsg );
            return FALSE;
        }
    }

    /* sending buf */
    bcount = 0;
    br = 0;
    while ( bcount < n )                 /* loop until full buffer */
    {
        br = send( s, buf, n - bcount, NETFLAGS );
        error = neterrno ;
        if( br > 0 )
        {
            bcount += br;                /* increment byte counter */
            buf += br;                   /* move buffer ptr for next read */
        }
        else
        {
            /* signal an error to the caller */
            errmsg = netstrerror( error );
            n_log( LOG_ERR, "Socket %d sending Error %d when sending message of size %d, neterrno: %S", s, br, n, _str( errmsg ) );
            FreeNoLog( errmsg );
            return FALSE;
        }
    }

    return bcount;

} /*send_php(...)*/



/*!\fn int recv_php( SOCKET s, int *_code , char **buf )
 *
 *\brief recv data from the socket
 *
 *\param s connected socket
 *\param _code pointer to store the code
 *\param buf pointer to buffer
 *
 *\return -1 on error, size on success
 */
int recv_php( SOCKET s, int *_code, char **buf )
{
    int bcount = 0 ; /* counts bytes read */
    int br = 0 ;     /* bytes read this pass */
    long int tmpnb = 0, size = 0 ;    /* size of message to receive */
    char *ptr = NULL ;

    int error = 0 ;
    char *errmsg = NULL ;

    char head[ HEAD_SIZE + 1 ] = "" ;
    char code[ HEAD_CODE + 1]  = "" ;

    /* Receiving total message size */
    bcount = br = 0 ;
    ptr = head;
    while ( bcount < HEAD_SIZE )
    {
        /* loop until full buffer */
        br = recv( s, ptr, HEAD_SIZE - bcount, NETFLAGS );
        error = neterrno ;
        if( br > 0 )
        {
            bcount += br;                    /* increment byte counter */
            ptr += br;                       /* move buffer ptr for next read */
        }
        else
        {
            if( br == 0 && bcount == ( HEAD_SIZE - bcount ) )
                break ;
            /* signal an error to the caller */
            errmsg = netstrerror( error );
            n_log( LOG_ERR, "Socket %d receive %d Error %s", s, br, _str( errmsg ) );
            FreeNoLog( errmsg );
            return FALSE;
        }
    }

    tmpnb = strtol( head, NULL, 10 );
    if( tmpnb == LONG_MIN || tmpnb == LONG_MAX )
    {
        n_log( LOG_ERR, "Size received ( %ld ) can not be determined on socket %d", tmpnb, s );
        return FALSE ;
    }

    size = tmpnb ;

    /* receiving request code */
    bcount = br = 0 ;
    ptr = code;
    while ( bcount < HEAD_CODE )
    {
        /* loop until full buffer */
        br = recv( s, ptr, HEAD_CODE - bcount, NETFLAGS );
        error = neterrno ;
        if( br > 0 )
        {
            bcount += br;                    /* increment byte counter */
            ptr += br;                       /* move buffer ptr for next read */
        }
        else
        {
            if( br == 0 && bcount == ( HEAD_CODE - bcount ) )
                break ;
            /* signal an error to the caller */
            errmsg = netstrerror( error );
            n_log( LOG_ERR, "Socket %d receive %d Error , neterrno: %s", s, br, _str( errmsg ) );
            FreeNoLog( errmsg );
            return FALSE;
        }
    }

    tmpnb = strtol( code, NULL, 10 );
    if( tmpnb == LONG_MIN || tmpnb == LONG_MAX )
    {
        n_log( LOG_ERR, "Code received ( %ld ) can not be determined on socket %d\n", tmpnb, s );
        return FALSE ;
    }

    (*_code) = tmpnb ;

    /* receving message */
    if( (*buf ) )
    {
        Free( (*buf ) );
    }
    Malloc( (*buf), char, (size+1) );
    if( !(*buf ) )
    {
        n_log( LOG_ERR, "Could not allocate PHP receive buf" );
        return FALSE ;
    }

    bcount = 0;
    br = 0;
    ptr = (*buf);
    while ( bcount < size )
    {
        /* loop until full buffer */
        br = recv( s, ptr, size - bcount, NETFLAGS );
        error = neterrno ;
        if( br > 0 )
        {
            bcount += br;                    /* increment byte counter */
            ptr += br;                       /* move buffer ptr for next read */
        }
        else
        {
            if( br == 0 && bcount == ( size - bcount ) )
                break ;
            /* signal an error to the caller */
            errmsg = netstrerror( error );
            n_log( LOG_ERR, "Socket %d receive %d Error neterrno: %s", s, br, _str( errmsg ) );
            FreeNoLog( errmsg );
            return FALSE;
        }
    }
    return size ;
} /*recv_php(...)*/



/*!\fn int netw_get_queue_status( NETWORK *netw, int *nb_to_send, int *nb_to_read )
 *
 *\brief retrieve network send queue status
 *\param netw NETWORK object
 *\param nb_to_send Number of messages still in send buffer (not yet submitted to kernel)
 *\param nb_to_read Number of message already read by the kernel, waiting in the local message list
 *\return TRUE or FALSE
 */
int netw_get_queue_status( NETWORK *netw, int *nb_to_send, int *nb_to_read )
{
    __n_assert( netw, return FALSE );

    pthread_mutex_lock( &netw -> sendbolt );
    (*nb_to_send) = netw -> send_buf -> nb_items ;
    pthread_mutex_unlock( &netw -> sendbolt );

    pthread_mutex_lock( &netw -> recvbolt );
    (*nb_to_read) = netw -> recv_buf -> nb_items ;
    pthread_mutex_unlock( &netw -> recvbolt );

    return TRUE ;
} /* get queue states */



/*!\fn NETWORK_POOL *netw_new_pool( int nb_min_element )
 *
 *\brief return a new network pool of nb_min_element
 *\return a new NETWORK_POOL or NULL
 */
NETWORK_POOL *netw_new_pool( int nb_min_element )
{
    NETWORK_POOL *netw_pool = NULL ;

    Malloc( netw_pool, NETWORK_POOL, 1 );
    __n_assert( netw_pool, return NULL );

    netw_pool -> pool = new_ht( nb_min_element );
    __n_assert( netw_pool -> pool, Free( netw_pool ) ; return NULL );

    init_lock( netw_pool -> rwlock );

    return netw_pool ;
} /* netw_new_pool() */



/*!\fn int netw_destroy_pool( NETWORK_POOL **netw_pool )
 *\brief free a network pool
 *\param the address of a pointer to free
 *\return TRUE or FALSE
 */
int netw_destroy_pool(  NETWORK_POOL **netw_pool )
{
    __n_assert( netw_pool&&(*netw_pool), return FALSE );

    write_lock( (*netw_pool) -> rwlock );
    if( (*netw_pool) -> pool )
        destroy_ht( &(*netw_pool) -> pool );
    unlock( (*netw_pool) -> rwlock );

    rw_lock_destroy( (*netw_pool) -> rwlock );

    Free( (*netw_pool) );

    return TRUE ;
} /* netw_destroy_pool() */



void netw_pool_netw_close( void *netw_ptr )
{
    NETWORK *netw = (NETWORK *)netw_ptr ;
    __n_assert( netw, return );
    n_log( LOG_DEBUG, "Network pool %p: network id %d still active !!", netw, netw -> link . sock );
    return ;
}



/* add network to pool */
int netw_pool_add( NETWORK_POOL *netw_pool, NETWORK *netw )
{
    __n_assert( netw_pool, return FALSE );
    __n_assert( netw, return FALSE );

    n_log( LOG_DEBUG, "Trying to add %lld to %p", (unsigned long long)netw -> link . sock, netw_pool -> pool );

    /* write lock the pool */
    write_lock( netw_pool -> rwlock );
    /* test if not already added */
    N_STR *key = NULL ;
    nstrprintf( key, "%lld", (unsigned long long)netw -> link . sock );
    HASH_NODE *node = NULL ;
    if( ht_get_ptr( netw_pool -> pool, _nstr( key ), (void *)&node ) == TRUE )
    {
        n_log( LOG_ERR, "Network id %d already added !", netw -> link . sock );
        free_nstr( &key );
        unlock( netw_pool -> rwlock );
        return FALSE ;
    }
    /* add it */
    int retval = ht_put_ptr( netw_pool -> pool, _nstr( key ), netw, &netw_pool_netw_close );
    free_nstr( &key );
    list_push( netw -> pools, netw_pool, NULL );

    /* unlock the pool */
    unlock( netw_pool -> rwlock );

    return retval ;
} /* netw_pool_add */



/* remove network to pool */
int netw_pool_remove( NETWORK_POOL *netw_pool, NETWORK *netw )
{
    __n_assert( netw_pool, return FALSE );
    __n_assert( netw, return FALSE );

    /* write lock the pool */
    write_lock( netw_pool -> rwlock );
    /* test if present */
    N_STR *key = NULL ;
    nstrprintf( key, "%lld", (unsigned long long int)netw -> link . sock );
    if( ht_remove( netw_pool -> pool, _nstr( key ) ) == TRUE )
    {
        LIST_NODE *node = list_search( netw -> pools, netw );
        if( node )
        {
            remove_list_node( netw -> pools, node, NETWORK_POOL );
        }
        unlock( netw_pool -> rwlock );
        n_log( LOG_DEBUG, "Network id %d removed !", netw -> link . sock );

        return TRUE ;
    }
    free_nstr( &key );
    n_log( LOG_ERR, "Network id %d already removed !", netw -> link . sock );
    /* unlock the pool */
    unlock( netw_pool -> rwlock );
    return FALSE ;
} /* netw_pool_remove */



/* add message to pool */
int netw_pool_broadcast( NETWORK_POOL *netw_pool, NETWORK *from, N_STR *net_msg )
{
    __n_assert( netw_pool, return FALSE );
    __n_assert( net_msg, return FALSE );

    /* write lock the pool */
    read_lock( netw_pool -> rwlock );
    ht_foreach( node, netw_pool -> pool )
    {
        NETWORK *netw = hash_val( node, NETWORK );
        if( from )
        {
            if( netw -> link . sock != from -> link . sock )
                netw_add_msg( netw, nstrdup( net_msg ) );
        }
        else
        {
            netw_add_msg( netw, nstrdup( net_msg ) );
        }
    }
    unlock( netw_pool -> rwlock );
    return TRUE ;
} /* netw_pool_broadcast */


/* get nb clients */
int netw_pool_nbclients( NETWORK_POOL *netw_pool )
{
    __n_assert( netw_pool, return -1 );

    int nb = -1 ;
    read_lock( netw_pool -> rwlock );
    nb = netw_pool -> pool -> nb_keys ;
    unlock( netw_pool -> rwlock );

    return nb ;
}


/* set user id on a netw */
int netw_set_user_id( NETWORK *netw, int id )
{
    __n_assert( netw, return FALSE );
    netw -> user_id = id ;
    return TRUE ;
}



/*!\fn netw_send_ping( NETWORK *netw , int id_from , int id_to , int time , int type )
 *\brief Add a ping reply to the network
 *\param netw The aimed NETWORK where we want to add something to send
 *\param id_from Identifiant of the sender
 *\param id_to Identifiant of the destination, -1 if the serveur itslef is targetted
 *\param time The time it was when the ping was sended
 *\param type NETW_PING_REQUEST or NETW_PING_REPLY
 *\return TRUE or FALSE
 */
int netw_send_ping( NETWORK *netw, int type, int id_from, int id_to, int time )
{
    N_STR *tmpstr = NULL ;
    __n_assert( netw, return FALSE );

    tmpstr = netmsg_make_ping( type, id_from, id_to, time );
    __n_assert( tmpstr, return FALSE );

    return netw_add_msg( netw, tmpstr );
} /* netw_send_ping( ... ) */



/*!\fn int netw_send_ident( NETWORK *netw , int type , int id , N_STR *name , N_STR *passwd  )
 *\brief Add a formatted NETWMSG_IDENT message to the specified network
 *\param netw The aimed NETWORK where we want to add something to send
 *\param type type of identification ( NETW_IDENT_REQUEST , NETW_IDENT_NEW )
 *\param id The ID of the sending client
 *\param name Username
 *\param passwd Password
 *\return TRUE or FALSE
 */
int netw_send_ident( NETWORK *netw, int type, int id, N_STR *name, N_STR *passwd  )
{
    N_STR *tmpstr = NULL ;

    __n_assert( netw, return FALSE );

    tmpstr = netmsg_make_ident( type, id, name, passwd  );
    __n_assert( tmpstr, return FALSE );

    return netw_add_msg( netw, tmpstr );
} /* netw_send_ident( ... ) */



/*!\fn netw_send_position( NETWORK *netw , int id , double X , double Y , double vx , double vy , double acc_x , double acc_y , int time_stamp )
 *\brief Add a formatted NETWMSG_IDENT message to the specified network
 *\param netw The aimed NETWORK where we want to add something to send
 *\param id The ID of the sending client
 *\param X X position inside a big grid
 *\param Y Y position inside a big grid
 *\param vx X speed
 *\param vy Y speed
 *\param acc_x Y acceleration
 *\param acc_y X acceleration
 *\param time_stamp Current Time when sending (for some delta we would want to compute )
 *\return TRUE or FALSE
 */
int netw_send_position( NETWORK *netw, int id, double X, double Y, double vx, double vy, double acc_x, double acc_y, int time_stamp )
{
    N_STR *tmpstr = NULL ;

    __n_assert( netw, return FALSE );

    tmpstr = netmsg_make_position_msg(  id, X, Y, vx, vy, acc_x, acc_y, time_stamp );

    __n_assert( tmpstr, return FALSE );

    return netw_add_msg( netw, tmpstr );
} /* netw_send_position( ... ) */



/*!\fn int netw_send_string_to( NETWORK *netw , int id_to , N_STR *name , N_STR *txt , int color )
 *\brief Add a string to the network, aiming a specific user
 *\param netw The aimed NETWORK where we want to add something to send
 *\param id_from The ID of the sending client
 *\param id_to The ID of the targetted client
 *\param name Sender Name
 *\param txt Sender text
 *\param color Sender text color
 *\return TRUE or FALSE
 */
int netw_send_string_to( NETWORK *netw, int id_to, N_STR *name, N_STR *chan, N_STR *txt, int color )
{
    N_STR *tmpstr = NULL ;

    __n_assert( netw, return FALSE );

    tmpstr = netmsg_make_string_msg( netw -> user_id, id_to, name, chan, txt, color );
    __n_assert( tmpstr, return FALSE );

    return netw_add_msg( netw, tmpstr );
} /* netw_send_string_to( ... ) */



/*!\fn netw_send_string_to_all( NETWORK *netw , int id , char *name , char *chan , char *txt , int color )
 *\brief Add a string to the network, aiming all server-side users
 *\param netw The aimed NETWORK where we want to add something to send
 *\param id The ID of the sending client
 *\param name Name of user
 *\param chan Target Channel, if any. Pass "ALL" to target the default channel
 *\param txt The text to send
 *\param color The color of the text
 *\return TRUE or FALSE;
 */

int netw_send_string_to_all( NETWORK *netw, N_STR *name, N_STR *chan, N_STR *txt, int color )
{
    N_STR *tmpstr = NULL ;

    __n_assert( netw, return FALSE );

    tmpstr = netmsg_make_string_msg( netw -> user_id,  -1, name, chan, txt, color );
    __n_assert( tmpstr, return FALSE );

    return netw_add_msg( netw, tmpstr );
} /* netw_send_string_to_all( ... ) */



/*!\fn int netw_send_quit( NETWORK *netw )
 *\brief Add a formatted NETMSG_QUIT message to the specified network
 *\param netw The aimed NETWORK
 *\return TRUE or FALSE
 */
int netw_send_quit( NETWORK *netw )
{
    __n_assert( netw, return FALSE );

    N_STR *tmpstr = NULL ;

    tmpstr = netmsg_make_quit_msg();
    __n_assert( tmpstr, return FALSE );

    return netw_add_msg( netw, tmpstr );
} /* netw_send_quit( ... ) */


