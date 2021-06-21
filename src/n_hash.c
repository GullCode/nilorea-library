/**\file n_hash.c
 * Hash functions and table
 *\author Castagnier Mickael
 *\version 2.0
 *\date 16/03/2015
 */

#include "nilorea/n_common.h"
#include "nilorea/n_log.h"
#include "nilorea/n_hash.h"

#include <pthread.h>
#include <string.h>

#ifdef __windows__
#include <winsock.h>
#else
#include <arpa/inet.h>
#endif

#ifndef _MSC_VER
/*!\fn static FORCE_INLINE uint32_t rotl32 ( uint32_t x, int8_t r )
 *\brief rotate byte 32bit version (from murmur's author)
 *\param x value
 *\param r rotation value
 *\return rotated value
 */
static FORCE_INLINE uint32_t rotl32( uint32_t x, int8_t r )
{
    return (x << r) | (x >> (32 - r));
} /* rotl32(...) */
#endif

/*!\fn static FORCE_INLINE uint32_t getblock( const uint32_t *p, int i )
 *\brief Block read - (from murmur's author)
 if your platform needs to do endian-swapping or can only
 handle aligned reads, do the conversion here
 *\param p value
 *\param i position
 *\return value at position inside block
 */
static FORCE_INLINE uint32_t getblock( const uint32_t *p, int i )
{
    uint32_t var ;
    memcpy( &var, p+i, sizeof( uint32_t ) );
#if( BYTEORDER_ENDIAN == BYTEORDER_LITTLE_ENDIAN )
    var = htonl( var );
#endif
    return var ;
} /* get_block(...) */



/*!\fn static FORCE_INLINE uint32_t fmix32 ( uint32_t h )
 *\brief Finalization mix - force all bits of a hash block to avalanche (from murmur's author)
 *\param h value
 *\return mixed value
 */
static FORCE_INLINE uint32_t fmix32 ( uint32_t h )
{
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;

    return h;
} /* fmix32(...) */

/*!\fn static FORCE_INLINE uint64_t fmix64 ( uint64_t k )
 *\brief Finalization mix - force all bits of a hash block to avalanche, 64bit version (from murmur's author)
 *\param k value
 *\return mixed value
 */
static FORCE_INLINE uint64_t fmix64 ( uint64_t k )
{
    k ^= k >> 33;
    k *= BIG_CONSTANT(0xff51afd7ed558ccd);
    k ^= k >> 33;
    k *= BIG_CONSTANT(0xc4ceb9fe1a85ec53);
    k ^= k >> 33;

    return k;
} /* fmix64(...) */



/*!\fn void MurmurHash3_x86_32 ( const void * key, int len, uint32_t seed, void * out )
 * brief MurmurHash3 was written by Austin Appleby, and is placed in the public
 domain. The author hereby disclaims copyright to this source code.
 Note - The x86 and x64 versions do _not_ produce the same results, as the
 algorithms are optimized for their respective platforms. You can still
 compile and run any of them on any platform, but your performance with the
 non-native version will be less than optimal.
 *\param key char *string as the key
 *\param len size of the key
 *\param seed seed value for murmur hash
 *\param out generated hash
 */
void MurmurHash3_x86_32 ( const void * key, int len, uint32_t seed, void * out )
{
    const uint8_t * data = (const uint8_t*)key;
    const int nblocks = len / 4;

    uint32_t h1 = seed;

    uint32_t c1 = 0xcc9e2d51;
    uint32_t c2 = 0x1b873593;

    /* body */
    const uint32_t * blocks = (const uint32_t *)(data + nblocks*4);

    int i = 0 ;
    for(i = -nblocks; i; i++)
    {
        uint32_t k1 = getblock(blocks,i);

        k1 *= c1;
        k1 = ROTL32(k1,15);
        k1 *= c2;

        h1 ^= k1;
        h1 = ROTL32(h1,13);
        h1 = h1*5+0xe6546b64;
    }

    /* tail */
    const uint8_t * tail = (const uint8_t*)(data + nblocks*4);

    uint32_t k1 = 0;

    switch(len & 3)
    {
    case 3:
        k1 ^= tail[2] << 16;
        FALL_THROUGH ;
    case 2:
        k1 ^= tail[1] << 8;
        FALL_THROUGH ;
    case 1:
        k1 ^= tail[0];
        k1 *= c1;
        k1 = ROTL32(k1,15);
        k1 *= c2;
        h1 ^= k1;
    default:
        break ;
    };

    /* finalization */
    h1 ^= len;
    h1 = fmix32(h1);
    *(uint32_t*)out = h1;
} /* MurmurHash3_x86_32(...) */



