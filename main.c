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



static  int     realTimeData_Seconds = 5;
static  int     otherData_Seconds = 10;
static  char    *brokerHost = "ec2-52-32-56-28.us-west-2.compute.amazonaws.com";
static  char    *controllerID = "1";
static  char    *devicePort = "/dev/ttyUSB0";
static  char    *clientID = "ls1024b";          // fix - add random


// -----------------------------------------------------------------------------
int main (int argc, char* argv[]) 
{
    modbus_t    *ctx;
    char        mqttClientID[ 256 ];
    
    parseCommandLine( argc, argv );
    Logger_Initialize( "ls1024b.log", 5 );
    
    //
    // Need to create a unique client ID
    MQTT_Initialize( controllerID, brokerHost );
    
    
    printf( "Opening %s, 115200 8N1\n", devicePort );
    ctx = modbus_new_rtu( devicePort, 115200, 'N', 8, 1 );
    if (ctx == NULL) {
        fprintf( stderr, "Unable to create the libmodbus context\n" );
        return -1;
    }

    printf( "Setting slave ID to %X\n", LANDSTAR_1024B_ID );
    modbus_set_slave( ctx, LANDSTAR_1024B_ID );

    puts( "Connecting" );
    if (modbus_connect( ctx ) == -1) {
        fprintf( stderr, "Connection failed: %s\n", modbus_strerror( errno ) );
        modbus_free( ctx );
        return -1;
    }
    
    
    
    //
    // Figure out how long we need to sleep every loop - it will be the smaller of the two
    int loopSleep = (realTimeData_Seconds < otherData_Seconds) ? realTimeData_Seconds : otherData_Seconds;
    
    
    RatedData_t             ratedData;
    RealTimeData_t          realTimeData;
    RealTimeStatus_t        realTimeStatusData;
    Settings_t              settingsData;
    StatisticalParameters_t statisticalParametersData;
    
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
        

        MQTT_PublishRatedData( controllerID, &ratedData );
        MQTT_PublishRealTimeData( controllerID, &realTimeData );
        MQTT_PublishRealTimeStatus( controllerID, &realTimeStatusData);
        MQTT_PublishSettings( controllerID, &settingsData );
        MQTT_PublishStatisticalParameters( controllerID, &statisticalParametersData );
        
        sleep( loopSleep );
    }
    
    MQTT_Teardown( NULL );
    puts( "Done" );
    modbus_close( ctx );
    Logger_Terminate();
    
    return (EXIT_SUCCESS);
}



// -----------------------------------------------------------------------------
static
void    parseCommandLine (int argc, char *argv[])
{
    //
    //  Options
    //  -h  <string>    MQTT host to connect to
    //  -t  N           send realtime data every N seconds (defaults to 60)
    //  -o  N           send all other data packets every N seconds (defaults to 300)
    //  -i  <string>    give this controller an identifier (defaults to LS1024B_1)
    //  -p  <string>    open this /dev/port to talk to contoller (defaults to /dev/ttyUSB0
    char    c;
    
    while (((c = getopt( argc, argv, "h:t:o:i:p:" )) != -1) && (c != 255)) {
        switch (c) {
            case 'h':   brokerHost = optarg;    break;
            case 't':   realTimeData_Seconds = atoi( optarg );      break;
            case 'o':   otherData_Seconds = atoi( optarg );      break;
            case 'i':   controllerID = optarg;    break;
            case 'p':   devicePort = optarg;    break;
        }
    }

}