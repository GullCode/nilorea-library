/**\file n_log.c
 * generic logging system
 *\author Castagnier Mickael
 *\date 2013-03-12
 */

#include "nilorea/n_common.h"
#include "nilorea/n_log.h"

#include <pthread.h>
#include <string.h>
#include <inttypes.h>

#ifndef __windows__
#include <syslog.h>
#else
#include <time.h>
#endif

/*! internal struct to handle log types */
typedef struct LOG_LEVELS
{
    /*! string of log type */
    char *c_name;
    /*! numeric value of log type */
    int	c_val;
    /*! event log value */
    char *w_name;

} LOG_LEVELS;

/*! array of log levels */
static LOG_LEVELS prioritynames[] =
{
    {	"EMERG",	LOG_EMERG,		"ERROR" },
    {	"ALERT",	LOG_ALERT,		"ERROR" },
    {	"CRITICAL",	LOG_CRIT,		"ERROR" },
    {	"ERR",	    LOG_ERR,		"ERROR" },
    {	"WARNING",	LOG_WARNING,	"WARNING" },
    {	"NOTICE",	LOG_NOTICE,		"SUCCESS" },
    {	"INFO",		LOG_INFO,		"INFORMATION" },
    {	"DEBUG",	LOG_DEBUG,		"INFORMATION" },
    {	NULL,		-1,			 NULL }
};

/*! static global maximum wanted log level value */
static int LOG_LEVEL = LOG_NULL ;

/*! static global logging type ( STDERR, FILE, SYSJRNL ) */
static int LOG_TYPE = LOG_STDERR ;

/*! static FILE handling if logging to file is enabled */
static FILE *log_file = NULL ;

/*! static proc name, for windows event log */
char *proc_name = NULL ;


/*!\fn open_sysjrnl( char *identity )
 *\brief Open connection to syslog or create internals for event log
 *\param identity Tag for syslog or NULL to use argv[0]
 *\return NULL or identity if success
 */
char *open_sysjrnl( char *identity )
{
    __n_assert( identity, return NULL );
#ifndef __windows__
    /* LOG_CONS: log to console if no syslog available
     * LOG_PID: add pid of calling process to log
     * Local use 7 : compat with older logging systems
     */
    openlog ( identity, LOG_CONS|LOG_PID|LOG_NDELAY, LOG_LOCAL7 );
#endif
    proc_name = strdup( identity );
    return proc_name ;
} /* open_sysjrnl */



/*!\fn void close_sysjrnl( void )
 * \brief Close syslog connection or clean internals for event log
 */
void close_sysjrnl( void )
{
#ifndef __windows__
    closelog();
#endif
    FreeNoLog( proc_name );
} /* close_sysjrnl */



/*!\fn void set_log_level( int log_level )
 *\brief Set the global log level value ( static int LOG_LEVEL )
 *\param log_level Log level value. Supported: JRNLONLY,NOLOG,LOG_NOTICE/INFO/ERR/DEBUG,LOG_FILE/STDERR/SYSJRNL
 */
void set_log_level( const int log_level )
{
    if( log_level == LOG_FILE )
    {
        LOG_TYPE = LOG_FILE ;
        return ;
    }
    if( log_level == LOG_STDERR )
    {
        LOG_TYPE = LOG_STDERR ;
        return ;
    }
    if( log_level == LOG_SYSJRNL )
    {
        LOG_TYPE = LOG_SYSJRNL ;
        return ;
    }
    LOG_LEVEL = log_level ;
} /* set_log_level() */



/*!\fn int get_log_level( )
 *\brief Get the global log level value
 *\return static int LOG_LEVEL
 */
int get_log_level( void )
{
    return LOG_LEVEL ;
} /* get_log_level() */



/*!\fn int set_log_file( char *file )
 *\brief Set the logging to a file instead of stderr
 *\param file The filename where to log
 *\return TRUE or FALSE
 */
int set_log_file( char *file )
{
    __n_assert( file, return FALSE );

    if( !log_file )
        log_file = fopen( file, "w" );
    else
    {
        fclose( log_file );
        log_file = fopen( file, "w" );
    }

    set_log_level( LOG_FILE );

    __n_assert( log_file, return FALSE );

    return TRUE ;
} /* set_log_file */



/*!\fn FILE *get_log_file()
 *\brief return the current log_file
 *\return a valid FILE handle or NULL
 */
FILE *get_log_file( void )
{
    return log_file ;
} /*get_log_level() */

#ifndef _GNU_SOURCE
#ifndef _vscprintf
/* For some reason, MSVC fails to honour this #ifndef. */
/* Hence function renamed to _vscprintf_so(). */
int _vscprintf_so(const char * format, va_list pargs)
{
    int retval;
    va_list argcopy;
    va_copy(argcopy, pargs);
    retval = vsnprintf(NULL, 0, format, argcopy);
    va_end(argcopy);
    return retval;
}
#endif

