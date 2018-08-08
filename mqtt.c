#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#include "mqtt.h"
#include "logger.h"
#include "ls1024b.h"
#include "cJSON.h"



static  int              MQTT_Connected = FALSE;
static  int              MQTT_defaultsSet = FALSE;


static  struct mosquitto    *myMQTTInstance = NULL;
static  char            subscriptionTopic[ 1024 ];          // MQTT Spec is 64K
static  char            publishTopic[ 1024 ];               // These two will hold Parent and Subtopics
static  int             QoS;                                // Quality of Service used by MQTT (0, 1 or 2)

static  char            *userData = NULL;


//
// Forward declarations
//  Mosquitto supports a number of 'callback' functions on various triggers. Here are some default handlers
static  void    defaultOnConnectedCallback( struct mosquitto *mosq, void *userdata, int result );
static  void    defaultOnDisconnectedCallback( struct mosquitto *mosq, void *userdata, int result );
static  void    defaultOnLogCallback( struct mosquitto *mosq, void *userdata, int level, const char *str );
static  void    defaultOnMessageReceivedCallback( struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message );
static  void    defaultOnSubscribedCallback( struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos );
static  void    defaultOnUnsubscribedCallback( struct mosquitto *mosq, void *userdata, int result );
static  void    defaultOnPublishedCallback( struct mosquitto *mosq, void *userdata, int result );



//
// This is the actual function that'll handle incoming MQTT messages
static  void    MQTT_MessageReceivedHandler( struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message );



// ---------------------------------------------------------------------------
void    MQTT_SetDefaults (const char *controllerID)
{    
    snprintf( publishTopic, sizeof( publishTopic ), "%s/%s", "LS1024B", controllerID );
    QoS = 0;    
    MQTT_defaultsSet = TRUE;
}


// ----------------------------------------------------------------------------
void    MQTT_Initialize (const char *controllerID, const char *brokerHost)
{
    //
    //  If we forgot to call 'setDefaults' do so
    if (!MQTT_defaultsSet)
        MQTT_SetDefaults( controllerID );
    
    
    //
    // Here we get into Broker Specifics, for Mosquitto (www.mosquitto.org)
    mosquitto_lib_init();
    
    //
    // We're seeing Connection Refused Errors - and I think I've seen this before when I've had a version
    //  mismatch, so print the version out
    int     mosqMajor = 0;
    int     mosqMinor = 0;
    int     mosqRevision = 0;
    mosquitto_lib_version( &mosqMajor, &mosqMinor, &mosqRevision );
    Logger_LogDebug( "Mosquitto Library Version. Major: %d  Minor: %d  Revision: %d\n", mosqMajor, mosqMinor, mosqRevision );

    
    //
    // CleanSession 
    //      Set to true to instruct the broker to clean all messages and subscriptions on disconnect, 
    //      false to instruct it to keep them.  See the man page mqtt(7) for more details.  
    //      Note that a client will never discard its own outgoing messages on disconnect.  
    //      Calling mosquitto_connect or mosquitto_reconnect will cause the messages to be resent.  
    //      Use mosquitto_reinitialise to reset a client to its original state.  
    //      Must be set to true if the id parameter is NULL.
    // UserData
    //      A way to pass data into all of the callbacks
    //
    int     cleanSession = TRUE;
    
    myMQTTInstance = mosquitto_new( NULL, 
                                    cleanSession, 
                                    userData );
    
    if (myMQTTInstance == NULL) {
        int errorNumber = errno;
        if (errorNumber == ENOMEM)
            Logger_LogFatal( "Out of memory - cannot create a new MQTT instance!" );
        else if (errorNumber = EINVAL)
            Logger_LogFatal( "Input parameters are invalid - cannot create a new MQTT instance!" );
    }
    
   
    //
    //  Set up some default callbacks
    //mosquitto_log_callback_set( myMQTTInstance, defaultOnLogCallback );
    mosquitto_connect_callback_set( myMQTTInstance, defaultOnConnectedCallback );
    mosquitto_message_callback_set( myMQTTInstance, defaultOnMessageReceivedCallback );
    mosquitto_disconnect_callback_set( myMQTTInstance, defaultOnDisconnectedCallback );
    mosquitto_publish_callback_set( myMQTTInstance, defaultOnPublishedCallback );
    mosquitto_subscribe_callback_set( myMQTTInstance, defaultOnSubscribedCallback );
    mosquitto_unsubscribe_callback_set( myMQTTInstance, defaultOnUnsubscribedCallback );
    
    Logger_LogInfo( "Attempting to connect to MQTT broker on host [%s], port [%d]...\n", brokerHost, 1883 );
    Logger_LogInfo( "If this call hangs, check to make sure the broker is running and reachable.\n" );
    
    int returnCode = 0;
    if ( (returnCode = mosquitto_connect( myMQTTInstance, brokerHost, 1883, 60 )) != MOSQ_ERR_SUCCESS ) {
        Logger_LogError( "Unable to connect MQTT to broker!\n" );
        if (returnCode == MOSQ_ERR_INVAL)
            Logger_LogError( "Check input parameters. One or more are invalid.\n" );
        else {
            returnCode = errno;
            Logger_LogError( "Connect call returned an error (errno) of [%s].\n", strerror( returnCode ) );
        }
        Logger_LogFatal( "Cannot connect to the MQTT broker\n" );
    }
    
    Logger_LogInfo( "Successfully connected to the broker\n" );
    
    //  even if we don't expect to receive any messages - things get published faster if we call "loop"
    mosquitto_loop_start( myMQTTInstance );
    MQTT_Connected = TRUE;
}


