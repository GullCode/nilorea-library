/**\example ex_gui_netclient.c Nilorea Library gui networking api test
 *\author Castagnier Mickael
 *\version 1.0
 *\date 02/05/2020
 */

#define WIDTH 640
#define HEIGHT 480

#define HAVE_ALLEGRO 1

#include "nilorea/n_common.h"
#include "nilorea/n_str.h"
#include "nilorea/n_list.h"
#include "nilorea/n_hash.h"
#include "nilorea/n_network.h"
#include "nilorea/n_network_msg.h"
#include "nilorea/n_allegro5.h"


/*! a simple structure to hold objects positions / timeouts */
typedef struct PEER_OBJECT
{
    /*! last received object position */
    double position[ 3 ];
    /*! remote object id */
    int id ;
    /*! current life time, updated when receiving a position from a peer. Object is removed if it reach 0 */
    long int life_time ;
} PEER_OBJECT ;



int update_peer( HASH_TABLE *peer_table, int id, double position[ 3 ] )
{
    __n_assert( peer_table, return FALSE );

    PEER_OBJECT *peer = NULL ;

    if( ht_get_ptr_ex( peer_table, id, (void *)&peer ) == TRUE )
    {
        memcpy( peer -> position, position, 3 * sizeof( double ) );
        peer -> life_time = 10000000 ; /* 10s in usec */
    }
    else
    {

        Malloc( peer, PEER_OBJECT, 1 );
        __n_assert( peer, return FALSE );

        memcpy( peer -> position, position, 3 * sizeof( double ) );
        peer -> id = id ;
        peer -> life_time = 10000000 ; /* 10s in usec */

        ht_put_ptr_ex( peer_table, id, peer, &free );
    }
    return TRUE ;
}

int manage_peers( HASH_TABLE *peer_table, int delta_t )
{
    LIST *to_kill = new_generic_list( -1 );
    ht_foreach( node, peer_table )
    {
        PEER_OBJECT *peer = hash_val( node, PEER_OBJECT );
        peer -> life_time -= delta_t ;
        if( peer -> life_time < 0 )
            list_push( to_kill, peer, NULL );
    }
    list_foreach( node, to_kill )
    {
        if( node && node -> ptr )
        {
            PEER_OBJECT *peer = (PEER_OBJECT *)node -> ptr ;
            ht_remove_ex( peer_table, peer -> id );
        }
    }
    return TRUE ;
}

int draw_peers( HASH_TABLE *peer_table, ALLEGRO_FONT *font, ALLEGRO_COLOR color )
{
    ht_foreach( node, peer_table )
    {
        PEER_OBJECT *peer = hash_val( node, PEER_OBJECT );
        /* mouse pointers */
        al_draw_line( peer -> position[ 0 ] - 5, peer -> position[ 1 ], peer -> position[ 0 ] + 5, peer -> position[ 1 ], al_map_rgb( 255, 0, 0 ), 1 );
        al_draw_line( peer -> position[ 0 ], peer -> position[ 1 ] + 5, peer -> position[ 0 ], peer -> position[ 1 ] - 5, al_map_rgb( 255, 0, 0 ), 1 );
        N_STR *textout = NULL ;
        nstrprintf( textout, "id: %d", peer -> id );
        al_draw_text( font, color, peer -> position[ 0 ] + 10, peer -> position[ 1 ], ALLEGRO_ALIGN_LEFT, _nstr( textout) );
        free_nstr( &textout );
    }
    return TRUE ;
}

void usage(void)
{
    fprintf( stderr, "     -v version\n"
             "     -V log level: LOG_INFO, LOG_NOTICE, LOG_ERR, LOG_DEBUG\n"
             "     -h help\n"
             "     -a serveur address name/ip to bind (server mode) (optionnal)\n"
             "     -s serveur address name/ip to connect (client mode)\n"
             "     -p port\n"
             "     -i [v4,v6] ip version (default support both ipv4 and ipv6 )\n" );
}

