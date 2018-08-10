/*
 * The goal is to crate a JSON message that has this structure
 * 
 * {
	"dateTime" : "xx",
	"controllerDateTime" : "xx",
	"isNightTime" : false,
	"batterySOC" : 100,
	"pvArrayVoltage" : 13.3,
	"pvArrayCurrent" : 4.3,
	"loadVoltage" : 13.3,
	"loadCurrent" : 4.3,
 
	"temperatures"  : {
		"unit" : "Farenheit",
		"battery" : 22.2,
		"case" : 23.3,
		"remoteSensor" : 24.4
	},

	"batteryStatus" : {
        	"voltage" : "Normal",
		"temperature" : "Normal",
		"innerResistance" : "Normal",
		"identification" : "Correct"
	},

	"chargingStatus" : {
		"status" : "Floating",
		"isNormal" : true,
		"isRunning" : true,
		"inputVoltage" : "Normal",
		"MOSFETShort" : false,
		"someMOSFETShort" : false,
		"antiReverseMOSFETShort" : false,
		"inputIsOverCurrent" : false,
		"loadIsOverCurrent" : false,
		"loadIsShort" : false,
		"loadMOSFETIsShort" : false,
		"pvInputIsShort" : false
	},

	"dischargingStatus" : {
		"isNormal" : true,
		"isRunning" : true,
		"inputVoltageStatus" : "Normal",
		"outputPower" : "Light Load",
		"shortCircuit" : false,
		"unableToDischarge" : false,
		"unableToStopDischarging" : false,
		"outputVoltageAbnormal" : false,
		"inputOverpressure" : false,
		"highVoltageSideShort" : false,
		"boostOverpressure" : false,
		"outputOverpressure" : false
	},

	"settings" : {
		"batteryType" : "Sealed",
		"batteryCapacity" : 5,
	    	"tempCompensationCoeff" : 11.1,
		"highVoltageDisconnect" : 11.1,
		"chargingLimitVoltage" : 11.1,
		"overVoltageReconnect" : 11.1,
		"equalizationVoltage" : 11.1,
		"boostVoltage" : 11.1,
		"floatVoltage" : 11.1,
		"boostReconnectVoltage" : 11.1,
		"lowVoltageReconnect" : 11.1,
		"underVoltageRecover" : 11.1,
		"underVoltageWarning" : 11.1,
		"lowVoltageDisconnect" : 11.1,
		"dischargingLimitVoltage" : 11.1,
	    	"batteryTempWarningUpperLimit" : 11.1,
		"batteryTempWarningLowerLimit" : 11.1,
		"controllerTempWarningUpperLimit" : 11.1,
		"controllerTempWarningLowerLimit" : 11.1,  
		"daytimeThresholdVoltage" : 11.1,
		"lightSignalStartupTime" : 1,
		"lighttimeThresholdVoltage" : 11.1,
		"lightSignalCloseDelayTime" : 2,
		"localControllingModes" : 3,
		"workingTimeLength1" : 4, 
		"workingTimeLength2" : 5
	},

	"statistics" : {
		"maximumInputVoltageToday" : 0.2,
		"minimumInputVoltageToday" : 1.2,
		"maximumBatteryVoltageToday" : 2.2,
		"minimumBatteryVoltageToday" : 3.2,

		"consumedEnergyToday" :  100.0,
		"consumedEnergyMonth" :  100.0,
		"consumedEnergyYear" :  100.0,
		"totalConsumedEnergy" :  100.0,
		"generatedEnergyToday" :  100.0,
		"generatedEnergyMonth" :  100.0,
		"generatedEnergyYear" :  100.0,
		"totalGeneratedEnergy" :  100.0,
    
		"batteryVoltage" : 12.2,
		"batteryCurrent" : 4.3
	}
    }
 * 
 */

#include <stdio.h>
#include "cJSON.h"
#include "ls1024b.h"