/*!\fn void MurmurHash3_x86_128( const void * key, int len, uint32_t seed, void * out )
 * brief MurmurHash3 was written by Austin Appleby, and is placed in the public
 domain. The author hereby disclaims copyright to this source code.
 Note - The x86 and x64 versions do _not_ produce the same results, as the
 algorithms are optimized for their respective platforms. You can still
 compile and run any of them on any platform, but your performance with the
 non-native version will be less than optimal.
 *\param key char *string as the key
 *\param len size of the key
 *\param seed seed value for murmur hash
 *\param out generated hash
 */
void MurmurHash3_x86_128 ( const void * key, const int len, uint32_t seed, void * out )
{
    const uint8_t * data = (const uint8_t*)key;
    const int nblocks = len / 16;

    uint32_t h1 = seed;
    uint32_t h2 = seed;
    uint32_t h3 = seed;
    uint32_t h4 = seed;

    uint32_t c1 = 0x239b961b;
    uint32_t c2 = 0xab0e9789;
    uint32_t c3 = 0x38b34ae5;
    uint32_t c4 = 0xa1e38b93;

    /* body */
    const uint32_t * blocks = (const uint32_t *)(data + nblocks*16);

    int i = 0 ;
    for( i = -nblocks ; i; i++)
    {
        uint32_t k1 = getblock(blocks,i*4+0);
        uint32_t k2 = getblock(blocks,i*4+1);
        uint32_t k3 = getblock(blocks,i*4+2);
        uint32_t k4 = getblock(blocks,i*4+3);

        k1 *= c1;
        k1  = ROTL32(k1,15);
        k1 *= c2;
        h1 ^= k1;
        h1 = ROTL32(h1,19);
        h1 += h2;
        h1 = h1*5+0x561ccd1b;

        k2 *= c2;
        k2  = ROTL32(k2,16);
        k2 *= c3;
        h2 ^= k2;
        h2 = ROTL32(h2,17);
        h2 += h3;
        h2 = h2*5+0x0bcaa747;

        k3 *= c3;
        k3  = ROTL32(k3,17);
        k3 *= c4;
        h3 ^= k3;
        h3 = ROTL32(h3,15);
        h3 += h4;
        h3 = h3*5+0x96cd1c35;

        k4 *= c4;
        k4  = ROTL32(k4,18);
        k4 *= c1;
        h4 ^= k4;
        h4 = ROTL32(h4,13);
        h4 += h1;
        h4 = h4*5+0x32ac3b17;
    }

    /* tail */
    const uint8_t * tail = (const uint8_t*)(data + nblocks*16);

    uint32_t k1 = 0;
    uint32_t k2 = 0;
    uint32_t k3 = 0;
    uint32_t k4 = 0;

    switch(len & 15)
    {
    case 15:
        k4 ^= tail[14] << 16;
        FALL_THROUGH ;
    case 14:
        k4 ^= tail[13] << 8;
        FALL_THROUGH ;
    case 13:
        k4 ^= tail[12] << 0;
        k4 *= c4;
        k4  = ROTL32(k4,18);
        k4 *= c1;
        h4 ^= k4;
        FALL_THROUGH ;
    case 12:
        k3 ^= tail[11] << 24;
        FALL_THROUGH ;
    case 11:
        k3 ^= tail[10] << 16;
        FALL_THROUGH ;
    case 10:
        k3 ^= tail[ 9] << 8;
        FALL_THROUGH ;
    case  9:
        k3 ^= tail[ 8] << 0;
        k3 *= c3;
        k3  = ROTL32(k3,17);
        k3 *= c4;
        h3 ^= k3;
        FALL_THROUGH ;
    case  8:
        k2 ^= tail[ 7] << 24;
        FALL_THROUGH ;
    case  7:
        k2 ^= tail[ 6] << 16;
        FALL_THROUGH ;
    case  6:
        k2 ^= tail[ 5] << 8;
        FALL_THROUGH ;
    case  5:
        k2 ^= tail[ 4] << 0;
        k2 *= c2;
        k2  = ROTL32(k2,16);
        k2 *= c3;
        h2 ^= k2;
        FALL_THROUGH ;
    case  4:
        k1 ^= tail[ 3] << 24;
        FALL_THROUGH ;
    case  3:
        k1 ^= tail[ 2] << 16;
        FALL_THROUGH ;
    case  2:
        k1 ^= tail[ 1] << 8;
        FALL_THROUGH ;
    case  1:
        k1 ^= tail[ 0] << 0;
        k1 *= c1;
        k1  = ROTL32(k1,15);
        k1 *= c2;
        h1 ^= k1;
    default:
        break ;
    };
    /* finalization */
    h1 ^= len;
    h2 ^= len;
    h3 ^= len;
    h4 ^= len;

    h1 += h2;
    h1 += h3;
    h1 += h4;
    h2 += h1;
    h3 += h1;
    h4 += h1;

    h1 = fmix32(h1);
    h2 = fmix32(h2);
    h3 = fmix32(h3);
    h4 = fmix32(h4);

    h1 += h2;
    h1 += h3;
    h1 += h4;
    h2 += h1;
    h3 += h1;
    h4 += h1;

    ((uint32_t*)out)[0] = h1;
    ((uint32_t*)out)[1] = h2;
    ((uint32_t*)out)[2] = h3;
    ((uint32_t*)out)[3] = h4;
} /* MurmurHash3_x86_32(...) */


