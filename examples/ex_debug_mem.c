/**\file ex_debug_mem.c
 *  Nilorea Library debug mem api test
 *\author Castagnier Mickael
 *\version 1.0
 *\date 03/01/2019
 *\example ex_debug_mem.c Debug mem module example
 */
#define DEBUG_MEM

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

#include "nilorea/n_common.h"
#include "nilorea/n_log.h"
#include "nilorea/n_str.h"

#ifndef __windows__
#include <sys/wait.h>
#endif

#include "nilorea/n_debug_mem_h.h"

void usage(void)
{
    fprintf( stderr, "     -v version\n"
             "     -V log level: LOG_INFO, LOG_NOTICE, LOG_ERR, LOG_DEBUG\n"
             "     -h help\n" );
}

void process_args( int argc, char **argv )
{
    int getoptret = 0,
        log_level = LOG_DEBUG ;  /* default log level */

    /* Arguments optionnels */
    /* -v version
     * -V log level
     * -h help
     */
    while( ( getoptret = getopt( argc, argv, "hvV:" ) ) != EOF)
    {
        switch( getoptret )
        {
        case 'v' :
            fprintf( stderr, "Date de compilation : %s a %s.\n", __DATE__, __TIME__ );
            exit( 1 );
        case 'V' :
            if( !strncmp( "LOG_NULL", optarg, 5 ) )
            {
                log_level = LOG_NULL ;
            }
            else
            {
                if( !strncmp( "LOG_NOTICE", optarg, 6 ) )
                {
                    log_level = LOG_NOTICE;
                }
                else
                {
                    if( !strncmp( "LOG_INFO", optarg, 7 ) )
                    {
                        log_level = LOG_INFO;
                    }
                    else
                    {
                        if( !strncmp( "LOG_ERR", optarg, 5 ) )
                        {
                            log_level = LOG_ERR;
                        }
                        else
                        {
                            if( !strncmp( "LOG_DEBUG", optarg, 5 ) )
                            {
                                log_level = LOG_DEBUG;
                            }
                            else
                            {
                                fprintf( stderr, "%s n'est pas un niveau de log valide.\n", optarg );
                                exit( -1 );
                            }
                        }
                    }
                }
            }
            break;
        default :
        case '?' :
        {
            if( optopt == 'V' )
            {
                fprintf( stderr, "\n      Missing log level\n" );
            }
            else if( optopt == 'p' )
            {
                fprintf( stderr, "\n      Missing port\n" );
            }
            else if( optopt != 's' )
            {
                fprintf( stderr, "\n      Unknow missing option %c\n", optopt );
            }
            usage();
            exit( 1 );
        }
        case 'h' :
        {
            usage();
            exit( 1 );
        }
        }
    }
    set_log_level( log_level );
} /* void process_args( ... ) */

int main(int argc, char **argv)
{
//    init_debug_mem();

    /* processing args and set log_level */
    process_args( argc, argv );

    n_log( LOG_INFO, "TRUE,true values: %d,%d / FALSE,false value: %d,%d", TRUE, true, FALSE, false );


    n_log( LOG_INFO, "_str(\"TEST\")=\"%s\" , _str(NULL)=\"%s\"", _str( "TEST" ), _str( NULL ) );
    n_log( LOG_INFO, "_strw(\"TEST\")=\"%s\" , _strw(NULL)=\"%s\"", _str( "TEST" ), _str( NULL ) );
    N_STR *nstr = char_to_nstr( "TEST" );
    n_log( LOG_INFO, "_nstr(nstrpointer)=\"%s\"", _nstr( nstr ) );
    n_log( LOG_INFO, "_nstrp(nstrpointer)=\"%s\"", _nstrp( nstr ) );
    free_nstr( &nstr );
    n_log( LOG_INFO, "_nstr(nstrpointer_freed)=\"%s\"", _nstr( nstr ) );
    n_log( LOG_INFO, "_nstrp(nstrpointer_freed)=\"%s\"", _nstrp( nstr ) );

    char *data = NULL ;
    __n_assert( data, n_log( LOG_INFO, "data is NULL" ) );
    Malloc( data, char, 1024 );
    __n_assert( data, n_log( LOG_INFO, "data is NULL" ) );
    Free( data );
    Malloc( data, char, 1024 );
    Realloc( data, char, 2048 );
    Free( data );
    Malloc( data, char, 1024 );
    Reallocz( data, char, 1024, 2048 );
    FreeNoLog( data );
    Alloca( data, 2048 );
    //Free( data ); alloca should not be free else it's double freeing on exit

    n_log( LOG_INFO, "next_odd(10):%d  , next_odd(11):%d", next_odd( 10 ), next_odd( 11 ) );
    n_log( LOG_INFO, "next_even(10):%d  , next_even(11):%d", next_even( 10 ), next_even( 11 ) );

    init_error_check();
    ifzero (0) endif ;
    ifzero (1) endif ;
    ifnull (data) endif ;
    ifnull (NULL) endif ;
    iffalse (FALSE) endif ;
    iftrue (TRUE) endif ;
    checkerror();
error:
    get_error();

    if( file_exist( argv[ 0 ] ) )
    {
        n_log( LOG_INFO, "%s exists !", argv[ 0 ] );
    }

    char *dir = NULL, *name = NULL ;
    dir = get_prog_dir();
    name = get_prog_name();
    n_log( LOG_INFO, "From %s/%s", dir, name );
    Free( dir );
    Free( name );

    N_STR *out = NULL ;
    int ret = -1 ;
    if( n_popen( "ls -ltr", 2048, (void *)&out, &ret ) == TRUE )
    {
        n_log( LOG_INFO, "ls returned %d : %s", ret, _nstr( out ) );
    }
    else
    {
        n_log( LOG_ERR, "popen s returned an error" );
    }
    free_nstr( &out );

#ifndef __windows__
    n_log( LOG_INFO, "before system_nb( sleep 3 )" );
    pid_t pid=system_nb( "sleep 3", NULL, NULL );
    n_log( LOG_INFO, "after system_nb( sleep 3 )" );
    n_log( LOG_INFO, "wait for nb sys call" );
    wait( &pid );
    n_log( LOG_INFO, "done" );
#endif
    n_abort( "Testing abort before exit" );
    n_log( LOG_INFO, "abort done" );

    exit( 0 );
}
