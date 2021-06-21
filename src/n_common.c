/**\file n_common.c
 *  common function
 *\author Castagnier Mickael
 *\version 1.0
 *\date 24/03/05
 */

#include "nilorea/n_common.h"
#include "nilorea/n_log.h"
#include "nilorea/n_str.h"


#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>

#ifdef __windows__
#include <windows.h>
#else
#include <errno.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#ifdef __linux__
#include <string.h>
#include <linux/limits.h>
#include <unistd.h>
#endif
#endif



/*! void n_abort( char const *format , ... )
 *\brief abort program with a text
 *\param format printf style format and args
 */
void n_abort( char const *format, ... )
{
    char str[1024] = "" ;
    va_list args;
    va_start(args, format);

    vsnprintf(str, sizeof str, format, args);
    va_end(args);
    fprintf(stderr, "%s", str);
    exit(1);
}



/*! int file_exist( const char *filename )
 *\brief test if file exist and if it's readable
 *\param filename Path/name of the file
 *\return TRUE or FALSE
 */
int file_exist( const char *filename )
{
    FILE *file = NULL ;
    if( ( file = fopen( filename, "r" ) ) != NULL )
    {
        fclose(file);
        return 1 ;
    }
    return 0;
} /* file_exist */



/*!\fn char *get_prog_dir( void )
 *\brief get current program running directory
 *\return A copy of the current program running directory inside a string
 */
char *get_prog_dir( void )
{
    char strbuf[ PATH_MAX ] = "" ;

    int error = 0 ;
#ifdef __windows__
    unsigned long int bytes = GetModuleFileName( NULL, strbuf, PATH_MAX );
    error = errno ;
    if(bytes != 0)
    {
        return strdup( dirname(strbuf ) );
    }
#else
    char procbuf[ PATH_MAX ] = "" ;
#ifdef __linux__
    sprintf( procbuf, "/proc/%d/exe", (int)getpid() );
#elif defined __sun
    sprintf( procbuf, "/proc/%d/path/a.out", (int)getpid() );
#endif
    int bytes = MIN( readlink( procbuf, strbuf, PATH_MAX ), PATH_MAX - 1 );
    error = errno ;
    if( bytes >= 0 )
    {
        strbuf[ bytes ] = '\0';
        return strdup( dirname( strbuf ) );
    }
#endif
    fprintf( stderr, "%s", strerror( error ) );
    return NULL ;
} /* get_prog_dir */


/*!\fn char *get_prog_name( void )
 *\brief get current program name
 *\return A copy of the current program name inside a string
 */
char *get_prog_name( void )
{
    char strbuf[ PATH_MAX ] = "" ;
    int error = 0 ;
#ifdef __windows__
    unsigned long int bytes = GetModuleFileName( NULL, strbuf, PATH_MAX );
    error = errno ;
    if(bytes != 0)
    {
        return strdup( basename(strbuf ) );
    }
#else
    char procbuf[ PATH_MAX ] = "" ;
#ifdef __linux__
    sprintf( procbuf, "/proc/%d/exe", (int)getpid() );
#elif defined __sun
    sprintf( procbuf, "/proc/%d/path/a.out", (int)getpid() );
#endif
    int bytes = MIN( readlink( procbuf, strbuf, PATH_MAX ), PATH_MAX - 1 );
    error = errno ;
    if( bytes >= 0 )
    {
        strbuf[ bytes ] = '\0';
        return strdup( basename( strbuf ) );
    }
#endif
    fprintf( stderr, "%s", strerror( error ) );
    return NULL ;
} /* get_prog_dir */



/*!\fn int n_popen( char *cmd , int read_buf_size , void *nstr_output , int *ret )
 * \brief launch a command abd return output and status
 * \param cmd The command to launch
 * \param read_buf_size popen read buf. Also serves as a minimum size for the dynamically allocated returned output if not already allocaded
 * \param output Pointer to a valid N_STR or NULL (see read_buf_size)
 * \param ret Command output if any. Should be initialized to -1 and tested against to check if it's valid.
 * \return TRUE or FALSE
 */
