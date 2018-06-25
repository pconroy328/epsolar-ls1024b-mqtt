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
} RatedData_t;


typedef struct  RealTimeData {
    float   pvArrayVoltage;
    float   pvArrayCurrent;
    float   pvArrayPower;
    float   batteryVoltage;
    float   batteryCurrent;
    float   batteryPower;
    float   loadVoltage;
    float   loadCurrent; 
    float   loadPower;
    float   batteryTemp;
    float   caseTemp;
    float   componentsTemp ;
} RealTimeData_t;

typedef  struct  RealTimeStatus {
    int     batteryStatus;
    int     chargingStatus;
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
    float   CO2Reduction;
    float   batteryCurrent;
    float   batteryTemp;
    float   ambientTemp;    
} StatisticalParameters_t;




void    getStatisticalParameters( modbus_t *ctx, StatisticalParameters_t *data );
void    getRatedData( modbus_t *ctx, RatedData_t *data );
void    getSettings( modbus_t *ctx, Settings_t *data );
void    getRealTimeStatus( modbus_t *ctx, RealTimeStatus_t *data );
void    getRealTimeData( modbus_t *ctx, RealTimeData_t *data );
int     getChargingDeviceStatus( modbus_t *ctx);
void    setChargingDeviceStatus( modbus_t *ctx, const int value);
int     getOutputControlMode( modbus_t *ctx );
void    setOutputControlMode( modbus_t *ctx, const int value );
int     getManualLoadControlMode( modbus_t *ctx );
void    setManualLoadControlMode( modbus_t *ctx, const int value );
int     getDefaultLoadControlMode( modbus_t *ctx );
void    setDefaultLoadControlMode( modbus_t *ctx, const int value );
int     getEnableLoadTestMode( modbus_t *ctx );
void    setEnableLoadTestMode( modbus_t *ctx, const int value );
void    forceLoadOnOff( modbus_t *ctx, const int value );
void    restoreSystemDefaults( modbus_t *ctx );
void    clearEnergyGeneratingStatistics( modbus_t *ctx );
int     getOverTemperatureInsideDevice( modbus_t *ctx );
int     isNightTime( modbus_t *ctx );
int     getBatteryStateOfCharge( modbus_t*ctx );
float   getRemoteBatteryTemp( modbus_t *ctx );




static  char    *batteryTypeToString( uint16_t batteryType );
static  char    *chargingModeToString( uint16_t mode );
static  int     getCoilValue( modbus_t *ctx, const int coilNum, const char *description);
static  void    setCoilValue( modbus_t *ctx, const int coilNum, int value, const char *description);
static  float   C2F( float tempC );

#ifdef __cplusplus
}
#endif

#endif /* LS1024B_H */

