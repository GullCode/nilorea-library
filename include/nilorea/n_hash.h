/**\file n_hash.h
 * Hash functions and table
 *\author Castagnier Mickael
 *\version 2.0
 *\date 16/03/2015
 */

#ifndef NILOREA_HASH_HEADER_GUARD
#define NILOREA_HASH_HEADER_GUARD

#ifdef __cplusplus
extern "C" {
#endif

/**\defgroup HASH_TABLE HASH_TABLE: hash table with multiples predefined type
   \addtogroup HASH_TABLE
  @{
*/


#if defined( __linux__ ) || defined( _AIX ) || defined( __sun )
#include <arpa/inet.h>
#include <string.h>
#else
#include <string.h>
#endif

#include <stdint.h>

#include "n_common.h"
#include "n_list.h"

#if defined(_MSC_VER)
#include <stdlib.h>
/*! compatibility with existing rot func */
#define ROTL32(x,y)     _rotl(x,y)
/*! compatibility with existing func */
#define BIG_CONSTANT(x) (x)
#else
/*! missing ROTL fix 32bit */
#define ROTL32(x,y)     rotl32(x,y)
/*! missing ROTL constant fix 64bit */
#define BIG_CONSTANT(x) (x##LLU)
#endif /* if defined MSVC ... */




/*! value of integral type inside the hash node */
#define HASH_INT       0
/*! value of double type inside the hash node */
#define HASH_DOUBLE    1
/*! value of char * type inside the hash node */
#define HASH_STRING    2
/*! value of pointer type inside the hash node */
#define HASH_PTR       3
/*! value of unknow type inside the hash node */
#define HASH_UNKNOWN    4
/*! value of string key type inside the hash node */
#define HASH_KEY_NUM 5
/*! value of string key type inside the hash node */
#define HASH_KEY_STRING 6


/*! union of the value of a node */
union HASH_VALUE
{
    /*! integral type */
    int    ival ;
    /*! double type */
    double fval ;
    /*! pointer type */
    void   *ptr ;
    /*! char *type */
    char   *string ;
};


/*! structure of a hash table node */
typedef struct HASH_NODE
{
    /*! string key of the node if any, else NULL */
    char *key ;
    /*! numeric key of the node if any, else < 0 */
    unsigned long int hash_value ;
    /*! type of the key */
    int key_type ;
    /*! type of the node */
    int type ;
    /*! data inside the node */
    union HASH_VALUE data ;
    /*! destroy_func */
    void (*destroy_func)( void *ptr );

} HASH_NODE ;


/*! structure of a hash table */
typedef struct HASH_TABLE
{
    /*! size of the hash table */
    unsigned long int size,
             /*! total number of used keys in the table */
             nb_keys ;
    /*! preallocated table */
    LIST **hash_table;
} HASH_TABLE ;


/*! ForEach macro helper */
#define ht_foreach( __ITEM_ , __HASH_  ) \
   if( !__HASH_ ) \
   { \
      n_log( LOG_ERR , "Error in ht_foreach, %s is NULL" , #__HASH_ ); \
   } \
   else \
   for( unsigned long int __hash_it = 0 ; __hash_it < __HASH_ -> size ; __hash_it ++ ) \
   for( LIST_NODE *__ITEM_ = __HASH_ -> hash_table[ __hash_it ] -> start ; __ITEM_ != NULL; __ITEM_ = __ITEM_ -> next )
/*! ForEach macro helper */
#define ht_foreach_r( __ITEM_ , __HASH_ , __ITERATOR_ ) \
   if( !__HASH_ ) \
   { \
      n_log( LOG_ERR , "Error in ht_foreach, %s is NULL" , #__HASH_ ); \
   } \
   else \
   for( unsigned long int __ITERATOR_ = 0 ; __ITERATOR_ < __HASH_ -> size ; __ITERATOR_ ++ ) \
   for( LIST_NODE *__ITEM_ = __HASH_ -> hash_table[ __ITERATOR_ ] -> start ; __ITEM_ != NULL; __ITEM_ = __ITEM_ -> next )


/*! Cast a HASH_NODE element */
#define hash_val( node , type )\
   ( (node && node -> ptr) ? ( (type *)(((HASH_NODE *)node -> ptr )-> data . ptr) ) : NULL )


/* murmur hash func 32bit */
void MurmurHash3_x86_32  ( const void * key, int len, uint32_t seed, void * out );
/* murmur hash func 128bit */
void MurmurHash3_x86_128 ( const void * key, int len, uint32_t seed, void * out );

/* get the type of the node */
char *ht_node_type( HASH_NODE *node );

/* new hash table allocator */
HASH_TABLE *new_ht( unsigned long int size );

/* put an integer */
int ht_put_int( HASH_TABLE *table, char * key, int    val );
/* put a double */
int ht_put_double( HASH_TABLE *table, char * key, double val );
/* put a a pointer */
int ht_put_ptr( HASH_TABLE *table, char * key, void  *val, void (*destructor)( void *ptr ) );
/* put a a pointer, provided numerical hashed value */
int ht_put_ptr_ex( HASH_TABLE *table, unsigned long int hash_value, void  *val, void (*destructor)( void *ptr ) );
/* put an char *string */
int ht_put_string( HASH_TABLE *table, char * key, char  *val );

/* get a given key's node */
HASH_NODE *ht_get_node( HASH_TABLE *table, char *key ) ;
/* get an int from a key's node */
int ht_get_int( HASH_TABLE *table, char * key, int *val );
/* get a double from a key's node */
int ht_get_double( HASH_TABLE *table, char * key, double *val );
/* get a pointer from a key's node */
int ht_get_ptr( HASH_TABLE *table, char * key, void  **val );
/* get a pointer from a key's node, provided numerical hashed value */
int ht_get_ptr_ex( HASH_TABLE *table, unsigned long int hash_value, void  **val );
/* get a char *string from a key's node */
int ht_get_string( HASH_TABLE *table, char * key, char  **val );

/* remove given's key node from the table */
int ht_remove( HASH_TABLE *table, char *key );
/* remove given's hash_value from the table */
int ht_remove_ex( HASH_TABLE *table, unsigned long int hash_value );
/* empty a hash table. char *strings are also freed */
int empty_ht( HASH_TABLE *table );
/* destroy a hash table*/
int destroy_ht( HASH_TABLE **table );

/**
@}
*/

#ifdef __cplusplus
}
#endif

#endif