/*!\fn void ht_node_destroy( void *node )
 *\brief destroy a HASH_NODE by first calling the HASH_NODE destructor
 *\param node The node to kill
 */
void ht_node_destroy( void *node )
{
    HASH_NODE *node_ptr = (HASH_NODE *)node ;
    __n_assert( node_ptr, return );
    if( node_ptr -> type == HASH_STRING )
    {
        Free( node_ptr  -> data . string );
    }
    if( node_ptr -> type == HASH_PTR )
    {
        if( node_ptr -> destroy_func && node_ptr -> data . ptr )
        {
            node_ptr -> destroy_func( node_ptr  -> data . ptr );
        }
        /* No free by default. must be passed as a destroy_func
           else
           {
           Free( node_ptr  -> data . ptr );
           }
           */
    }
    FreeNoLog( node_ptr -> key );
    Free( node_ptr )
} /* ht_node_destroy */



/*!\fn char *ht_node_type( HASH_NODE *node )
 *\brief get the type of a node , text version
 *\param node node to check
 *\return NULL or the associated node type
 */
char *ht_node_type( HASH_NODE *node )
{
    static char *HASH_TYPE_STR[ 5 ] = { "HASH_INT", "HASH_DOUBLE", "HASH_STRING", "HASH_PTR", "HASH_UNKNOWN" };

    __n_assert( node, return NULL );

    if( node -> type >= 0 && node -> type < HASH_UNKNOWN )
        return HASH_TYPE_STR[ node -> type ];

    return NULL ;
} /* ht_node_type(...) */



/*!\fn HASH_TABLE *new_ht( int size )
 *\brief Create a hash table with the given size
 *\param size Size of the root hash node table
 *\return NULL or the new allocated hash table
 */
HASH_TABLE *new_ht( unsigned long int size )
{
    HASH_TABLE *table = NULL ;

    if( size < 1 )
    {
        n_log( LOG_ERR, "Invalide size %d for new_ht()", size );
        return NULL ;
    }
    Malloc( table, HASH_TABLE, 1 );
    __n_assert( table, n_log( LOG_ERR, "Error allocating HASH_TABLE *table" ); return NULL );

    table -> size = size ;
    table -> nb_keys = 0 ;
    errno = 0 ;
    Malloc( table -> hash_table, LIST *, size );
    // table -> hash_table = (LIST **)calloc( size, sizeof( LIST *) );
    __n_assert( table -> hash_table, n_log( LOG_ERR, "Can't allocate table -> hash_table with size %d !", size ); Free( table ); return NULL );

    unsigned long int it = 0 ;
    for(  it = 0 ; it < size ; it ++ )
    {
        table -> hash_table[ it ] = new_generic_list( 0 );
        // if no valid table then unroll previsouly created slots
        if( !table -> hash_table[ it ] )
        {
            n_log( LOG_ERR, "Can't allocate table -> hash_table[ %d ] !", it );
            unsigned long int it_delete = 0 ;
            for( it_delete = 0 ; it_delete < it ; it_delete ++ )
            {
                list_destroy( &table -> hash_table[ it_delete ] );
            }
            Free( table -> hash_table );
            Free( table );
            return NULL ;
        }
    }
    return table ;
} /* new_ht */



/*!\fn int ht_put_int( HASH_TABLE *table , char * key , int val )
 *\brief put an integral value with given key in the targeted hash table
 *\param table Targeted hash table
 *\param key Size Associated value's key
 *\param val integral value to put
 *\return TRUE or FALSE
 */
