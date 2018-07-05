/* 
 * File:   main.c
 * Author: pconroy
 *
 * Code to communicate with the LandStar LS1024B PWM Solar Charge Controller
 * and send status information out as JSON Packets over MQTT
 * 
 * Created on June 13, 2018, 11:46 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>

#include <modbus/modbus.h>

#include "logger.h"
#include "ls1024b.h"
#include "mqtt.h"

//  
// Forwards
static  void    parseCommandLine( int, char ** );



static  int     sleepSeconds = 15;
static  char    *brokerHost = "ec2-52-32-56-28.us-west-2.compute.amazonaws.com";
static  char    *controllerID = "1";
static  char    *devicePort = "/dev/ttyUSB0";
static  char    *clientID = "ls1024b";          // fix - add random

static  char    *topTopic = "LS1024B";
static  char    fullTopic[ 1024 ];


extern char *createJSONMessage (modbus_t *ctx, const char *topic, const RatedData_t *ratedData, 
        const RealTimeData_t *rtData, const RealTimeStatus_t *rtStatusData, 
        const Settings_t *setData, const StatisticalParameters_t *stats);

// -----------------------------------------------------------------------------
int main (int argc, char* argv[]) 
{
    modbus_t    *ctx;
    char        mqttClientID[ 256 ];
    
    printf( "LS1024B_MQTT application - version 0.0.6\n" );

    parseCommandLine( argc, argv );
    Logger_Initialize( "ls1024b.log", 5 );
    
    //
    // Need to create a unique client ID
    MQTT_Initialize( controllerID, brokerHost );
    
    Logger_LogInfo( "Opening %s, 115200 8N1\n", devicePort );
    ctx = modbus_new_rtu( devicePort, 115200, 'N', 8, 1 );
    if (ctx == NULL) {
        Logger_LogFatal( "Unable to create the libmodbus context\n" );
        return -1;
    }

    Logger_LogInfo( "Setting slave ID to %X\n", LANDSTAR_1024B_ID );
    modbus_set_slave( ctx, LANDSTAR_1024B_ID );

    puts( "Connecting" );
    if (modbus_connect( ctx ) == -1) {
        Logger_LogFatal( "Connection failed: %s\n", modbus_strerror( errno ) );
        modbus_free( ctx );
        return -1;
    }
    

    snprintf( fullTopic, sizeof fullTopic, "%s/%s/%s", topTopic, controllerID, "DATA" );
    Logger_LogInfo( "Publishing messaages to MQTT Topic [%s]\n", fullTopic );
    
    RatedData_t             ratedData;
    RealTimeData_t          realTimeData;
    RealTimeStatus_t        realTimeStatusData;
    Settings_t              settingsData;
    StatisticalParameters_t statisticalParametersData;

    setRealtimeClockToNow( ctx );
    int seconds, minutes, hour, day, month, year;
    getRealtimeClock( ctx, &seconds, &minutes, &hour, &day, &month, &year );
    Logger_LogInfo( "System Clock set to: %02d/%02d/%02d %02d:%02d:%02d\n", month, day, year, hour, minutes, seconds );
    Logger_LogInfo( "Setting battery capacity to 5AH, type to '1' and loadControlMode to 0x00" );
    setBatteryCapacity( ctx, 5 );
    setBatteryType( ctx, 1 );
    setLoadControlMode( ctx, 0x00 );

    
    while (1) {      
        //
        //  every time thru the loop - zero out the structs!
        memset( &ratedData, '\0', sizeof( RatedData_t ) );
        memset( &realTimeData, '\0', sizeof( RealTimeData_t ) );
        memset( &realTimeStatusData, '\0', sizeof( RealTimeStatus_t ) );
        memset( &settingsData, '\0', sizeof( Settings_t ) );
        memset( &statisticalParametersData, '\0', sizeof( StatisticalParameters_t ) );
        
        getRatedData( ctx, &ratedData );
        getRealTimeData( ctx, &realTimeData );
        getRealTimeStatus( ctx, &realTimeStatusData);
        getSettings( ctx, &settingsData );
        getStatisticalParameters( ctx, &statisticalParametersData );
        
        char    *jsonMessage = createJSONMessage( ctx, 
                                            fullTopic,
                                            &ratedData, 
                                            &realTimeData, 
                                            &realTimeStatusData,
                                            &settingsData,
                                            &statisticalParametersData
                );
        
        MQTT_PublishData( fullTopic, jsonMessage, strlen( jsonMessage ) );
        
        //
        //
        free( jsonMessage );
        sleep( sleepSeconds );
    }
    
    MQTT_Teardown( NULL );
    Logger_LogInfo( "Done" );
    modbus_close( ctx );
    Logger_Terminate();
    
    return (EXIT_SUCCESS);
}

static
void    showHelp()
{
    puts( "Options" );
    puts( "  -h  <string>    MQTT host to connect to" );
    puts( "  -t  <string>    MQTT top level topic" );
    puts( "  -s  N           sleep between sends <seconds>" );
    puts( "  -i  <string>    give this controller an identifier (defaults to '1')" );
    puts( "  -p  <string>    open this /dev/port to talk to contoller (defaults to /dev/ttyUSB0)" );
    exit( 1 ); 
}

// -----------------------------------------------------------------------------
static
void    parseCommandLine (int argc, char *argv[])
{
    //
    //  Options
    //  -h  <string>    MQTT host to connect to
    //  -t  <string>    MQTT top level topic
    //  -s  N           sleep between sends <seconds>
    //  -i  <string>    give this controller an identifier (defaults to LS1024B_1)
    //  -p  <string>    open this /dev/port to talk to contoller (defaults to /dev/ttyUSB0
    char    c;
    
    while (((c = getopt( argc, argv, "h:t:o:i:p:" )) != -1) && (c != 255)) {
        switch (c) {
            case 'h':   brokerHost = optarg;    break;
            case 's':   sleepSeconds = atoi( optarg );      break;
            case 't':   topTopic = optarg;      break;
            case 'i':   controllerID = optarg;    break;
            case 'p':   devicePort = optarg;    break;
            
            default:    showHelp();     break;
        }
    }

}
