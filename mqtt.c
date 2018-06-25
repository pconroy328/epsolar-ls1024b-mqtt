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
static  char    *getCurrentDateTime (void);




//
// This is the actual function that'll handle incoming MQTT messages
static  void    MQTT_MessageReceivedHandler( struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message );




// ---------------------------------------------------------------------------
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
    Logger_LogError( "We are not expecting to receive any MQTT messages!\n" );
}

// ----------------------------------------------------------------------------
void    MQTT_PublishRatedData (const char *controllerID, const RatedData_t *data)
{
    char    message[ 1024 ];
    int     messageID;
        
    //
    //  format:
    //  { "topic":"SOLAR/<ID>/RATED", "dateTime":"<datetime>", 
    //      "batteryCurrent":xx.x
    //      "batteryPower":xx.x
    //      "batteryVoltage":xx.x
    //      "pvArrayCurrent":xx.x
    //      "pvArrayPower":xx.x
    //      "pvArrayVoltage":xx.x
    //      "chargingMode":x }
    char topic[1024];
    snprintf( topic, sizeof topic, "%s/%s", publishTopic, "RATED" );
    
    int length = snprintf( message, sizeof message,
            "{ \"topic\":\"%s\", \"dateTime\":\"%s\", \
\"batteryCurrent\":%0.2f, \
\"batteryPower\":%0.2f, \
\"batteryVoltage\":%0.2f, \
\"pvArrayCurrent\":%0.2f, \
\"pvArrayPower\":%0.2f, \
\"pvArrayVoltage\":%0.2f, \
\"chargingMode\": \"%s\" } ",
    
        topic, getCurrentDateTime(),

        data->batteryRatedCurrent,
        data->batteryRatedPower,
        data->batteryRatedVoltage,
        data->pvArrayRatedCurrent,
        data->pvArrayRatedPower,
        data->pvArrayRatedVoltage,
        data->chargingMode );

    printf( "Handformed JSON [%s]\n\n", message );
    
    int result = mosquitto_publish( myMQTTInstance,
                        &messageID, 
                        topic,
                        length,
                        message,
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
void    MQTT_PublishRealTimeData (const char *controllerID, const RealTimeData_t *data)
{
    char    message[ 1024 ];
    int     messageID;
        
    
    char topic[1024];
    snprintf( topic, sizeof topic, "%s/%s", publishTopic, "REALTIME" );
    
    int length = snprintf( message, sizeof message,
            "{ \"topic\":\"%s\", \"dateTime\":\"%s\", \
\"batteryCurrent\":%0.2f, \
\"batteryPower\":%0.2f, \
\"batteryTemp\":%0.1f, \
\"batteryVoltage\":%0.2f, \
\"caseTemp\":%0.1f, \
\"componentsTemp\":%0.1f, \
\"loadCurrent\":%0.2f, \
\"loadPower\":%0.2f, \
\"loadVoltage\":%0.2f, \
\"pvArrayCurrent\":%0.2f, \
\"pvArrayPower\":%0.2f, \
\"pvArrayVoltage\":%0.2f } ",
    
        topic, getCurrentDateTime(),
        data->batteryCurrent,
        data->batteryPower,
        data->batteryTemp,
        data->batteryVoltage,
        data->caseTemp,
        data->componentsTemp,
        data->loadCurrent,
        data->loadPower,
        data->loadVoltage,
        data->pvArrayCurrent,
        data->pvArrayPower,
        data->pvArrayVoltage );
    

    printf( "Handformed JSON [%s]\n\n", message );
    
    int result = mosquitto_publish( myMQTTInstance,
                        &messageID, 
                        topic,
                        length,
                        message,
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
void    MQTT_PublishRealTimeStatus (const char *controllerID, const RealTimeStatus_t *data)
{
    char    message[ 1024 ];
    int     messageID;
        
    
    char topic[1024];
    snprintf( topic, sizeof topic, "%s/%s", publishTopic, "REALTIMESTATUS" );


    //
    //  battery bits
    //  D3-D0: 01H Overvolt , 00H Normal , 02H Under Volt, 03H Low Volt Disconnect, 04H Fault
    //  D7-D4: 00H Normal, 01H Over Temp. (Higher than the warning settings), 02H Low Temp.(
    //      Lower than the warning settings),
    //  D8: Battery inerternal resistance abnormal 1, normal 0
    //  D15: 1-Wrong identification for rated voltage
    
    int mask = 0x000F;
    int bitValue = data->batteryStatus & mask;
    char    *bStatus1 = "???";
    
    switch (bitValue) {
        case    0x00:   bStatus1 = "Normal";    break;
        case    0x01:   bStatus1 = "Over";  break;
        case    0x02:   bStatus1 = "Under";  break;
        case    0x03:   bStatus1 = "Low Voltage Disconnect";  break;
        case    0x04:   bStatus1 = "Fault";  break;
    }

    mask = 0x00F0;
    bitValue = data->batteryStatus & mask;
    char    *bStatus2 = "???";
    
    switch (bitValue) {
        case    0x00:   bStatus2 = "Normal";    break;
        case    0x01:   bStatus2 = "High";  break;
        case    0x02:   bStatus2 = "Low";  break;
    }

    mask = 0x0100;
    bitValue = data->batteryStatus & mask;
    char    *bStatus3 = "???";

    switch (bitValue) {
        case    0x00:   bStatus3 = "Normal";    break;
        case    0x01:   bStatus3 = "Abnormal";  break;
    }
    
    mask = 0x8000;
    bitValue = data->batteryStatus & mask;
    char    *bStatus4 = "???";
    bStatus4 = (bitValue == 1 ? "Wrong" : "Correct");
    
    
    /*
     D15-D14: Input volt status. 00 normal, 01 no power connected, 02H Higher volt input, 03H Input volt error.
D13: Charging MOSFET is short.
D12: Charging or Anti-reverse MOSFET is short.
D11: Anti-reverse MOSFET is short.
D10: Input is over current.
D9: The load is Over current.
D8: The load is short.
D7: Load MOSFET is short.
D4: PV Input is short.
D3-2: Charging status. 00 No charging,01 Float,02 Boost,03 Equlization.
D1: 0 Normal, 1 Fault.
D0: 1 Running, 0 Standby.*/
    int ivMask          = 0b1100000000000000;
    int mosfetMask      = 0b0010000000000000;
    int carMosfetMask   = 0b0001000000000000;
    int arMosfetMask    = 0b0000100000000000;
    int iocMask         = 0b0000010000000000;
    int locMask         = 0b0000001000000000;
    int loadShortMask   = 0b0000000100000000;
    int loadMosfetMask  = 0b0000000010000000;
    
    int pvInputMask     = 0b0000000000010000;
    int cs3Mask         = 0b0000000000001100;
    int cs2Mask         = 0b0000000000000010;
    int cs1Mask         = 0b0000000000000001;

    char    *iv = "???";
    switch ((data->chargingStatus & ivMask) >> 14) {
        case    0x00:   iv = "Normal";              break;
        case    0x01:   iv = "No Power Connected";              break;
        case    0x02:   iv = "Higher Volt Input";              break;
        case    0x03:   iv = "Input Volt Error";              break;
    }
    
    char    *cs = "???";
    switch ((data->chargingStatus & cs3Mask) >> 2) {
        case    0x00:   cs = "Not Charging";              break;
        case    0x01:   cs = "Float(ing)";              break;
        case    0x02:   cs = "Boost(ing)";              break;
        case    0x03:   cs = "Equalizing";              break;
    }
    
    int length = snprintf( message, sizeof message,
            "{ \"topic\":\"%s\", \"dateTime\":\"%s\", \
\"batteryStatusHex\":%d, \
\"chargingStatusHex\":%d, \
\"batteryVoltage\":\"%s\", \
\"batteryTemp\":\"%s\", \
\"batteryResistance\":\"%s\", \
\"batteryIDType\":\"%s\", \
\"chargingInputVoltStatus\":\"%s\",\
\"chargingMOSFETShort\":\"%s\",\
\"chargingOrAntiReverseMOSFETShort\":\"%s\",\
\"antiReverseMOSFETShort\":\"%s\",\
\"inputOverCurrent\":\"%s\",\
\"loadOverCurrent\":\"%s\",\
\"loadShort\":\"%s\",\
\"loadMOSFETShort\":\"%s\",\
\"pvInputShort\":\"%s\",\
\"chargingStatus\":\"%s\",\
\"chargingStatusFault\":\"%s\",\
\"chargingStatusRunning\":\"%s\" }",
    
        topic, getCurrentDateTime(),
        data->batteryStatus,
        data->chargingStatus,
        bStatus1, bStatus2, bStatus3, bStatus4,
        iv,
        ((data->chargingStatus & mosfetMask) ? "SHORT" : "OK" ),
        ((data->chargingStatus & carMosfetMask) ? "SHORT" : "OK" ),
        ((data->chargingStatus & arMosfetMask) ? "SHORT" : "OK" ),
        ((data->chargingStatus & iocMask) ? "SHORT" : "OK"),
        ((data->chargingStatus & locMask) ? "SHORT" : "OK" ),
        ((data->chargingStatus & loadShortMask) ? "SHORT" : "OK"),
        ((data->chargingStatus & loadMosfetMask) ? "SHORT" : "OK"),
        ((data->chargingStatus & pvInputMask) ? "SHORT" : "OK"),
        cs,
        ((data->chargingStatus & cs2Mask) ? "Normal" : "Fault"),
        ((data->chargingStatus & cs1Mask) ? "Standby" : "Running")
    );

    printf( "Handformed JSON [%s]\n\n", message );
    
    int result = mosquitto_publish( myMQTTInstance,
                        &messageID, 
                        topic,
                        length,
                        message,
                        QoS, 
                        0 );
    
    if (result != MOSQ_ERR_SUCCESS) {
        Logger_LogError( "Unable to publish the message. Mosquitto error code: %d\n", result );
    }    
}

// ----------------------------------------------------------------------------
void    MQTT_PublishSettings (const char *controllerID, const Settings_t *data)
{
    char    message[ 1024 ];
    int     messageID;
        
    char topic[1024];
    snprintf( topic, sizeof topic, "%s/%s", publishTopic, "SETTINGS" );
    
    int length = snprintf( message, sizeof message,
            "{ \"topic\":\"%s\", \"dateTime\":\"%s\", \
\"batteryCapacity\":%0d, \
\"batteryType\":\"%s\", \
\"boostReconnectVoltage\":%0.2f, \
\"boostVoltage\":%0.2f, \
\"chargingLimitVoltage\":%0.2f, \
\"equalizationVoltage\":%0.2f, \
\"floatVoltage\":%0.2f, \
\"highVoltageDisconnect\":%0.2f, \
\"overVoltageReconnect\":%0.2f, \
\"tempCompensationCoeff\":%0.2f }",
    
        topic, getCurrentDateTime(),
        data->batteryCapacity,
        data->batteryType,
        data->boostReconnectVoltage,
        data->boostVoltage,
        data->chargingLimitVoltage,
        data->equalizationVoltage,
        data->floatVoltage,
        data->highVoltageDisconnect,
        data->overVoltageReconnect,
        data->tempCompensationCoeff );

    printf( "Handformed JSON [%s]\n\n", message );
    
    int result = mosquitto_publish( myMQTTInstance,
                        &messageID, 
                        topic,
                        length,
                        message,
                        QoS, 
                        0 );
    
    if (result != MOSQ_ERR_SUCCESS) {
        Logger_LogError( "Unable to publish the message. Mosquitto error code: %d\n", result );
    }    
}

// ----------------------------------------------------------------------------
void    MQTT_PublishStatisticalParameters (const char *controllerID, const StatisticalParameters_t *data)
{
    char    message[ 1024 ];
    int     messageID;
        
    
    char topic[1024];
    snprintf( topic, sizeof topic, "%s/%s", publishTopic, "STATISTICS" );
    
    int length = snprintf( message, sizeof message,
            "{ \"topic\":\"%s\", \"dateTime\":\"%s\", \
\"consumedEnergyToday\":%0.2f, \
\"consumedEnergyMonth\":%0.2f, \
\"consumedEnergyYear\":%0.2f, \
\"totalConsumedEnergy\":%0.2f, \
\"generatedEnergyToday\":%0.2f, \
\"generatedEnergyMonth\":%0.2f, \
\"generatedEnergyYear\":%0.2f, \
\"totalGeneratedEnergy\":%0.2f, \
\"minimumBatteryVoltageToday\":%0.2f, \
\"maximumBatteryVoltageToday\":%0.2f, \
\"minimumInputVoltageToday\":%0.2f, \
\"maximumInputVoltageToday\":%0.2f, \
\"CO2Reduction\":%0.1f, \
\"ambientTemp\":%0.1f, \
\"batteryCurrent\":%0.2f, \
\"batteryTemp\":%0.1f }",    
        topic, getCurrentDateTime(),
            data->consumedEnergyToday,
            data->consumedEnergyMonth,
            data->consumedEnergyYear,
            data->totalConsumedEnergy,
            data->generatedEnergyToday,
            data->generatedEnergyMonth,
            data->generatedEnergyYear,
            data->totalGeneratedEnergy,
            data->minimumBatteryVoltageToday,
            data->maximumBatteryVoltageToday,
            data->minimumInputVoltageToday,
            data->maximumInputVoltageToday,
            data->CO2Reduction,
            data->ambientTemp,
            data->batteryCurrent,
            data->batteryTemp );

    printf( "Handformed JSON [%s]\n\n", message );
    
    int result = mosquitto_publish( myMQTTInstance,
                        &messageID, 
                        topic,
                        length,
                        message,
                        QoS, 
                        0 );
    
    if (result != MOSQ_ERR_SUCCESS) {
        Logger_LogError( "Unable to publish the message. Mosquitto error code: %d\n", result );
    }    
}

// ----------------------------------------------------------------------------
void    MQTT_Teardown (void *aSystem)
{
    Logger_LogInfo( "MQTT_Teardown() - we're shutting down the MQTT pipe.\n" );
    // MQTT_Unsubscribe( aSystem );
    
    mosquitto_destroy( myMQTTInstance );
    mosquitto_lib_cleanup();
    MQTT_Connected = FALSE;
}


// ----------------------------------------------------------------------------
void    MQTT_Subscribe  (void *aSystem)
{
    Logger_LogError( "We are not expecting to receive any MQTT messages!\n" );
#if 0    
    int returnCode = mosquitto_subscribe( myMQTTInstance,
                        NULL,                   // message ID, not needed
                        subscriptionTopic,         // remember this is concatenated parent + subscribe
                        aSystem->mqtt.QoS );
  
    if (returnCode != MOSQ_ERR_SUCCESS)
        Logger_LogError( "Unable to subscribe to topic [%s], reason: %d\n", subscriptionTopic, returnCode );
#endif    
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

// -----------------------------------------------------------------------------
void    MQTT_SetLastWillAndTestament (void *aSystem) 
{
#if 0 
    Logger_LogInfo( "MQTT Setting Last Will And Testament\n" );
    
    /* From the documentation 
        mosquitto_will_set

            libmosq_EXPORT int mosquitto_will_set(	struct 	mosquitto 	*	mosq,
                const 	char 	*	topic,
                        int 		payloadlen,
                const 	void 	*	payload,
                        int 		qos,
                        bool 		retain	)

        Configure will information for a mosquitto instance.  By default, clients do not have a will.  This must be called before calling mosquitto_connect.
        Parameters
        mosq	a valid mosquitto instance.
        topic	the topic on which to publish the will.
        payloadlen	the size of the payload (bytes).  Valid values are between 0 and 268,435,455.
        payload	pointer to the data to send.  If payloadlen > 0 this must be a valid memory location.
        qos	integer value 0, 1 or 2 indicating the Quality of Service to be used for the will.
        retain	set to true to make the will a retained message.
        Returns
        MOSQ_ERR_SUCCESS	on success.
        MOSQ_ERR_INVAL	if the input parameters were invalid.
        MOSQ_ERR_NOMEM	if an out of memory condition occurred.
        MOSQ_ERR_PAYLOAD_SIZE	if payloadlen is too large.

    */


    //
    //  Let's create our custom, time based (so it's unique) ID first
//    char    timebasedID[ 16 ];
//    (void) make_UTC_TimeBasedID( timebasedID, 16 );
    
    
    
    //
    //  Payload should be JSON like: { "topic" : "CSCS/LWT", "ID" : "xx.xx", "system" : "aaa", "payload" : "Connection Lost" }
    static  char    *lwtPayload = "{ \"topic\" : \"CSCS/LWT\", \"ID\" : \"%s\", \"system\" : \"BARCODE SCANNER\", \"payload\" : \"Connection Lost\" } ";
    char            message[ 256 ];
    
    int len = snprintf( message, sizeof( message ), lwtPayload, timebasedID );
    
    
    int result = mosquitto_will_set( myMQTTInstance, 
                        "CSCS/LWT",
                        len, message,
                        0, false );
    
    if (result != MOSQ_ERR_SUCCESS) {
        Logger_LogError( "Unable to set the last will and testament. Mosquitto error code: %d\n", result );
    //  MOSQ_ERR_INVAL	if the input parameters were invalid.
    //  MOSQ_ERR_NOMEM	if an out of memory condition occurred.
    //  MOSQ_ERR_PAYLOAD_SIZE	if payloadlen is too large.)
    } 
#endif
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


// -----------------------------------------------------------------------------
static  char    currentDateTimeBuffer[ 80 ];
static
char    *getCurrentDateTime (void)
{
    //
    // Something quick and dirty... Fix this later - thread safe
    time_t  current_time;
    struct  tm  *tmPtr;
 
    memset( currentDateTimeBuffer, '\0', sizeof currentDateTimeBuffer );
    
    /* Obtain current time as seconds elapsed since the Epoch. */
    current_time = time( NULL );
    if (current_time > 0) {
        /* Convert to local time format. */
        tmPtr = localtime( &current_time );
 
        if (tmPtr != NULL) {
            strftime( currentDateTimeBuffer,
                    sizeof currentDateTimeBuffer,
                    "%FT%T%z",                           // ISO 8601 Format
                    tmPtr );
        }
    }
    
    return &currentDateTimeBuffer[ 0 ];
}