int ht_put_int( HASH_TABLE *table, char * key, int val )
{
    uint32_t hash_value = 0 ;
    unsigned long int index = 0;

    HASH_NODE *new_hash_node = NULL ;
    HASH_NODE *node_ptr = NULL ;

    __n_assert( table, return FALSE );
    __n_assert( key, return FALSE );

    MurmurHash3_x86_32( key, strlen( key ), 42, &hash_value );
    index= (hash_value)%(table->size) ;

    /* we have some nodes here. Let's check if the key already exists */
    list_foreach( list_node, table -> hash_table[ index ] )
    {
        node_ptr = (HASH_NODE *)list_node -> ptr ;
        /* if we found the same key we just replace the value and return */
        if( node_ptr -> key_type == HASH_KEY_STRING && !strcmp( key, node_ptr -> key ) )
        {
            /* let's check the key isn't already assigned with another data type */
            if( node_ptr -> type == HASH_INT )
            {
                node_ptr -> data . ival = val ;
                return TRUE ;
            }
            n_log( LOG_ERR, "Can't add key[\"%s\"] with type HASH_INT, key already exist with type %s", node_ptr -> key, ht_node_type( node_ptr ) );
            return FALSE ; /* key registered with another data type */
        }
    }
    Malloc( new_hash_node, HASH_NODE, 1 );
    __n_assert( new_hash_node, n_log( LOG_ERR, "Could not allocate new_hash_node" ); return FALSE );

    new_hash_node -> key = strdup( key );
    new_hash_node -> key_type = HASH_KEY_STRING ;
    new_hash_node -> hash_value = 0 ;
    new_hash_node -> data. ival = val ;
    new_hash_node -> type = HASH_INT ;
    new_hash_node -> destroy_func = NULL ;

    table -> nb_keys ++ ;

    return list_push( table -> hash_table[ index ], new_hash_node, &ht_node_destroy );
} /*ht_put_int() */



/*!\fn int ht_put_double( HASH_TABLE *table , char * key , double val )
 *\brief put a double value with given key in the targeted hash table
 *\param table Targeted hash table
 *\param key Size Associated value's key
 *\param val double value to put
 *\return TRUE or FALSE
 */
int ht_put_double( HASH_TABLE *table, char * key, double val )
{
    uint32_t hash_value = 0 ;
    unsigned long int index = 0;

    HASH_NODE *new_hash_node = NULL ;
    HASH_NODE *node_ptr = NULL ;

    __n_assert( table, return FALSE );
    __n_assert( key, return FALSE );

    MurmurHash3_x86_32( key, strlen( key ), 42, &hash_value );
    index= (hash_value)%(table->size) ;

    /* we have some nodes here. Let's check if the key already exists */
    list_foreach( list_node, table -> hash_table[ index ] )
    {
        node_ptr = (HASH_NODE *)list_node -> ptr ;
        /* if we found the same key we just replace the value and return */
        if( node_ptr -> key_type == HASH_KEY_STRING && !strcmp( key, node_ptr -> key ) )
        {
            /* let's check the key isn't already assigned with another data type */
            if( node_ptr -> type == HASH_DOUBLE )
            {
                node_ptr -> data . fval = val ;
                return TRUE ;
            }
            n_log( LOG_ERR, "Can't add key[\"%s\"] with type HASH_DOUBLE, key already exist with type %s", node_ptr -> key, ht_node_type( node_ptr ) );
            return FALSE ; /* key registered with another data type */
        }
    }

    Malloc( new_hash_node, HASH_NODE, 1 );
    __n_assert( new_hash_node, n_log( LOG_ERR, "Could not allocate new_hash_node" ); return FALSE );

    new_hash_node -> key = strdup( key );
    new_hash_node -> key_type = HASH_KEY_STRING ;
    new_hash_node -> hash_value = 0 ;
    new_hash_node -> data. fval = val ;
    new_hash_node -> type = HASH_DOUBLE ;
    new_hash_node -> destroy_func = NULL ;

    table -> nb_keys ++ ;

    return list_push( table -> hash_table[ index ], new_hash_node, &ht_node_destroy );
} /*ht_put_double()*/



/*!\fn int ht_put_ptr( HASH_TABLE *table , char *key , void  *val , void (*destructor)(void *ptr ) )
 *\brief put a pointer value with given key in the targeted hash table
 *\param table Targeted hash table
 *\param key Associated value's key
 *\param val pointer value to put
 *\param destructor Pointer to the ptr type destructor function. Leave to NULL if there isn't
 *\return TRUE or FALSE
 */
