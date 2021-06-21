/**\file n_config_file.c
 *  common function
 *\author Castagnier Mickael
 *\version 1.0
 *\date 24/03/05
 */
#include <stdio.h>
#include <errno.h>

#include "nilorea/n_common.h"
#include "nilorea/n_config_file.h"
#include "nilorea/n_pcre.h"


/*!\fn void free_no_null( void *ptr )
 *\brief local free for config entries
 *\param ptr the ptr to free
 */
void free_no_null( void *ptr )
{
    char *strptr = ptr ;
    if( strptr )
    {
        free( strptr );
    }
    return ;
}

/*!\fn void destroy_config_file_section( void *ptr )
 *\brief Destroy a config file section
 *\param ptr A pointer to a config file section
 */
void destroy_config_file_section( void *ptr )
{
    CONFIG_FILE_SECTION *section = (CONFIG_FILE_SECTION *)ptr;
    if( !section )
        return ;
    Free( section -> section_name );
    destroy_ht( &section -> entries );
    Free( section );
} /* destroy_config_file_section(...) */



/*!\fn CONFIG_FILE *load_config_file( char *filename , int *errors )
 *\brief load a config file
 *\param filename Filename of the config file
 *\param errors pointer to an int which will represent the number of errors encountered when reading config file
 *\return NULL or a loaded config file
 */