// -----------------------------------------------------------------------------
static  
void    MQTT_MessageReceivedHandler (struct mosquitto *mosq, void *userdata, const struct mosquitto_message *msg)
{
    //
    //  Examples we expect
    //  { "topic" : "LS1024B/x/COMMAND", "dateTime" : "x", "parameter": "Load", "value" : "Off" }   
    //  { "topic" : "LS1024B/x/COMMAND", "dateTime" : "x", "parameter": "FactoryReset", "value" : true }
    //  { "topic" : "LS1024B/x/COMMAND", "dateTime" : "x", "parameter": "BatteryType", "value" : "Sealed" }
    //  { "topic" : "LS1024B/x/COMMAND", "dateTime" : "x", "parameter": "BatteryCapacity", "value" : 5 }
    char    *jsonPayload = msg->payload;
    int     jsonLength = msg->payloadlen;
    char    *topic = msg->topic;
#if 0    
    if (jsonPayload != NULL && jsonLength > 0) {
        cJSON *json = cJSON_Parse( jsonPayload );

        cJSON   *value = NULL;
        cJSON   *parameter = cJSON_GetObjectItem( json, "parameter" );
        if (cJSON_IsString( parameter ) && (parameter->valuestring != NULL)) {
            
            char *value = cJSON_GetObjectItem( json, "value" );
            if (cJSON_IsString( value ) && (value->valuestring != NULL)) {
                
            } else {
                Logger_LogError( "No attributed named 'value' after the 'parameter' attribute in the json message!\n" );            
            }
            
        } else {
            Logger_LogError( "No attributed named 'parameter' in the json message!\n" );
        }
        
        cJSON_Delete( json );       
    } else {
        Logger_LogError( "Received a null or zero length message\n" );
    }
#endif
}

// ----------------------------------------------------------------------------
void    MQTT_PublishData (const char *topic, const char *jsonMessage, const int length)
{
    int     messageID;
   
    int result = mosquitto_publish( myMQTTInstance,
                        &messageID, 
                        topic,
                        length,
                        jsonMessage,
                        QoS, 
                        0 );
    
    if (result != MOSQ_ERR_SUCCESS) {
        Logger_LogError( "Unable to publish the message. Mosquitto error code: %d\n", result );
    //  MOSQ_ERR_INVAL	if the input parameters were invalid.
    //  MOSQ_ERR_NOMEM	if an out of memory condition occurred.
    //  MOSQ_ERR_NO_CONN	if the client isn’t connected to a broker.
    //  MOSQ_ERR_PROTOCOL	if there is a protocol error communicating with the broker.
    //  MOSQ_ERR_PAYLOAD_SIZE	if payloadlen is too large.)
    }        
}

// ----------------------------------------------------------------------------
void    MQTT_Teardown ()
{
    Logger_LogInfo( "MQTT_Teardown() - we're shutting down the MQTT pipe.\n" );
    // MQTT_Unsubscribe( aSystem );
    
    mosquitto_destroy( myMQTTInstance );
    mosquitto_lib_cleanup();
    MQTT_Connected = FALSE;
}


// ----------------------------------------------------------------------------
void    MQTT_Subscribe  (const char *topic, const int QoS)
{     
    int returnCode = mosquitto_subscribe( myMQTTInstance,
                        NULL,                   // message ID, not needed
                        topic,                  // remember this is concatenated parent + subscribe
                        QoS );
  
    if (returnCode != MOSQ_ERR_SUCCESS)
        Logger_LogError( "Unable to subscribe to topic [%s], reason: %d\n", topic, returnCode );    
}