void process_args( int argc, char **argv, char **server, char **port, int *ip_version )
{
    int getoptret = 0,
        log_level = LOG_ERR;  /* default log level */

    /* Arguments optionnels */
    /* -v version
     * -V log level
     * -h help
     * -s serveur address name/ip to connect (client mode)
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
    while( ( getoptret = getopt( argc, argv, "hvs:p:i:V:" ) ) != EOF)
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
        case 's' :
            (*server) = strdup( optarg );
            break ;
        case 'p' :
            (*port) = strdup( optarg );
            break ;
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
        break ;
        case 'h' :
        {
            usage();
            exit( 1 );
        }
        break ;
        }
    }
    set_log_level( log_level );
} /* void process_args( ... ) */


enum APP_KEYS
{
    KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_ESC, KEY_SPACE, KEY_CTRL
};
int key[ 7 ] = {false,false,false,false,false,false,false};


/* start */
int main( int argc, char *argv[] )
{
    int		DONE = 0, QUIT_ASKED = 0,                      /* Flag to check if we are always running */
            getoptret = 0,				  /* launch parameter check */
            log_level = LOG_ERR ;			 /* default LOG_LEVEL */

    ALLEGRO_DISPLAY *display  = NULL ;
    ALLEGRO_BITMAP *scr_buf       = NULL ;
    ALLEGRO_TIMER *fps_timer = NULL ;
    ALLEGRO_TIMER *logic_timer = NULL ;
    ALLEGRO_TIMER *network_heartbeat_timer = NULL ;

    char *server = NULL ;
    char *port = NULL ;
    LIST *chatbuf = NULL;
    HASH_TABLE *peer_table = NULL ;
    ALLEGRO_USTR *chat_line = NULL;

    N_STR *name = NULL,
           *password = NULL ;

    NETWORK  *netw   = NULL   ; /*! Network for managing conenctions */
    int ip_version = NETWORK_IPALL ;
    unsigned long ping_time = 0 ;

    int mx = 0, my = 0, mouse_b1 = 0, mouse_b2 = 0 ;
    int do_draw = 0, do_logic = 0, do_network_update = 0;
    N_STR *netw_exchange = NULL ;
    int it = -1, ident = -1 ;

    static pthread_t netw_thr ;

    ALLEGRO_COLOR white_color = al_map_rgba_f(1, 1, 1, 1);
    ALLEGRO_FONT *font = NULL ;


    set_log_level( LOG_ERR );

    /* processing args and set log_level */
    process_args( argc, argv, &server, &port, &ip_version );
    if( !server )
    {
        n_log( LOG_ERR, "No server given. Exiting." );
        exit( -1 );
    }
    if( !port )
    {
        n_log( LOG_ERR, "No port given. Exiting." );
        exit( -1 );
    }

#ifdef __linux__
    struct sigaction sa;
    sa.sa_handler = netw_sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }
#endif

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

    fps_timer = al_create_timer( 1.0/60.0 );
    logic_timer = al_create_timer( 1.0/60.0 );
    network_heartbeat_timer = al_create_timer( 1.0/10.0 );

    al_set_new_display_flags( ALLEGRO_OPENGL|ALLEGRO_WINDOWED );
    display = al_create_display( WIDTH, HEIGHT );
    if( !display )
    {
        n_abort("Unable to create display\n");
    }
    al_set_window_title( display, argv[ 0 ] );

    al_set_new_bitmap_flags( ALLEGRO_VIDEO_BITMAP );

    peer_table = new_ht( 1024 );
    chatbuf = new_generic_list( 1000 );
    chat_line = al_ustr_new( "" );
    font = al_load_font( "2Dumb.ttf", 18, 0 );
    if (! font )
    {
        n_log( LOG_ERR, "Unable to load 2Dumb.ttf" );
        exit( 1 );
    }
    al_register_event_source(event_queue, al_get_display_event_source(display));

    al_start_timer( fps_timer );
    al_start_timer( logic_timer );
    al_start_timer( network_heartbeat_timer );

    al_register_event_source(event_queue, al_get_timer_event_source(fps_timer));
    al_register_event_source(event_queue, al_get_timer_event_source(logic_timer));
    al_register_event_source(event_queue, al_get_timer_event_source(network_heartbeat_timer));

    al_register_event_source(event_queue, al_get_keyboard_event_source());
    al_register_event_source(event_queue, al_get_mouse_event_source());

    ALLEGRO_BITMAP *scrbuf = al_create_bitmap( WIDTH, HEIGHT );

    al_hide_mouse_cursor(display);

    if( netw_connect( &netw, server, port, ip_version ) != TRUE )
    {
        n_log( LOG_ERR, "Could not connect to %s:%s", _str( server ), _str( port ) );
    }
    netw_start_thr_engine( netw );

    name = char_to_nstr( "ClientName" );
    password = char_to_nstr( "ClientPassword" );

    if( netw_send_ident( netw, NETMSG_IDENT_REQUEST, 0, name, password ) == FALSE )
    {
        n_log( LOG_ERR, "Sending ident request failed" );
        goto exit_client;
    }
    n_log( LOG_INFO,"Ident request sended");

    unsigned long int ident_timeout = 10000000 ;
    do
    {
        netw_exchange = netw_get_msg( netw );
        if( netw_exchange )
        {
            int type = netw_msg_get_type( netw_exchange );
            if( type == NETMSG_IDENT_REPLY_OK )
            {
                break ;
            }
            if( type == NETMSG_IDENT_REPLY_NOK )
            {
                n_log( LOG_ERR, "Login refused !!!" );
                goto exit_client ;
            }
        }
        ident_timeout -= 1000 ;
        usleep( 1000 );
    }
    while( ident_timeout > 0 );
    free_nstr( &name );
    free_nstr( &password );
    netw_get_ident( netw_exchange, &it, &ident, &name, &password );
    n_log( LOG_INFO,"Id Message received, pointer:%p size %d id %d", netw_exchange -> data, netw_exchange -> length, ident );

    if( ident == -1 )
    {
        n_log( LOG_ERR,"No ident received");
        goto exit_client;
    }
    /* We got our id ! */
    n_log( LOG_NOTICE, "Id is:%d", ident );
    DONE = 0 ;
    QUIT_ASKED = 0 ;
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
        else if( ev.keyboard.keycode == ALLEGRO_KEY_ENTER )
        {
            N_STR *txtmsg = new_nstr( 1024 );
            al_ustr_trim_ws( chat_line ) ;
            al_ustr_to_buffer(  chat_line, txtmsg -> data, txtmsg -> length );

            N_STR *netmsg = netmsg_make_string_msg( netw -> link . sock, -1, name, txtmsg, txtmsg, 1 );
            netw_add_msg( netw, netmsg );
            free_nstr( &txtmsg );

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
            else if( al_get_timer_event_source( network_heartbeat_timer ) == ev.any.source )
            {
                do_network_update = 1;
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
        else if( ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE )
        {
            DONE = 1 ;
        }

        /* Processing inputs */
        get_keyboard( chat_line, ev );

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
            N_STR *netmsg = NULL ;
            netmsg = netw_get_msg( netw );
            if( netmsg )
            {
                int type = -1 ;
                type = netw_msg_get_type( netmsg );
                switch( type )
                {
                case( NETMSG_GET_BOX ):
                    /* ask for world box */
                    break ;
                case( NETMSG_BOX ):
                    /* a world object at position X,Y,Z with associated datas, add/update local world cache */
                    break ;
                case( NETMSG_POSITION ):
                {

                    /* add/update object with id at position */
                    double pos[ 3 ];
                    double spe[ 3 ];
                    int id = -1, timestamp = -1 ;
                    netw_get_position( netmsg, &id, &pos[ 0 ],&pos[ 1 ],&pos[ 2 ], &spe[ 0 ], &spe[ 1 ], &spe[ 2 ], &timestamp );
                    // too much log n_log( LOG_INFO, "Received position from %d: %g %g", id, pos[ 0 ], pos[ 1 ] );
                    update_peer( peer_table, id, pos );
                }
                break;
                case( NETMSG_STRING ):
                {
                    /* add text to chat */
                    int id = -1, color = -1 ;
                    N_STR *name = NULL, *txt = NULL, *chan = NULL ;
                    netw_get_string( netmsg, &id, &name, &chan, &txt, &color );
                    n_log( LOG_INFO, "Received chat from %s:%s:%s (id: %d , color:%d)", name, chan, txt, id, color );
                }
                break;
                case( NETMSG_PING_REQUEST ):
                    /* compare to sent pings and add string to chat with times */
                    break;
                case( NETMSG_PING_REPLY ):
                    /* compare to sent pings and add string to chat with times */
                    break;
                case( NETMSG_QUIT ):
                    n_log( LOG_INFO, "Quit received !" );
                    DONE = 1 ;
                    break ;
                default:
                    n_log( LOG_ERR, "Unknow message type %d", type );
                    DONE = 1 ;
                    break ;
                }
            }
            if( mouse_button == 1 )
            {

            }
            ping_time += 1.0 / 60.0 ;

            if( key[KEY_ESC] )
            {
                if( QUIT_ASKED != 1 )
                {
                    QUIT_ASKED = 1 ;
                    n_log( LOG_INFO, "Asking server to quit..." );
                    netw_send_quit( netw );
                }
            }
            manage_peers( peer_table, 1000000 / 60.0 );
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
            /*al_draw_line( mx - 5, my, mx + 5, my, al_map_rgb( 255, 0, 0 ), 1 );
            al_draw_line( mx, my + 5, mx, my - 5, al_map_rgb( 255, 0, 0 ), 1 );*/

            draw_peers( peer_table, font, white_color );

            al_draw_ustr( font, white_color, 0, HEIGHT - 20, ALLEGRO_ALIGN_LEFT, chat_line );

            al_flip_display();
            do_draw = 0 ;
        }

        if( do_network_update == 1 )
        {
            /* send position here */
            netw_send_position( netw, ident, mx, my, 0, 0, 0, 0, 0 );
            do_network_update = 0 ;
        }

    }
    while( !DONE );

exit_client:
    netw_wait_close( &netw );

    return 0;

}