int ht_put_ptr( HASH_TABLE *table, char *key, void  *val, void (*destructor)(void *ptr ) )
{
    uint32_t hash_value = 0 ;
    unsigned long int index = 0;

    HASH_NODE *new_hash_node = NULL ;
    HASH_NODE *node_ptr = NULL ;

    __n_assert( table, return FALSE );
    __n_assert( key, return FALSE );
    if( strlen( key ) == 0 )
        return FALSE ;


    MurmurHash3_x86_32( key, strlen( key ), 42, &hash_value );
    index= (hash_value)%(table->size) ;

    /* we have some nodes here. Let's check if the key already exists */
    list_foreach( list_node, table -> hash_table[ index ] )
    {
        node_ptr = (HASH_NODE *)list_node -> ptr ;
        /* if we found the same key we just replace the value and return */
        if( node_ptr -> key_type == HASH_KEY_STRING && !strcmp( key, node_ptr -> key ) )
        {
            /* let's check the key isn't already assigned with another data type */
            if( node_ptr -> type == HASH_PTR )
            {
                if( list_node -> destroy_func )
                {
                    list_node -> destroy_func( node_ptr -> data . ptr );
                    node_ptr -> data . ptr = val ;
                    list_node -> destroy_func = destructor ;
                }
                return TRUE ;
            }
            n_log( LOG_ERR, "Can't add key[\"%s\"] with type HASH_PTR , key already exist with type %s", node_ptr -> key, ht_node_type( node_ptr ) );
            return FALSE ; /* key registered with another data type */
        }
    }

    Malloc( new_hash_node, HASH_NODE, 1 );
    __n_assert( new_hash_node, n_log( LOG_ERR, "Could not allocate new_hash_node" ); return FALSE );

    new_hash_node -> key = strdup( key );
    new_hash_node -> key_type = HASH_KEY_STRING ;
    new_hash_node -> hash_value = 0 ;
    new_hash_node -> data . ptr = val ;
    new_hash_node -> type = HASH_PTR ;
    new_hash_node -> destroy_func = destructor ;

    table -> nb_keys ++ ;

    return list_push( table -> hash_table[ index ], new_hash_node, &ht_node_destroy );
}/* ht_put_ptr() */


/*!\fn int ht_put_ptr_ex( HASH_TABLE *table, unsigned long int hash_value , void  *val, void (*destructor)( void *ptr ) )
 *\brief put a pointer value with given key in the targeted hash table
 *\param table Targeted hash table
 *\param hash_value numerical hash key
 *\param val pointer value to put
 *\param destructor Pointer to the ptr type destructor function. Leave to NULL if there isn't
 *\return TRUE or FALSE
 */
int ht_put_ptr_ex( HASH_TABLE *table, unsigned long int hash_value, void  *val, void (*destructor)( void *ptr ) )
{
    unsigned long int index = 0;

    HASH_NODE *new_hash_node = NULL ;
    HASH_NODE *node_ptr = NULL ;

    __n_assert( table, return FALSE );

    index = (hash_value)%(table->size) ;

    /* we have some nodes here. Let's check if the key already exists */
    list_foreach( list_node, table -> hash_table[ index ] )
    {
        node_ptr = (HASH_NODE *)list_node -> ptr ;
        /* if we found the same key we just replace the value and return */
        if( node_ptr -> key_type == HASH_KEY_NUM && hash_value == node_ptr -> hash_value )
        {
            /* let's check the key isn't already assigned with another data type */
            if( node_ptr -> type == HASH_PTR )
            {
                if( list_node -> destroy_func )
                {
                    list_node -> destroy_func( node_ptr -> data . ptr );
                    node_ptr -> data . ptr = val ;
                    list_node -> destroy_func = destructor ;
                }
                return TRUE ;
            }
            n_log( LOG_ERR, "Can't add key[\"%s\"] with type HASH_PTR , key already exist with type %s", node_ptr -> key, ht_node_type( node_ptr ) );
            return FALSE ; /* key registered with another data type */
        }
    }

    Malloc( new_hash_node, HASH_NODE, 1 );
    __n_assert( new_hash_node, n_log( LOG_ERR, "Could not allocate new_hash_node" ); return FALSE );

    new_hash_node -> key = NULL ;
    new_hash_node -> hash_value = hash_value ;
    new_hash_node -> key_type = HASH_KEY_NUM ;
    new_hash_node -> data . ptr = val ;
    new_hash_node -> type = HASH_PTR ;
    new_hash_node -> destroy_func = destructor ;

    table -> nb_keys ++ ;

    return list_push( table -> hash_table[ index ], new_hash_node, &ht_node_destroy );
}/* ht_put_ptr() */



/*!\fn int ht_put_string( HASH_TABLE *table , char *key , char  *val )
 *\brief put a null terminated char *string with given key in the targeted hash table
 *\param table Targeted hash table
 *\param key Associated value's key
 *\param val string value to put (will be strdup'ed)
 *\return TRUE or FALSE
 */