CONFIG_FILE *load_config_file( char *filename, int *errors )
{
    CONFIG_FILE *cfg_file = NULL ;
    CONFIG_FILE_SECTION *section = NULL ;
    char buffer[ MAX_CONFIG_LINE_LEN ] = "" ;
    char **split_result = NULL ;
    N_PCRE *npcre = npcre_new( "(.*?=)(.*)", 99, 0 );
    int error = 0 ;
    FILE *in = NULL ;

    __n_assert( filename, return NULL );
    in = fopen( filename, "r" );
    error = errno ;
    if( !in )
    {
        n_log( LOG_ERR, "Unable to open %s: %s", _str( filename ), strerror( error ) );
        (*errors)++;
        npcre_delete( &npcre );
        return NULL ;
    }
    Malloc( cfg_file, CONFIG_FILE, 1 );
    if( !cfg_file )
    {
        fclose( in );
        npcre_delete( &npcre );
        return NULL ;
    }
    cfg_file -> sections = new_generic_list( 0 );
    if( !cfg_file -> sections )
    {
        Free( cfg_file );
        fclose( in );
        npcre_delete( &npcre );
        return NULL ;
    }
    cfg_file -> filename = strdup( filename );
    if( !cfg_file -> filename )
    {
        n_log( LOG_ERR, "couldn't strdup( %s )", _str( filename ) );
        (*errors)++;
        Free( cfg_file );
        fclose( in );
        npcre_delete( &npcre );
        return NULL ;
    }
    /* adding default section */
    CONFIG_FILE_SECTION *default_section = NULL ;
    Malloc( default_section, CONFIG_FILE_SECTION, 1 );
    if( !default_section )
    {
        n_log( LOG_ERR, "couldn't allocate default_section" );
        (*errors)++;
        Free( cfg_file );
        fclose( in );
        npcre_delete( &npcre );
        return NULL ;
    }

    default_section -> section_name = strdup( "__DEFAULT__" );
    if( !default_section -> section_name )
    {
        n_log( LOG_ERR, "couldn't allocate default_section name" );
        (*errors)++;
        Free( cfg_file );
        Free( default_section );
        fclose( in );
        npcre_delete( &npcre );
        return NULL ;
    }

    default_section -> entries = new_ht( CONFIG_SECTION_HASH_TABLE_LEN );
    if( !default_section -> entries )
    {
        n_log( LOG_ERR, "error creating hash table of size %d for default_section", CONFIG_SECTION_HASH_TABLE_LEN );
        (*errors)++;
        Free( default_section -> section_name );
        Free( default_section );
        Free( cfg_file );
        fclose( in );
        npcre_delete( &npcre );
        return NULL ;
    }
    list_push( cfg_file -> sections, default_section, &destroy_config_file_section );

    int line_number = 0 ;
    while( fgets( buffer, MAX_CONFIG_LINE_LEN, in ) )
    {
        NSTRBYTE start = 0, end = 0 ;

        char *bufptr = (char *)&buffer ;
        line_number ++ ;

        bufptr = trim_nocopy( buffer );
        if( strlen( bufptr ) == 0 )
        {
            continue ;
        }

        switch( bufptr[ 0 ] )
        {
        /* comment */
        case '#' :
        case ';' :
            continue ;
        /* new section */
        case '[' :
            end = 0 ;
            /* go to begin of section name */
            if( skipu( bufptr, ']', &end, 1 ) != TRUE )
            {
                n_log( LOG_ERR, "coulnd't find end of section at line %d of %s (string:%s)", line_number, filename, buffer );
                (*errors)++;
                continue ;
            }
            /* keep only the name */
            bufptr ++ ;
            bufptr[ end - 1 ] = '\0' ;
            bufptr = trim_nocopy( bufptr );
            if( strlen( bufptr ) == 0 )
            {
                n_log( LOG_ERR, "section without name at line %d", line_number );
                (*errors)++;
                continue ;
            }

            Malloc( section, CONFIG_FILE_SECTION, 1 );
            __n_assert( section, break );

            section -> section_name = strdup( bufptr );
            if( !section -> section_name )
            {
                n_log( LOG_ERR, "couldn't duplicate %s", bufptr );
                (*errors)++;
                continue ;
            }

            section -> entries = new_ht( CONFIG_SECTION_HASH_TABLE_LEN );
            if( !section -> entries )
            {
                n_log( LOG_ERR, "error creating [%s] hash table of size %d", bufptr, CONFIG_SECTION_HASH_TABLE_LEN );
                (*errors)++;
                continue ;
            }
            list_push( cfg_file -> sections, section, &destroy_config_file_section );
            continue ;

        /* if it's not a comment, not a section, not empty, then it's an entry ! */
        default:
            if( !section )
            {
                section = default_section ;
                n_log( LOG_DEBUG, "No section, setting __DEFAULT__ starting line %d of %s (string:%s)", line_number, filename, bufptr );
            }

            if( npcre_match( bufptr, npcre ) == TRUE )
            {
                Malloc( split_result, char *, 3 );

                split_result[ 2 ] = NULL ;
                split_result[ 0 ] = str_replace( npcre -> match_list[ 1 ], "=", " " );
                split_result[ 1 ] = strdup( npcre -> match_list[ 2 ] );
            }
            else
            {
                split_result = NULL ;
            }

            if( !split_result )
            {
                n_log( LOG_ERR, "couldn't find entry separator '=' at line %d of %s (string:%s)", line_number, filename, bufptr );
                (*errors)++;
                continue ;
            }
            if( split_count( split_result ) < 2 )
            {
                n_log( LOG_ERR, "Invalid split count at line %d of %s (string:%s, count:%d)", line_number, filename, bufptr, split_count( split_result ) );
                (*errors)++;
                free_split_result( &split_result );
                continue ;
            }
            if( strlen( trim_nocopy( split_result[ 0 ] ) ) == 0 )
            {
                free_split_result( &split_result );
                n_log( LOG_ERR, "couldn't find entry name at line %d of %s (string:%s)", line_number, filename, buffer[ start ] );
                (*errors)++;
                continue ;
            }

            /* value pointer */
            char *valptr = NULL ;
            /* flags */
            int closing_delimiter_found = 0 ;
            int opening_delimiter_found = 0 ;
            /* initialize with empty delimiter */
            char delimiter = '\0' ;

            /* If the trimmed value have a zero length it means we only had spaces or tabs
               In that particular case we set the val to NULL */
            if( strlen( trim_nocopy( split_result[ 1 ] ) ) == 0 )
            {
                Free( split_result[ 1 ] );
                valptr = NULL ;
                delimiter = '\0' ;
            }
            else  /* we have a value, or something between delimiters */
            {
                valptr = trim_nocopy( split_result[ 1 ] );
                int it = strlen( valptr );

                /* get value right boundary */
                while( it >= 0 )
                {
                    /* if comments tags replace them with end of string */
                    if( split_result[ 1 ][ it ] == ';' || split_result[ 1 ][ it ] == '#' )
                    {
                        split_result[ 1 ][ it ] = '\0' ;
                    }
                    else
                    {
                        /* we found a delimiter on the right */
                        if( split_result[ 1 ][ it ] == '"'  || split_result[ 1 ][ it ] == '\'' )
                        {
                            delimiter = split_result[ 1 ][ it ] ;
                            split_result[ 1 ][ it ] = '\0' ;
                            closing_delimiter_found = 1 ;
                            break ;
                        }
                    }
                    it-- ;
                }

                if( strlen( trim_nocopy( split_result[ 1 ] ) ) == 0 )
                {
                    Free( split_result[ 1 ] );
                    valptr = NULL ;
                    delimiter = '\0' ;
                }

                /* get value left boundary */
                it = 0 ;
                while( split_result[ 1 ][ it ] != '\0' )
                {
                    if( split_result[ 1 ][ it ] == ';' || split_result[ 1 ][ it ] == '#' )
                    {
                        split_result[ 1 ][ it ] = '\0' ;
                    }
                    else
                    {
                        if( delimiter != '\0' && split_result[ 1 ][ it ] == delimiter )
                        {
                            opening_delimiter_found = 1 ;
                            it ++ ;
                            valptr = split_result[ 1 ]+ it ;
                            break ;
                        }
                        it ++ ;
                    }
                }
                if( strlen( trim_nocopy( split_result[ 1 ] ) ) == 0 )
                {
                    Free( split_result[ 1 ] );
                    valptr = NULL ;
                    delimiter = '\0' ;
                }
            }
            if( delimiter != '\0' && ( ( opening_delimiter_found && !closing_delimiter_found ) || ( !opening_delimiter_found && closing_delimiter_found ) ) )
            {
                free_split_result( &split_result );
                n_log( LOG_ERR, "Only one delimiter found at line %d of %s (string:%s)", line_number, filename, buffer );
                (*errors)++;
                continue ;
            }

            char *result = NULL ;
            N_STR *entry_key = NULL ;
            int nb_entry_in_sections = 0 ;
            while( TRUE )
            {
                nstrprintf( entry_key, "%s%d", trim_nocopy( split_result[ 0 ] ), nb_entry_in_sections );
                if( ht_get_ptr( section -> entries, _nstr( entry_key ), (void **)&result )  == FALSE )
                {
                    break ;
                }
                nb_entry_in_sections ++ ;
                free_nstr( &entry_key );
            }
            char *strptr = NULL ;
            if( valptr )
            {
                strptr = strdup( valptr );
            }
            ht_put_ptr( section -> entries, _nstr( entry_key ), strptr, &free_no_null );
            free_nstr( &entry_key );
            free_split_result( &split_result );
            continue ;
        }
    }
    npcre_delete( &npcre );
    if( !feof( in ) )
    {
        n_log( LOG_ERR, "couldn't read EOF for %s", filename );
        (*errors)++;
    }
    fclose( in );

    return cfg_file ;

} /*load_config_file */



