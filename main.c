/* 
 * File:   main.c
 * Author: pconroy
 *
 * Code to communicate with the LandStar LS1024B PWM Solar Charge Controller (SCC)
 * and send status information out as JSON Packets over MQTT.
 * 
 * Also to listen for incoming MQTT commands that set parameters within the 
 * Controller.
 * 
 * Created on June 13, 2018, 11:46 AM
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#include <modbus/modbus.h>

#include "logger.h"
#include "ls1024b.h"
#include "mqtt.h"
#include "doCommand.h"
#include "commandQueue.h"
#include "jsonMessage.h"


//  
// Forwards
static  void    parseCommandLine( int, char ** );


static  char    *version = "LS1024B_MQTT SCC Controller - version 2.0.3 (controlling FP precision)";


static  int     sleepSeconds = 15;                  // How often to send out SCC data packets
static  char    *brokerHost = "10.0.0.11";          // default address of our MQTT broker
static  char    *controllerID = "1";                // Assigned an arbitrary ID to our SCC in case we have more than one
static  char    *devicePort = "/dev/ttyUSB0";       // Port where our SCC is connected
static  int     loggingLevel = 3;

static  char    *topTopic = "LS1024B";              // MQTT top level topic
static  char    publishTopic[ 1024 ];               // published data will be on "<topTopic>/<controlleID>/DATA"
static  char    subscriptionTopic[ 1024 ];          // subscribe to <"<topTopic>/<controlleID>/COMMAND"





// -----------------------------------------------------------------------------
int main (int argc, char* argv[]) 
{
    modbus_t    *ctx;
    
    printf( "%s\n", version );
    
    parseCommandLine( argc, argv );
    Logger_Initialize( "ls1024b.log", loggingLevel );           
    Logger_LogWarning( "%s\n", version );
    
    //
    // Create a FIFO queue for our incoming Commands over MQTT
    createQueue( 0, 0 );

    //
    // Connect to our MQTT Broker
    MQTT_Initialize( controllerID, brokerHost );
    
    
    //
    // Modbus - open the SCC port. We know it's 115.2K 8N1
    Logger_LogInfo( "Opening %s, 115200 8N1\n", devicePort );
    ctx = modbus_new_rtu( devicePort, 115200, 'N', 8, 1 );
    if (ctx == NULL) {
        Logger_LogFatal( "Unable to create the libmodbus context\n" );
        return -1;
    }
    
    
    //
    // I don't know if we need to set the SCC Slave ID or not
    Logger_LogInfo( "Setting slave ID to %X\n", LANDSTAR_1024B_ID );
    modbus_set_slave( ctx, LANDSTAR_1024B_ID );

    if (modbus_connect( ctx ) == -1) {
        Logger_LogFatal( "Connection failed: %s\n", modbus_strerror( errno ) );
        modbus_free( ctx );
        return -1;
    }
    
    Logger_LogInfo( "Port to Solar Charge Controller is open.\n", devicePort );

    
    //
    //  Start up a new thread to watch the Command Queue
    pthread_t   commandProcessingThread;
    if(pthread_create( &commandProcessingThread, NULL, processInboundCommand, ctx ) ) {
        Logger_LogFatal( "Unable to start the command processing thread!\n" );
        perror( "Error:" );
        return -1;
    }


    //
    //  Concatenate topTopic and controller ID to create our Pub and Sub Topics
    snprintf( publishTopic, sizeof publishTopic, "%s/%s/%s", topTopic, controllerID, "DATA" );
    Logger_LogInfo( "Publishing messages to MQTT Topic [%s]\n", publishTopic );
    
    snprintf( subscriptionTopic, sizeof subscriptionTopic, "%s/%s/%s", topTopic, controllerID, "COMMAND" );
    Logger_LogInfo( "Subscribing to commands on MQTT Topic [%s]", subscriptionTopic );
    MQTT_Subscribe ( subscriptionTopic, 0 );

    
    //
    //  I have 5 Structures because that's the way the SCC Documentation was organized
    //  There's no reason to break it up into 5.  It could have been one, or several
    RatedData_t             ratedData;
    RealTimeData_t          realTimeData;
    RealTimeStatus_t        realTimeStatusData;
    Settings_t              settingsData;
    StatisticalParameters_t statisticalParametersData;

    setRealtimeClockToNow( ctx );
    int seconds, minutes, hour, day, month, year;
    getRealtimeClock( ctx, &seconds, &minutes, &hour, &day, &month, &year );
    Logger_LogInfo( "System Clock set to: %02d/%02d/%02d %02d:%02d:%02d\n", month, day, year, hour, minutes, seconds );

    //
    //  Loop forever - read SCC data and send it out
    while (TRUE) {      
        //
        //  every time thru the loop - zero out the structs!
        memset( &ratedData, '\0', sizeof( RatedData_t ) );
        memset( &realTimeData, '\0', sizeof( RealTimeData_t ) );
        memset( &realTimeStatusData, '\0', sizeof( RealTimeStatus_t ) );
        memset( &settingsData, '\0', sizeof( Settings_t ) );
        memset( &statisticalParametersData, '\0', sizeof( StatisticalParameters_t ) );
       
        //
        // make the modbus calls to pull the data 
        getRatedData( ctx, &ratedData );
        getRealTimeData( ctx, &realTimeData );
        getRealTimeStatus( ctx, &realTimeStatusData);
        getSettings( ctx, &settingsData );
        getStatisticalParameters( ctx, &statisticalParametersData );
        
        //
        // craft a JSON message from the data 
        char    *jsonMessage = createJSONMessage( ctx, 
                                            publishTopic,
                                            &ratedData, 
                                            &realTimeData, 
                                            &realTimeStatusData,
                                            &settingsData,
                                            &statisticalParametersData
                );
       
        //
        // Publish it to our MQTT broker 
        MQTT_PublishData( publishTopic, jsonMessage, strlen( jsonMessage ) );
        
        free( jsonMessage );
        sleep( sleepSeconds );
    }

    
    //
    // we never get here!
    MQTT_Unsubscribe( subscriptionTopic );
    MQTT_Teardown( NULL );

    if (pthread_join( commandProcessingThread, NULL )) {
        Logger_LogError( "Shutting down but unable to join the commandProcessingThread\n" );
    }    
    
    destroyQueue();
    
    modbus_close( ctx );
    modbus_free( ctx );
    Logger_LogInfo( "Done" );
    Logger_Terminate();
    
    return( EXIT_SUCCESS );
}

// -----------------------------------------------------------------------------
static
void    showHelp()
{
    puts( "Options" );
    puts( "  -h  <string>   MQTT host to connect to" );
    puts( "  -t  <string>   MQTT top level topic" );
    puts( "  -s  N          sleep between sends <seconds>" );
    puts( "  -i  <string>   give this controller an identifier (defaults to '1')" );
    puts( "  -p  <string>   open this /dev/port to talk to contoller (defaults to /dev/ttyUSB0)" );
    puts( "  -v  N          logging level 1..5" );
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
    
    while (((c = getopt( argc, argv, "h:t:s:i:p:v:" )) != -1) && (c != 255)) {
        switch (c) {
            case 'h':   brokerHost = optarg;            break;
            case 's':   sleepSeconds = atoi( optarg );  break;
            case 't':   topTopic = optarg;              break;
            case 'i':   controllerID = optarg;          break;
            case 'p':   devicePort = optarg;            break;
            case 'V':   loggingLevel = atoi( optarg );  break;
            
            default:    showHelp();     break;
        }
    }
}
