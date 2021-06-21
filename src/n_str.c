/**\file n_str.c
 *  string function
 *  Everything you need to use string is here
 *\author Castagnier Mickael
 *\version 1.0
 *\date 01/04/05
 */


#ifndef NO_NSTR


#include "nilorea/n_common.h"
#include "nilorea/n_log.h"
#include "nilorea/n_str.h"

#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <dirent.h>


#ifdef __windows__
const char *strcasestr(const char *s1, const char *s2)
{
    __n_assert( s1, return NULL );
    __n_assert( s2, return NULL );

    size_t n = strlen(s2);
    while( *s1 )
    {
        if( !strnicmp( s1++, s2,n ) )
            return ( s1-1 );
    }
    return NULL ;
}
#endif


/*!\fn void free_nstr_ptr( void *ptr )
 *\brief Free a N_STR pointer structure
 *\param ptr A N_STR *object to free
 */
void free_nstr_ptr( void *ptr )
{
    N_STR *strptr = (N_STR *)ptr ;
    if( strptr )
    {
        FreeNoLog( strptr -> data );
        FreeNoLog( strptr );
    }
} /* free_nstr_ptr( ... ) */



/*!\fn int _free_nstr( N_STR **ptr )
 *\brief Free a N_STR structure and set the pointer to NULL
 *\param ptr A N_STR *object to free
 *\return TRUE or FALSE
 */
int _free_nstr( N_STR **ptr )
{
    __n_assert( ptr&&(*ptr), return FALSE );

    FreeNoLog( (*ptr) -> data );
    FreeNoLog( (*ptr) );

    return TRUE ;
} /* free_nstr( ... ) */



/*!\fn int free_nstr_nolog( N_STR **ptr )
 *\brief Free a N_STR structure and set the pointer to NULL
 *\param ptr A N_STR *object to free
 *\return TRUE or FALSE
 */
int free_nstr_nolog( N_STR **ptr )
{

    if( (*ptr) )
    {
        FreeNoLog( (*ptr) -> data );
        FreeNoLog( (*ptr) );
    }

    return TRUE ;
} /* free_nstr( ... ) */



/*!\fn void free_nstr_ptr_nolog( void *ptr )
 *\brief Free a N_STR pointer structure
 *\param ptr A N_STR *object to free
 */
void free_nstr_ptr_nolog( void *ptr )
{
    N_STR *strptr = (N_STR *)ptr ;
    if( strptr )
    {
        FreeNoLog( strptr -> data );
        FreeNoLog( strptr );
    }
} /* free_nstr_ptr_nolog( ... ) */



/*!\fn char *trim_nocopy(char *s)
 *\brief trim and zero end the string, WARNING: keep and original pointer to delete the string correctly
 *\param s The string to trim
 *\return the trimmed string or NULL
 */
char *trim_nocopy(char *s)
{
    __n_assert( s, return NULL );

    if( strlen( s ) == 0 )
        return s ;

    char *start = s;

    /* skip spaces at start */
    while(*start && isspace(*start))
        start++;

    char *end = s + strlen( s ) - 1 ;
    /* iterate over the rest remebering last non-whitespace */
    while( *end && isspace( *end) && end > s )
    {
        end --;
    }
    end ++ ;
    /* write the terminating zero after last non-whitespace */
    *end = 0;

    return start;
} /* trim_nocopy */


/*!\fn char *trim(char *s)
 *\brief trim and put a \0 at the end, return new char *
 *\param s The string to trim
 *\return the trimmed string or NULL
 */
char *trim(char *s)
{
    __n_assert( s, return NULL );

    return strdup( trim_nocopy( s ) );
} /* trim_nocopy */



/*!\fn char *nfgets( char *buffer , int size , FILE *stream )
 *\brief try to fgets
 *\param buffer The string where to read the file
 *\param size Size of the string
 *\param stream The file to read
 *\return NULL or the captured string
 */
char *nfgets( char *buffer, NSTRBYTE size, FILE *stream )
{
    __n_assert( buffer, return NULL );
    __n_assert( stream, return NULL );

    NSTRBYTE it = 0  ;

    if( !fgets( buffer, size, stream ) )
    {
        return NULL ;
    }

    if( it == ( size - 1 ) )
    {
        n_log( LOG_DEBUG, "buffer %p size %d fully filled by fgets on stream %p", buffer, size, stream );
    }

    return buffer ;
} /* nfgets(...) */



/*!\fn int empty_nstr( N_STR *nstr )
 *\brief empty a N_STR string
 *\param nstr the N_STR *str string to empty
 *\return TRUE or FALSE
 */
int empty_nstr( N_STR *nstr )
{
    __n_assert( nstr, return FALSE );
    __n_assert( nstr -> data, return FALSE );

    nstr -> written = 0 ;
    memset( nstr -> data, 0, nstr -> length );

    return TRUE ;
}

/*!\fn N_STR *new_nstr( NSTRBYTE size )
 *\brief create a new N_STR string
 *\param size Size of the new string. 0 for no allocation.
 *\return A new allocated N_STR or NULL
 */
N_STR *new_nstr( NSTRBYTE size )
{
    N_STR *str = NULL ;

    Malloc( str, N_STR, 1 );
    __n_assert( str, return NULL );

    str -> written = 0 ;
    if( size <= 0 )
    {
        str -> data = NULL ;
        str -> length = 0 ;
    }
    else
    {
        n_log( LOG_DEBUG , "size: %d" , size );
	Malloc( str -> data, char, size );
        __n_assert( str -> data, Free(str) ; return NULL );
        str -> length = size  ;
    }
    return str ;
} /* new_nstr(...) */



/*!\fn char_to_nstr_ex( const char *from , NSTRBYTE nboct , N_STR **to )
 *\brief Convert a char into a N_STR, extended version
 *\param from A char *string to convert
 *\param nboct  The size to copy, from 1 octet to nboctet (ustrsizez( from ) )
 *\param to A N_STR pointer who will be Malloced
 *\return True on success, FALSE on failure ( to will be set to NULL )
 */
