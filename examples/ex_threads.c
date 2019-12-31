/**\file ex_network.c
 *  Nilorea Library n_network api test
 *\author Castagnier Mickael
 *\version 1.0
 *\date 26/05/2015
 */

#include <stdio.h>
#include <errno.h>

#include "nilorea/n_log.h"
#include "nilorea/n_thread_pool.h"

static pthread_t netw_thr ;

void usage(void)
{
    fprintf( stderr, "     -v version\n"
             "     -h help\n"
             "     -V LOG_LEVEL (LOG_DEBUG,INFO,NOTICE,ERR)\n" );
}

void process_args( int argc, char **argv )
{
    int getoptret = 0,
        log_level = LOG_ERR;  /* default log level */

    while( ( getoptret = getopt( argc, argv, "vhV:" ) ) != EOF)
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
            usage();
            exit( 1 );
        }
        case 'h' :
        {
            usage();
            exit( 1 );
        }
        } /* switch */
        set_log_level( log_level );
    }
} /* void process_args( ... ) */

void *occupy_thread( void *rest )
{
    __n_assert( rest , return NULL );

    intptr_t sleep_value = (intptr_t)(rest) ;

    n_log( LOG_INFO, "Starting to sleep on thread %d", pthread_self() );

    sleep( sleep_value );

    return NULL ;
}

int main(int argc, char **argv)
{
    set_log_level( LOG_DEBUG );

    /* processing args and set log_level */
    process_args( argc, argv );

    /* testing with thread pool */
    int error = 0 ;
    THREAD_POOL *thread_pool = new_thread_pool( 2, 128 );

    int sleep_value = 5+rand()%10 ;
    if( add_threaded_process( thread_pool, &occupy_thread, (void *)(intptr_t)sleep_value, DIRECT_PROC) == FALSE )
    {
        n_log( LOG_ERR, "Error ading client management to thread pool" );
    }
    refresh_thread_pool( thread_pool );


    n_log( LOG_NOTICE, "Waiting thread_pool..." );
    wait_for_threaded_pool( thread_pool, 50000 );
    n_log( LOG_NOTICE, "Destroying thread_pool..." );
    destroy_threaded_pool( &thread_pool, 50000 );

    exit( 0 );
} /* END_OF_MAIN() */
