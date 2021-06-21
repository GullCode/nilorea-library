/**\file n_user.c
*
*  USERS fonctions
*
*  USERS handling for everything you want
*
*\author Castagnier Mickael
*
*\version 1.0
*
*\date 20/02/2006
*
*/



#include "nilorea/n_user.h"



N_USERLIST *userlist_new( int max )
{
    N_USERLIST *ulist = NULL ;
    Malloc( ulist, N_USERLIST, 1 );
    __n_assert( ulist, return NULL );

    Malloc( ulist -> list, N_USER, max );
    __n_assert( ulist -> list, Free( ulist ) ; return NULL );

    ulist -> max = max ;
    ulist -> highter = 0;
    for( int it = 0; it < max ; it ++ )
    {
        ulist -> list[ it ] . state = 0 ;
        ulist -> list[ it ] . nb_rec_pos = 10 ;
        ulist -> list[ it ] . only_last_pos = 1 ;
        ulist -> list[ it ] . id = -1 ;
        Malloc( ulist -> list[ it ] . last_positions, VECTOR3D, 10 );
        ulist -> list[ it ] . netw_waitlist = new_generic_list( 0 );
        memset( ulist -> list[ it ] . position, 0, 3 * sizeof( double ) );
        ulist -> list[ it ] . netw  = NULL ;
        memset( ulist -> list[ it ] . name, 0, 1024 );
        ulist -> list[ it ] . netw  = NULL ;
    }

    init_lock( ulist -> user_rwbolt );

    return ulist ;
}



int userlist_set_position_behavior( N_USERLIST *ulist, int id, int nb_rec_pos, int only_last_pos )
{
    __n_assert( ulist, return FALSE );
    __n_assert( nb_rec_pos < 1, return FALSE );
    __n_assert( only_last_pos < 0 || only_last_pos > 1, return FALSE );

    read_lock( ulist -> user_rwbolt );
    __n_assert( ulist -> list[ id ] . state == 0, return FALSE );
    unlock( ulist -> user_rwbolt );

    write_lock( ulist -> user_rwbolt );
    ulist -> list[ id ] . nb_rec_pos = nb_rec_pos ;
    ulist -> list[ id ] . only_last_pos = only_last_pos ;
    Realloc(  ulist -> list[ id ] . last_positions, VECTOR3D, nb_rec_pos );
    __n_assert( ulist -> list[ id ] . last_positions, unlock( ulist -> user_rwbolt ); return FALSE );
    unlock( ulist -> user_rwbolt );

    return TRUE ;
}



int userlist_add_user( N_USERLIST *ulist, NETWORK *netw )
{

    int it = -1;
    write_lock( ulist -> user_rwbolt );
    do
    {
        it++;
    }
    while( ulist -> list[ it ] . state != 0);
    ulist -> list[ it ] . state = 1;
    ulist -> list[ it ] . netw = netw ;

    if( it > ulist -> highter ) ulist -> highter = it;
    unlock( ulist -> user_rwbolt );

    return it;
}



int userlist_del_user( N_USERLIST *ulist, int id )
{
    write_lock( ulist -> user_rwbolt );
    if( ulist -> list[ id ] . state == 0 )
    {
        unlock( ulist -> user_rwbolt );
        return FALSE;
    }
    ulist -> list[ id ] . state = 0 ;
    ulist -> list[ id ] . nb_rec_pos = 10 ;
    ulist -> list[ id ] . only_last_pos = 1 ;
    ulist -> list[ id ] . id = -1 ;
    Realloc(  ulist -> list[ id ] . last_positions, VECTOR3D, 10 );
    list_empty( ulist -> list[ id ] . netw_waitlist );
    memset( ulist -> list[ id ] . position, 0, 3 * sizeof( double ) );
    ulist -> list[ id ] . netw  = NULL ;
    memset( ulist -> list[ id ] . name, 0, 1024 );
    if( id >=  ulist -> highter )
    {
        int it =  ulist -> highter ;
        while( it >= 0 )
        {
            if( ulist -> list[ it ] . state == 1 )
            {
                ulist -> highter  = it ;
                break ;
            }
            it -- ;
        }
    }
    unlock( ulist -> user_rwbolt );
    return TRUE;
}

