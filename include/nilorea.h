/**\file nilorea.h
 *  Header to include for use with libnilorea
 *\author Castagnier Mickael
 *\version 1.0
 *\date 24/03/05
 */

#ifndef __NILOREA_Unified_Library__

#define __NILOREA_Unified_Library__

#ifdef __cplusplus
extern "C" {
#endif

/**\defgroup GLOBALHEADER HEADER: a global header to include all the modules
   \addtogroup GLOBALHEADER
  @{
*/
#include <nilorea/n_3d.h>

#include <nilorea/n_common.h>
#include <nilorea/n_config_file.h>
#include <nilorea/n_exceptions.h>
#include <nilorea/n_games.h>
#include <nilorea/n_hash.h>
#include <nilorea/n_iso_engine.h>
#include <nilorea/n_list.h>
#include <nilorea/n_log.h>
#include <nilorea/n_network.h>
#include <nilorea/n_network_msg.h>
#include <nilorea/n_nodup_log.h>
#include <nilorea/n_pcre.h>
#include <nilorea/n_signals.h>
#include <nilorea/n_stack.h>
#include <nilorea/n_str.h>
#include <nilorea/n_thread_pool.h>
#include <nilorea/n_time.h>
#include <nilorea/n_user.h>
#include <nilorea/n_zlib.h>

/* wip_unstable #include <nilorea/n_neural_networks.h> */
/* wip_unstable #include <nilorea/n_file.h> */

/* Must revamp this
#include <nilorea/n_debug_mem.h>
#include <nilorea/n_debug_mem_h.h>
*/

#ifdef HAVE_ALLEGRO
#include <nilorea/n_gfx.h>
#include <nilorea/n_anim.h>
#include <nilorea/n_gui.h>
#include <nilorea/n_particles.h>
/* wip_unstable #include <nilorea/n_resources.h> */
#endif



/**
@}
*/

#ifdef __cplusplus
}
#endif

#endif /*#ifndef __NILOREA_Unified_Library__*/