#ifndef vasprintf
int vasprintf(char **strp, const char *fmt, va_list ap)
{
    long long int len = _vscprintf_so(fmt, ap);
    if (len == -1)
        return -1;
    char *str = NULL ;
    Malloc( str, char, (size_t) len + 1 );
    if (!str)
        return -1;
    int r = vsnprintf(str, (size_t)(len+1), fmt, ap);  /* "secure" version of vsprintf */
    if (r == -1)
        return free(str), -1;
    *strp = str;
    return r;
}
#endif

#ifndef asprintf
int asprintf(char *strp[], const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int r = vasprintf(strp, fmt, ap);
    va_end(ap);
    return r;
}
#endif
#endif // _GNU_SOURCE

/*!\fn void _n_log( int level , const char *file , const char *func , int line , const char *format , ... )
 *\brief Logging function. log( level , const char *format , ... ) is a macro around _log
 *\param level Logging level
 *\param file File containing the emmited log
 *\param func Function emmiting the log
 *\param line Line of the log
 *\param format Format and string of the log, printf style
 */
void _n_log( int level, const char *file, const char *func, int line, const char *format, ... )
{
    FILE *out = NULL ;

    if( level == LOG_NULL )
        return ;

    if( !log_file )
        out = stderr ;
    else
        out = log_file ;

    int log_level = get_log_level();

    if( level <= log_level )
    {
        va_list args ;
        char *syslogbuffer = NULL ;
        char *eventbuffer = NULL ;
#ifdef __windows__
        size_t needed = 0 ;
        char *name = "NULL" ;
        if( proc_name )
            name = proc_name ;
#endif

        switch( LOG_TYPE )
        {
        case LOG_SYSJRNL :
            va_start (args, format);
            vasprintf( &syslogbuffer, format, args );
            va_end( args );
#ifdef __windows__
            needed = (unsigned long long)snprintf( NULL, 0, "start /B EventCreate /t %s /id 666 /l APPLICATION /so %s /d \"%s\" > NUL 2>&1", prioritynames[ level ] . w_name, name, syslogbuffer );
            Malloc( eventbuffer, char, needed + 4 );
            sprintf( eventbuffer, "start /B EventCreate /t %s /id 666 /l APPLICATION /so %s /d \"%s\" > NUL 2>&1", prioritynames[ level ] . w_name, name, syslogbuffer );
            system( eventbuffer );
#else
            syslog( level, "%s->%s:%d %s", file, func, line, syslogbuffer );
#endif
            FreeNoLog( syslogbuffer );
            FreeNoLog( eventbuffer );
            break ;
        default:
            fprintf( out, "%s:%jd:%s->%s:%d ", prioritynames[ level ] . c_name, (intmax_t)time( NULL ), file, func, line  );
            va_start (args, format);
            vfprintf( out, format, args );
            va_end( args );
            fprintf( out, "\n" );
            break ;
        }
        fflush( out );
    }
}  /* _n_log( ... ) */



/*!\fn open_safe_logging( TS_LOG **log , char *pathname , char *opt )
 *\brief Open a thread-safe logging file
 *\param log A TS_LOG handler
 *\param pathname The file path (if any) and name
 *\param opt Options for opening (please never forget to use "w")
 *\return TRUE on success , FALSE on error , -1000 if already open
 */
int open_safe_logging( TS_LOG **log, char *pathname, char *opt )
{

    if( (*log) )
    {

        if( (*log) -> file )
            return -1000; /* already open */

        return FALSE;
    }

    if( !pathname )
        return FALSE; /* no path/filename */

    Malloc( (*log), TS_LOG, 1 );

    if( !(*log) )
        return FALSE;

    pthread_mutex_init( &(*log) -> LOG_MUTEX, NULL );

    (*log) -> file = fopen( pathname, opt );

    if( !(*log) -> file )
        return FALSE;

    return TRUE;

}/* open_safe_logging(...) */



/*!\fn write_safe_log( TS_LOG *log , char *pat , ... )
 *\brief write to a thread-safe logging file
 *\param log A TS_LOG handler
 *\param pat Pattern for writting (i.e "%d %d %s")
 *\return TRUE on success, FALSE on error
 */
int write_safe_log( TS_LOG *log, char *pat, ... )
{
    /* argument list */
    va_list arg;
    char str[2048] = "" ;

    if( !log )
        return FALSE;

    va_start( arg, pat );

    vsnprintf( str, sizeof(str), pat, arg );

    va_end( arg );

    pthread_mutex_lock( &log -> LOG_MUTEX );
    fprintf( log -> file, "%s", str );
    fflush( log -> file );
    pthread_mutex_unlock( &log -> LOG_MUTEX );

    return TRUE;

} /* write_safe_log( ... ) */



/*!\fn close_safe_logging( TS_LOG *log )
 *\brief close a thread-safe logging file
 *\param log A TS_LOG handler
 *\return TRUE on success, FALSE on error
 */
int close_safe_logging( TS_LOG *log )
{
    if( !log )
        return FALSE;

    pthread_mutex_lock( &log -> LOG_MUTEX );
    fflush( log -> file );
    pthread_mutex_unlock( &log -> LOG_MUTEX );

    pthread_mutex_destroy( &log -> LOG_MUTEX );

    fclose( log -> file );

    return TRUE;

}/* close_safe_logging( ...) */
