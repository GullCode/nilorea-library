/**\example  ex_gui_netserver.c Nilorea Library gui networking api test
 *\author Castagnier Mickael
 *\version 1.0
 *\date 02/05/2020
 */

#define WIDTH 640
#define HEIGHT 480

#include "nilorea/n_common.h"
#include "nilorea/n_str.h"
#include "nilorea/n_list.h"
#include "nilorea/n_hash.h"
#include "nilorea/n_network.h"
#include "nilorea/n_network_msg.h"
#include "nilorea/n_thread_pool.h"

#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_ttf.h>

void usage(void)
{
    fprintf( stderr, "     -v version\n"
             "     -V log level: LOG_INFO, LOG_NOTICE, LOG_ERR, LOG_DEBUG\n"
             "     -h help\n"
             "     -a serveur address name/ip to bind (server mode)\n"
             "     -p port\n"
             "     -i [v4,v6] ip version (default support both ipv4 and ipv6 )\n" );
} /* usage() */



void process_args( int argc, char **argv, char **bind_addr, char **port, int *ip_version )
{
    int getoptret = 0,
        log_level = LOG_ERR;  /* default log level */

    /* Arguments optionnels */
    /* -v version
     * -V log level
     * -h help
     * -a serveur address name/ip to bind (server mode, optionnal)
     * -p port
     * -i v4 ip version (default support both ipv4 and ipv6 )
     * -i v6 ip version (   "       "     "    "    "   "   )
     */
    if( argc == 1 )
    {
        fprintf( stderr,  "No arguments given, help:\n" );
        usage();
        exit( 1 );
    }
    while( ( getoptret = getopt( argc, argv, "hva:p:i:V:" ) ) != EOF)
    {
        switch( getoptret )
        {
        case 'i' :
            if( !strcmp( "v4", optarg ) )
            {
                (*ip_version) = NETWORK_IPV4 ;
                n_log( LOG_NOTICE, "IPV4 selected" );
            }
            else if( !strcmp( "v6", optarg ) )
            {
                (*ip_version) = NETWORK_IPV6 ;
                n_log( LOG_NOTICE, "IPV6 selected" );
            }
            else
            {
                n_log( LOG_NOTICE, "IPV4/6 selected" );
            }
            break;
        case 'v' :
            fprintf( stderr, "Date de compilation : %s a %s.\n", __DATE__, __TIME__ );
            exit( 1 );
        case 'V' :
            if( !strncmp( "LOG_NULL", optarg, 8 ) )
            {
                log_level = LOG_NULL ;
            }
            else
            {
                if( !strncmp( "LOG_NOTICE", optarg, 10 ) )
                {
                    log_level = LOG_NOTICE;
                }
                else
                {
                    if( !strncmp( "LOG_INFO", optarg, 8 ) )
                    {
                        log_level = LOG_INFO;
                    }
                    else
                    {
                        if( !strncmp( "LOG_ERR", optarg, 7 ) )
                        {
                            log_level = LOG_ERR;
                        }
                        else
                        {
                            if( !strncmp( "LOG_DEBUG", optarg, 9 ) )
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
        case 'a' :
            (*bind_addr) = strdup( optarg );
            break ;
        case 'p' :
            (*port) = strdup( optarg );
            break ;
        default :
        case '?' :
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
            break ;
        case 'h' :
            usage();
            exit( 1 );
            break ;
        }
    }
    set_log_level( log_level );
} /* void process_args( ... ) */



int process_clients( NETWORK_POOL *netw_pool )
{
    __n_assert( netw_pool, return FALSE );

    /* write lock the pool */
    read_lock( netw_pool -> rwlock );
    ht_foreach( node, netw_pool -> pool )
    {
        N_STR *netw_exchange = NULL ;
        NETWORK *netw = hash_val( node, NETWORK );
        /* process all data sent by the client */
        while( ( netw_exchange = netw_get_msg( netw ) ) )
        {
            int type =  netw_msg_get_type( netw_exchange ) ;
            switch( type )
            {
            case( NETMSG_POSITION ):
                // add/update object with id at position
                break;
            case( NETMSG_STRING ):
                // add text to chat
                netw_pool_broadcast( netw_pool, NULL, netw_exchange );
                break;
            case( NETMSG_PING_REQUEST ):
                // compare to sent pings and add string to chat with times
                break;
            case( NETMSG_GET_BOX ):
                // a world object at position X,Y,Z with associated datas, add/update local world cache
                break;
            case( NETMSG_IDENT_REQUEST ):
            {
                N_STR *name = NULL, *passwd = NULL ;
                int  d_it = 0, ident = 0 ;
                netw_get_ident( netw_exchange, &d_it, &ident, &name, &passwd );
                n_log( LOG_NOTICE, "Ident %d received", netw -> link . sock );
                netw_send_ident( netw, NETMSG_IDENT_REPLY_OK, netw -> link . sock, name, passwd );
                n_log( LOG_NOTICE, "Answer sent", netw -> link . sock );
            }
            break ;
            default:
                n_log( LOG_ERR, "Unknow message type %d", type );
                break ;
            }
            free_nstr( &netw_exchange );
        }
    }
    unlock( netw_pool -> rwlock );
    return TRUE ;
} /* process clients datas */


void* manage_client( void *ptr )
{
    NETWORK *netw  = (NETWORK *)ptr ;
    N_STR *netw_exchange = NULL ;
    int state = 0, thr_engine_state = 0 ;

    n_log( LOG_NOTICE, "manage_client started for netw %d", netw -> link . sock );

    netw_start_thr_engine( netw );

    int DONE = 0 ;
    while( !DONE )
    {
        if( ( netw_exchange = netw_get_msg( netw ) ) )
        {
            list_foreach( node, netw -> pools )
            {
                NETWORK_POOL *pool = (NETWORK_POOL *)node -> ptr ;
                netw_pool_broadcast( pool, NULL, netw_exchange );
            }

            /*N_STR *hostname = NULL, *data = NULL ;
            int type =  netw_msg_get_type( netw_exchange ) ;
            switch( type )
            {
            case( NETMSG_POSITION ):
                // add/update object with id at position
                break;
            case( NETMSG_STRING ):
                // add text to chat
                break;
            case( NETMSG_PING_REQUEST ):
                // compare to sent pings and add string to chat with times
                break;
            case( NETMSG_GET_BOX ):
                // a world object at position X,Y,Z with associated datas, add/update local world cache
                break;
            default:
                n_log( LOG_ERR, "Unknow message type %d", type );
                DONE = 1 ;
                break ;
            }
            if( data )
                free_nstr( &data );
            if( hostname )
                free_nstr( &hostname ); */
            if( netw_exchange )
                free_nstr( &netw_exchange );
        }
        else
        {
            u_sleep( 1000 );
        }
        netw_get_state( netw, &state, &thr_engine_state );
        if( (state&NETW_EXITED) || (state&NETW_ERROR ) || (state&NETW_EXIT_ASKED) )
            DONE = 1 ;
    }/* while( !DONE ) */

    SOCKET sockid = netw -> link . sock ;
    n_log( LOG_NOTICE, "manage_client stopping for netw %d...", sockid );
    netw_wait_close( &netw );
    n_log( LOG_NOTICE, "network closed for netw %d !", sockid );

    return NULL ;
} /* manage_client(...) */



int main( int argc, char *argv[] )
{
    ALLEGRO_DISPLAY *display  = NULL ;

    int		DONE = 0,                    /* Flag to check if we are always running */
            getoptret = 0,				  /* launch parameter check */
            log_level = LOG_ERR,  /* default log level */
            ip_version = NETWORK_IPALL ;

    ALLEGRO_BITMAP *scr_buf       = NULL ;

    ALLEGRO_TIMER *fps_timer = NULL ;
    ALLEGRO_TIMER *logic_timer = NULL ;

    NETWORK *server = NULL ;
    NETWORK_POOL *netw_pool = NULL ;
    char *bind_addr = NULL,
          *port = NULL ;

    set_log_level( LOG_NOTICE );

    /* processing args and set log_level */
    process_args( argc, argv, &bind_addr, &port, &ip_version );
    if( !port )
    {
        n_log( LOG_ERR, "No port given. Exiting." );
        exit( -1 );
    }
#ifdef __linux__
    struct sigaction sa;
    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }
#endif

    netw_pool = netw_new_pool( 1024 );

    n_log( LOG_INFO, "Creating listening network for %s:%s %d", _str( bind_addr ), _str( port ), ip_version );
    /* create listening network */
    if( netw_make_listening( &server, bind_addr, port, 128, ip_version ) == FALSE )
    {
        n_log( LOG_ERR, "Fatal error with network initialization" );
        netw_unload();
        exit( -1 );
    }

    n_log( LOG_NOTICE, "%s is starting ...", argv[ 0 ] );
    /* allegro 5 + addons loading */
    if (!al_init())
    {
        n_abort("Could not init Allegro.\n");
    }
    if (!al_install_audio())
    {
        n_abort("Unable to initialize audio addon\n");
    }
    if (!al_init_acodec_addon())
    {
        n_abort("Unable to initialize acoded addon\n");
    }
    if (!al_init_image_addon())
    {
        n_abort("Unable to initialize image addon\n");
    }
    if (!al_init_primitives_addon() )
    {
        n_abort("Unable to initialize primitives addon\n");
    }
    if( !al_init_font_addon() )
    {
        n_abort("Unable to initialize font addon\n");
    }
    if( !al_init_ttf_addon() )
    {
        n_abort("Unable to initialize ttf_font addon\n");
    }
    if( !al_install_keyboard() )
    {
        n_abort("Unable to initialize keyboard handler\n");
    }
    if( !al_install_mouse())
    {
        n_abort("Unable to initialize mouse handler\n");
    }
    ALLEGRO_EVENT_QUEUE *event_queue = NULL;

    event_queue = al_create_event_queue();
    if(!event_queue)
    {
        fprintf(stderr, "failed to create event_queue!\n");
        al_destroy_display(display);
        return -1;
    }

    fps_timer = al_create_timer( 1.0/30.0 );
    logic_timer = al_create_timer( 1.0/50.0 );

    al_set_new_display_flags( ALLEGRO_OPENGL|ALLEGRO_WINDOWED );
    display = al_create_display( WIDTH, HEIGHT );
    if( !display )
    {
        n_abort("Unable to create display\n");
    }
    al_set_window_title( display, argv[ 0 ] );

    al_set_new_bitmap_flags( ALLEGRO_VIDEO_BITMAP );

    DONE = 0 ;

    LIST *active_object = new_generic_list( -1 );

    enum APP_KEYS
    {
        KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_ESC, KEY_SPACE, KEY_CTRL
    };
    int key[ 7 ] = {false,false,false,false,false,false,false};

    al_register_event_source(event_queue, al_get_display_event_source(display));

    al_start_timer( fps_timer );
    al_start_timer( logic_timer );
    al_register_event_source(event_queue, al_get_timer_event_source(fps_timer));
    al_register_event_source(event_queue, al_get_timer_event_source(logic_timer));

    al_register_event_source(event_queue, al_get_keyboard_event_source());
    al_register_event_source(event_queue, al_get_mouse_event_source());

    ALLEGRO_BITMAP *scrbuf = al_create_bitmap( WIDTH, HEIGHT );

    al_hide_mouse_cursor(display);

    int mx = 0, my = 0, mouse_b1 = 0, mouse_b2 = 0 ;
    int do_draw = 0, do_logic = 0 ;

    THREAD_POOL *thread_pool = new_thread_pool( 8, 0 );

    do
    {

        ALLEGRO_EVENT ev ;

        al_wait_for_event(event_queue, &ev);

        if(ev.type == ALLEGRO_EVENT_KEY_DOWN)
        {
            switch(ev.keyboard.keycode)
            {
            case ALLEGRO_KEY_UP:
                key[KEY_UP] = 1;
                break;
            case ALLEGRO_KEY_DOWN:
                key[KEY_DOWN] = 1;
                break;
            case ALLEGRO_KEY_LEFT:
                key[KEY_LEFT] = 1;
                break;
            case ALLEGRO_KEY_RIGHT:
                key[KEY_RIGHT] = 1;
                break;
            case ALLEGRO_KEY_ESCAPE:
                key[KEY_ESC] = 1 ;
                break;
            case ALLEGRO_KEY_SPACE:
                key[KEY_SPACE] = 1 ;
                break;
            case ALLEGRO_KEY_LCTRL:
            case ALLEGRO_KEY_RCTRL:
                key[KEY_CTRL] = 1 ;
            default:
                break;
            }
        }
        else if(ev.type == ALLEGRO_EVENT_KEY_UP)
        {
            switch(ev.keyboard.keycode)
            {
            case ALLEGRO_KEY_UP:
                key[KEY_UP] = 0;
                break;
            case ALLEGRO_KEY_DOWN:
                key[KEY_DOWN] = 0;
                break;
            case ALLEGRO_KEY_LEFT:
                key[KEY_LEFT] = 0;
                break;
            case ALLEGRO_KEY_RIGHT:
                key[KEY_RIGHT] =0;
                break;
            case ALLEGRO_KEY_ESCAPE:
                key[KEY_ESC] = 0 ;
                break;
            case ALLEGRO_KEY_SPACE:
                key[KEY_SPACE] = 0 ;
                break;
            case ALLEGRO_KEY_LCTRL:
            case ALLEGRO_KEY_RCTRL:
                key[KEY_CTRL] = 0 ;
            default:
                break;
            }
        }
        else if( ev.type == ALLEGRO_EVENT_TIMER )
        {
            if( al_get_timer_event_source( fps_timer ) == ev.any.source )
            {
                do_draw = 1 ;
            }
            else if( al_get_timer_event_source( logic_timer ) == ev.any.source )
            {
                do_logic = 1;
            }
        }
        else if( ev.type == ALLEGRO_EVENT_MOUSE_AXES )
        {
            mx = ev.mouse.x;
            my = ev.mouse.y;
        }
        else if( ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN )
        {
            if( ev.mouse.button == 1 )
                mouse_b1 = 1 ;
            if( ev.mouse.button == 2 )
                mouse_b2 = 1 ;
        }
        else if( ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP )
        {
            if( ev.mouse.button == 1 )
                mouse_b1 = 0 ;
            if( ev.mouse.button == 2 )
                mouse_b2 = 0 ;
        }


        /* Processing inputs */

        /* dev mod: right click to temporary delete a block
           left click to temporary add a block */
        int mouse_button = -1 ;
        if( mouse_b1==1 )
            mouse_button = 1 ;
        if( mouse_b2==1 )
            mouse_button = 2 ;


        if( key[ KEY_UP ] )
        {
        }
        else
        {
        }


        if( key[ KEY_DOWN ] )
        {
        }
        else
        {
        }

        if( key[ KEY_LEFT ] )
        {
        }
        else
        {
        }


        if( key[ KEY_RIGHT ] )
        {
        }
        else
        {
        }


        if( key[KEY_CTRL ] || mouse_button == 1 )
        {

        }
        else
        {

        }

        if( do_logic == 1 )
        {
            if( mouse_button == 1 )
            {
                /* do something with mouse button */

            }
            /* accept new connections */
            NETWORK *netw = NULL ;
            int error = 0 ;
            if ( ( netw = netw_accept_from_ex(  server, 0, 0, 0, 0, 0, -1, &error ) ) )
            {
                netw_start_thr_engine( netw );
                netw_pool_add( netw_pool, netw );
                /*if( add_threaded_process( thread_pool, &manage_client, (void *)&netw, DIRECT_PROC ) == FALSE )
                {
                    n_log( LOG_ERR, "Error adding client management to thread pool" );
                    netw_pool_remove( netw_pool , netw );
                }*/
            }
            process_clients( netw_pool );
            do_logic = 0 ;
        }

        if( do_draw == 1 )
        {
            al_acknowledge_resize( display );
            int w = al_get_display_width(  display );
            int h = al_get_display_height( display );

            al_set_target_bitmap( scrbuf );
            al_clear_to_color( al_map_rgba( 0, 0, 0, 255 ) );

            al_set_target_bitmap( al_get_backbuffer( display ) );

            al_clear_to_color( al_map_rgba( 0, 0, 0, 255 ) );
            al_draw_bitmap( scrbuf, 0, 0, 0 );

            /* mouse pointer */
            al_draw_line( mx - 5, my, mx + 5, my, al_map_rgb( 255, 0, 0 ), 1 );
            al_draw_line( mx, my + 5, mx, my - 5, al_map_rgb( 255, 0, 0 ), 1 );

            al_flip_display();
            do_draw = 0 ;
        }

    }
    while( !key[KEY_ESC] && !DONE );

    list_destroy( &active_object );

    return 0;

}

