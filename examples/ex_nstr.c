/**\example ex_nstr.c Nilorea Library string  api test
 *\author Castagnier Mickael
 *\version 1.0
 *\date 26/05/2015
 */

#include "nilorea/n_str.h"
#include "nilorea/n_log.h"

int main( void )
{
    set_log_level( LOG_DEBUG );

    char *chardest = NULL ;
    NSTRBYTE written = 0,
             length = 0 ;

    write_and_fit( &chardest, &length, &written, "Hello" );
    n_log( LOG_INFO, "charstr (%d/%d): %s\n", written, length, chardest );
    write_and_fit( &chardest, &length, &written, " " );
    n_log( LOG_INFO, "charstr (%d/%d): %s\n", written, length, chardest );
    write_and_fit( &chardest, &length, &written, "world !" );
    n_log( LOG_INFO, "charstr (%d/%d): %s\n", written, length, chardest );
    write_and_fit( &chardest, &length, &written, "world ! " );
    n_log( LOG_INFO, "charstr (%d/%d): %s\n", written, length, chardest );
    write_and_fit( &chardest, &length, &written, "world ! " );
    n_log( LOG_INFO, "charstr (%d/%d): %s\n", written, length, chardest );
    write_and_fit( &chardest, &length, &written, "world ! " );
    n_log( LOG_INFO, "charstr (%d/%d): %s\n", written, length, chardest );

    Free( chardest );
    written = length = 0 ;

    write_and_fit_ex( &chardest, &length, &written, "Hello", 5, 0 );
    n_log( LOG_INFO, "charstr (%d/%d): %s\n", written, length, chardest );
    write_and_fit_ex( &chardest, &length, &written, " ", 1, 0 );
    n_log( LOG_INFO, "charstr (%d/%d): %s\n", written, length, chardest );
    write_and_fit_ex( &chardest, &length, &written, "world !", 7, 0 );
    n_log( LOG_INFO, "charstr (%d/%d): %s\n", written, length, chardest );
    write_and_fit_ex( &chardest, &length, &written, "Hello", 5, 0 );
    n_log( LOG_INFO, "charstr (%d/%d): %s\n", written, length, chardest );
    write_and_fit_ex( &chardest, &length, &written, " ", 1, 10 );      // alocate 10 more byte if resize needed
    n_log( LOG_INFO, "charstr (%d/%d): %s\n", written, length, chardest );
    write_and_fit_ex( &chardest, &length, &written, "world !", 7, 0 );
    n_log( LOG_INFO, "charstr (%d/%d): %s\n", written, length, chardest );

    Free( chardest );

    N_STR *nstr = NULL ;

    n_log( LOG_INFO, "str:%s\n", _nstr( nstr ) );

    nstrprintf( nstr, "Hello, file is %s line %d date %s", __FILE__, __LINE__, __TIME__ );

    n_log( LOG_INFO, "str:%s\n", _nstr( nstr ) );

    nstrprintf_cat( nstr, " - This will be added at file %s line %d date %s", __FILE__, __LINE__, __TIME__ );

    n_log( LOG_INFO, "str:%s\n", _nstr( nstr ) );

    free_nstr( &nstr );

    nstr = new_nstr( 0 );

    n_log( LOG_INFO, "str:%s\n", _nstr( nstr ) );

    nstrprintf( nstr, "Hello, file is %s line %d date %s", __FILE__, __LINE__, __TIME__ );

    n_log( LOG_INFO, "str:%s\n", _nstr( nstr ) );

    nstrprintf_cat( nstr, " - This will be added at file %s line %d date %s", __FILE__, __LINE__, __TIME__ );

    n_log( LOG_INFO, "str:%s\n", _nstr( nstr ) );

    nstrprintf_cat( nstr, " - some more texte" );

    N_STR *nstr2 = nstrdup( nstr );

    n_log( LOG_INFO, "str: %s\n str2: %s\n", _nstr( nstr ), _nstr( nstr2 ) );

    N_STR *nstr3 = NULL ;

    nstrcat( nstr3, nstr );
    nstrcat( nstr3, nstr2 );

    n_log( LOG_INFO, "str:%s\n", _nstr( nstr3 ) );

    nstr3 = new_nstr( 10 );

    nstrcat( nstr3, nstr );
    nstrcat( nstr3, nstr2 );

    n_log( LOG_INFO, "str:%s\n", _nstr( nstr3 ) );

    free_nstr( &nstr );
    free_nstr( &nstr2 );
    free_nstr( &nstr3 );

    exit( 0 );
}