int n_popen( char *cmd, int read_buf_size, void **nstr_output, int *ret )
{
    __n_assert( cmd, return FALSE );

    N_STR *output_pointer = new_nstr( read_buf_size + 1 );

    FILE *fp = NULL ;
    int status = 0 ;
    char read_buf[ read_buf_size ] ;

    fp = popen( cmd, "w" );
    int err = errno ;
    if (fp == NULL)
    {
        n_log( LOG_ERR, "popen( %s ) returned NULL , %s (%d)", cmd, strerror( err ), err );
        return FALSE ;
    }

    int length  = 0 ;
    while( fgets( read_buf, read_buf_size, fp ) != NULL )
    {
        length = strlen( read_buf );
        if( read_buf[ length - 1 ] == '\n' )
        {
            read_buf[ length - 1 ] = '\0' ;
        }
        nstrprintf_cat( output_pointer, "%s", read_buf );
    }
    status = pclose( fp );
    err = errno ;
    if( status == -1 )
    {
        n_log( LOG_ERR, "pclose( %s ) returned -1 , %s (%d)", cmd, strerror( err ), err );
        if( WIFEXITED( status ) )
        {
            n_log( LOG_ERR, "Child exited with RC=%d", WEXITSTATUS( status ) );
            (*ret) =  WEXITSTATUS( status );
        }
        return FALSE ;
    }
    (*ret) =  WEXITSTATUS( status );
    (*nstr_output) = output_pointer ;
    return TRUE ;
} /* n_popen( ... ) */


#ifndef __windows__
/*!\fn n_daemonize( void )
 *\brief Daemonize program
 *\return TRUE or FALSE
 */
int n_daemonize( void )
{
    /* Fork off the parent process */
    pid_t pid = fork();
    if( pid < 0 )
    {
        n_log( LOG_ERR, "Error: unable to fork child: %s", strerror( errno ) );
        exit( 1 );
    }
    if( pid > 0 )
    {
        n_log( LOG_NOTICE, "Child started successfuly !" );
        exit( 0 );
    }
    /* Close all open file descriptors */
    int x = 0 ;
    for(x = sysconf( _SC_OPEN_MAX ) ; x >= 0 ; x-- )
    {
        close(x);
    }
    int fd = open("/dev/null",O_RDWR, 0);
    if (fd != -1)
    {
        dup2 (fd, STDIN_FILENO);
        dup2 (fd, STDOUT_FILENO);
        dup2 (fd, STDERR_FILENO);
        if (fd > 2)
        {
            close (fd);
        }
    }
    else
    {
        n_log( LOG_ERR, "Failed to open /dev/null" );
    }

    /* On success: The child process becomes session leader */
    if( setsid() < 0 )
    {
        n_log( LOG_ERR, "Error: unable to set session leader with setsid(): %s", strerror( errno ) );
        exit( 1 );
    }
    else
    {
        n_log( LOG_NOTICE, "Session leader set !" );
    }

    return TRUE ;
} /* n_daemonize(...) */



/*!\fn pid_t system_nb(const char * command, int * infp, int * outfp)
 *\brief Non blocking system call
 *\param command to call
 *\param infp stdin file descriptor holder or NULL
 *\param outfp stdout file descriptor holder or NULL
 *\return The system command pid or -1
 */
pid_t system_nb(const char * command, int * infp, int * outfp)
{
    __n_assert( command, return -1 );

    int p_stdin[2] = { -1, -1 };
    int p_stdout[2] = { -1, -1 };
    pid_t pid = -1 ;

    if( pipe( p_stdin ) == -1 )
    {
        n_log( LOG_ERR, "Couldn't pipe stdin for command %s", command );
        return -1;
    }
    if (pipe(p_stdout) == -1)
    {
        close(p_stdin[0]);
        close(p_stdin[1]);
        n_log( LOG_ERR, "Couldn't pipe stdout for command %s", command );
        return -1;
    }

    pid = fork();
    if (pid < 0)
    {
        close(p_stdin[0]);
        close(p_stdin[1]);
        close(p_stdout[0]);
        close(p_stdout[1]);
        n_log( LOG_ERR, "Couldn't fork command %s", command );
        return pid;
    }
    else if (pid == 0)
    {
        close(p_stdin[1]);
        dup2(p_stdin[0], 0);
        close(p_stdout[0]);
        dup2(p_stdout[1], 1);
        dup2( open("/dev/null", O_RDONLY), 2);

        /// Close all other descriptors for the safety sake.
        for (int i = 3; i < 4096; ++i)
        {
            close(i);
        }
        setsid();
        execl("/bin/sh", "sh", "-c", command, NULL);
        _exit(1);
    }

    close( p_stdin[0] );
    close( p_stdout[1] );
    if( infp == NULL)
    {
        close(p_stdin[1]);
    }
    else
    {
        *infp = p_stdin[1];
    }
    if (outfp == NULL)
    {
        close(p_stdout[0]);
    }
    else
    {
        *outfp = p_stdout[0];
    }

    return pid;
} /* system_nb */

#endif /* ifndef __windows__ */
