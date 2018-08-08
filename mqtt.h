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
extern  void    MQTT_Initialize( const char *controllerID, const char *brokerHost );
extern  void    MQTT_Teardown( );
extern  void    *MQTT_MainLoop( void *threadArgs );
extern  void    MQTT_Subscribe( const char *topic, const int QoS );
extern  void    MQTT_Unsubscribe( void *aSystem );
extern  int     MQTT_SendReceive( void *aSystem );
extern  int     MQTT_HandleError( void *aSystem, int errorCode );
extern  void    MQTT_PublishData( const char *topic, const char *data, const int length );

extern  void    MQTT_SetLastWillAndTestament( void *aSystem );




#ifdef	__cplusplus
}
#endif

#endif	/* MQTT_H */