/*!\fn int get_nb_config_file_sections( CONFIG_FILE *cfg_file , char *section_name )
 *\brief Get the number of config file with section_name
 *\param cfg_file Config file to process
 *\param section_name name of sections to search , or NULL to have a count of all available sections
 *\return The number of named sections or a negative value
 */
int get_nb_config_file_sections( CONFIG_FILE *cfg_file, char *section_name )
{
    __n_assert( cfg_file, return -1 );
    __n_assert( cfg_file -> sections, return -2 );

    int nb_sections = 0 ;
    list_foreach( listnode, cfg_file -> sections )
    {
        CONFIG_FILE_SECTION *section = (CONFIG_FILE_SECTION *)listnode -> ptr ;
        if( section_name )
        {
            if( !strcmp( section -> section_name, section_name ) )
            {
                nb_sections ++ ;
            }
        }
        else
        {
            nb_sections ++ ;
        }
    }
    return nb_sections ;
} /* get_nb_config_file_sections(...) */



/*!\fn int get_nb_config_file_sections_entries( CONFIG_FILE *cfg_file , char *section_name ,  int section_position , char *entry )
 *\brief Get the number of config file with section_name
 *\param cfg_file Config file to process
 *\param section_name name of sections to search
 *\param section_position position of the section if there are multiples same name sections
 *\param entry name of entry to retrieve
 *\return The number of named sections entries or a negative value
 */