int char_to_nstr_ex( const char *from, NSTRBYTE nboct, N_STR **to )
{
    if( (*to) )
    {
        n_log( LOG_ERR, "destination N_STR **str is not NULL (%p), it contain (%s). You must provide an empty destination.", (*to), ((*to)&&(*to)->data)?(*to)->data:"DATA_IS_NULL" );
        n_log( LOG_ERR, "Data to copy: %s", _str( from ) );
        return FALSE ;
    };

    (*to) = new_nstr( nboct + 1 );
    __n_assert( to&&(*to), return FALSE );
    /* added a sizeof( void * ) to add a consistant and secure padding at the end */
    __n_assert( (*to) -> data, Free( (*to) ); return FALSE );

    memcpy( (*to) -> data, from, nboct );
    (*to) -> written = nboct ;

    return TRUE;
} /* char_to_nstr(...) */



/*!\fn N_STR *char_to_nstr( const char *src )
 *\brief Convert a char into a N_STR, short version
 *\param src A char *string to convert
 *\return A N_STR copy of src or NULL
 */
N_STR *char_to_nstr( const char *src )
{
    N_STR *strptr = NULL ;
    char_to_nstr_ex( src, strlen( src ), &strptr ) ;
    return strptr ;
} /* char_to_str(...) */




/*!\fn N_STR *file_to_nstr( char *filename )
 *\brief Load a whole file into a N_STR. Be aware of the NSTRBYTE addressing limit (2GB commonly)
 *\param filename The filename to load inside a N_STR
 *\return A valid N_STR or NULL
 */
N_STR *file_to_nstr( char *filename )
{
    N_STR *tmpstr = NULL ;
    struct stat filestat ;
    FILE *in = NULL ;

    __n_assert( filename, n_log( LOG_ERR, "Unable to make a string from NULL filename") ; return NULL );

    if( stat( filename, &filestat ) != 0 )
    {
#ifdef __linux__
        if( errno == EOVERFLOW )
        {
            n_log( LOG_ERR, "%s size is too big ,EOVERFLOW)", filename );
        }
        else
        {
#endif
            n_log( LOG_ERR, "Couldn't stat %s. Errno: %s", filename, strerror( errno ) );
#ifdef __linux__
        }
#endif
        return NULL ;
    }
    if( ( filestat . st_size + 1 ) >= ( pow( 2, 32 ) - 1 ) )
    {
        n_log( LOG_ERR, "file size >= 2GB is not possible yet. %s is %lld oct", filename, filestat . st_size );
        return NULL ;
    }

    n_log( LOG_DEBUG, "%s file size is: %lld", filename, filestat . st_size );

    in = fopen( filename, "rb" );
    __n_assert( in, n_log( LOG_ERR, "Unable to open %s", filename ); return NULL );

    tmpstr = new_nstr( filestat . st_size + 1 );
    __n_assert( tmpstr, fclose( in ); n_log( LOG_ERR, "Unable to get a new nstr of %ld octets", filestat . st_size + 1 ); return NULL );

    tmpstr -> written = filestat . st_size ;

    if( fread( tmpstr -> data, sizeof( char ), tmpstr -> written, in ) == 0 )
    {
        n_log( LOG_ERR, "Couldn't read %s, fread return 0", filename );
        free_nstr( &tmpstr );
        fclose( in );
        return NULL ;
    }
    if( ferror( in ) )
    {
        n_log( LOG_ERR, "There were some errors when reading %s", filename );
        free_nstr( &tmpstr );
        fclose( in );
        return NULL ;
    }

    fclose( in );

    return tmpstr ;
} /*file_to_nstr */


/*!\fn int nstr_to_fd( N_STR *str , FILE *out , int lock )
 *\brief Write a N_STR content into a file
 *\param str The N_STR *content to write down
 *\param out An opened FILE *handler
 *\param lock a write lock will be put if lock = 1
 *\return TRUE or FALSE
 */

/* Write a whole N_STR into a file */
int nstr_to_fd( N_STR *str, FILE *out, int lock )
{
#ifdef __linux__
    struct flock out_lock ;
#endif
    __n_assert( out, return FALSE );
    __n_assert( str, return FALSE );

    int ret = TRUE ;

    if( lock == 1 )
    {
#ifdef __linux__
        memset( &out_lock, 0, sizeof( out_lock ) );
        out_lock.l_type = F_WRLCK ;
        /* Place a write lock on the file. */
        fcntl( fileno( out ), F_SETLKW, &out_lock );
#else
        lock = 2 ; /* compiler warning suppressor */
#endif
    }

    size_t written_to_file = 0 ;
    if( (written_to_file = fwrite( str -> data, sizeof( char ), str -> written, out ) ) !=  (size_t)str -> written )
    {
        n_log( LOG_ERR, "Couldn't write file, fwrite %d of %d octets", written_to_file, str -> written  );
        ret = FALSE ;
    }
    if( ferror( out ) )
    {
        n_log( LOG_ERR, "There were some errors when writing to %d", fileno( out ) );
        ret = FALSE ;
    }

    if( lock == 1 )
    {
#ifdef __linux__
        memset( &out_lock, 0, sizeof( out_lock ) );
        out_lock.l_type = F_WRLCK ;
        /* Place a write lock on the file. */
        fcntl( fileno( out ), F_SETLKW, &out_lock );
#else
        lock = 2 ; /* compiler warning suppressor */
#endif
    }

    return ret ;
} /* nstr_to_fd( ... ) */



/*!\fn int nstr_to_file( N_STR *str , char *filename )
 *\brief Write a N_STR content into a file
 *\param str The N_STR *content to write down
 *\param filename The destination filename
 *\return TRUE or FALSE
 */
int nstr_to_file( N_STR *str, char *filename )
{
    FILE *out = NULL ;

    __n_assert( str, return FALSE );
    __n_assert( filename, return FALSE );

    out = fopen( filename, "wb" );

    __n_assert( out, n_log( LOG_DEBUG, "Couldn't open %s", _str( filename ) ) ; return FALSE );

    nstr_to_fd( str, out, 1 );

    fclose( out );

    return TRUE ;
} /* nstr_to_file( ... ) */



/*!\fn int str_to_int_ex( const char *s , NSTRBYTE start , NSTRBYTE end , int *i, const int base )
 * \brief Helper for string[start to end] to integer. Automatically add /0 for
 conversion. Leave values untouched if any error occur. Work on a copy of the
 chunk.
 * \param s String to convert
 * \param start Start position of the chunk
 * \param end End position of the chunk
 * \param i A pointer to an integer variable which will receieve the value.
 * \param base Base for converting values
 * \return TRUE or FALSE
 */
