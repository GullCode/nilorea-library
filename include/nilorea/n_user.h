/**\file n_user.h
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


#ifndef N_USER_MANAGEMENT

#define N_USER_MANAGEMENT

#ifdef __cplusplus
extern "C"
{
#endif

#include "nilorea/n_common.h"
#include "nilorea/n_log.h"
#include "nilorea/n_str.h"
#include "nilorea/n_network.h"
#include "nilorea/n_3d.h"

#define USERLIST_ALL 0
#define USERLIST_ALL_EXCEPT 1
#define USERLIST_ONE 2

/*! USER management cell */
typedef struct N_USER
{
    /*! User Name */
    char name[ 1024 ];

    /*! Associated NETWORK */
    NETWORK *netw;

    /*! State of the current user */
    int state,
        /*! Number of saved positions , default: 10 */
        nb_rec_pos,
        /*! 1 => keep only_last_position in waitlist , 0 => send all the positions, default: 1 */
        only_last_pos,
        /*! Unique world ident */
        id;

    /*! Last nb_rec_pos position messages, for a better dead reckoning / lag simulation */
    VECTOR3D *last_positions;
    /*! actual position */
    VECTOR3D position;

    /*! N_STR *messages waiting to be sent */
    LIST *netw_waitlist;

} N_USER ;



/*! USER list */
typedef struct N_USERLIST
{
    /*! Pointer to the allocated list of user */
    N_USER *list;

    /*! Maximum of user inside the list */
    int max,
        /*! Position of the highter user inside the list */
        highter;

    /*! Mutex for thread safe user management */
    pthread_rwlock_t user_rwbolt;

} N_USERLIST ;



N_USERLIST *userlist_new( int max );
int userlist_set_position_behavior( N_USERLIST *ulist, int id, int nb_rec_pos, int only_last_pos );
int userlist_add_user( N_USERLIST *ulist, NETWORK *netw );
int userlist_del_user( N_USERLIST *ulist, int id );
int userlist_add_msg_to_ex( N_USERLIST *ulist, N_STR *msg, int mode, int id );
int userlist_add_msg_to( N_USERLIST *ulist, N_STR *msg, int id );
int userlist_add_msg_to_all( N_USERLIST *ulist, N_STR *msg );
int userlist_add_msg_to_all_except( N_USERLIST *ulist, N_STR *msg, int id );
int userlist_destroy( N_USERLIST **ulist );
int userlist_user_add_waiting_msg( N_USERLIST *ulist, int id, N_STR *netmsg );
int userlist_user_send_waiting_msgs( N_USERLIST *ulist, int id );
int userlist_send_waiting_msgs( N_USERLIST *ulist );


#ifdef __cplusplus
}
#endif

#endif /* #ifndef N_USER_MANAGEMENT */
