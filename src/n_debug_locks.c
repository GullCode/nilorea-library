/*!\file n_debug_locks.c
 * Helpers for debugging thread locks, on an idea of  Aurelian Melinte ( see https://linuxgazette.net/150/melinte.html )
 *\author Castagnier Mickael
 *\date 2014-10-10
 */

#include "nilorea/n_debug_mem.h"
#include "nilorea/n_common.h"
#include "nilorea/n_log.h"
#include "nilorea/n_hash.h"

#define N_DEBUGLOCK_INVALID_IDX     (-1)
#define N_DEBUGLOCK_INVALID_THREAD  ((pthread_t)0)
#define N_DEBUGLOCK_INVALID_MUTEX   ((pthread_mutex_t*)0)

/*!\internal :flag for enabling or disabling debugging on locks */
static int DEBUG_LOCKS = 0 ;
/*!\internal :flag for to check initialisation status */
static int DEBUG_LOCKS_INITIALIZED = 0 ;
/*!\internal : mutex to lock debuglock infos */
pthread_mutex_t debuglock_lock ;



/*! Debuglock thread informations */
typedef struct N_PTHREAD_INFO
{
    /*! thread id */
    pthread_t thread;
    /*! owned mutexes */
    LIST *mutexes ;
    /*! wanted mutex if any */
    const pthread_mutex_t *mutex_to_lock;
} N_PTHREAD_INFO ;



/*! Debuglock mutexes informations */
typedef struct N_MUTEX_INFO
{
    /*! pointer to mutex */
    const pthread_mutex_t *mutex;
    /*! owning thread */
    pthread_t   owning_thread ;
    /*! pointer to thread info */
    LIST_NODE   *owning_thread_list_node ;
} N_MUTEX_INFO ;




LIST *n_thread_list = NULL ;
LIST *n_mutex_list = NULL ;

int n_pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr)
{

    pthread_mutex_init( mutex, mutexattr );
}

int n_pthread_mutex_lock(pthread_mutex_t *mutex)
{
    pthread_mutex_lock( mutex );

}
int n_pthread_mutex_trylock(pthread_mutex_t *mutex)
{
    pthread_mutex_trylock( mutex );
}
int n_pthread_mutex_unlock(pthread_mutex_t *mutex);
{
    pthread_mutex_unlock( mutex );
}

int n_pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);
{
    pthread_rwlock_rdlock( rwlock );
}

int n_pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock)
{
    pthread_rwlock_tryrdlock( rwlock );
}

int n_pthread_rwlock_wrlock(pthread_rwlock_t *wrlock )
{
    pthread_rwlock_wrlock( wrlock );
}

int pthread_rwlock_trywrlock(pthread_rwlock_t *wrlock)
{
    pthread_rwlock_trywrlock( wrlock );
}

int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
    pthread_mutex_destroy( mutex );
}











void lp_lock_precheck_diagnose(const pthread_mutex_t *mutex)
{
    int rc = -1;
    /*Highest ranking mutex currently acquired by 'me'*/
    int maxmidx = LP_INVALID_IDX;
    int midx = LP_INVALID_IDX;
    pthread_t me = pthread_self();

    pthread_t             thr = LP_INVALID_THREAD;
    const pthread_mutex_t *mtx = NULL;

    /* Thread tries to lock a mutex it has already locked? */
    if ((rc=lp_is_mutex_locked(mutex, me)) != 0)
    {
        lp_report("Mutex M%lx is already locked by thread.\n", mutex);
        lp_dump();
    }

    /* Is mutex order respected? */
    maxmidx = lp_max_idx_mutex_locked(me);
    midx = lp_mutex_idx(mutex);
    if (midx < maxmidx)
    {
        lp_report("Mutex M%lx will be locked out of order (idx=%d, crt max=%d).\n",
                  mutex, midx, maxmidx);
        lp_dump();
    }

    /* Will deadlock? Check for a circular chain. */
    lp_report("Checking if it will deadlock...\n");
    thr = me;
    mtx = mutex;
    while ((thr=lp_thread_owning(mtx)) != LP_INVALID_THREAD)
    {
        lp_report("  Mutex M%lx is owned by T%lx.\n", mtx, thr);
        mtx = lp_mutex_wanted(thr);
        lp_report("  Thread T%lx waits for M%lx.\n", thr, mtx);

        if (mtx == LP_INVALID_MUTEX)
            break; /*no circular dead; */

        if (0 != pthread_equal(thr, me))
        {
            lp_report("  Deadlock condition detected.\n");
            lp_dump();
            break;
        }
    }
}

void lp_unlock_precheck_diagnose(const pthread_mutex_t *mutex)
{
    int rc = -1;
    int maxmidx = LP_INVALID_IDX, midx = LP_INVALID_IDX;

    /* Thread tries to unlock a mutex it does not have? */
    if ((rc=lp_is_mutex_locked(mutex, pthread_self())) == 0)
    {
        lp_report("Attempt to release M%lx NOT locked by thread.\n", mutex);
        lp_dump();
    }

    /* Are mutexes released in reverse order? */
    maxmidx = lp_max_idx_mutex_locked(pthread_self());
    midx = lp_mutex_idx(mutex);
    if (midx < maxmidx)
    {
        lp_report("Mutex M%lx will be released out of order (idx=%d, crt max=%d).\n",
                  mutex, midx, maxmidx);
        lp_dump();
    }
}



int pthread_mutex_lock(pthread_mutex_t *mutex)
{
    int rc = EINVAL;

    if (mutex != NULL)
    {
        lp_lock_precheck(mutex);

        /* Call the real pthread_mutex_lock() */
        rc = next_pthread_mutex_lock(mutex);

        lp_lock_postcheck(mutex, rc);
    }
    else
    {
        lp_report("%s(): mutex* is NULL.\n", __FUNCTION__ );
    }

    return rc;
}

int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
    int rc = EINVAL;

    if (mutex != NULL)
    {
        lp_lock_precheck(mutex);

        /* Call the real pthread_mutex_lock() */
        rc = next_pthread_mutex_trylock(mutex);

        lp_lock_postcheck(mutex, rc);
    }
    else
    {
        lp_report("%s(): mutex* is NULL.\n", __FUNCTION__ );
    }

    return rc;
}

int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
    int rc = EINVAL;

    if (mutex != NULL)
    {
        lp_lock_precheck(mutex);

        /* Call the real pthread_mutex_lock() */
        rc = next_pthread_mutex_trylock(mutex);

        lp_lock_postcheck(mutex, rc);
    }
    else
    {
        lp_report("%s(): mutex* is NULL.\n", __FUNCTION__ );
    }

    return rc;
}