int str_to_int_ex( const char *s, NSTRBYTE start, NSTRBYTE end, int *i, const int base )
{
    char *tmpstr = NULL ;
    char *endstr = NULL ;
    long  l = 0;

    __n_assert( s, return FALSE );

    Malloc( tmpstr, char,  sizeof( int ) + end - start + 8 );
    __n_assert( tmpstr, n_log( LOG_ERR, "Unable to Malloc( tmpstr , char ,  sizeof( int ) + %d - %d )", end, start ); return FALSE );

    memcpy( tmpstr, s + start, end - start );

    errno = 0;
    l = strtol(tmpstr, &endstr, base);
    if( ( errno == ERANGE && l == LONG_MAX) || l > INT_MAX )
    {
        n_log( LOG_ERR, "OVERFLOW reached when converting %s to int", tmpstr );
        Free( tmpstr );
        return FALSE;
    }
    if( ( errno == ERANGE && l == LONG_MIN) || l < INT_MIN )
    {
        n_log( LOG_ERR, "UNDERFLOW reached when converting %s to int", tmpstr );
        Free( tmpstr );
        return FALSE;
    }
    if( *endstr != '\0' && *endstr != '\n' )
    {
        n_log( LOG_ERR, "Impossible conversion for %s", tmpstr );
        Free( tmpstr );
        return FALSE;
    }
    Free( tmpstr );
    *i = l;
    return TRUE;
}/* str_to_int_ex( ... ) */




/*!\fn int str_to_int_nolog( const char *s , NSTRBYTE start , NSTRBYTE end , int *i, const int base , N_STR **infos )
 * \brief Helper for string[start to end] to integer. Automatically add /0 for
 conversion. Leave values untouched if any error occur. Work on a copy of the
 chunk.
 * \param s String to convert
 * \param start Start position of the chunk
 * \param end End position of the chunk
 * \param i A pointer to an integer variable which will receieve the value.
 * \param base Base for converting values
 * \param infos If not NULL , contain the errors. remember to free the pointer if returned !!
 * \return TRUE or FALSE
 */
int str_to_int_nolog( const char *s, NSTRBYTE start, NSTRBYTE end, int *i, const int base, N_STR **infos )
{
    char *tmpstr = NULL ;
    char *endstr = NULL ;
    long  l = 0;

    __n_assert( s, return FALSE );

    Malloc( tmpstr, char,  sizeof( int ) + end - start + 8 );
    __n_assert( tmpstr, n_log( LOG_ERR, "Unable to Malloc( tmpstr , char ,  sizeof( int ) + %d - %d )", end, start ); return FALSE );

    memcpy( tmpstr, s + start, end - start );

    errno = 0;
    l = strtol(tmpstr, &endstr, base);
    if( ( errno == ERANGE && l == LONG_MAX) || l > INT_MAX )
    {
        nstrprintf( (*infos), "OVERFLOW reached when converting %s to int", tmpstr );
        Free( tmpstr );
        return FALSE;
    }
    if( ( errno == ERANGE && l == LONG_MIN) || l < INT_MIN )
    {
        nstrprintf( (*infos), "UNDERFLOW reached when converting %s to int", tmpstr );
        Free( tmpstr );
        return FALSE;
    }
    if( *endstr != '\0' && *endstr != '\n' )
    {
        nstrprintf( (*infos), "Impossible conversion for %s", tmpstr );
        Free( tmpstr );
        return FALSE;
    }
    Free( tmpstr );
    *i = l;
    return TRUE;
}/* str_to_int_nolog( ... ) */



/*!\fn int str_to_int( const char *s , int *i, const int base )
 * \brief Helper for string to integer
 * \param s String to convert
 * \param i A pointer to an integer variable which will receieve the value.
 * \param base Base for converting values
 * \return TRUE or FALSE
 */
int str_to_int( const char *s, int *i, const int base )
{
    int ret = FALSE ;
    if( s )
    {
        ret = str_to_int_ex( s, 0, strlen( s ), i, base );
    }
    return ret ;
} /* str_to_int(...) */



/*!\fn int str_to_long_ex( const char *s , NSTRBYTE start , NSTRBYTE end , long int *i, const int base )
 * \brief Helper for string[start to end] to long integer. Automatically add /0 for
 conversion. Leave values untouched if any error occur. Work on a copy of the
 chunk.
 * \param s String to convert
 * \param start Start position of the chunk
 * \param end End position of the chunk
 * \param i A pointer to an integer variable which will receieve the value.
 * \param base Base for converting values
 * \return TRUE or FALSE
 */
int str_to_long_ex( const char *s, NSTRBYTE start, NSTRBYTE end, long int *i, const int base )
{
    char *tmpstr = NULL ;
    char *endstr = NULL ;
    long  l = 0;

    __n_assert( s, return FALSE );

    Malloc( tmpstr, char,  sizeof( int ) + end - start  + 8 );
    __n_assert( tmpstr, n_log( LOG_ERR, "Unable to Malloc( tmpstr , char ,  sizeof( int ) + %d - %d )", end, start ); return FALSE );

    memcpy( tmpstr, s + start, end - start );

    errno = 0;
    l = strtol(tmpstr, &endstr, base);
    int error = errno ;

    /* test return to number and errno values */
    if (tmpstr == endstr)
    {
        n_log( LOG_ERR, " number : %lu  invalid  (no digits found, 0 returned)", l );
        Free( tmpstr );
        return FALSE;
    }
    else if (error == ERANGE && l == LONG_MIN)
    {
        n_log( LOG_ERR, " number : %lu  invalid  (underflow occurred)", l );
        Free( tmpstr );
        return FALSE;
    }
    else if (error == ERANGE && l == LONG_MAX)
    {
        n_log( LOG_ERR, " number : %lu  invalid  (overflow occurred)", l );
        Free( tmpstr );
        return FALSE;
    }
    else if (error == EINVAL)  /* not in all c99 implementations - gcc OK */
    {
        n_log( LOG_ERR, " number : %lu  invalid  (base contains unsupported value)", l );
        Free( tmpstr );
        return FALSE;
    }
    else if (error != 0 && l == 0)
    {

        n_log( LOG_ERR, " number : %lu  invalid  (unspecified error occurred)", l );
        Free( tmpstr );
        return FALSE;
    }
    else if (error == 0 && tmpstr && !*endstr)
    {
        n_log( LOG_DEBUG, " number : %lu    valid  (and represents all characters read)", l );
    }
    else if (error == 0 && tmpstr && *endstr != 0)
    {
        n_log( LOG_DEBUG," number : %lu    valid  (but additional characters remain", l );
    }
    Free( tmpstr );
    *i = l;
    return TRUE;
}/* str_to_long_ex( ... ) */



