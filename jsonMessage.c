/*
 * File:    jsonMessage.c
 * author:  patrick conroy
 * 
 * The goal is to crate a JSON message that has a structure like this:
 * 
 * {
	"dateTime" : "xx",
	"controllerDateTime" : "xx",
	"isNightTime" : false,
	"batterySOC" : 100,
	"loadCurrent" : 4.3,
	"temperatures"  : {
		"battery" : 22.2,
		"case" : 23.3,
		"remoteSensor" : 24.4
	},

	"batteryStatus" : {
	},

	"chargingStatus" : {
	},

	"settings" : {
		"batteryType" : "Sealed",
 		"localControllingModes" : 3,
		"workingTimeLength1" : 4, 
		"workingTimeLength2" : 5
	},

	"statistics" : {    
		"batteryVoltage" : 12.2,
		"batteryCurrent" : 4.3
	}
    }
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cjson/cJSON.h"
#include "ls1024b.h"

extern char    *getCurrentDateTime( void );

//
//  Quickie Macros to control Floating Point Precision
#define FP21P(x) ( (((int)((x) * 10 + .5))/10.0) )                  // Floating Point to 1 point
#define FP22P(x) ( (((int)((x) * 100 + .5))/100.0) )                // Floating Point to 2 points
#define FP23P(x) ( (((int)((x) * 1000 + .5))/1000.0) )              // Floating Point to 3 points
#define FP24P(x) ( (((int)((x) * 10000 + .5))/10000.0) )            // Floating Point to 4 points




// -----------------------------------------------------------------------------
static  char    currentDateTimeBuffer[ 80 ];
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

// -----------------------------------------------------------------------------
char *createJSONMessage (modbus_t *ctx, const char *topic, const RatedData_t *ratedData, 
                        const RealTimeData_t *rtData, const RealTimeStatus_t *rtStatusData, 
                        const Settings_t *setData, const StatisticalParameters_t *stats)
{
    //
    //  Dave Gamble's C library to create JSON
    cJSON *message = cJSON_CreateObject();

    cJSON_AddStringToObject( message, "topic", topic );
    cJSON_AddStringToObject( message, "version", "2.0" );
    
    
    //
    //  cJSON uses a "%1.15g" format for number formatting which means we can get some
    //  very large FP numbers in the output.  Let's round and truncate before we have cJSON
    //  format the numbers.
    //
    cJSON_AddStringToObject( message, "dateTime", getCurrentDateTime() );
    cJSON_AddStringToObject( message, "controllerDateTime", setData->realtimeClock );
    cJSON_AddBoolToObject( message, "isNightTime", isNightTime( ctx ) );
    cJSON_AddNumberToObject( message, "batterySOC", rtData->batterySOC );
    cJSON_AddNumberToObject( message, "pvArrayVoltage", FP22P( rtData->pvArrayVoltage ) );
    cJSON_AddNumberToObject( message, "pvArrayCurrent", FP22P( rtData->pvArrayCurrent ) );
    cJSON_AddNumberToObject( message, "loadVoltage", FP22P( rtData->loadVoltage ) );
    cJSON_AddNumberToObject( message, "loadCurrent", FP22P( rtData->loadCurrent ) );

    //
    //  Temperatures - nested object
    cJSON *temperatures = cJSON_CreateObject();
    cJSON_AddStringToObject( temperatures, "unit", "Fahrenheit" );
    cJSON_AddNumberToObject( temperatures, "battery", FP21P( rtData->batteryTemp ) );
    cJSON_AddNumberToObject( temperatures, "case", FP21P( rtData->caseTemp ) );
    cJSON_AddNumberToObject( temperatures, "remoteSensor", FP21P( rtData->remoteBatteryTemperature ) ); 
    cJSON_AddItemToObject( message, "temperatures", temperatures );
    
    //
    //  batteryStatus - nested object
    cJSON *batteryStatus = cJSON_CreateObject();
    cJSON_AddStringToObject( batteryStatus, "voltage", rtStatusData->batteryStatusVoltage );
    cJSON_AddStringToObject( batteryStatus, "temperature", rtStatusData->batteryStatusTemperature );
    cJSON_AddStringToObject( batteryStatus, "innerResistance", rtStatusData->batteryInnerResistance );
    cJSON_AddStringToObject( batteryStatus, "identification", rtStatusData->batteryCorrectIdentification );
    cJSON_AddItemToObject( message, "batteryStatus", batteryStatus );

    //
    //  chargingStatus - nested object
    cJSON *chargingStatus = cJSON_CreateObject();
    cJSON_AddStringToObject( chargingStatus, "status", rtStatusData->chargingStatus );
    cJSON_AddBoolToObject( chargingStatus, "isNormal", rtStatusData->chargingStatusNormal );
    cJSON_AddBoolToObject( chargingStatus, "isRunning", rtStatusData->chargingStatusRunning );
    cJSON_AddStringToObject( chargingStatus, "inputVoltage", rtStatusData->chargingInputVoltageStatus );
    cJSON_AddBoolToObject( chargingStatus, "MOSFETShort", rtStatusData->chargingMOSFETShort );
    cJSON_AddBoolToObject( chargingStatus, "someMOSFETShort",rtStatusData->someMOSFETShort );
    cJSON_AddBoolToObject( chargingStatus, "antiReverseMOSFETShort", rtStatusData->antiReverseMOSFETShort );
    cJSON_AddBoolToObject( chargingStatus, "inputIsOverCurrent", rtStatusData->inputIsOverCurrent );
    cJSON_AddBoolToObject( chargingStatus, "inputIsOverPressure", rtStatusData->inputOverpressure );
    cJSON_AddBoolToObject( chargingStatus, "loadIsOverCurrent", rtStatusData->loadIsOverCurrent );
    cJSON_AddBoolToObject( chargingStatus, "loadIsShort", rtStatusData->loadIsShort );
    cJSON_AddBoolToObject( chargingStatus, "loadMOSFETIsShort", rtStatusData->loadMOSFETIsShort );
    cJSON_AddBoolToObject( chargingStatus, "pvInputIsShort", rtStatusData->pvInputIsShort );
    cJSON_AddItemToObject( message, "chargingStatus", chargingStatus );
        
    //
    //  dischargingStatus - nested object
    cJSON *dischargingStatus = cJSON_CreateObject();
    cJSON_AddBoolToObject( dischargingStatus, "isNormal", rtStatusData->dischargingStatusNormal );
    cJSON_AddBoolToObject( dischargingStatus, "isRunning", rtStatusData->dischargingStatusRunning );
    cJSON_AddStringToObject( dischargingStatus, "inputVoltageStatus", rtStatusData->dischargingInputVoltageStatus );
    cJSON_AddStringToObject( dischargingStatus, "outputPower", rtStatusData->dischargingOutputPower );
    cJSON_AddBoolToObject( dischargingStatus, "shortCircuit", rtStatusData->dischargingShortCircuit );
    cJSON_AddBoolToObject( dischargingStatus, "unableToDischarge", rtStatusData->unableToDischarge );
    cJSON_AddBoolToObject( dischargingStatus, "unableToStopDischarging", rtStatusData->unableToStopDischarging );
    cJSON_AddBoolToObject( dischargingStatus, "outputVoltageAbnormal", rtStatusData->outputVoltageAbnormal );
    cJSON_AddBoolToObject( dischargingStatus, "inputOverpressure", rtStatusData->outputOverpressure );
    cJSON_AddBoolToObject( dischargingStatus, "highVoltageSideShort", rtStatusData->highVoltageSideShort );
    cJSON_AddBoolToObject( dischargingStatus, "boostOverpressure", rtStatusData->boostOverpressure );
    cJSON_AddBoolToObject( dischargingStatus, "outputOverpressure", rtStatusData->outputOverpressure );
    cJSON_AddItemToObject( message, "dischargingStatus", dischargingStatus );

    //
    //  settings - nested object
    cJSON *settings = cJSON_CreateObject();
    cJSON_AddStringToObject( settings, "batteryType", setData->batteryType );
    cJSON_AddNumberToObject( settings, "batteryCapacity", setData->batteryCapacity );
    cJSON_AddNumberToObject( settings, "tempCompensationCoeff", FP21P( setData->tempCompensationCoeff ) );
    
    cJSON_AddNumberToObject( settings, "highVoltageDisconnect", FP21P( setData->highVoltageDisconnect ) );
    cJSON_AddNumberToObject( settings, "chargingLimitVoltage", FP21P( setData->chargingLimitVoltage ) );
    cJSON_AddNumberToObject( settings, "overVoltageReconnect", FP21P( setData->overVoltageReconnect ) );
    
    cJSON_AddNumberToObject( settings, "equalizationVoltage", FP21P( setData->equalizationVoltage ) );
    cJSON_AddNumberToObject( settings, "boostVoltage", FP21P( setData->boostVoltage ) );
    cJSON_AddNumberToObject( settings, "floatVoltage", FP21P( setData->floatVoltage ) );
    
    cJSON_AddNumberToObject( settings, "boostReconnectVoltage", FP21P( setData->boostReconnectVoltage ) );
    cJSON_AddNumberToObject( settings, "lowVoltageReconnect", FP21P( setData->lowVoltageReconnect ) );
    cJSON_AddNumberToObject( settings, "underVoltageRecover", FP21P( setData->underVoltageRecover ) );
    cJSON_AddNumberToObject( settings, "underVoltageWarning", FP21P( setData->underVoltageWarning ) );
    cJSON_AddNumberToObject( settings, "lowVoltageDisconnect", FP21P( setData->lowVoltageDisconnect ) );
    
    cJSON_AddNumberToObject( settings, "dischargingLimitVoltage", FP21P( setData->dischargingLimitVoltage ) );
    //cJSON_AddNumberToObject( settings, "equalizationChargingCycle", setData->equalizationChargingCycle );
    
    cJSON_AddNumberToObject( settings, "batteryTempWarningUpperLimit", FP21P( setData->batteryTempWarningUpperLimit ) );
    cJSON_AddNumberToObject( settings, "batteryTempWarningLowerLimit", FP21P( setData->batteryTempWarningLowerLimit ) );
 
    cJSON_AddNumberToObject( settings, "controllerInnerTempUpperLimit", FP21P( setData->controllerInnerTempUpperLimit ) );
    cJSON_AddNumberToObject( settings, "controllerInnerTempUpperLimitRecover", FP21P( setData->controllerInnerTempUpperLimitRecover ) );
    
    cJSON_AddNumberToObject( settings, "powerComponentTempUpperLimit", FP21P( setData->powerComponentTempUpperLimit ) );
    cJSON_AddNumberToObject( settings, "powerComponentTempUpperLimitRecover", FP21P( setData->powerComponentTempUpperLimitRecover ) );
    //cJSON_AddNumberToObject( settings, "lineImpedence", setData->lineImpedence ) );
    
    cJSON_AddNumberToObject( settings, "daytimeThresholdVoltage", FP21P( setData->daytimeThresholdVoltage ) );
    cJSON_AddNumberToObject( settings, "lightSignalStartupTime", setData->lightSignalStartupTime );
    cJSON_AddNumberToObject( settings, "lighttimeThresholdVoltage", FP21P( setData->lighttimeThresholdVoltage ) );
    cJSON_AddNumberToObject( settings, "lightSignalCloseDelayTime", setData->lightSignalCloseDelayTime );
    cJSON_AddNumberToObject( settings, "localControllingModes", setData->localControllingModes );


    char    dtBuffer[ 32 ];
    snprintf( dtBuffer, sizeof dtBuffer, "%02d:%02d", (setData->workingTimeLength1 >> 8), (setData->workingTimeLength1 & 0XFF) );
    cJSON_AddStringToObject( settings, "workingTimeLength1", dtBuffer );

    snprintf( dtBuffer, sizeof dtBuffer, "%02d:%02d", (setData->workingTimeLength2 >> 8), (setData->workingTimeLength2 & 0XFF) );
    cJSON_AddStringToObject( settings, "workingTimeLength2", dtBuffer );
    
    
    snprintf( dtBuffer, sizeof dtBuffer, "%02d:%02d:%02d", setData->turnOnTiming1_hours, setData->turnOnTiming1_minutes, setData->turnOnTiming1_seconds );
    cJSON_AddStringToObject( settings, "turnOnTiming1", dtBuffer );
            
    snprintf( dtBuffer, sizeof dtBuffer, "%02d:%02d:%02d", setData->turnOffTiming1_hours, setData->turnOffTiming1_minutes, setData->turnOffTiming1_seconds );
    cJSON_AddStringToObject( settings, "turnOffTiming1", dtBuffer );
            
    snprintf( dtBuffer, sizeof dtBuffer, "%02d:%02d:%02d", setData->turnOnTiming2_hours, setData->turnOnTiming2_minutes, setData->turnOnTiming2_seconds );
    cJSON_AddStringToObject( settings, "turnOnTiming2", dtBuffer );
            
    snprintf( dtBuffer, sizeof dtBuffer, "%02d:%02d:%02d", setData->turnOffTiming2_hours, setData->turnOffTiming2_minutes, setData->turnOffTiming2_seconds );
    cJSON_AddStringToObject( settings, "turnOffTiming2", dtBuffer );

    snprintf( dtBuffer, sizeof dtBuffer, "%02d:%02d", ((setData->lengthOfNight & 0xFF00) >> 8), (setData->lengthOfNight & 0x00FF) ); 
    cJSON_AddStringToObject( settings, "lengthOfNight", dtBuffer );
    
    

    cJSON_AddStringToObject( settings, "batteryRatedVoltageCode", (setData->batteryRatedVoltageCode == 0) ? "Auto" : ( (setData->batteryRatedVoltageCode == 1) ? "12V" : "24V") );
    cJSON_AddStringToObject( settings, "loadTimingControlSelection", (setData->batteryRatedVoltageCode == 0) ? "1 Timer" : "2 Timers" );
    cJSON_AddStringToObject( settings, "defaultLoadOnOffManualMode", (setData->batteryRatedVoltageCode == 0) ? "Off" : "On" );
    
    cJSON_AddNumberToObject( settings, "equalizeDuration", setData->equalizeDuration );
    cJSON_AddNumberToObject( settings, "boostDuration", setData->boostDuration );
    cJSON_AddNumberToObject( settings, "dischargingPercentage", setData->dischargingPercentage );
    cJSON_AddNumberToObject( settings, "chargingPercentage", setData->chargingPercentage );
    cJSON_AddNumberToObject( settings, "batteryManagementMode", setData->batteryManagementMode );
     
    cJSON_AddItemToObject( message, "settings", settings );

    //
    //  statistics - nested object
    cJSON *statistics = cJSON_CreateObject();
    cJSON_AddNumberToObject( statistics, "maximumInputVoltageToday", FP22P( stats->maximumInputVoltageToday ) );
    cJSON_AddNumberToObject( statistics, "minimumInputVoltageToday", FP22P( stats->minimumInputVoltageToday ) );
    cJSON_AddNumberToObject( statistics, "maximumBatteryVoltageToday", FP22P( stats->maximumBatteryVoltageToday ) );
    cJSON_AddNumberToObject( statistics, "minimumBatteryVoltageToday", FP22P( stats->minimumBatteryVoltageToday ) );
    cJSON_AddNumberToObject( statistics, "consumedEnergyToday", FP22P( stats->consumedEnergyToday ) );
    cJSON_AddNumberToObject( statistics, "consumedEnergyMonth", FP22P( stats->consumedEnergyMonth ) );
    cJSON_AddNumberToObject( statistics, "consumedEnergyYear", FP22P( stats->consumedEnergyYear ) );
    cJSON_AddNumberToObject( statistics, "totalConsumedEnergy", FP22P( stats->totalConsumedEnergy ) );
    cJSON_AddNumberToObject( statistics, "generatedEnergyToday", FP22P( stats->generatedEnergyToday ) );
    cJSON_AddNumberToObject( statistics, "generatedEnergyMonth", FP22P( stats->generatedEnergyMonth ) );
    cJSON_AddNumberToObject( statistics, "generatedEnergyYear", FP22P( stats->generatedEnergyYear ) );
    cJSON_AddNumberToObject( statistics, "totalGeneratedEnergy", FP22P( stats->totalGeneratedEnergy ) );
    cJSON_AddNumberToObject( statistics, "batteryVoltage", FP22P( stats->batteryVoltage ) );
    cJSON_AddNumberToObject( statistics, "batteryCurrent", FP21P( stats->batteryCurrent ) );
    cJSON_AddItemToObject( message, "statistics", statistics );

    //
    //  From the cJSON notes: Important: If you have added an item to an array 
    //  or an object already, you mustn't delete it with cJSON_Delete. Adding 
    //  it to an array or object transfers its ownership so that when that 
    //  array or object is deleted, it gets deleted as well.
    char *string = cJSON_Print( message );
    cJSON_Delete( message );
    
    return string;
}