int ht_put_string( HASH_TABLE *table, char *key, char  *val )
{
    uint32_t hash_value = 0 ;
    unsigned long int index = 0;

    HASH_NODE *new_hash_node = NULL ;
    HASH_NODE *node_ptr = NULL ;

    __n_assert( table, return FALSE );
    __n_assert( key, return FALSE );
    if( strlen( key ) == 0 )
        return FALSE ;

    MurmurHash3_x86_32( key, strlen( key ), 42, &hash_value );
    index= (hash_value)%(table->size) ;

    /* we have some nodes here. Let's check if the key already exists */
    list_foreach( list_node, table -> hash_table[ index ] )
    {
        node_ptr = (HASH_NODE *)list_node -> ptr ;
        /* if we found the same key we just replace the value and return */
        if( node_ptr -> key_type == HASH_KEY_STRING && !strcmp( key, node_ptr -> key ) )
        {
            /* let's check the key isn't already assigned with another data type */
            if( node_ptr -> type == HASH_STRING )
            {
                Free( node_ptr -> data . string );
                node_ptr -> data . string = strdup( val );
                return TRUE ;
            }
            n_log( LOG_ERR, "Can't add key[\"%s\"] with type HASH_STRING, key already exist with type %s", node_ptr -> key, ht_node_type( node_ptr ) );
            return FALSE ; /* key registered with another data type */
        }
    }

    Malloc( new_hash_node, HASH_NODE, 1 );
    __n_assert( new_hash_node, n_log( LOG_ERR, "Could not allocate new_hash_node" ); return FALSE );

    new_hash_node -> key = strdup( key );
    new_hash_node -> key_type = HASH_KEY_STRING ;
    new_hash_node -> hash_value = 0 ;
    new_hash_node -> data . string = strdup( val );
    new_hash_node -> type = HASH_STRING ;
    new_hash_node -> destroy_func = NULL ;

    table -> nb_keys ++ ;

    return list_push( table -> hash_table[ index ], new_hash_node, &ht_node_destroy );
} /*ht_put_string */



/*!\fn HASH_NODE *ht_get_node( HASH_TABLE *table , char *key )
 *\brief return the associated key's node inside the hash_table
 *\param table Targeted hash table
 *\param key Associated value's key
 *\return The found node, or NULL
 */
HASH_NODE *ht_get_node( HASH_TABLE *table, char *key )
{
    uint32_t hash_value = 0 ;
    unsigned long int index = 0;

    HASH_NODE *node_ptr = NULL ;

    __n_assert( table, return NULL );
    __n_assert( key, return NULL );

    if( strlen( key ) == 0 )
        return FALSE ;

    MurmurHash3_x86_32( key, strlen( key ), 42, &hash_value );
    index= (hash_value)%(table->size) ;

    if( !table -> hash_table[ index ] -> start )
    {
        return NULL ;
    }

    list_foreach( list_node, table -> hash_table[ index ] )
    {
        node_ptr = (HASH_NODE *)list_node -> ptr ;
        if( node_ptr -> key_type == HASH_KEY_STRING && !strcmp( key, node_ptr -> key ) )
        {
            return node_ptr ;
        }
    }
    return NULL ;
} /* ht_get_node() */



/*!\fn HASH_NODE *ht_get_node_ex( HASH_TABLE *table , unsigned long int hash_value )
 *\brief return the associated key's node inside the hash_table
 *\param table Targeted hash table
 *\param hash_value Associated hash_value
 *\return The found node, or NULL
 */
HASH_NODE *ht_get_node_ex( HASH_TABLE *table, unsigned long int hash_value )
{
    unsigned long int index = 0;

    HASH_NODE *node_ptr = NULL ;

    __n_assert( table, return NULL );

    index= (hash_value)%(table->size) ;
    if( !table -> hash_table[ index ] -> start )
    {
        return NULL ;
    }

    list_foreach( list_node, table -> hash_table[ index ] )
    {
        node_ptr = (HASH_NODE *)list_node -> ptr ;
        if( node_ptr -> key_type == HASH_KEY_NUM && hash_value == node_ptr -> hash_value )
        {
            return node_ptr ;
        }
    }
    return NULL ;
} /* ht_get_node() */



/*!\fn int ht_get_int( HASH_TABLE *table , char *key , int *val )
 *\brief Retrieve an integral value in the hash table, at the given key. Leave val untouched if key is not found.
 *\param table Targeted hash table
 *\param key Size Associated value's key
 *\param val A pointer to a destination integer
 *\return TRUE or FALSE.
 */