/*!\fn int str_to_long_long_ex( const char *s , NSTRBYTE start , NSTRBYTE end , long long int *i, const int base )
 * \brief Helper for string[start to end] to long long integer. Automatically add /0 for
 conversion. Leave values untouched if any error occur. Work on a copy of the
 chunk.
 * \param s String to convert
 * \param start Start position of the chunk
 * \param end End position of the chunk
 * \param i A pointer to an integer variable which will receieve the value.
 * \param base Base for converting values
 * \return TRUE or FALSE
 */
int str_to_long_long_ex( const char *s, NSTRBYTE start, NSTRBYTE end, long long int *i, const int base )
{
    char *tmpstr = NULL ;
    char *endstr = NULL ;
    long long  l = 0;

    __n_assert( s, return FALSE );

    Malloc( tmpstr, char,  sizeof( int ) + end - start + 8 );
    __n_assert( tmpstr, n_log( LOG_ERR, "Unable to Malloc( tmpstr , char ,  sizeof( int ) + %d - %d )", end, start ); return FALSE );

    memcpy( tmpstr, s + start, end - start );

    errno = 0;
    l = strtoll(tmpstr, &endstr, base);
    int error = errno ;

    /* test return to number and errno values */
    if (tmpstr == endstr)
    {
        n_log( LOG_ERR, " number : %llu  invalid  (no digits found, 0 returned)", l );
        Free( tmpstr );
        return FALSE;
    }
    else if (error == ERANGE && l == LLONG_MIN)
    {
        n_log( LOG_ERR, " number : %llu  invalid  (underflow occurred)", l );
        Free( tmpstr );
        return FALSE;
    }
    else if (error == ERANGE && l == LLONG_MAX)
    {
        n_log( LOG_ERR, " number : %llu  invalid  (overflow occurred)", l );
        Free( tmpstr );
        return FALSE;
    }
    else if (error == EINVAL)  /* not in all c99 implementations - gcc OK */
    {
        n_log( LOG_ERR, " number : %llu  invalid  (base contains unsupported value)", l );
        Free( tmpstr );
        return FALSE;
    }
    else if (error != 0 && l == 0)
    {

        n_log( LOG_ERR, " number : %llu  invalid  (unspecified error occurred)", l );
        Free( tmpstr );
        return FALSE;
    }
    else if (error == 0 && tmpstr && !*endstr)
    {
        n_log( LOG_DEBUG, " number : %llu    valid  (and represents all characters read)", l );
    }
    else if (error == 0 && tmpstr && *endstr != 0)
    {
        n_log( LOG_DEBUG," number : %llu    valid  (but additional characters remain", l );
    }
    Free( tmpstr );
    *i = l;
    return TRUE;
}/* str_to_long_long_ex( ... ) */



/*!\fn int str_to_long( const char *s , long int *i, const int base )
 * \brief Helper for string to integer
 * \param s String to convert
 * \param i A pointer to an integer variable which will receieve the value.
 * \param base Base for converting values
 * \return TRUE or FALSE
 */
int str_to_long( const char *s, long int *i, const int base )
{
    int ret = FALSE ;
    if( s )
    {
        ret = str_to_long_ex( s, 0, strlen( s ), i, base );
    }
    return ret ;
} /* str_to_long(...) */



/*!\fn int str_to_long_long( const char *s , long int *i, const int base )
 * \brief Helper for string to integer
 * \param s String to convert
 * \param i A pointer to an integer variable which will receieve the value.
 * \param base Base for converting values
 * \return TRUE or FALSE
 */
int str_to_long_long( const char *s, long long int *i, const int base )
{
    int ret = FALSE ;
    if( s )
    {
        ret = str_to_long_long_ex( s, 0, strlen( s ), i, base );
    }
    return ret ;
} /* str_to_long(...) */



/*!\fn N_STR *nstrdup( N_STR *str )
 *\brief Duplicate a N_STR
 *\param str A N_STR *object to free
 *\return A valid N_STR or NULL
 */
N_STR *nstrdup( N_STR *str )
{
    N_STR *new_str = NULL ;

    __n_assert( str, return NULL );
    __n_assert( str -> data, return NULL );

    new_str = new_nstr( str -> length );
    if( new_str )
    {
        if( new_str -> data )
        {
            memcpy(  new_str -> data, str -> data, str -> written );
            new_str -> length = str -> length ;
            new_str -> written = str -> written ;
        }
        else
        {
            Free( new_str );
            n_log( LOG_ERR, "Error duplicating N_STR %p -> data", str );
        }
    }
    else
    {
        n_log( LOG_ERR, "Error duplicating N_STR %p", str );
    }
    return new_str ;
} /* nstrdup(...) */



/*!\fn int skipw( char *string , char toskip , NSTRBYTE *iterator , NSTRBYTE inc )
 *\brief skip while 'toskip' occurence is found from 'iterator' to the next non 'toskip' position.
 * The new iterator index is automatically stored, returning to it first value if an error append.
 *\param string a char *string to search in
 *\param toskip skipping while char character 'toskip' is found
 *\param iterator an int iteraor position on the string
 *\param inc an int to specify the step of skipping
 *\return TRUE if success FALSE or ABORT if not
 */
int skipw( char *string, char toskip, NSTRBYTE *iterator, int inc )
{
    int error_flag = 0 ;
    //NSTRBYTE previous = 0 ;

    __n_assert( string, return FALSE );

    //previous = *iterator;
    if( toskip == ' ' )
    {
        while( *iterator <= (NSTRBYTE)strlen ( string ) && isspace( string[ *iterator ] ) )
        {
            if( inc < 0 && *iterator == 0 )
            {
                error_flag = 1 ;
                break ;
            }
            else
                *iterator = *iterator + inc;
        }
    }
    else
    {
        while( *iterator <= (NSTRBYTE)strlen( string ) && string[ *iterator ] == toskip )
        {
            if( inc < 0 && *iterator == 0 )
            {
                error_flag = 1 ;
                break ;
            }
            else
                *iterator = *iterator + inc;
        }
    }
    if( error_flag == 1 || *iterator > (NSTRBYTE)strlen ( string ) )
    {
        //*iterator = previous ;
        return FALSE;
    }

    return TRUE;
} /*skipw(...)*/



