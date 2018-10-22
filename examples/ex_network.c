/**\file ex_n_network.c
 *  Nilorea Library n_network api test
 *\author Castagnier Mickael
 *\version 1.0
 *\date 26/05/2015
 */
#include "ex_network.h"

#define SERVER 0
#define CLIENT 1

int NB_ATTEMPTS=  2 ;

NETWORK *server = NULL , /*! Network for server mode, accepting incomming */
		*netw = NULL   ; /*! Network for managing conenctions */

int mode = -1 ;

static pthread_t netw_thr ;

void usage(void)
{
	fprintf( stderr , "     -v version\n"
			"     -V log level: NOLOG, LOG_INFO, LOG_NOTICE, LOG_ERR, LOG_DEBUG\n"
			"     -h help\n"
			"     -s serveur name/ip (client mode)\n"
			"     -p port\n" );
}



void process_args( int argc , char **argv , char **server , char **port , int *nb )
{
	int getoptret = 0 , 
		log_level = LOG_ERR;  /* default log level */

	set_log_level( log_level );

	/* Arguments optionnels */
	/* -v version
	 * -V log level
	 * -h help 
	 * -s serveur name/ip
	 * -p port
	 */

	if( argc == 1 )
	{
		fprintf( stderr ,  "No arguments given, help:\n" );
		usage();
		exit( 1 );
	}


	while( ( getoptret = getopt( argc, argv, "hvs:V:p:n:" ) ) != EOF) 
	{
		switch( getoptret )
		{
			case 'v' :
				fprintf( stderr , "Date de compilation : %s a %s.\n" , __DATE__ , __TIME__ );
				exit( 1 );
			case 'V' :
				if( !strncmp( "LOG_NULL" , optarg, 5 ) ) 
				{
					log_level = LOG_NULL ;
				}
				else 
				{
					if( !strncmp( "LOG_NOTICE" , optarg , 6 ) ) 
					{
						log_level = LOG_NOTICE;
					}
					else 
					{
						if( !strncmp( "LOG_INFO" , optarg, 7 ) ) 
						{
							log_level = LOG_INFO;
						}
						else 
						{
							if( !strncmp( "LOG_ERR" , optarg , 5 ) ) 
							{
								log_level = LOG_ERR;
							}
							else 
							{
								if( !strncmp( "LOG_DEBUG" , optarg , 5 ) ) 
								{
									log_level = LOG_DEBUG;
								}
								else 
								{
									fprintf( stderr , "%s n'est pas un niveau de log valide.\n" , optarg );
									exit( -1 );
								}
							}
						}
					}
				}
				break;
			case 's' :
				(*server) = strdup( optarg ); 
				break ;
			case 'n' :
				(*nb) = atoi( optarg ); 
				break ;
			case 'p' :
				(*port) = strdup( optarg ); 
				break ;
			default :   
			case '?' :
				{
					if( optopt == 'V' )
					{
						fprintf( stderr , "\n      Missing log level\n" );
					}else if( optopt == 'p' )
					{
						fprintf( stderr , "\n      Missing port\n" );
					}else if( optopt != 's' )
					{
						fprintf( stderr , "\n      Unknow missing option %c\n" , optopt );
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




int main(int argc, char **argv) {

	char *srv = NULL ;
	char *port = NULL ;

	/* processing args and set log_level */
	process_args( argc, argv , &srv , &port ,&NB_ATTEMPTS );

	if( !port )
	{
		n_log( LOG_ERR , "No port given. Exiting." );
		exit( -1 );
	}

	if( srv )
	{
		n_log( LOG_INFO , "Client mode, connecting to %s:%s" , srv , port );
		mode = CLIENT ;
	}
	else
	{
		n_log( LOG_INFO , "Server mode , waiting client on port %s" , port );
		mode = SERVER ;
	}

	#ifdef LINUX
	struct sigaction sa;
	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}
	#endif
	if( mode == SERVER )
	{

		/* create listening network */
		if( netw_make_listening( &server , port , 10 ) == FALSE ) 
		{
			n_log( LOG_ERR , "Fatal error with network initialization" );
			exit( -1 );
		}
		int it = 0 ;
		for( it = 0 ; it < (NB_ATTEMPTS/2) ; it ++ )
		{
			/* get any accepted client on a network */
			if ( !( netw = netw_accept_from( server ) ) )
			{
				n_log( LOG_ERR , "Error on accept" );
			}
			else
			{
				/* someone is connected. starting some dialog */
				if( pthread_create ( &netw_thr , NULL, manage_client , (void *)netw ) != 0 )
				{
					n_log( LOG_ERR , "Error creating client management thread" );
					exit( -1 );
				}
			}

			pthread_join( netw_thr , NULL );

		}
		/* testing with thread pool */
		int error = 0 , DONE = 0 ;
		THREAD_POOL *thread_pool = new_thread_pool( 2 , 128 );
		while( it < NB_ATTEMPTS )
		{
			/* get any accepted client on a network */
			if( ( netw = netw_accept_from_ex( server , 0 , 0 , 0 , 0 , 0 , -1 , &error ) ) ) 
			{
				/* someone is connected. starting some dialog */
				if( add_threaded_process( thread_pool , &manage_client , (void *)netw , DIRECT_PROC) == FALSE )
				{
					n_log( LOG_ERR , "Error ading client management to thread pool" );
				}
				it ++ ;
			}
			else
			{
				u_sleep( 100000 );
			}
			refresh_thread_pool( thread_pool );  
		}

		n_log( LOG_NOTICE , "Waiting thread_pool..." );
		wait_for_threaded_pool( thread_pool , 50000 );
		n_log( LOG_NOTICE , "Destroying thread_pool..." );
		destroy_threaded_pool( &thread_pool , 50000 );
		n_log( LOG_NOTICE , "Closing server" );
		netw_close( &server );
	}
	else if( mode == CLIENT )
	{
		for( int it = 0 ; it < NB_ATTEMPTS ; it ++ )
		{
			n_log( LOG_NOTICE , "Attempt %d/%d: Connected to %s:%s" , it+1 , NB_ATTEMPTS , srv , port );
			if( netw_connect( &netw , srv , port ) != TRUE ) 
			{
				/* there were some error when trying to connect */
				n_log( LOG_ERR , "Unable to connect to %s:%s" , srv , port );
				exit( 1 );
			}

			/* backgrounding network send / recv */
			netw_start_thr_engine( netw );

			N_STR *sended_data = NULL , *recved_data = NULL , *hostname = NULL , *tmpstr = NULL ;

			sended_data = char_to_nstr( "TEST ENVOI DE DONNEES" );
			send_net_datas( netw , sended_data );

			free_nstr( &sended_data );

			/* let's check for an answer: test each 250000 usec, with
			 * a limit of 1000000 usec */
			tmpstr = netw_wait_msg( netw , 25000 , 10000000 );

			if( tmpstr )
			{
				get_net_datas( tmpstr , &hostname , &recved_data );
				n_log( LOG_NOTICE , "Got %s from %s" , recved_data ->data , hostname ->data );
				free_nstr( &tmpstr );
				free_nstr( &recved_data );
				free_nstr( &hostname );
			}
			else
			{
				n_log( LOG_ERR , "Error getting back answer from server" );
			}

			netw_stop_thr_engine( netw );
			n_log( LOG_NOTICE , "Closing client" );
			netw_close( &netw );
		}
	}

	FreeNoLog( srv );
	FreeNoLog( port )

		exit( 0 );
} /* END_OF_MAIN() */
