/* 
 * File:   mqtt.h
 * Author: patrick.conroy
 *
 * Created on April 2, 2014, 7:41 PM
 */

#ifndef MQTT_H
#define	MQTT_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <mosquitto.h>
#include "ls1024b.h"
    
#define MQTT_NOT_CONNECTED      (-1)
    
    
extern  void    MQTT_SetDefaults( const char *controllerID );
extern  void    MQTT_Initialize( const char *clientID, const char *controllerID, const char *brokerHost );
extern  void    MQTT_Teardown( void *aSystem );
extern  void    *MQTT_MainLoop( void *threadArgs );
extern  void    MQTT_Subscribe( void *aSystem );
extern  void    MQTT_Unsubscribe( void *aSystem );
extern  int     MQTT_SendReceive( void *aSystem );
extern  int     MQTT_HandleError( void *aSystem, int errorCode );
extern  void    MQTT_PublishRatedData( const char *controllerID, const RatedData_t *data );
extern  void    MQTT_PublishRealTimeData( const char *controllerID, const RealTimeData_t *data );
extern  void    MQTT_PublishRealTimeStatus( const char *controllerID, const RealTimeStatus_t *data );
extern  void    MQTT_PublishSettings( const char *controllerID, const Settings_t *data );
extern  void    MQTT_PublishStatisticalParameters( const char *controllerID, const StatisticalParameters_t *data );


extern  void    MQTT_SetLastWillAndTestament( void *aSystem );




#ifdef	__cplusplus
}
#endif

#endif	/* MQTT_H */