/*!\fn skipu( char *string , char toskip , NSTRBYTE *iterator , int inc )
 *\brief skip until 'toskip' occurence is found from 'iterator' to the next 'toskip' value.
 * The new iterator index is automatically stored, returning to it first value if an error append.
 *\param string a char *stri:wqng to search in
 *\param toskip skipping while char character 'toskip' isnt found
 *\param iterator an int iteraor position on the string
 *\param inc an int to specify the step of skipping
 *\return TRUE if success FALSE or ABORT if not
 */
int skipu( char *string, char toskip, NSTRBYTE *iterator, int inc )
{
    int error_flag = 0 ;
    //NSTRBYTE previous = 0 ;

    __n_assert( string, return FALSE );

    //previous = *iterator;
    if( toskip == ' ' )
    {
        while( *iterator <= (NSTRBYTE)strlen ( string ) && !isspace( string[ *iterator ] ) )
        {
            if( inc < 0 && *iterator == 0 )
            {
                error_flag = 1 ;
                break ;
            }
            else
                *iterator = *iterator + inc;
        }
    }
    else
    {
        while( *iterator <= (NSTRBYTE)strlen( string ) && string[ *iterator ] != toskip )
        {
            if( inc < 0 && *iterator == 0 )
            {
                error_flag = 1 ;
                break ;
            }
            else
                *iterator = *iterator + inc;
        }
    }

    if( error_flag == 1 || *iterator > (NSTRBYTE)strlen ( string ) )
    {
        //*iterator = previous ;
        return FALSE;
    }

    return TRUE;
} /*Skipu(...)*/



/*!\fn int strup( char *string , char *dest )
 *\brief Upper case a string
 *\param string the string to change to upper case
 *\param dest the string where storing result
 *\warning string must be same size as dest
 *\return TRUE or FALSE
 */
int strup( char *string, char *dest )
{

    NSTRBYTE it = 0 ;

    __n_assert( string, return FALSE );
    __n_assert( dest, return FALSE );

    for( it = 0 ; it < (NSTRBYTE)strlen( string ) ; it++ )
        dest[ it ] = toupper ( string[ it ] );

    return TRUE;
} /*strup(...)*/



/*!\fn strlo( char *string , char *dest )
 *\brief Upper case a string
 *\param string the string to change to lower case
 *\param dest the string where storing result
 *\warning string must be same size as dest
 *\return TRUE or FALSE
 */
int strlo( char *string, char *dest )
{

    NSTRBYTE it = 0 ;

    __n_assert( string, return FALSE );
    __n_assert( dest, return FALSE );

    for( it = 0 ; it < (NSTRBYTE)strlen( string ) ; it++ )
        dest[ it ] = tolower ( string[ it ] );

    return TRUE;
} /*strlo(...)*/



/*!\fn int strcpy_u( char *from , char *to , NSTRBYTE to_size , char split , NSTRBYTE *it )
 *\brief Copy from start to dest until from[ iterator ] == split
 *\param from Source string
 *\param to Dest string
 *\param to_size the maximum size to write
 *\param split stopping character
 *\param it Save of iterator
 *\return TRUE or FALSE
 */
int strcpy_u( char *from, char *to, NSTRBYTE to_size, char split, NSTRBYTE *it )
{
    NSTRBYTE _it = 0;

    __n_assert( from, return FALSE );
    __n_assert( to, return FALSE );
    __n_assert( it, return FALSE );

    while( _it < to_size && from[ (*it) ] != '\0' && from[ (*it) ] != split  )
    {
        to[ _it ] = from[ (*it) ] ;
        (*it) = (*it) + 1 ;
        _it = _it +1 ;
    }

    if( _it == to_size )
    {
        _it -- ;
        to[ _it ] = '\0' ;
        n_log(  LOG_DEBUG, "strcpy_u: not enough space to write %d octet to dest (%d max) , %s: %d \n", _it, to_size, __FILE__, __LINE__ );
        return FALSE;
    }

    to[ _it ] = '\0' ;

    if( from[ (*it) ] != split )
    {
        n_log(  LOG_DEBUG, "strcpy_u: split value not found, written %d octet to dest (%d max) , %s: %d \n", _it, to_size, __FILE__, __LINE__ );
        return FALSE;
    }
    return TRUE ;
} /* strcpy_u(...) */



/*!\fn char **split( char* str , const char* delim , int empty )
 *\brief Split the strings into a an array of char *pointer	, ended by a NULL one. Max 256 splitted elements per call.
 *\param str The char *str to split
 *\param delim The delimiter, one or more characters
 *\param empty Empty flag. If 1, then empty delimited areas will be added as NULL entries, else they will be skipped.
 *\return An array of char *, ended by a NULL entry.
 */
char** split( const char* str, const char* delim, int empty )
{
    char** tab=NULL; /* result array */
    char *ptr = NULL ; /* tmp pointer */
    int sizeStr=0; /* string token size */
    int sizeTab=0; /* array size */
    const char* largestring = NULL; /* pointer to start of string */

    __n_assert( str, return NULL );
    __n_assert( delim, return NULL );

    int sizeDelim=strlen(delim);
    largestring = str;

    while( ( ptr = strstr( largestring, delim ) ) != NULL )
    {
        sizeStr=ptr-largestring;
        if( empty == 1 || sizeStr != 0 )
        {
            sizeTab++;
            tab= (char**) realloc(tab,sizeof(char*)*sizeTab);

            Malloc( tab[sizeTab -1], char, (int)(sizeof(char)*(sizeStr+1)) );
            __n_assert( tab[ sizeTab - 1 ], goto error );

            memcpy(tab[sizeTab-1],largestring,sizeStr);
            tab[sizeTab-1][sizeStr]='\0';
        }
        ptr=ptr+sizeDelim;
        largestring=ptr;
    }

    /* adding last part if any */
    if( strlen(largestring) !=0 )
    {
        sizeStr=strlen(largestring);
        sizeTab++;

        tab= (char**) realloc(tab,sizeof(char*)*sizeTab);

        Malloc( tab[sizeTab-1], char,(int)(  sizeof(char)*(sizeStr+1) ) );
        __n_assert( tab[ sizeTab - 1 ], goto error );

        memcpy(tab[sizeTab-1],largestring,sizeStr);
        tab[sizeTab-1][sizeStr]='\0';
    }
    else
    {
        if( empty == 1 )
        {
            sizeTab++;
            tab= (char**) realloc(tab,sizeof(char*)*sizeTab);

            Malloc( tab[sizeTab-1], char, (int)(sizeof(char)*1) );
            __n_assert( tab[ sizeTab - 1 ], goto error );

            tab[sizeTab-1][0]='\0';
        }
    }

    /* on ajoute une case a null pour finir le tableau */
    sizeTab++;
    tab= (char**) realloc(tab,sizeof(char*)*sizeTab);
    tab[sizeTab-1]=NULL;

    return tab;

error:
    free_split_result( &tab );
    return NULL;
} /* split( ... ) */