int get_nb_config_file_sections_entries( CONFIG_FILE *cfg_file, char *section_name, int section_position, char *entry )
{
    __n_assert( cfg_file, return -1 );
    __n_assert( cfg_file -> sections, return -2 );
    __n_assert( section_name, return -3 );
    __n_assert( entry, return -4 );

    int nb_sections = 0 ;
    int nb_entry_in_sections = 0 ;
    list_foreach( listnode, cfg_file -> sections )
    {
        CONFIG_FILE_SECTION *section = (CONFIG_FILE_SECTION *)listnode -> ptr ;
        if( !strcmp( section -> section_name, section_name ) )
        {
            if( nb_sections == section_position )
            {
                char *result = NULL ;
                N_STR *entry_key = NULL ;
                while( TRUE )
                {
                    nstrprintf( entry_key, "%s%d", entry, nb_entry_in_sections );
                    if( ht_get_ptr( section -> entries, _nstr( entry_key ), (void **)&result )  == FALSE )
                    {
                        free_nstr( &entry_key );
                        break ;
                    }
                    nb_entry_in_sections ++ ;
                    free_nstr( &entry_key );
                }
                break ;
            }
            nb_sections ++ ;
        }
    }
    return nb_entry_in_sections ;
} /* get_nb_config_file_sections_entries(...) */



/*!\fn char *get_config_section_value( CONFIG_FILE *cfg_file , char *section_name , int section_position , char *entry , int entry_position )
 *\brief Function to parse sections and get entries values
 *\param cfg_file name of config file
 *\param section_name name of section
 *\param section_position section number
 *\param entry entry name
 *\param entry_position entry number
 *\return the value of the named entry in section[ position ]
 */
char *get_config_section_value( CONFIG_FILE *cfg_file, char *section_name, int section_position, char *entry, int entry_position )
{
    __n_assert( cfg_file, return NULL );
    __n_assert( cfg_file -> sections, return NULL );
    __n_assert( section_name, return NULL );
    __n_assert( entry, return NULL );

    if( section_position >= cfg_file -> sections -> nb_items )
    {
        n_log( LOG_DEBUG, "section_position (%d) is higher than the number of item in list (%d)", section_position,  cfg_file -> sections -> nb_items );
        return NULL ;
    }

    int section_position_it = 0 ;
    HASH_TABLE *entries = NULL ;
    list_foreach( listnode, cfg_file -> sections )
    {
        CONFIG_FILE_SECTION *section = (CONFIG_FILE_SECTION *)listnode -> ptr ;
        if( !strcmp( section -> section_name, section_name ) )
        {
            if( section_position_it == section_position )
            {
                entries = section -> entries ;
                break ;
            }
            section_position_it ++ ;
        }
    }

    char *result = NULL ;
    N_STR *entry_key = NULL ;
    nstrprintf( entry_key, "%s%d", entry, entry_position );
    if( ht_get_ptr( entries, _nstr( entry_key ), (void **)&result )  == FALSE )
    {
        /* n_log( LOG_DEBUG , "ht_get_ptr( %p , trim_nocopy( %s ) , (void **)&result ) returned FALSE !" , entries , _nstr( entry_key ) ); */
        free_nstr( &entry_key );
        return NULL ;
    }
    free_nstr( &entry_key );

    return result ;
} /* get_config_section_value */



/*!\fn int destroy_config_file( CONFIG_FILE **cfg_file )
 *\brief Destroy a loaded config file
 *\param cfg_file pointer to a loaded config file to destroy
 *\return TRUE or FALSE
 */
int destroy_config_file( CONFIG_FILE **cfg_file )
{
    __n_assert( cfg_file&&(*cfg_file), return FALSE );

    Free( (*cfg_file) -> filename );

    if( (*cfg_file) -> sections )
    {
        list_destroy( &(*cfg_file) -> sections );
    }

    Free( (*cfg_file) );

    return TRUE ;
} /* destroy_config_file */