// ----------------------------------------------------------------------------
void    MQTT_Unsubscribe  (void *aSystem)
{
    int returnCode = mosquitto_unsubscribe( myMQTTInstance,
                        NULL,                       // message ID, not needed
                        subscriptionTopic );        // remember this is concatenated parent + subscribe
  
    if (returnCode != MOSQ_ERR_SUCCESS)
        Logger_LogError( "Unable to UNsubscribe to topic [%s], reason: %d\n", subscriptionTopic, returnCode );    
}          

// ------------------------------------------------------------------------------
//
//  Connect Callback is called when the broker acknowledges the connection
//
static
void defaultOnConnectedCallback (struct mosquitto *mosq, void *userdata, int result)
{
    Logger_LogInfo( "MQTT Connection Acknowledge Callback. Result: %d\n", result );

    //
    //  Result will be one of:
    //      0 - success
    //      1 - connection refused (unacceptable protocol version)
    //      2 - connection refused (identifier rejected)
    //      3 - connection refused (broker unavailable)
    //      4-255 - reserved for future use
    //
    if (result == 0) {
        MQTT_Connected = TRUE;
        Logger_LogDebug( "MQTT Connection Callback. Connection to the broker was successful.\n" );
        
    } else {
        Logger_LogError( "MQTT Connection refused by broker --  " );
        switch (result) {
            case    1:  Logger_LogError( "unacceptable protocol version\n" );       break;
            case    2:  Logger_LogError( "unacceptable protocol version\n" );       break;
            case    3:  Logger_LogError( "unacceptable protocol version\n" );       break;
            
            default:    Logger_LogError( "Unknown reason. [ Was a reserved value at compile time. ] Reason code: %d\n", result );       
                        Logger_LogError( "Consult the most recent Mosquitto MQTT documentation for more information.\n", result );       
                        break;
            
        }
        
        Logger_LogError( "MQTT Connection refused by broker - MQTT is not connected to broker. No messages will be published or received.\n" );
        Logger_LogError( "MQTT Mosquitto - Library connection refused reason [%s]\n", mosquitto_connack_string( result ) );
        Logger_LogError( "MQTT Mosquitto - Library version [%d]\n", mosquitto_lib_version( (int *) 0, (int *) 0, (int *) 0) );
        
        MQTT_Connected = FALSE;
    }
}

// ------------------------------------------------------------------------------
//
//  Disconnect Callback is called when the broker acknowledges the DISconnect request from
//  the client
//
static
void defaultOnDisconnectedCallback (struct mosquitto *mosq, void *userdata, int result)
{
    Logger_LogInfo( "MQTT *DIS*Connection Acknowledge Callback - Consider the **DIS**connection successful.\n" );
}


// ------------------------------------------------------------------------------
//
//  Call when subscription to a topic has succeeded
//
static
void defaultOnSubscribedCallback (struct mosquitto *mosq, void *userdata, int mid, int qos_count, const int *granted_qos)
{
    Logger_LogInfo( "MQTT Subscribed to Topic Callback - Consider the subscription successful.\n" );
    //
    //  Most of the parameters can be ignored
}


// ------------------------------------------------------------------------------
//
//  Call when subscription to a topic has succeeded
//
static
void defaultOnUnsubscribedCallback (struct mosquitto *mosq, void *userdata, int result)
{
    Logger_LogInfo( "MQTT *UN*subscribed to Topic Callback - Consider the **UN**subscription successful.\n" );
    //
    //  Most of the parameters can be ignored
}

// ------------------------------------------------------------------------------
static
void defaultOnPublishedCallback (struct mosquitto *mosq, void *userdata, int result)
{
    Logger_LogInfo( "MQTT Published to Topic Callback - The broker has received your message.\n" );    
}

// ------------------------------------------------------------------------------
static
void defaultOnLogCallback (struct mosquitto *mosq, void *userdata, int level, const char *str)
{
    /* Print all log messages regardless of level. */
    // debug_print("MQTT Logging: [%s]\n", str );
}

// ------------------------------------------------------------------------------
//  Called when a message has been RECEIVED from the broker
//  
static
void defaultOnMessageReceivedCallback (struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
    Logger_LogInfo( "MQTT Message Received Callback - the broker has sent us a message.\n" );
    Logger_LogWarning( "MQTT Message Received Callback - This is the Default Callback. Nothing will happen. This function should have been overridden.\n" );
    
    /*. Payload length: %d\n", message->payloadlen );
    if (message->payloadlen > 0) {
        Logger_LogDebug( "Topic [%s] Data [%s]\n", (char *) message->topic, (char *) message->payload);
        
    } else {
        Logger_LogDebug(" MQTT Topic [%s] with no payload\n", message->topic );
    }
     * */
}