/*!\fn int split_count( char **split_result )
 *\brief Count split elements
 *\param split_result A char **result from a split call
 *\return The number of elements or -1 if errors
 */
int split_count( char **split_result )
{
    __n_assert( split_result, return -1 );
    __n_assert( split_result[ 0 ], return -1 );

    int it = 0 ;
    while( split_result[ it ] )
    {
        it ++ ;
    }
    return it ;
} /* split_count(...) */



/*!\fn int free_split_result( char ***tab )
 *\brief Free a split result allocated array
 *\param tab A pointer to a split result to free
 *\return TRUE
 */
int free_split_result( char ***tab )
{
    if( !(*tab) )
        return FALSE ;

    int it = 0 ;
    while( (*tab)[ it ] )
    {
        Free( (*tab)[ it ] );
        it++;
    }
    Free( (*tab) );

    return TRUE ;
}/* free_split_result(...)*/



/*!\fn int nstrcat_ex( N_STR *dest , void *src , NSTRBYTE size , NSTRBYTE blk_size , int resize_flag )
 *\brief Append data into N_STR using internal N_STR size and cursor position.
 *\param dest The N_STR *destination (accumulator)
 *\param src The data to append
 *\param size The number of octet of data we want to append in dest
 *\param blk_size In case of resizing, this is the increment which will be used to reach ( dest -> written + size )
 *\param resize_flag Set it to a positive non zero value to allow resizing, or to zero to forbid resizing
 *\return TRUE or FALSE
 */
int nstrcat_ex( N_STR *dest, void *src, NSTRBYTE size, NSTRBYTE blk_size, int resize_flag )
{
    char *ptr = NULL ;
    int realloc_flag = 0 ;

    if( !src )
    {
        n_log( LOG_ERR, "src is NULL" );
        return FALSE ;
    }

    if( dest )
    {
        if( ( dest -> written + size ) >= dest -> length  && resize_flag == 0 )
        {
            n_log( LOG_ERR, "%p to %p: not enough space. Resize forbidden. %lld needed, %lld available", dest, src,  dest -> written + size + 1, dest -> length );
            return FALSE ;
        }
    }
    else
    {
        n_log( LOG_ERR, "dest is NULL" );
        return FALSE ;
    }

    while( ( dest -> written + size ) >= dest -> length )
    {
        dest -> length += blk_size ;
        realloc_flag = 1 ;
    }

    if( realloc_flag == 1 )
    {
        Reallocz( dest -> data, char, dest -> written, dest -> length );
        __n_assert( dest -> data, Free( dest ); return FALSE );
    }

    ptr = dest -> data + dest -> written ;
    memcpy( ptr, src, size );
    dest -> written += size ;

    dest -> data[ dest -> written ] = '\0' ;

    return TRUE ;
} /* nstrcat_ex( ... ) */



/*!\fn int nstrcat( N_STR *dst , N_STR *src )
 *\brief Add N_STR *src content to N_STR *dst, resizing it if needed.
 *\param dst The N_STR *destination
 *\param src The N_STR *source to concatenate into dst
 *\return TRUE or FALSE
 */
int nstrcat( N_STR *dst, N_STR *src )
{
    return nstrcat_ex( dst, src -> data, src -> written, src -> written + 64, 1 );
} /* nstrcat( ... ) */



/*!\fn int nstrcat_bytes_ex( N_STR *dest , void *data , NSTRBYTE size )
 *\brief Append data into N_STR using internal N_STR size and cursor position. Wrapper to nstrcat_ex, with resize_flag on and a block size one byte higher than size.
 *\param dest The N_STR *destination (accumulator)
 *\param data The data to append
 *\param size The number of octet of data we want to append in dest
 *\return TRUE or FALSE
 */
int nstrcat_bytes_ex( N_STR *dest, void *data, NSTRBYTE size )
{
    __n_assert( dest, return FALSE );
    __n_assert( data, return FALSE );

    if( size <= 0 )
    {
        n_log( LOG_ERR, "Could not copy 0 or less (%ld) octet!", size );
        return FALSE ;
    }

    return nstrcat_ex( dest, data, size, size + 64, 1 );
} /* nstrcat_bytes_ex( ... )*/


/*!\fn int nstrcat_bytes( N_STR *dest , void *data )
 *\brief Append data into N_STR using internal N_STR size and cursor position. Wrapper to nstrcat_ex, with resize_flag on and a block size one byte higher than size.
 *\param dest The N_STR *destination (accumulator)
 *\param data The data to append
 *\return TRUE or FALSE
 */
int nstrcat_bytes( N_STR *dest, void *data )
{
    __n_assert( dest, return FALSE );
    __n_assert( data, return FALSE );

    NSTRBYTE size = strlen( (char *)data );
    if( size <= 0 )
    {
        n_log( LOG_ERR, "Could not copy 0 or less (%ld) octet!", size );
        return FALSE ;
    }
    return nstrcat_bytes_ex( dest, data, size );
} /* nstrcat_bytes( ... )*/






