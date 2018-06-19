/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>

#include "logger.h"



static  int     Logger_Log (char *format, char *level, ...);
static  char    *getCurrentDateTime( void );
static  char    currentDateTimeBuffer[ 80 ];



static  FILE    *fp;
static  int     logFileOpen = FALSE;
static  int     debugValue;              // from the INI file

// ----------------------------------------------------------------------------
//
// How "debugValue" works.  It's an integer
// 0 = Log nothing         
// 5 = Log Fatal and Error and Warning and Debug and Info
// 4 = Log Fatal and Error and Warning and Debug
// 3 = Log Fatal and Error and Warning
// 2 = Log Fatal and Error
// 1 = Log Fatal

// ----------------------------------------------------------------------------
void    Logger_Initialize (char *fileName, int debugLevel)
{
    // logCat = log4c_category_get( "homeheartbeat" );
    // (void) log4c_init();
    printf( "ATTEMPTING TO OPEN [%s] DEBUG LEVEL [%d]\n", fileName, debugLevel ); fflush(stdout);
    if (fileName != (char *) 0 ) {
        debugValue = debugLevel;
        fp = fopen( fileName, "a" );
        if (fp != (FILE *) 0)
            logFileOpen = TRUE;
    }
}

// ----------------------------------------------------------------------------
void    Logger_Terminate()
{
    // (void) log4c_fini();
    if (logFileOpen)
        fclose( fp );
}

// ----------------------------------------------------------------------------
void    Logger_LogDebug (char *format, ... )
{
    if (!logFileOpen || debugValue < 4)
        return;
    
    va_list args;
    
    va_start( args, format );                   // the last fixed parameter
    int numWritten = fprintf( fp, "DEBUG|%s|", getCurrentDateTime() );
    numWritten += vfprintf( fp, format, args );
    va_end( args );
    
    fflush( fp );

}

// ----------------------------------------------------------------------------
void    Logger_LogWarning (char *format, ... )
{
    if (!logFileOpen || debugValue < 3)
        return;
    
    va_list args;
    
    va_start( args, format );                   // the last fixed parameter
    int numWritten = fprintf( fp, "WARNING|%s|", getCurrentDateTime() );
    numWritten += vfprintf( fp, format, args );
    va_end( args );
    
    fflush( fp );
}

// ----------------------------------------------------------------------------
void    Logger_LogError (char *format, ... )
{
    if (!logFileOpen || debugValue < 2)
        return;
    
    va_list args;
    
    va_start( args, format );                   // the last fixed parameter
    int numWritten = fprintf( fp, "ERROR|%s|", getCurrentDateTime() );
    numWritten += vfprintf( fp, format, args );
    va_end( args );
    
    fflush( fp );
}

// ----------------------------------------------------------------------------
void    Logger_LogFatal (char *format, ... )
{
    if (!logFileOpen || debugValue < 1)
        return;
    
    va_list args;
    
    va_start( args, format );                   // the last fixed parameter
    int numWritten = fprintf( fp, "FATAL|%s|", getCurrentDateTime() );
    numWritten += vfprintf( fp, format, args );
    va_end( args );
    
    fflush( fp );
    exit( 1 );
}

// ----------------------------------------------------------------------------
void    Logger_LogInfo (char *format, ... )
{
    if (!logFileOpen || debugValue < 5)
        return;
    
    va_list args;
    
    va_start( args, format );                   // the last fixed parameter
    int numWritten = fprintf( fp, "INFO|%s|", getCurrentDateTime() );
    numWritten += vfprintf( fp, format, args );
    va_end( args );
    
    fflush( fp );
}

// ----------------------------------------------------------------------------
static  int    Logger_Log (char *level, char *format, ...)
{
    if (!logFileOpen)
        return 0;
    
    va_list args;
    
    va_start( args, format );                   // the last fixed parameter
   
    int numWritten = fprintf( fp, "%s|%s|", level, getCurrentDateTime() );
    numWritten += vfprintf( fp, format, args );

    va_end( args );
    
    return numWritten;
}

// ----------------------------------------------------------------------------
static  char    *getCurrentDateTime()
{
    //
    // Something quick and dirty... Fix this later - thread safe
    time_t  current_time;
    struct  tm      *tmPtr;
    struct  timeval tv;
    
    memset( currentDateTimeBuffer, '\0', sizeof currentDateTimeBuffer );
    
    int milliSeconds = -1;
    
    if (gettimeofday( &tv, NULL ) == 0) {
        milliSeconds = ((int) tv.tv_usec) / 1000;
    }
    
    int len = 0;
    /* Obtain current time as seconds elapsed since the Epoch. */
    current_time = time( NULL );
    
    if (current_time > 0) {
        /* Convert to local time format. */
        tmPtr = localtime( &current_time );
 
        if (tmPtr != NULL) {
            len = strftime( currentDateTimeBuffer,
                    sizeof currentDateTimeBuffer,
                    "%F %T",
                    tmPtr );
            
        }
    }
    
    //
    //  strftime returns number of bytes put into the string
    if (len > 0 && milliSeconds >= 0) {
        currentDateTimeBuffer[ len ] = '.';
        currentDateTimeBuffer[ len + 1 ] = (milliSeconds % 1000) / 100 + '0';
        currentDateTimeBuffer[ len + 2 ] = ((milliSeconds % 100) / 10) + '0';
        currentDateTimeBuffer[ len + 3 ] = (milliSeconds % 10) + '0';
        currentDateTimeBuffer[ len + 4 ] = '\0';       
        
        //printf( "Milliseconds = %d, string [%s]\n", milliSeconds, currentDateTimeBuffer );
    }
    return &currentDateTimeBuffer[ 0 ];    
}