int ht_get_int( HASH_TABLE *table, char *key, int *val )
{
    __n_assert( table, return FALSE );
    __n_assert( key, return FALSE );
    if( strlen( key ) == 0 )
        return FALSE ;

    HASH_NODE *node = ht_get_node( table, key );

    if( !node )
        return FALSE ;

    if( node -> type != HASH_INT )
    {
        n_log( LOG_ERR, "Can't get key[\"%s\"] of type HASH_INT, key is type %s", key, ht_node_type( node ) );
        return FALSE ;
    }

    (*val) = node -> data . ival ;

    return TRUE ;
} /* ht_get_int() */



/*!\fn int ht_get_double( HASH_TABLE *table , char *key , double *val )
 *\brief Retrieve a double value in the hash table, at the given key. Leave val untouched if key is not found.
 *\param table Targeted hash table
 *\param key Size Associated value's key
 *\param val A pointer to a destination double
 *\return TRUE or FALSE.
 */
int ht_get_double( HASH_TABLE *table, char * key, double *val )
{
    __n_assert( table, return FALSE );
    __n_assert( key, return FALSE );

    if( strlen( key ) == 0 )
        return FALSE ;

    HASH_NODE *node = ht_get_node( table, key );

    if( !node )
        return FALSE ;

    if( node -> type != HASH_DOUBLE )
    {
        n_log( LOG_ERR, "Can't get key[\"%s\"] of type HASH_DOUBLE, key is type %s", key, ht_node_type( node ) );
        return FALSE ;
    }

    (*val) = node -> data . fval ;

    return TRUE ;
} /* ht_get_double()*/



/*!\fn int ht_get_ptr( HASH_TABLE *table , char *key , void **val )
 *\brief Retrieve a pointer value in the hash table, at the given key. Leave val untouched if key is not found.
 *\param table Targeted hash table
 *\param key Size Associated value's key
 *\param val A pointer to an empty pointer store
 *\return TRUE or FALSE.
 */
int ht_get_ptr( HASH_TABLE *table, char * key, void  **val )
{
    __n_assert( table, return FALSE );
    __n_assert( key, return FALSE );
    if( strlen( key ) == 0 )
        return FALSE ;

    HASH_NODE *node = ht_get_node( table, key );
    if( !node )
        return FALSE ;

    if( node -> type != HASH_PTR )
    {
        n_log( LOG_ERR, "Can't get key[\"%s\"] of type HASH_PTR, key is type %s", key, ht_node_type( node ) );
        return FALSE ;
    }

    (*val) = node -> data . ptr ;

    return TRUE ;
} /* ht_get_ptr() */



/*!\fn int ht_get_ptr_ex( HASH_TABLE *table, unsigned long int hash_value , void  **val )
 *\brief Retrieve a pointer value in the hash table, at the given key. Leave val untouched if key is not found.
 *\param table Targeted hash table
 *\param key Size Associated value's key
 *\param val A pointer to an empty pointer store
 *\return TRUE or FALSE.
 */
int ht_get_ptr_ex( HASH_TABLE *table, unsigned long int hash_value, void  **val )
{
    __n_assert( table, return FALSE );

    HASH_NODE *node = ht_get_node_ex( table, hash_value );
    if( !node )
        return FALSE ;

    if( node -> type != HASH_PTR )
    {
        n_log( LOG_ERR, "Can't get key[\"%lld\"] of type HASH_PTR, key is type %s", hash_value, ht_node_type( node ) );
        return FALSE ;
    }

    (*val) = node -> data . ptr ;

    return TRUE ;
} /* ht_get_ptr_ex() */


/*!\fn int ht_get_string( HASH_TABLE *table , char *key , char **val )
 *\brief Retrieve a char *string value in the hash table, at the given key. Leave val untouched if key is not found.
 *\param table Targeted hash table
 *\param key Size Associated value's key
 *\param val A pointer to an empty destination char *string
 *\return TRUE or FALSE.
 */
int ht_get_string( HASH_TABLE *table, char * key, char  **val )
{
    __n_assert( table, return FALSE );
    __n_assert( key, return FALSE );
    if( strlen( key ) == 0 )
        return FALSE ;

    HASH_NODE *node = ht_get_node( table, key );
    if( !node )
        return FALSE ;

    if( node -> type != HASH_STRING )
    {
        n_log( LOG_ERR, "Can't get key[\"%s\"] of type HASH_STRING, key is type %s", key, ht_node_type( node ) );
        return FALSE ;
    }

    (*val) = node -> data . string ;

    return TRUE ;
} /* ht_get_string() */



/*!\fn int ht_remove( HASH_TABLE *table , char *key )
 *\brief Remove a key from a hash table
 *\param table Targeted hash table
 *\param key Key to remove
 *\return TRUE or FALSE.
 */