/*!\fn int write_and_fit_ex( char **dest, NSTRBYTE *size, NSTRBYTE *written, char *src , NSTRBYTE src_size , NSTRBYTE additional_padding )
 *\brief concatenate a copy of src of size src_size to dest, starting at dest[ written ], updating written and size variable, allocation of new blocks of (needed size + additional_padding ) if resize is needed. If dest is NULL it will be allocated.
 *\param dest The dest string
 *\param size The current size, will be updated if written + strlen( dest) > size
 *\param written the number of octet added
 *\param src The source string to add
 *\param additional_padding In case the destination is reallocated, number of additional bytes that will be added (provisionning)
 *\return TRUE on success or FALSE on a realloc error
 */
int write_and_fit_ex( char **dest, NSTRBYTE *size, NSTRBYTE *written, char *src, NSTRBYTE src_size, NSTRBYTE additional_padding )
{
    char *ptr = NULL ;
    NSTRBYTE needed_size = (*written) + src_size + 1 ;

    // realloc if needed , also if destination is not allocated
    if( needed_size > (*size) || !(*dest) )
    {
        n_log( LOG_DEBUG, "dest will grow from %d to %d", (*size), needed_size + additional_padding );

        // +1 for the \0 , + additional padding for eventual futur concatenation space
        Reallocz( (*dest), char, (*size), needed_size + additional_padding );
        (*size) = needed_size + additional_padding ;
        if( !(*dest) )
        {
            n_log(  LOG_ERR, "reallocation error !!!!" );
            return FALSE ;
        }
    }

    ptr = (*dest) + (*written) ;
    memcpy( ptr, src, src_size );
    (*written) += src_size ;

    (*dest)[ (*written) ] = '\0' ;

    return TRUE ;
} /* write_and_fit_ex( ...) */



/*!\fn int write_and_fit( char **dest , NSTRBYTE *size , NSTRBYTE *written , char *src )
 *\brief concatenate a copy of src of size strlen( src ) to dest, starting at dest[ written ], updating written and size variable, allocation of new blocks of (needed size + 512) if resize is needed. If dest is NULL it will be allocated.
 *\param dest The dest string
 *\param size The current size, will be updated if written + strlen( dest) > size
 *\param written the number of octet added
 *\param src The source string to add
 *\return TRUE on success or FALSE on a realloc error
 */
int write_and_fit( char **dest, NSTRBYTE *size, NSTRBYTE *written, char *src )
{
    return write_and_fit_ex( dest, size, written, src, strlen( src ), 512 );
} /* write_and_fit( ...) */



/*!\fn int scan_dir( const char *dir, LIST *result , const int recurse )
 *\brief Scan a list of directory and return a list of char *file
 *\param dir The directory to scan
 *\param result A pointer to a valid LIST for the results
 *\param recurse Recursive search if TRUE, directory only if FALSE
 *\return TRUE or FALSE
 */
int scan_dir( const char *dir, LIST *result, const int recurse )
{
    return scan_dir_ex( dir, "*", result, recurse, 0 );
}



/*!\fn int scan_dir_ex( const char *dir, const char *pattern , LIST *result , const int recurse , const int mode )
 *\brief Scan a list of directory and return a list of char *file
 *\param dir The directory to scan
 *\param pattern Pattern that files must follow to figure in the list
 *\param result A pointer to a valid LIST for the results
 *\param recurse Recursive search if TRUE, directory only if FALSE
 *\param mode 0 for a list of char* , 1 for a list of N_STR *
 *\return TRUE or FALSE
 */
int scan_dir_ex( const char *dir, const char *pattern, LIST *result, const int recurse, const int mode )
{
    DIR *dp = NULL ;
    struct dirent *entry = NULL;
    struct stat statbuf;

    if( !result )
        return FALSE ;

    if( ( dp = opendir( dir ) ) == NULL)
    {
        n_log( LOG_ERR, "cannot open directory: %s", dir );
        return FALSE ;
    }

    N_STR *newname = NULL ;
    while( ( entry = readdir( dp ) ) != NULL )
    {
        nstrprintf( newname, "%s/%s", dir, entry -> d_name );

        if( stat( newname -> data, &statbuf ) >= 0 )
        {
            if( S_ISDIR( statbuf . st_mode ) != 0 )
            {
                if( strcmp( ".", entry -> d_name ) == 0 || strcmp( "..", entry -> d_name ) == 0 )
                {
                    free_nstr( &newname );
                    continue;
                }

                /* Recurse */
                if( recurse != FALSE )
                {
                    if( scan_dir_ex( newname -> data, pattern, result, recurse, mode ) != TRUE )
                    {
                        n_log( LOG_ERR, "scan_dir_ex( %s , %s , %p , %d , %d ) returned FALSE !", newname -> data, pattern, result, recurse, mode );
                    }
                }
                free_nstr( &newname );
                continue ;
            }
            else if( S_ISREG( statbuf . st_mode ) != 0 )
            {
                if( wildmatcase( newname -> data, pattern ) == TRUE )
                {
                    if( mode == 0 )
                    {
                        char *file = strdup( newname -> data );
                        if( file )
                        {
                            list_push( result, file, &free );
                        }
                        else
                        {
                            n_log( LOG_ERR, "Error adding %s/%s to list", dir, entry -> d_name  );
                        }
                    }
                    else if( mode == 1 )
                    {
                        list_push( result, newname, &free_nstr_ptr );
                        newname = NULL ;
                    }
                }
            }
        }
        if( newname )
            free_nstr( &newname );
    }
    closedir( dp );
    return TRUE ;
} /*scan_dir(...) */



/*!\fn int wildmat(register const char *text, register const char *p)
 *\brief Written by Rich Salz rsalz at osf.org, refurbished by me. Wildcard pattern matching .
 *\param text The source text to search
 *\param p The text to search, with wildcards
 *\return return TRUE, FALSE, or WILDMAT_ABORT.
 */
