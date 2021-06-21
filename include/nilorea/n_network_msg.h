/**\file n_network_msg.h
 *  network messages declaration
 *  Network Engine
 *\author Castagnier Mickael
 *\version 1.0
 *\date 10/05/2005
 */
#ifndef N_NETWORK_MESSAGES
#define N_NETWORK_MESSAGES

#ifdef __cplusplus
extern "C"
{
#endif

/**\defgroup NETWORK_MSG NETWORK MESSAGE: tools to serialize integers, double, N_STR from and back to a single N_STR *message
   \addtogroup NETWORK_MSG
  @{
*/

#include <sys/param.h>

#include "n_str.h"
#include "n_list.h"
#include "n_3d.h"

/*! Network Message is identification request: (int)type , (int)id , (N_STR *)name , (N_STR *)password */
#define NETMSG_IDENT_REQUEST 0
/*! Network Message is identification reply OK : (int)type , (int)id , (N_STR *)name , (N_STR *)password */
#define NETMSG_IDENT_REPLY_OK 1
/*! Network Message is identification reply NON OK: (int)type , (int)id , (N_STR *)name , (N_STR *)password */
#define NETMSG_IDENT_REPLY_NOK 2
/*! Network Message is string: (int)type , (int)id , (N_STR *)name , (N_STR *)chan , (N_STR *)text , (int)color */
#define NETMSG_STRING 3
/*! Network Message position: (int)type , (int)id , (int)X , (int)Y , (int)x_shift , (int)y_shift ,(int)vx ,(int)vy , (int)speed , (int)acceleration , (int)time_stamp */
#define NETMSG_POSITION 4
/*! Network Message is ping request: (int)type , (int)id_from , (int)id_to */
#define NETMSG_PING_REQUEST 5
/*! Network Message is ping reply: (int)type , (int)id_from , (int)id_to */
#define NETMSG_PING_REPLY 6
/*! Network Message is box retrieve: int x , int y , int z */
#define NETMSG_GET_BOX 7
/*! Network Message is box retrieving reply: (int x , int y , int z , N_STR *data ) */
#define NETMSG_BOX 8
/*! Network asking for exit */
#define NETMSG_QUIT 9

/*!network message, array of char and int*/
typedef struct NETW_MSG
{
    /*! array of int */
    LIST *tabint;
    /*! array of N_STR */
    LIST *tabstr;
    /*! array of casted double value */
    LIST *tabflt ;
} NETW_MSG ;

/* swap bytes */
double double_swap( double value );
/* host to network double */
double htond( double value );
/* network to host double */
double ntohd( double value );

/* Create a NETW_MSG *object */
int create_msg( NETW_MSG **msg );
/* Delete a NETW_MSG *object */
int delete_msg( NETW_MSG **msg );
/* Empty a NETW_MSG *object */
int empty_msg( NETW_MSG **msg );
/* Add an float to the the message */
int add_nb_to_msg( NETW_MSG *msg, double value );
/* Add an int to the the message */
int add_int_to_msg( NETW_MSG *msg, int value );
/* Add a string to the the message */
int add_nstrptr_to_msg( NETW_MSG *msg, N_STR *str );
/* Add a string to the the message */
int add_nstrdup_to_msg( NETW_MSG *msg, N_STR *str );
/* Add a char *string to the the message */
int add_strdup_to_msg( NETW_MSG *msg, char *str );
/* Get an int from the message */
int get_int_from_msg( NETW_MSG *msg, int *value );
/* Get a float value from the message */
int get_nb_from_msg( NETW_MSG *msg, double *value );
/* Get a N_STR *string from the message */
int get_nstr_from_msg( NETW_MSG *msg, N_STR **str );
/* Get a char *string from the message */
int get_str_from_msg( NETW_MSG *msg, char **str );
/* Make a single string of the message */
N_STR *make_str_from_msg( NETW_MSG *msg );
/* Make a message from a single string */
NETW_MSG *make_msg_from_str( N_STR *str );

N_STR *netmsg_make_ping( int type, int id_from, int id_to, int time );
N_STR *netmsg_make_ident( int type, int id, N_STR *name, N_STR *passwd );
N_STR *netmsg_make_position_msg( int id, double X, double Y, double vx, double vy, double acc_x, double acc_y, int time_stamp );
N_STR *netmsg_make_string_msg( int id_from, int id_to, N_STR *name, N_STR *chan, N_STR *txt, int color );
N_STR *netmsg_make_quit_msg( void );

int netw_msg_get_type( N_STR *msg );

int netw_get_ident( N_STR *msg, int *type, int *ident, N_STR **name, N_STR **passwd );
int netw_get_position( N_STR *msg, int *id, double *X, double *Y, double *vx, double *vy, double *acc_x, double *acc_y, int *time_stamp );
int netw_get_string( N_STR *msg, int *id, N_STR **name, N_STR **chan, N_STR **txt, int *color );
int netw_get_ping( N_STR *msg, int *type, int *from, int *to, int *time );
int netw_get_quit( N_STR *msg );

/**
@}
*/

#ifdef __cplusplus
}
#endif

#endif /*#ifndef N_NETWORK*/