// -----------------------------------------------------------------------------
char *createJSONMessage (modbus_t *ctx, const char *topic, const RatedData_t *ratedData, const RealTimeData_t *rtData, const RealTimeStatus_t *rtStatusData, const Settings_t *setData, const StatisticalParameters_t *stats)
{
    cJSON *message = cJSON_CreateObject();
    cJSON_AddStringToObject( message, "topic", topic );
    cJSON_AddStringToObject( message, "version", "1.2" );
    
    cJSON_AddStringToObject( message, "dateTime", getCurrentDateTime() );
    cJSON_AddStringToObject( message, "controllerDateTime", setData->realtimeClock );
    cJSON_AddBoolToObject( message, "isNightTime", isNightTime( ctx ) );
    cJSON_AddNumberToObject( message, "batterySOC", rtData->batterySOC );
    cJSON_AddNumberToObject( message, "pvArrayVoltage", rtData->pvArrayVoltage );
    cJSON_AddNumberToObject( message, "pvArrayCurrent", rtData->pvArrayCurrent );
    cJSON_AddNumberToObject( message, "loadVoltage", rtData->loadVoltage );
    cJSON_AddNumberToObject( message, "loadCurrent", rtData->loadCurrent );

    //
    //  Temperatures - nested object
    cJSON *temperatures = cJSON_CreateObject();
    cJSON_AddStringToObject( temperatures, "unit", "Farenheit" );
    cJSON_AddNumberToObject( temperatures, "battery", rtData->batteryTemp );
    cJSON_AddNumberToObject( temperatures, "case", rtData->caseTemp );
    cJSON_AddNumberToObject( temperatures, "remoteSensor", rtData->remoteBatteryTemperature );
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
    cJSON_AddNumberToObject( settings, "tempCompensationCoeff", setData->tempCompensationCoeff );
    cJSON_AddNumberToObject( settings, "highVoltageDisconnect", setData->highVoltageDisconnect );
    cJSON_AddNumberToObject( settings, "chargingLimitVoltage", setData->chargingLimitVoltage );
    cJSON_AddNumberToObject( settings, "overVoltageReconnect", setData->overVoltageReconnect );
    cJSON_AddNumberToObject( settings, "equalizationVoltage", setData->equalizationVoltage );
    cJSON_AddNumberToObject( settings, "boostVoltage", setData->boostVoltage );
    cJSON_AddNumberToObject( settings, "floatVoltage", setData->floatVoltage );
    cJSON_AddNumberToObject( settings, "boostReconnectVoltage", setData->boostReconnectVoltage );
    cJSON_AddNumberToObject( settings, "lowVoltageReconnect", setData->lowVoltageReconnect );
    cJSON_AddNumberToObject( settings, "underVoltageRecover", setData->underVoltageRecover );
    cJSON_AddNumberToObject( settings, "underVoltageWarning", setData->underVoltageWarning );
    cJSON_AddNumberToObject( settings, "lowVoltageDisconnect", setData->lowVoltageDisconnect );
    cJSON_AddNumberToObject( settings, "dischargingLimitVoltage", setData->dischargingLimitVoltage );
    cJSON_AddNumberToObject( settings, "batteryTempWarningUpperLimit", setData->batteryTempWarningUpperLimit );
    cJSON_AddNumberToObject( settings, "batteryTempWarningLowerLimit", setData->batteryTempWarningLowerLimit );
 
    cJSON_AddNumberToObject( settings, "controllerInnerTempUpperLimit", setData->controllerInnerTempUpperLimit );
    cJSON_AddNumberToObject( settings, "controllerInnerTempUpperLimitRecover", setData->controllerInnerTempUpperLimitRecover );
    
    cJSON_AddNumberToObject( settings, "powerComponentTempUpperLimit", setData->powerComponentTempUpperLimit );
    cJSON_AddNumberToObject( settings, "powerComponentTempUpperLimitRecover", setData->powerComponentTempUpperLimitRecover );
    cJSON_AddNumberToObject( settings, "lineImpedence", setData->lineImpedence );
    
    cJSON_AddNumberToObject( settings, "daytimeThresholdVoltage", setData->daytimeThresholdVoltage );
    cJSON_AddNumberToObject( settings, "lightSignalStartupTime", setData->lightSignalStartupTime );
    cJSON_AddNumberToObject( settings, "lighttimeThresholdVoltage", setData->lighttimeThresholdVoltage );
    cJSON_AddNumberToObject( settings, "lightSignalCloseDelayTime", setData->lightSignalCloseDelayTime );
    cJSON_AddNumberToObject( settings, "localControllingModes", setData->localControllingModes );
    cJSON_AddNumberToObject( settings, "workingTimeLength1", setData->workingTimeLength1 );
    cJSON_AddNumberToObject( settings, "workingTimeLength2", setData->workingTimeLength2 );
    
    
    char    dtBuffer[ 32 ];
    snprintf( dtBuffer, sizeof dtBuffer, "%02d:%02d:%02d", setData->turnOnTiming1_hours, setData->turnOnTiming1_minutes, setData->turnOnTiming1_seconds );
    cJSON_AddStringToObject( settings, "turnOnTiming1", dtBuffer );
            
    snprintf( dtBuffer, sizeof dtBuffer, "%02d:%02d:%02d", setData->turnOffTiming1_hours, setData->turnOffTiming1_minutes, setData->turnOffTiming1_seconds );
    cJSON_AddStringToObject( settings, "turnOffTiming1", dtBuffer );
            
    snprintf( dtBuffer, sizeof dtBuffer, "%02d:%02d:%02d", setData->turnOnTiming2_hours, setData->turnOnTiming2_minutes, setData->turnOnTiming2_seconds );
    cJSON_AddStringToObject( settings, "turnOnTiming2", dtBuffer );
            
    snprintf( dtBuffer, sizeof dtBuffer, "%02d:%02d:%02d", setData->turnOffTiming2_hours, setData->turnOffTiming2_minutes, setData->turnOffTiming2_seconds );
    cJSON_AddStringToObject( settings, "turnOffTiming2", dtBuffer );

    //
    //  Set default values of the whole night - top 8 bits are hours, bottom 8 are minutes
    snprintf( dtBuffer, sizeof dtBuffer, "%02d:%02d", ((setData->lengthOfNight & 0xFF00) >> 8), (setData->lengthOfNight & 0x00FF) ); 
    cJSON_AddStringToObject( settings, "lengthOfNight", dtBuffer );
    
    
    cJSON_AddStringToObject( settings, "batteryRatedVoltageCode", (setData->batteryRatedVoltageCode == 0) ? "Auto" : ( (setData->batteryRatedVoltageCode == 1) ? "12V" : "24V") );
    
    
    // cJSON_AddNumberToObject( settings, "loadTimingControlSelection", setData->loadTimingControlSelection );
    cJSON_AddStringToObject( settings, "loadTimingControlSelection", (setData->batteryRatedVoltageCode == 0) ? "1 Timer" : "2 Timers" );
    
    
    // cJSON_AddNumberToObject( settings, "defaultLoadOnOffManualMode", setData->defaultLoadOnOffManualMode );
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
    cJSON_AddNumberToObject( statistics, "maximumInputVoltageToday", stats->maximumInputVoltageToday );
    cJSON_AddNumberToObject( statistics, "minimumInputVoltageToday", stats->minimumInputVoltageToday );
    cJSON_AddNumberToObject( statistics, "maximumBatteryVoltageToday", stats->maximumBatteryVoltageToday );
    cJSON_AddNumberToObject( statistics, "minimumBatteryVoltageToday", stats->minimumBatteryVoltageToday );
    cJSON_AddNumberToObject( statistics, "consumedEnergyToday", stats->consumedEnergyToday );
    cJSON_AddNumberToObject( statistics, "consumedEnergyMonth", stats->consumedEnergyMonth );
    cJSON_AddNumberToObject( statistics, "consumedEnergyYear", stats->consumedEnergyYear );
    cJSON_AddNumberToObject( statistics, "totalConsumedEnergy", stats->totalConsumedEnergy );
    cJSON_AddNumberToObject( statistics, "generatedEnergyToday", stats->generatedEnergyToday );
    cJSON_AddNumberToObject( statistics, "generatedEnergyMonth", stats->generatedEnergyMonth );
    cJSON_AddNumberToObject( statistics, "generatedEnergyYear", stats->generatedEnergyYear );
    cJSON_AddNumberToObject( statistics, "totalGeneratedEnergy", stats->totalGeneratedEnergy );
    cJSON_AddNumberToObject( statistics, "batteryVoltage", stats->batteryVoltage );
    cJSON_AddNumberToObject( statistics, "batteryCurrent", stats->batteryCurrent );
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