int wildmat(register const char *text, register const char *p)
{
    register int last = 0 ;
    register int matched = 0;
    register int reverse = 0;

    for ( ; *p; text++, p++)
    {
        if (*text == '\0' && *p != '*')
            return WILDMAT_ABORT;
        switch (*p)
        {
        case '\\':
            /* Literal match with following character. */
            p++;
        /* FALLTHROUGH */
        default:
            if (*text != *p)
                return FALSE;
            continue;
        case '?':
            /* Match anything. */
            continue;
        case '*':
            while (*++p == '*')
                /* Consecutive stars act just like one. */
                continue;
            if (*p == '\0')
                /* Trailing star matches everything. */
                return TRUE;
            while (*text)
                if ((matched = wildmat(text++, p)) != FALSE)
                    return matched;
            return WILDMAT_ABORT;
        case '[':
            reverse = p[1] == WILDMAT_NEGATE_CLASS ? TRUE : FALSE;
            if (reverse)
                /* Inverted character class. */
                p++;
            matched = FALSE;
            if (p[1] == ']' || p[1] == '-')
                if (*++p == *text)
                    matched = TRUE;
            for (last = *p; *++p && *p != ']'; last = *p)
                /* This next line requires a good C compiler. */
                if (*p == '-' && p[1] != ']'
                        ? *text <= *++p && *text >= last : *text == *p)
                    matched = TRUE;
            if (matched == reverse)
                return FALSE;
            continue;
        }
    }
#ifdef WILDMAT_MATCH_TAR_PATTERN
    if (*text == '/')
        return TRUE;
#endif /* MATCH_TAR_ATTERN */
    return *text == '\0';
} /* wildmatch(...) */



/*!\fn int wildmatcase( register const char *text , register const char *p )
 *\brief Written by Rich Salz rsalz at osf.org, refurbished by me. Wildcard pattern matching case insensitive.
 *\param text The source text to search
 *\param p The text to search, with wildcards
 *\return return TRUE, FALSE, or WILDMAT_ABORT.
 */
int wildmatcase(register const char *text, register const char *p)
{
    register int last;
    register int matched;
    register int reverse;

    for ( ; *p; text++, p++)
    {
        if (*text == '\0' && *p != '*')
            return WILDMAT_ABORT;
        switch (*p)
        {
        case '\\':
            /* Literal match with following character. */
            p++;
        /* FALLTHROUGH */
        default:
            if (toupper(*text) != toupper(*p))
                return FALSE;
            continue;
        case '?':
            /* Match anything. */
            continue;
        case '*':
            while (*++p == '*')
                /* Consecutive stars act just like one. */
                continue;
            if (*p == '\0')
                /* Trailing star matches everything. */
                return TRUE;
            while (*text)
                if ((matched = wildmatcase(text++, p)) != FALSE)
                    return matched;
            return WILDMAT_ABORT;
        case '[':
            reverse = p[1] == WILDMAT_NEGATE_CLASS ? TRUE : FALSE;
            if (reverse)
                /* Inverted character class. */
                p++;
            matched = FALSE;
            if (p[1] == ']' || p[1] == '-')
                if (toupper(*++p) == toupper(*text))
                    matched = TRUE;
            for (last = toupper(*p); *++p && *p != ']'; last = toupper(*p))
                if (*p == '-' && p[1] != ']'
                        ? toupper(*text) <= toupper(*++p) && toupper(*text) >= last : toupper(*text) == toupper(*p))
                    matched = TRUE;
            if (matched == reverse)
                return FALSE;
            continue;
        }
    }
#ifdef WILDMAT_MATCH_TAR_PATTERN
    if (*text == '/')
        return TRUE;
#endif /* MATCH_TAR_ATTERN */
    return *text == '\0';
} /* wildmatcase(...) */




/*!\fn char *str_replace ( const char *string, const char *substr, const char *replacement )
 *\brief Replace "substr" by "replacement" inside string
 taken from http://coding.debuntu.org/c-implementing-str_replace-replace-all-occurrences-substring
 By Chantra
 *\param string Original string to modify
 *\param substr String to search
 *\param replacement Substitution string
 *\return A copy of the sustituted string or NULL
 */
char *str_replace ( const char *string, const char *substr, const char *replacement )
{
    char *tok = NULL;
    char *newstr = NULL;
    char *oldstr = NULL;
    char *head = NULL;

    /* if either substr or replacement is NULL, duplicate string a let caller handle it */
    if ( substr == NULL || replacement == NULL )
        return strdup (string);
    newstr = strdup (string);
    head = newstr;
    while ( (tok = strstr ( head, substr )))
    {
        oldstr = newstr;
        Malloc( newstr, char, strlen ( oldstr ) - strlen ( substr ) + strlen ( replacement ) + 8 );
        /*failed to alloc mem, free old string and return NULL */
        if ( newstr == NULL )
        {
            free (oldstr);
            return NULL;
        }
        memcpy ( newstr, oldstr, tok - oldstr );
        memcpy ( newstr + (tok - oldstr), replacement, strlen ( replacement ) );
        memcpy ( newstr + (tok - oldstr) + strlen( replacement ), tok + strlen ( substr ), strlen ( oldstr ) - strlen ( substr ) - ( tok - oldstr ) );
        memset ( newstr + strlen ( oldstr ) - strlen ( substr ) + strlen ( replacement ), 0, 1 );
        /* move back head right after the last replacement */
        head = newstr + (tok - oldstr) + strlen( replacement );
        free (oldstr);
    }
    return newstr;
}

/*!\fn int str_sanitize_ex( char *string, const NSTRBYTE string_len, const char *mask, const NSTRBYTE masklen, const char replacement )
 * \brief clean a string by replacing evil characteres
 * \param string The string to change
 * \param string_len Size of the data to treat
 * \param mask The caracters to kill
 * \param masklen size of mask
 * \param replacement replacement for mask
 * \return TRUE or FALSE
 */
int str_sanitize_ex( char *string, const NSTRBYTE string_len, const char *mask, const NSTRBYTE masklen, const char replacement )
{
    __n_assert( string, return FALSE );
    __n_assert( mask, return FALSE );

    NSTRBYTE it = 0 ;
    for( it = 0 ; it < string_len ; it ++ )
    {
        NSTRBYTE mask_it = 0 ;
        while( mask_it<masklen && mask[mask_it]!='\0')
        {
            if( string[ it ] == mask[ mask_it ] )
                string[ it ] = replacement ;
            mask_it ++ ;
        }
    }
    return TRUE ;
}


/*!\fn int str_sanitize( char *string, const char *mask, const char replacement )
 * \brief clean a string by replacing evil characteres
 * \param string The string to change
 * \param mask The caracters to kill
 * \param replacement replacement for mask
 * \return TRUE or FALSE
 */
int str_sanitize( char *string, const char *mask, const char replacement )
{
    return str_sanitize_ex( string, strlen( string ), mask, strlen( mask ), replacement );
}


#endif /* #ifndef NOSTR */