/* mode = USERLIST_ALL , USERLIST_ONE , USERLIST_ALL_EXCEPT */
int userlist_add_msg_to_ex( N_USERLIST *ulist, N_STR *msg, int mode, int id )
{
    __n_assert( ulist, return FALSE );

    switch( mode )
    {
    case( USERLIST_ALL ):
        read_lock( ulist -> user_rwbolt );
        for( int it = 0 ; it <= ulist -> highter ; it ++ )
        {
            if( ulist -> list[ it ] . state == 1 )
                netw_add_msg( ulist -> list[ it ] . netw, nstrdup( msg ) );
        }
        unlock( ulist -> user_rwbolt );
        break;
    case( USERLIST_ALL_EXCEPT ):
        read_lock( ulist -> user_rwbolt );
        for( int it = 0 ; it <= ulist -> highter ; it ++ )
        {
            if( it == id )
                continue ;
            if( ulist -> list[ it ] . state == 1 )
                netw_add_msg( ulist -> list[ it ] . netw, nstrdup( msg ) );
        }
        unlock( ulist -> user_rwbolt );
        break;
    default:
    case( USERLIST_ONE ):
        read_lock( ulist -> user_rwbolt );
        netw_add_msg( ulist -> list[ id ] . netw, msg );
        unlock( ulist -> user_rwbolt );
        break;
    }
    return TRUE ;
}



int userlist_add_msg_to( N_USERLIST *ulist, N_STR *msg, int id )
{
    return userlist_add_msg_to_ex( ulist, msg, USERLIST_ONE, id );
}



int userlist_add_msg_to_all( N_USERLIST *ulist, N_STR *msg )
{
    return userlist_add_msg_to_ex( ulist, msg, USERLIST_ALL, -1 );
}



int userlist_add_msg_to_all_except( N_USERLIST *ulist, N_STR *msg, int id )
{
    return userlist_add_msg_to_ex( ulist, msg, USERLIST_ALL_EXCEPT, id );
}


int userlist_destroy( N_USERLIST **ulist )
{
    __n_assert( ulist&&(*ulist), return FALSE );

    write_lock( (*ulist) -> user_rwbolt );
    for( int it = 0 ; it < (*ulist)  -> max ; it ++ )
    {
        Free( (*ulist) -> list[it ] . last_positions );
        list_destroy( &(*ulist) -> list[it ] . netw_waitlist );
    }
    Free( (*ulist) -> list );
    unlock( (*ulist) -> user_rwbolt );

    pthread_rwlock_destroy( &(*ulist) -> user_rwbolt );
    Free( (*ulist) );

    return FALSE ;
}


int userlist_user_add_waiting_msg( N_USERLIST *ulist, int id, N_STR *netmsg )
{
    __n_assert( ulist, return FALSE );

    int ret = FALSE ;

    read_lock( ulist -> user_rwbolt );
    if( id <= ulist -> highter )
    {
        if( ulist -> list[ id ] . state == 1 )
        {
            ret = list_push( ulist -> list[ id ] . netw_waitlist, netmsg, NULL );
        }
    }
    unlock( ulist -> user_rwbolt );
    return ret ;
}

int userlist_user_send_waiting_msgs( N_USERLIST *ulist, int id )
{
    read_lock( ulist -> user_rwbolt );
    if( id <= ulist -> highter )
    {
        if( ulist -> list[ id ] . state == 1 )
        {
            list_foreach( node, ulist -> list[ id ] . netw_waitlist )
            {
                N_STR *netmsg = (N_STR *)node -> ptr ;
                netw_add_msg( ulist -> list[ id ] . netw, netmsg );
            }
            list_empty( ulist -> list[ id ] . netw_waitlist );
        }
    }
    unlock( ulist -> user_rwbolt );
    return TRUE ;
}



int userlist_send_waiting_msgs( N_USERLIST *ulist )
{
    read_lock( ulist -> user_rwbolt );
    for( int id = 0 ; id <= ulist -> highter ; id ++ )
    {
        if( id <= ulist -> highter )
        {
            if( ulist -> list[ id ] . state == 1 )
            {
                list_foreach( node, ulist -> list[ id ] . netw_waitlist )
                {
                    N_STR *netmsg = (N_STR *)node -> ptr ;
                    netw_add_msg( ulist -> list[ id ] . netw, netmsg );
                }
                list_empty( ulist -> list[ id ] . netw_waitlist );
            }
        }
    }
    unlock( ulist -> user_rwbolt );
    return TRUE ;
}
