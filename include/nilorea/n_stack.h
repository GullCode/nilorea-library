/**\file n_stack.c
 *  Stack header definitions
 *\author Castagnier Mickael
 *\version 2.0
 *\date 01/01/2018
 */

#ifndef __N_STACK_HEADER
#define __N_STACK_HEADER

#ifdef __cplusplus
extern "C"
{
#endif

/**\defgroup STACK STACK: generic type stack
   \addtogroup STACK
  @{
*/

/*! STACK structure */
typedef struct STACK
{
    /*! STACK array */
    void **stack ;
    /*! Size of array */
    unsigned int size ;
} STACK ;

/**
@}
*/

#ifdef __cplusplus
}
#endif

#endif
