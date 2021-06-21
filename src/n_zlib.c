/**\file n_zlib.c
 *  ZLIB compression handler
 *\author Castagnier Mickael
 *\version 1.0
 *\date 27/06/2017
 */


#include "nilorea/n_zlib.h"
#include "nilorea/n_log.h"
#include "limits.h"

#ifndef __windows__
#include <arpa/inet.h>
#else
#include <stdint.h>
#include <winsock2.h>
#endif


/*!\fn int GetMaxCompressedLen( unsigned int nLenSrc )
 *\brief Return the maximum compressed size
 *\param nLenSrc
 *\return The size in bytes
 */
int GetMaxCompressedLen( unsigned int nLenSrc )
{
    unsigned int n16kBlocks = (nLenSrc+16383) / 16384; // round up any fraction of a block
    return ( nLenSrc + 6 + (n16kBlocks*5) );
} /* GetMaxCompressedLen */



/*!\fn int CompressData( unsigned char *abSrc, unsigned int nLenSrc, unsigned char *abDst, unsigned int nLenDst )
 *\brief Compress a string to another
 *\param abSrc source string
 *\param nLenSrc size of source string
 *\param abDst destination string
 *\param nLenDst destination length
 *\return -1 or lenght of output
 */
int CompressData( unsigned char *abSrc, unsigned int nLenSrc, unsigned char *abDst, unsigned int nLenDst )
{
    __n_assert( abSrc, return -1 );
    __n_assert( abDst, return -1 );

    if( nLenSrc == 0 )
    {
        n_log( LOG_ERR, "nLenSrc (%d) <= 0", nLenSrc );
        return -1 ;
    }
    if( nLenDst == 0 )
    {
        n_log( LOG_ERR, "nLenDst (%d) <= 0", nLenDst );
        return -1 ;
    }

    z_stream zInfo = {0};
    zInfo.total_in=  zInfo.avail_in = nLenSrc;
    zInfo.total_out= zInfo.avail_out= nLenDst;
    zInfo.next_in= (unsigned char *)abSrc;
    zInfo.next_out= abDst;

    int nErr, nRet= -1;
    nErr= deflateInit( &zInfo, Z_DEFAULT_COMPRESSION ); // zlib function
    switch( nErr )
    {
    case Z_OK:
        // all is fine
        break ;
    default:
        n_log( LOG_ERR, "%s on string %p size %d", zError( nErr ), abSrc, nLenSrc );
        return -1 ;
    }
    nErr= deflate( &zInfo, Z_FINISH );              // zlib function
    if ( nErr == Z_STREAM_END )
    {
        nRet= zInfo.total_out;
    }
    else
    {
        n_log( LOG_ERR, "%s on string %p size %d", zError( nErr ), abSrc, nLenSrc );
    }
    deflateEnd( &zInfo );    // zlib function
    return( nRet );
} /* CompressData */



/*!\fn int UncompressData( unsigned char *abSrc, unsigned int nLenSrc, unsigned char *abDst, unsigned int nLenDst )
 *\brief Uncompress a string to another
 *\param abSrc source string
 *\param nLenSrc size of source string
 *\param abDst destination string
 *\param nLenDst destination length
 *\return -1 or lenght of output
 */
int UncompressData( unsigned char *abSrc, unsigned int nLenSrc, unsigned char *abDst, unsigned int nLenDst )
{
    __n_assert( abSrc, return -1 );
    __n_assert( abDst, return -1 );

    if( nLenSrc == 0 )
    {
        n_log( LOG_ERR, "nLenSrc (%d) <= 0", nLenSrc );
        return -1 ;
    }
    if( nLenDst == 0 )
    {
        n_log( LOG_ERR, "nLenDst (%d) <= 0", nLenDst );
        return -1 ;
    }

    z_stream zInfo = {0};
    zInfo.total_in=  zInfo.avail_in=  nLenSrc;
    zInfo.total_out= zInfo.avail_out= nLenDst;
    zInfo.next_in= (unsigned char *)abSrc;
    zInfo.next_out= abDst;

    int nErr, nRet= -1;
    nErr= inflateInit( &zInfo );               // zlib function
    switch( nErr )
    {
    case Z_OK:
        // all is fine
        break ;
    default:
        n_log( LOG_ERR, "%s on string %p size %d", zError( nErr ), abSrc, nLenSrc );
        return -1 ;
    }
    nErr= inflate( &zInfo, Z_FINISH );     // zlib function
    if( nErr == Z_STREAM_END )
    {
        nRet= zInfo.total_out;
    }
    else
    {
        n_log( LOG_ERR, "%s on string %p size %d", zError( nErr ), abSrc, nLenSrc );
    }
    inflateEnd( &zInfo );   // zlib function
    return( nRet ); // -1 or len of output
} /* UncompressData */