int ht_remove( HASH_TABLE *table, char *key )
{
    uint32_t hash_value = 0 ;
    unsigned long int index = 0;

    HASH_NODE *node_ptr = NULL ;
    LIST_NODE *node_to_kill = NULL ;

    __n_assert( table, return FALSE );
    __n_assert( key, return FALSE );
    if( strlen( key ) == 0 )
        return FALSE ;

    MurmurHash3_x86_32( key, strlen( key ), 42, &hash_value );
    index= (hash_value)%(table->size) ;

    if( !table -> hash_table[ index ] -> start )
    {
        n_log( LOG_ERR, "Can't remove key[\"%s\"], table is empty", key );
        return FALSE ;
    }

    list_foreach( list_node, table -> hash_table[ index ] )
    {
        node_ptr = (HASH_NODE *)list_node -> ptr ;
        /* if we found the same */
        if( node_ptr -> key_type == HASH_KEY_STRING && !strcmp( key, node_ptr -> key ) )
        {
            node_to_kill = list_node ;
            break ;
        }
    }
    if( node_to_kill )
    {
        node_ptr = remove_list_node( table -> hash_table[ index ], node_to_kill, HASH_NODE );
        ht_node_destroy( node_ptr );

        table -> nb_keys -- ;

        return TRUE ;
    }
    n_log( LOG_ERR, "Can't delete key[\"%s\"]: inexisting key", key );
    return FALSE ;
}/* ht_remove() */



/*!\fn int ht_remove_ex( HASH_TABLE *table , unsigned long int hash_value )
 *\brief Remove a key from a hash table
 *\param table Targeted hash table
 *\param key Key to remove
 *\return TRUE or FALSE.
 */
int ht_remove_ex( HASH_TABLE *table, unsigned long int hash_value )
{
    unsigned long int index = 0;

    HASH_NODE *node_ptr = NULL ;
    LIST_NODE *node_to_kill = NULL ;

    __n_assert( table, return FALSE );

    index= (hash_value)%(table->size) ;
    if( !table -> hash_table[ index ] -> start )
    {
        n_log( LOG_ERR, "Can't remove key[\"%d\"], table is empty", hash_value );
        return FALSE ;
    }

    list_foreach( list_node, table -> hash_table[ index ] )
    {
        node_ptr = (HASH_NODE *)list_node -> ptr ;
        /* if we found the same */
        if( node_ptr -> key_type == HASH_KEY_NUM && hash_value == node_ptr -> hash_value )
        {
            node_to_kill = list_node ;
            break ;
        }
    }
    if( node_to_kill )
    {
        node_ptr = remove_list_node( table -> hash_table[ index ], node_to_kill, HASH_NODE );
        ht_node_destroy( node_ptr );

        table -> nb_keys -- ;

        return TRUE ;
    }
    n_log( LOG_ERR, "Can't delete key[\"%d\"]: inexisting key", hash_value );
    return FALSE ;
}/* ht_remove_ex() */



/*!\fn int empty_ht( HASH_TABLE *table )
 *\brief Empty a hash table
 *\param table Targeted hash table
 *\return TRUE or FALSE.
 */
int empty_ht( HASH_TABLE *table )
{
    HASH_NODE *hash_node = NULL ;

    __n_assert( table, return FALSE );

    unsigned long int index = 0 ;
    for( index = 0 ; index < table -> size ; index ++ )
    {
        while( table -> hash_table[ index ] && table -> hash_table[ index ] -> start )
        {
            hash_node = remove_list_node( table -> hash_table[ index ], table -> hash_table[ index ] -> start, HASH_NODE );
            ht_node_destroy( hash_node );
        }
    }
    table -> nb_keys = 0 ;
    return TRUE ;
} /* empty_ht */



/*!\fn int destroy_ht( HASH_TABLE **table )
 *\brief Free and set the table to NULL
 *\param table Targeted hash table
 *\return TRUE or FALSE.
 */
int destroy_ht( HASH_TABLE **table )
{
    __n_assert( table&&(*table), n_log( LOG_ERR, "Can't destroy table: already NULL" ); return FALSE );

    if( (*table )-> hash_table )
    {
        //empty_ht( (*table) );

        unsigned long int it = 0 ;
        for( it = 0 ; it < (*table) -> size ; it ++ )
        {
            if( (*table) -> hash_table[ it ] )
                list_destroy( &(*table) -> hash_table[ it ] );
        }
        Free( (*table )->hash_table );
    }
    Free( (*table ) );
    return TRUE ;
} /* destroy_ht */




