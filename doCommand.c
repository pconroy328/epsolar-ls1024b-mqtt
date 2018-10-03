/*
 * File:   doCommand.c
 * Author: pconroy
 *
 * When a MQTT pack comes in on the COMMAND topic, it's parsed into a structure
 * and that struct is placed on our FIFO queue.
 * 
 * The 'processInboundCommand' thread will pull it off the queue and pass it to
 * 'doCommand'.  doCommand matches the command in the MQTT packet against the big
 * table 'commandTable[]'.
 * 
 * If a match is found, the corresponding function pointer is called. That fp points
 * to a SCC function found in the "LS10x4B SCC" shared library.
 * 
 * Created on Septmeber 13, 2018, 11:46 AM
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ls10x4b.h"
#include "logger.h"
#include "commandQueue.h"

//
// Define a function pointer - leave args ambiguous
typedef void (*fptr) ();

//
// Define a entry into our Command Dispatch Table
typedef struct  commandMap {
    char        *command;           // Command that triggers the function
    int         fargs;              // Whether function takes Ints, Floats or something else
    fptr        f;                  // Function pointer to a Solar Charge Controller function
} commandMap_t;



//
//  SCC functions have different argument types - see sccls10x4b.h
#define NOARG       0               // Function takes no parameters
#define INTARG      1               // Function takes an Int
#define FLOATARG    2               // Function takes a FP (double)
#define HHMMARG     3               // Function takes an "HH:MM"
#define HHMMSSARG   4               // Function takes an "HH:MM:SS"


//
//  The Command Dispatch Table
static  commandMap_t    commandTable[] = {
    { .command = "BT",      .fargs = INTARG,        .f = setBatteryType },
    { .command = "TCC",     .fargs = FLOATARG,      .f = setTempertureCompensationCoefficient },    
    { .command = "BC",      .fargs = INTARG,        .f = setBatteryCapacity },
    { .command = "HVD",     .fargs = FLOATARG,      .f = setHighVoltageDisconnect },
    { .command = "CLV",     .fargs = FLOATARG,      .f = setChargingLimitVoltage },
    { .command = "OVR",     .fargs = FLOATARG,      .f = setOverVoltageReconnect },
    { .command = "EV",      .fargs = FLOATARG,      .f = setEqualizationVoltage },
    { .command = "BV",      .fargs = FLOATARG,      .f = setBoostVoltage },
    { .command = "FV",      .fargs = FLOATARG,      .f = setFloatVoltage },
    { .command = "BRV",     .fargs = FLOATARG,      .f = setBoostReconnectVoltage },
    { .command = "LVR",     .fargs = FLOATARG,      .f = setLowVoltageReconnect },
    
    { .command = "WTL1",    .fargs = HHMMARG,       .f = setWorkingTimeLength1 },
    { .command = "WTL2",    .fargs = HHMMARG,       .f = setWorkingTimeLength2 },
    
    { .command = "SLON",    .fargs = HHMMARG,       .f = setLengthOfNight },
    
    { .command = "TONT1",   .fargs = HHMMSSARG,     .f = setTurnOnTiming1 },
    { .command = "TOFFT1",  .fargs = HHMMSSARG,     .f = setTurnOffTiming1 },
    { .command = "TONT2",   .fargs = HHMMSSARG,     .f = setTurnOnTiming2 },
    { .command = "TOFFT2",  .fargs = HHMMSSARG,     .f = setTurnOffTiming2 },
    
    { .command = "CDON",    .fargs = NOARG,         .f = setChargingDeviceOn },
    { .command = "CDOFF",   .fargs = NOARG,         .f = setChargingDeviceOff },
    { .command = "LDON",    .fargs = NOARG,         .f = setLoadDeviceOn },
    { .command = "LDOFF",   .fargs = NOARG,         .f = setLoadDeviceOff },

    { .command = "RSD",     .fargs = NOARG,         .f = restoreSystemDefaults },
    { .command = "CGES",    .fargs = NOARG,         .f = clearEnergyGeneratingStatistics },
    { .command = "NULL",    .fargs = 0,             .f = NULL }
};

#define NUM_COMMANDS  ( sizeof( commandTable ) / sizeof( commandMap_t ))


//
// Forwards
static  int     hhmmStringToHour( const char * );
static  int     hhmmStringToMinute( const char * );
static  int     hhmmssStringToSecond( const char * );





// -----------------------------------------------------------------------------
static
int doCommand (modbus_t *ctx, mqttCommand_t *cmd)
{
    Logger_LogInfo( "doCommand. Command [%s], Int parameter [%d], Float parameter [%0.2f]\n", cmd->command, cmd->iParam, cmd->fParam );
    
    int i = 0;
    
    //
    // Loop thru table. Inefficient, but...
    while (i < NUM_COMMANDS && commandTable[ i ].f != NULL) {
        
        //
        //  Find a match between inbound Command (cmd->command) and entry in the table
        if (strncmp( cmd->command, commandTable[ i ].command, strlen( commandTable[ i ].command )) == 0) {
            
            //
            // Figure out if function that executes command takes an Int or Float or...?
            if (commandTable[ i ].fargs == INTARG) {
                Logger_LogDebug( "Dispatching INT function for command [%s] parameter [%d]\n", cmd->command, cmd->iParam );
                commandTable[ i ].f( ctx, cmd->iParam );
                
            } else if (commandTable[ i ].fargs == FLOATARG) {
                Logger_LogDebug( "Dispatching Float function for command [%s] parameter [%0.2f]\n", cmd->command, cmd->fParam );
                commandTable[ i ].f( ctx, cmd->fParam );
                
            } else if (commandTable[ i ].fargs == HHMMARG) {
                int hour = hhmmStringToHour( cmd->cParam );
                int min = hhmmStringToMinute( cmd->cParam );
                Logger_LogDebug( "Dispatching HH:MM function for command [%s] hour [%d] minute [%d]\n", cmd->command, hour, min );
                commandTable[ i ].f( ctx, hour, min );
                
            } else if (commandTable[ i ].fargs == HHMMSSARG) {
                int hour = hhmmStringToHour( cmd->cParam );
                int min = hhmmStringToMinute( cmd->cParam );
                int sec = hhmmssStringToSecond( cmd->cParam );
                Logger_LogDebug( "Dispatching HH:MM:SS function for command [%s] hour [%d] minute [%d] second [%d]\n", cmd->command, hour, min, sec );
                commandTable[ i ].f( ctx, hour, min, sec );
                
            } else if (commandTable[ i ].fargs == NOARG) {
                Logger_LogDebug( "Dispatching No Arg function for command [%s]\n", cmd->command );
                commandTable[ i ].f( ctx );
            }
            
            break;
        }
        
        i += 1;
    }
    
    return 1;
}

// -----------------------------------------------------------------------------
static
int hhmmStringToHour (const char *hhmmString)
{
    char    buffer[ 8 ];
    
    buffer[ 0 ] = hhmmString[ 0 ];
    buffer[ 1 ] = hhmmString[ 1 ];
    buffer[ 2 ] = '\0';
    
    return atoi( buffer );
}

// -----------------------------------------------------------------------------
static
int hhmmStringToMinute (const char *hhmmString)
{
    char    buffer[ 8 ];
    
    //            "hh:mm"
    buffer[ 0 ] = hhmmString[ 3 ];
    buffer[ 1 ] = hhmmString[ 4 ];
    buffer[ 2 ] = '\0';
    
    return atoi( buffer );
}

// -----------------------------------------------------------------------------
static
int hhmmssStringToSecond (const char *hhmmssString)
{
    char    buffer[ 8 ];
    
    //            "hh:mm:ss"
    buffer[ 0 ] = hhmmssString[ 6 ];
    buffer[ 1 ] = hhmmssString[ 7 ];
    buffer[ 2 ] = '\0';
    
    return atoi( buffer );
}



// -----------------------------------------------------------------------------
void    *processInboundCommand (void *argPtr)
{
    //
    //  This function is started by a new thread
    //
    Logger_LogDebug( "processInBoundCommand - starting thread.\n" );
    modbus_t    *ctx;
    
    ctx = (modbus_t *) argPtr;

    //
    //  Loop forever
    while (TRUE) {
        //
        //  did someone send us a command?
        mqttCommand_t   *command = removeElementAndWait();
        if (command != NULL) {
            doCommand( ctx, command );
            free( command );
        }
    }
    
    return (void *) 0;
}