/*!\fn N_STR *zip_nstr( N_STR *src )
 *\param src The source string
 *\return The compressed string or NULL
 */
N_STR *zip_nstr( N_STR *src )
{
    __n_assert( src, return NULL );
    __n_assert( src -> data, return NULL );

    if( src -> length <= 0 )
    {
        n_log( LOG_ERR, "length of src (%d) <= 0", src -> length );
        return NULL ;
    }
    if( src -> written <= 0 )
    {
        n_log( LOG_ERR, "written of src (%d) <= 0", src -> written );
        return NULL ;
    }

    /* storage for original string size + zipped string  + padding */
    unsigned int zip_max_size = GetMaxCompressedLen( src -> length );

    N_STR *zipped = new_nstr( 4 + zip_max_size );

    __n_assert( zipped, return NULL );
    __n_assert( zipped -> data, return NULL );

    /* copying size */
    int32_t src_length = htonl( src -> length );
    memcpy( zipped -> data, &src_length, sizeof( int32_t ) );
    char *dataptr =  zipped -> data + 4 ;

    int compressed_size = 0 ;
    compressed_size = CompressData( (unsigned char *)src -> data, src -> written, (unsigned char *)dataptr, zip_max_size );
    if( compressed_size == -1 )
    {
        free_nstr( &zipped );
        n_log( LOG_ERR, "unable to zip string %p  %d/%d bytes", src -> data, src -> written, src -> length );
        return NULL ;
    }
    zipped -> written = 4 + compressed_size ;
    n_log( LOG_DEBUG, "zip :%d original: %d", zipped -> written, src -> length );

    return zipped ;
} /* zip_nstr */



/*!\fn N_STR *unzip_nstr( N_STR *src )
 *\param src The source string
 *\return The uncompressed string or NULL
 */
N_STR *unzip_nstr( N_STR *src )
{
    __n_assert( src, return NULL );
    __n_assert( src -> data, return NULL );

    if( src -> length <= 0 )
    {
        n_log( LOG_ERR, "length of src (%d) <= 0", src -> length );
        return NULL ;
    }
    if( src -> written <= 0 )
    {
        n_log( LOG_ERR, "written of src (%d) <= 0", src -> written );
        return NULL ;
    }

    int32_t original_size = 0 ;
    memcpy( &original_size, src -> data, sizeof( int32_t ) );
    original_size = ntohl( original_size );
    if( original_size <= 0 )
    {
        n_log( LOG_ERR, "original size (%d) <= 0", original_size );
        return NULL ;
    }
    /* storage for original string size + zipped string  + padding */
    N_STR *unzipped = new_nstr( original_size );
    __n_assert( unzipped, return NULL );
    __n_assert( unzipped -> data, return NULL );

    /* copying size */
    unzipped -> written = UncompressData( ((unsigned char *)src -> data) + 4, src -> written, (unsigned char *)unzipped -> data, original_size );
    if( unzipped -> written == -1 )
    {
        n_log( LOG_ERR, "unable to unzip string %p  %d/%d bytes", unzipped -> data, unzipped -> written, unzipped -> length );
        free_nstr( &unzipped );
        return NULL ;
    }
    n_log( LOG_DEBUG, "Size: zip: %d => unzip :%d original:%d", src -> written, unzipped -> written, original_size );

    return unzipped ;
} /* unzip_nstr */
