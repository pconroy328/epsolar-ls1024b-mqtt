/*
 */

/* 
 * File:   ls1024b.h
 * Author: pconroy
 *
 * Created on June 13, 2018, 12:08 PM
 */

#ifndef LS1024B_H
#define LS1024B_H

#ifdef __cplusplus
extern "C" {
#endif

#include <modbus/modbus.h>
    
    
#define LANDSTAR_1024B_ID       0x01


//
//  I'm using the terms "RatedData", "RealTime Data" as they are used in the 
//  documentation I have from EPSolar
//
typedef struct RatedData {
    float   pvArrayRatedVoltage;
    float   pvArrayRatedCurrent;
    float   pvArrayRatedPower;
    float   batteryRatedVoltage;
    float   batteryRatedCurrent;
    float   batteryRatedPower;
    char    *chargingMode;
    float   ratedCurrentOfLoad;
} RatedData_t;


typedef struct  RealTimeData {
    float   pvArrayVoltage;
    float   pvArrayCurrent;
    float   pvArrayPower;
    float   batteryPower;
    float   loadVoltage;
    float   loadCurrent; 
    float   loadPower;
    float   batteryTemp;
    float   caseTemp;
    int     batterySOC;
    float   remoteBatteryTemperature;
    float   batteryRealRatedPower;
} RealTimeData_t;

typedef  struct  RealTimeStatus {
    int     batteryStatusValue;
    int     chargingStatusValue;
    int     dischargingStatusValue;

    char    *batteryStatusVoltage;
    char    *batteryStatusTemperature;
    char    *batteryInnerResistance;
    char    *batteryCorrectIdentification;
    
    char    *chargingStatus;
    char    *chargingInputVoltageStatus;
    int chargingMOSFETShort;
    int someMOSFETShort;
    int antiReverseMOSFETShort;
    int inputIsOverCurrent;
    int loadIsOverCurrent;
    int loadIsShort;
    int loadMOSFETIsShort;
    int pvInputIsShort;
    int chargingStatusNormal;
    int chargingStatusRunning;
    
    int dischargingStatusNormal;
    char    *dischargingInputVoltageStatus;
    char    *dischargingOutputPower;
    int dischargingShortCircuit;
    int unableToDischarge;
    int unableToStopDischarging;
    int outputVoltageAbnormal;
    int inputOverpressure;
    int highVoltageSideShort;
    int boostOverpressure;
    int outputOverpressure;
    int dischargingStatusRunning;
} RealTimeStatus_t;

typedef struct  Settings {
    char    *batteryType;
    int     batteryCapacity;
    float   tempCompensationCoeff;
    float   highVoltageDisconnect;
    float   chargingLimitVoltage;
    float   overVoltageReconnect;
    float   equalizationVoltage;
    float   boostVoltage;
    float   floatVoltage;
    float   boostReconnectVoltage;
    float   lowVoltageReconnect;
    float   underVoltageRecover;
    float   underVoltageWarning;
    float   lowVoltageDisconnect;
    float   dischargingLimitVoltage;
    char    *realtimeClock;
    float   batteryTempWarningUpperLimit;
    float   batteryTempWarningLowerLimit;
    float   controllerTempWarningUpperLimit;
    float   controllerTempWarningLowerLimit;
    float   daytimeThresholdVoltage;
    int     lightSignalStartupTime;
    float   lighttimeThresholdVoltage;
    int     lightSignalCloseDelayTime;
    int     localControllingModes;
    int     workingTimeLength1;
    int     workingTimeLength2;
} Settings_t;

typedef struct  StatisticalParameters {
    float   maximumInputVoltageToday;
    float   minimumInputVoltageToday;
    float   maximumBatteryVoltageToday;
    float   minimumBatteryVoltageToday;
    float   consumedEnergyToday;
    float   consumedEnergyMonth;
    float   consumedEnergyYear;
    float   totalConsumedEnergy;
    float   generatedEnergyToday;
    float   generatedEnergyMonth;
    float   generatedEnergyYear;
    float   totalGeneratedEnergy;
    float   batteryCurrent;
    float   batteryVoltage;
} StatisticalParameters_t;

typedef struct  UsefulData {
    //
    // Some of the data I think would be handy is scattered around the other
    //  structures. Let's cobble together something that combines a few of them
    
    //
    // First these from RealTimeData_t
    float   pvArrayVoltage;
    float   pvArrayCurrent;
    float   loadVoltage;
    float   loadCurrent; 
    float   batteryTemp;
    float   caseTemp;
    int     batterySOC;
    
    int     isNightTime;            // boolean
    int     caseTempTooHot;         //  boolean 
    
        
} UsefulData_t;


extern  void    getStatisticalParameters( modbus_t *ctx, StatisticalParameters_t *data );
extern  void    getRatedData( modbus_t *ctx, RatedData_t *data );
extern  void    getSettings( modbus_t *ctx, Settings_t *data );
extern  void    getRealTimeStatus( modbus_t *ctx, RealTimeStatus_t *data );
extern  void    getRealTimeData( modbus_t *ctx, RealTimeData_t *data );
extern  int     getChargingDeviceStatus( modbus_t *ctx);
extern  void    setChargingDeviceStatus( modbus_t *ctx, const int value);
extern  int     getOutputControlMode( modbus_t *ctx );
extern  void    setOutputControlMode( modbus_t *ctx, const int value );
extern  int     getManualLoadControlMode( modbus_t *ctx );
extern  void    setManualLoadControlMode( modbus_t *ctx, const int value );
extern  int     getDefaultLoadControlMode( modbus_t *ctx );
extern  void    setDefaultLoadControlMode( modbus_t *ctx, const int value );
extern  int     getEnableLoadTestMode( modbus_t *ctx );
extern  void    setEnableLoadTestMode( modbus_t *ctx, const int value );
extern  void    forceLoadOnOff( modbus_t *ctx, const int value );
extern  void    restoreSystemDefaults( modbus_t *ctx );
extern  void    clearEnergyGeneratingStatistics( modbus_t *ctx );
extern  int     getOverTemperatureInsideDevice( modbus_t *ctx );
extern  int     isNightTime( modbus_t *ctx );
extern  int     getBatteryStateOfCharge( modbus_t*ctx );
extern  float   getRemoteBatteryTemperature( modbus_t *ctx );
extern  float   getBatteryRealRatedPower( modbus_t *ctx );
extern  void    getRealtimeClock( modbus_t *ctx, int *seconds, int *minutes, int *hour, int *day, int *month, int *year );
extern  void    setRealtimeClock( modbus_t *ctx, const int seconds, const int minutes, const int hour, const int day, const int month, const int year );
extern  void    setRealtimeClockToNow( modbus_t *ctx );
extern  void    setBatteryType( modbus_t *ctx, int batteryTypeCode );
extern  void    setBatteryCapacity( modbus_t *ctx, int batteryCapacityAH );
extern  void    setHighVoltageDisconnect( modbus_t *ctx, float value );
extern  void    setLoadControlMode (modbus_t *ctx, int value);
extern  char    *getRealtimeClockStr (modbus_t *ctx, char *buffer, const int buffSize);
extern  char    *getCurrentDateTime( void );





static  char    *batteryTypeToString( uint16_t batteryType );
static  char    *chargingModeToString( uint16_t mode );
static  int     getCoilValue( modbus_t *ctx, const int coilNum, const char *description);
static  void    setCoilValue( modbus_t *ctx, const int coilNum, const int value, const char *description);
static  float   C2F( float tempC );
static  void    decodeBatteryStatusBits( RealTimeStatus_t *data, const int value );
static  void    decodeChargingStatusBits( RealTimeStatus_t *data, const int value );
static  void    decodeDischargingStatusBits( RealTimeStatus_t *data, const int value );
static  int     setFloatSettingParameter( modbus_t *ctx, const int registerAddress, const float floatValue );
static  int     setIntSettingParameter( modbus_t *ctx, const int registerAddress, const int intValue );


#ifdef __cplusplus
}
#endif

#endif /* LS1024B_H */

