#include <stdio.h>
#include <errno.h>
#include <modbus/modbus.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "ls1024b.h"
#include "logger.h"

static
int   int_read_input_register ( modbus_t *ctx,
                                    const int registerAddress,
                                    const int numBytes,
                                    const char *description,
                                    const int badReadValue );
static
float   float_read_input_register ( modbus_t *ctx,
                                    const int registerAddress,
                                    const int numBytes,
                                    const char *description,
                                    const float badReadValue );


static
int   int_read_register ( modbus_t *ctx,
                                    const int registerAddress,
                                    const int numBytes,
                                    const char *description,
                                    const int badReadValue );
static
float   float_read_register ( modbus_t *ctx,
                                    const int registerAddress,
                                    const int numBytes,
                                    const char *description,
                                    const float badReadValue );


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



//
//
// These functions are named after the sections of the EPSOLAR documentation

// -----------------------------------------------------------------------------
void    getRatedData (modbus_t *ctx, RatedData_t *data)
{
   
    data->pvArrayRatedVoltage   = float_read_input_register( ctx, 0x3000, 1, "PV Array Rated Voltage", -1.0 );
    data->pvArrayRatedCurrent   = float_read_input_register( ctx, 0x3001, 1, "PV Array Rated Current", -1.0 );
    data->pvArrayRatedPower     = float_read_input_register( ctx, 0x3002, 2, "PV Array Rated Power", -1.0 );

    data->batteryRatedVoltage   = float_read_input_register( ctx, 0x3004, 1, "Battery Rated Voltage", -1.0 );
    data->batteryRatedCurrent   = float_read_input_register( ctx, 0x3005, 1, "Battery Rated Current", -1.0 );
    data->batteryRatedPower     = float_read_input_register( ctx, 0x3006, 2, "PV Array Rated Power", -1.0 );

    data->chargingMode = chargingModeToString( int_read_input_register( ctx, 0x3008, 1, "Charging Mode", -1 ) );

    data->ratedCurrentOfLoad = float_read_input_register( ctx, 0x300E, 1, "Rated Current Load", -1.0 );
}


// -----------------------------------------------------------------------------
void    getRealTimeData (modbus_t *ctx, RealTimeData_t *data)
{
    data->pvArrayVoltage =  float_read_input_register( ctx, 0x3100, 1, "PV Array Voltage", -1.0 );
    data->pvArrayCurrent =  float_read_input_register( ctx, 0x3101, 1, "PV Array Current", -1.0 );
    data->pvArrayPower   =  float_read_input_register( ctx, 0x3102, 2, "PV Array Power", -1.0 );
      
    data->batteryPower   =  float_read_input_register( ctx, 0x3106, 2, "Battery Power", -1.0 );
    
    data->loadVoltage =  float_read_input_register( ctx, 0x310C, 1, "Load Voltage", -1.0 );
    data->loadCurrent =  float_read_input_register( ctx, 0x310D, 1, "Load Current", -1.0 );
    data->loadPower   =  float_read_input_register( ctx, 0x310E, 2, "Load Power", -1.0 );
    
    data->batteryTemp = C2F( float_read_input_register( ctx, 0x3110, 1, "Battery Temp", -100.0 ) );
    data->caseTemp    = C2F( float_read_input_register( ctx, 0x3111, 1, "Case Temp", -100.0 ) );

    data->batterySOC = (int) (float_read_input_register( ctx, 0x311A, 1, "Battery SoC", -1.0 ) * 100.0);
    data->remoteBatteryTemperature = C2F( float_read_input_register( ctx, 0x311B, 1, "Remote Temp Sensor", -100.0 ) );
    data->batteryRealRatedPower = float_read_input_register( ctx, 0x311D, 1, "Battery Real Rated Power", -1.0 );
}


// -----------------------------------------------------------------------------
void    getRealTimeStatus (modbus_t *ctx, RealTimeStatus_t *data)
{   
    data->batteryStatusValue =  int_read_input_register( ctx, 0x3200, 1, "Battery Status", 0xFFFF );
    decodeBatteryStatusBits( data, data->batteryStatusValue );
    
    data->chargingStatusValue =  int_read_input_register( ctx, 0x3201, 1, "Charging Equipment Status", 0xFFFF );
    decodeChargingStatusBits( data, data->chargingStatusValue );

    data->dischargingStatusValue =  int_read_input_register( ctx, 0x3202, 1, "Discharging Equipment Status", 0xFFFF );
    decodeDischargingStatusBits( data, data->dischargingStatusValue );
}


// -----------------------------------------------------------------------------
void    getStatisticalParameters (modbus_t *ctx, StatisticalParameters_t *data)
{
    data->maximumInputVoltageToday = float_read_input_register( ctx, 0x3300, 1, "Max PV Voltage Today", -1.0 );
    data->minimumInputVoltageToday = float_read_input_register( ctx, 0x3301, 1, "Min PV Voltage Today", -1.0 );
    data->maximumBatteryVoltageToday = float_read_input_register( ctx, 0x3302, 1, "Max PV Voltage Today", -1.0 );
    data->minimumBatteryVoltageToday = float_read_input_register( ctx, 0x3303, 1, "Min PV Voltage Today", -1.0 );

    data->consumedEnergyToday = float_read_input_register( ctx, 0x3304, 2, "Consumed Energy Today", -1.0 );
    data->consumedEnergyMonth = float_read_input_register( ctx, 0x3306, 2, "Consumed Energy This Month", -1.0 );
    data->consumedEnergyYear = float_read_input_register( ctx, 0x3308, 2, "Consumed Energy This Year", -1.0 );
    
    data->totalConsumedEnergy = float_read_input_register( ctx, 0x330A, 2, "Total Consumed Energyr", -1.0 );

    data->generatedEnergyToday = float_read_input_register( ctx, 0x330C, 2, "Generated Energy Today", -1.0 );
    data->generatedEnergyMonth = float_read_input_register( ctx, 0x330E, 2, "Generated Energy This Month", -1.0 );
    data->generatedEnergyYear = float_read_input_register( ctx, 0x3310, 2, "Generated Energy This Year", -1.0 );
    data->totalGeneratedEnergy = float_read_input_register( ctx, 0x3312, 2, "Total Generated Energy", -1.0 );
    
    data->batteryVoltage = float_read_input_register( ctx, 0x331A, 1, "Battery Voltage", -1.0 );
    data->batteryCurrent = float_read_input_register( ctx, 0x331B, 2, "Battery Current", -1.0 );
}


// -----------------------------------------------------------------------------
void    getSettings (modbus_t *ctx, Settings_t *data)
{
    data->batteryType =  batteryTypeToString( int_read_register( ctx, 0x9000, 1, "Battery Type", -1 ) );
    data->batteryCapacity = int_read_register( ctx, 0x9001, 1, "Battery Capacity", -1 );
    
    data->tempCompensationCoeff   = float_read_register( ctx, 0x9002, 1, "Temperature Compensation Coefficient", -1.0 );
    data->highVoltageDisconnect   = float_read_register( ctx, 0x9003, 1, "High Voltage Disconnect", -1.0 );
    data->chargingLimitVoltage    = float_read_register( ctx, 0x9004, 1, "Charging Limit Voltage", -1.0 );
    data->overVoltageReconnect    = float_read_register( ctx, 0x9005, 1, "Over Voltage Reconnect", -1.0 );
    data->equalizationVoltage     = float_read_register( ctx, 0x9006, 1, "Equalization Voltage", -1.0 );
    data->boostVoltage            = float_read_register( ctx, 0x9007, 1, "Boost Voltage", -1.0 );
    data->floatVoltage            = float_read_register( ctx, 0x9008, 1, "Float Voltage", -1.0 );
    data->boostReconnectVoltage   = float_read_register( ctx, 0x9009, 1, "Boost Reconnect Voltage", -1.0 );

    data->lowVoltageReconnect     = float_read_register( ctx, 0x900A, 1, "Low Voltage Reconnect", -1.0 );
    data->underVoltageRecover     = float_read_register( ctx, 0x900B, 1, "Under Voltage Recover", -1.0 );
    data->underVoltageWarning     = float_read_register( ctx, 0x900C, 1, "Under Voltage Warning", -1.0 );
    data->lowVoltageDisconnect    = float_read_register( ctx, 0x900D, 1, "Low Voltage Disconnect", -1.0 );
    data->dischargingLimitVoltage = float_read_register( ctx, 0x900E, 1, "Discharging Limit Voltage", -1.0 );
    
    
    //
    // Real Time Clock is 0x9013, to 0x9015
    static  char    rtc[ 40 ];
    memset( rtc, '\0', sizeof rtc );
    data->realtimeClock = getRealtimeClockStr( ctx, rtc, sizeof rtc );
    
    data->batteryTempWarningUpperLimit = C2F( float_read_register( ctx, 0x9017, 1, "Battery Temperature Warning Upper Limit", -1.0 ) );
    data->batteryTempWarningLowerLimit = C2F( float_read_register( ctx, 0x9018, 1, "Battery Temperature Warning Lower Limit", -1.0 ) );
    data->controllerInnerTempUpperLimit = C2F( float_read_register( ctx, 0x9019, 1, "Controller Inner Temperature Upper Limit", -1.0 ) );
    data->controllerInnerTempUpperLimitRecover = C2F( float_read_register( ctx, 0x901A, 1, "Controller Inner Temperature Lower Limit", -1.0 ) );
    
    
    data->daytimeThresholdVoltage    = float_read_register( ctx, 0x901E, 1, "Daytime Threshold Voltage (Sundown)", -1.0 );
    data->lightSignalStartupTime     = int_read_register( ctx, 0x901F, 1, "Light Signal Startup Delay Time (Night)", -1 );
    data->lighttimeThresholdVoltage  = float_read_register( ctx, 0x9020, 1, "Night Time Threshold Voltage (Sunrise)", -1.0 );
    data->lightSignalCloseDelayTime  =  int_read_register( ctx, 0x9021, 1, "Light Signal Close Delay Time (Day)", -1 );

    
    data->localControllingModes = int_read_register( ctx, 0x903D, 1, "Light Controlling Modes", -1 );
    data->workingTimeLength1    = int_read_register( ctx, 0x903E, 1, "Working Time Length One", -1 );
    data->workingTimeLength2    = int_read_register( ctx, 0x903F, 1, "Working Time Length Two", -1 );


    data->turnOnTiming1_seconds = int_read_register( ctx, 0x9042, 1, "Turn On Timing One (Seconds)", -1 );
    data->turnOnTiming1_minutes = int_read_register( ctx, 0x9043, 1, "Turn On Timing One (Minutes)", -1 );
    data->turnOnTiming1_hours   = int_read_register( ctx, 0x9044, 1, "Turn On Timing One (Hours)", -1 );

    data->turnOffTiming1_seconds = int_read_register( ctx, 0x9045, 1, "Turn Off Timing One (Seconds)", -1 );
    data->turnOffTiming1_minutes = int_read_register( ctx, 0x9046, 1, "Turn Off Timing One (Minutes)", -1 );
    data->turnOffTiming1_hours   = int_read_register( ctx, 0x9047, 1, "Turn Off Timing One (Hours)", -1 );
    
    data->turnOnTiming2_seconds = int_read_register( ctx, 0x9048, 1, "Turn On Timing Two (Seconds)", -1 );
    data->turnOnTiming2_minutes = int_read_register( ctx, 0x9049, 1, "Turn On Timing Two (Minutes)", -1 );
    data->turnOnTiming2_hours   = int_read_register( ctx, 0x904A, 1, "Turn On Timing Two (Hours)", -1 );

    data->turnOffTiming2_seconds = int_read_register( ctx, 0x904B, 1, "Turn Off Timing Two (Seconds)", -1 );
    data->turnOffTiming2_minutes = int_read_register( ctx, 0x904C, 1, "Turn Off Timing Two (Minutes)", -1 );
    data->turnOffTiming2_hours   = int_read_register( ctx, 0x904D, 1, "Turn Off Timing Two (Hours)", -1 );
    
    data->backlightTime = int_read_register( ctx, 0x9063, 1, "Backlight on duration", -1 );
    data->lengthOfNight = int_read_register( ctx, 0x9065, 1, "Length of Night @ 0x9065", -1 );
    
    // 
    //  0x9066 doesn't apply to my LS1024B
    //data->deviceConfigureMainPower = int_read_register( ctx, 0x9066, 1, "Device Configuration of Main Power Supply", -1 );
    
    data->batteryRatedVoltageCode   = int_read_register( ctx, 0x9067, 1, "batteryRatedVoltageCode @ 0x9067", -1 );
    
    data->defaultLoadOnOffManualMode = int_read_register( ctx, 0x906A, 1, "defaultLoadOnOffManualMode @ 0x906A", -1 );
    data->equalizeDuration          = int_read_register( ctx, 0x906B, 1, "equalizeDuration @ 0x906B", -1 );
    data->boostDuration             = int_read_register( ctx, 0x906C, 1, "boostDuration @ 0x906C", -1 );
    data->dischargingPercentage     = float_read_register( ctx, 0x906D, 1, "dischargingPercentage @ 0x906D", -1.0 );
    data->chargingPercentage        = float_read_register( ctx, 0x906E, 1, "chargingPercentage @ 0x906E", -1.0 );
    
    data->batteryManagementMode = int_read_register( ctx, 0x9070, 1, "batteryManagementMode @ 0x9070", -1 );
}


// -----------------------------------------------------------------------------
//
//  Set Parameters
// -----------------------------------------------------------------------------
void    getRealtimeClock (modbus_t *ctx, int *seconds, int *minutes, int *hour, int *day, int *month, int *year)
{
    int         registerAddress = 0x9013;
    int         numBytes = 0x03;
    uint16_t    buffer[ 32 ];

    memset( buffer, '\0', sizeof buffer );
    
    if (modbus_read_registers( ctx, registerAddress, numBytes, buffer ) == -1) {
        Logger_LogError( "getRealtimeClock() - Read of 3 at 0x9013 failed: %s\n", modbus_strerror( errno ));
    }
    
    *seconds = (buffer[ 0 ] & 0x00FF);
    *minutes = ((buffer[ 0 ] & 0xFF00) >> 8);
    *hour = (buffer[ 1 ] & 0x00FF);
    *day = ((buffer[ 1 ] & 0xFF00) >> 8);
    *month = (buffer[ 2 ] & 0x00FF);
    *year = ((buffer[ 2 ] & 0xFF00) >> 8);
}


// -----------------------------------------------------------------------------
char    *getRealtimeClockStr (modbus_t *ctx, char *buffer, const int buffSize)
{
    int seconds, minutes, hour, day, month, year;

    getRealtimeClock( ctx, &seconds, &minutes, &hour, &day, &month, &year );
    snprintf( buffer, buffSize, "%02d/%02d/%02d %02d:%02d:%02d", month, day, year, hour, minutes, seconds );
    return buffer;
}


// -----------------------------------------------------------------------------
void    setRealtimeClock (modbus_t *ctx, const int seconds, const int minutes, const int hour, const int day, const int month, const int year)
{
    assert( seconds >= 0 && seconds <= 59 );
    assert( minutes >= 0 && minutes <= 59 );
    assert( hour >= 0 && hour <= 23 );
    assert( day >= 1 && day <= 31 );
    assert( month >= 1 && month <= 12 );
    assert( year >= 1 && year <= 99 );
    
    uint16_t    buffer[ 32 ];
    
    memset( buffer, '\0', sizeof buffer );
    buffer[ 0 ] = (minutes << 8) | seconds;
    buffer[ 1 ] = (day << 8) | hour;
    buffer[ 2 ] = (year << 8) | month;
    
    int se = (buffer[ 0 ] & 0x00FF);
    int mi = ((buffer[ 0 ] & 0xFF00) >> 8);
    int hr = (buffer[ 1 ] & 0x00FF);
    int dy = ((buffer[ 1 ] & 0xFF00) >> 8);
    int mn = (buffer[ 2 ] & 0x00FF);
    int yr = ((buffer[ 2 ] & 0xFF00) >> 8);
    
    assert( se == seconds );
    assert( mi == minutes );
    assert( hr == hour );
    assert( dy == day );
    assert( mn == month );
    assert( yr == year );

    int         registerAddress = 0x9013;
    int         numBytes = 0x03;
    if (modbus_write_registers( ctx, registerAddress, numBytes, buffer ) == -1) {
        Logger_LogError( "setRealTimeClock() - write failed: %s\n", modbus_strerror( errno ));
    }
}

// -----------------------------------------------------------------------------
void    setRealtimeClockToNow (modbus_t *ctx)
{
    struct tm   *timeInfo;
    time_t      now;

    time( &now );
    timeInfo = localtime( &now );
    int  seconds = timeInfo->tm_sec;
    int  minutes = timeInfo->tm_min;
    int  hour = timeInfo->tm_hour;
    int  day = timeInfo->tm_mday;
    int  month = timeInfo->tm_mon + 1;
    int  year = timeInfo->tm_year % 100;
    
    setRealtimeClock( ctx, seconds, minutes, hour, day, month, year );
}


//------------------------------------------------------------------------------
void    setBatteryType (modbus_t *ctx, int batteryTypeCode)
{
    assert( batteryTypeCode >= 0x00 && batteryTypeCode <= 0x03 );
    // setIntSettingParameter( ctx, 0x9000, batteryTypeCode );
}    
    


//------------------------------------------------------------------------------
void    setBatteryCapacity (modbus_t *ctx, int batteryCapacityAH)
{
    assert( batteryCapacityAH >= 0x00 );
    // setIntSettingParameter( ctx, 0x9001, batteryCapacityAH );
}

//------------------------------------------------------------------------------
void    setTempertureCompensationCoefficient (modbus_t *ctx, int value)
{
    assert( value >= 0x00  && value <= 0x09);
    setIntSettingParameter( ctx, 0x9002, value );
}
//------------------------------------------------------------------------------
void    setHighVoltageDisconnect (modbus_t *ctx, double value)
{
    //
    //  Manual says 'User' must be between 9 and 17 volts
    assert( value >= 9.0  && value <= 17.0 );
    setFloatSettingParameter( ctx, 0x9003, (float) value );
}

//------------------------------------------------------------------------------
void    setChargingLimitVoltage (modbus_t *ctx, float value)
{
    //
    //  Manual says 'User' must be between 9 and 17 volts
    assert( value >= 9.0  && value <= 17.0);
    setFloatSettingParameter( ctx, 0x9004, value );
}

//------------------------------------------------------------------------------
void    setOverVoltageReconnect (modbus_t *ctx, float value)
{
    //
    //  Manual says 'User' must be between 9 and 17 volts
    assert( value >= 9.0  && value <= 17.0);
    setFloatSettingParameter( ctx, 0x9005, value );
}

//------------------------------------------------------------------------------
void    setEqualizationVoltage (modbus_t *ctx, float value)
{
    //
    //  Manual says 'User' must be between 9 and 17 volts
    assert( value >= 9.0  && value <= 17.0);
    setFloatSettingParameter( ctx, 0x9006, value );
}

//------------------------------------------------------------------------------
void    setBoostVoltage (modbus_t *ctx, float value)
{
    //
    //  Manual says 'User' must be between 9 and 17 volts
    assert( value >= 9.0  && value <= 17.0);
    setFloatSettingParameter( ctx, 0x9007, value );
}

//------------------------------------------------------------------------------
void    setFloatVoltage (modbus_t *ctx, float value)
{
    //
    //  Manual says 'User' must be between 9 and 17 volts
    assert( value >= 9.0  && value <= 17.0);
    setFloatSettingParameter( ctx, 0x9008, value );
}

//------------------------------------------------------------------------------
void    setBoostReconnectVoltage (modbus_t *ctx, float value)
{
    //
    //  Manual says 'User' must be between 9 and 17 volts
    assert( value >= 9.0  && value <= 17.0);
    setFloatSettingParameter( ctx, 0x9009, value );
}

//------------------------------------------------------------------------------
void    setLowVoltageReconnect (modbus_t *ctx, float value)
{
    //
    //  Manual says 'User' must be between 9 and 17 volts
    assert( value >= 9.0  && value <= 17.0);
    setFloatSettingParameter( ctx, 0x900A, value );
}

//------------------------------------------------------------------------------
void    setUnderVoltageRecover (modbus_t *ctx, float value)
{
    //
    //  Manual says 'User' must be between 9 and 17 volts
    assert( value >= 9.0  && value <= 17.0);
    setFloatSettingParameter( ctx, 0x900B, value );
}

//------------------------------------------------------------------------------
void    setUnderVoltageWarning (modbus_t *ctx, float value)
{
    //
    //  Manual says 'User' must be between 9 and 17 volts
    assert( value >= 9.0  && value <= 17.0);
    setFloatSettingParameter( ctx, 0x900C, value );
}

//------------------------------------------------------------------------------
void    setLowVoltageDisconnect (modbus_t *ctx, float value)
{
    //
    //  Manual says 'User' must be between 9 and 17 volts
    assert( value >= 9.0  && value <= 17.0);
    setFloatSettingParameter( ctx, 0x900D, value );
}

//------------------------------------------------------------------------------
void    setDischargingLimitVoltage (modbus_t *ctx, float value)
{
    //
    //  Manual says 'User' must be between 9 and 17 volts
    assert( value >= 9.0  && value <= 17.0);
    setFloatSettingParameter( ctx, 0x900E, value );
}

//------------------------------------------------------------------------------
void    setBatteryTemperatureWarningUpperLimit (modbus_t *ctx, float value)
{
    setFloatSettingParameter( ctx, 0x9017, value );
}

//------------------------------------------------------------------------------
void    setBatteryTemperatureWarningLowerLimit (modbus_t *ctx, float value)
{
    setFloatSettingParameter( ctx, 0x9018, value );
}

//------------------------------------------------------------------------------
void    setControllerInnerTemperatureUpperLimit (modbus_t *ctx, float value)
{
    setFloatSettingParameter( ctx, 0x9019, value );
}

//------------------------------------------------------------------------------
void    setControllerInnerTemperatureUpperLimitRecover (modbus_t *ctx, float value)
{
    setFloatSettingParameter( ctx, 0x901A, value );
}

//------------------------------------------------------------------------------
void    setDayTimeThresholdVoltage (modbus_t *ctx, float value)
{
    setFloatSettingParameter( ctx, 0x901E, value );
}

//------------------------------------------------------------------------------
void    setLightSignalStartupDelayTime (modbus_t *ctx, int value)
{
    setIntSettingParameter( ctx, 0x901F, value );
}

//------------------------------------------------------------------------------
void    setNightTimeThresholdVoltage (modbus_t *ctx, float value)
{
    setFloatSettingParameter( ctx, 0x9020, value );
}

//------------------------------------------------------------------------------
void    setLightSignalCloseDelayTime (modbus_t *ctx, int value)
{
    setIntSettingParameter( ctx, 0x9021, value );
}

//------------------------------------------------------------------------------
void    setLoadControllingModes (modbus_t *ctx, int value)
{
    assert( value >= 0x00 && value <= 0x03 );
    setIntSettingParameter( ctx, 0x903D, value );
}

//------------------------------------------------------------------------------
void    setWorkingTimeLength1 (modbus_t *ctx, int hour, int minute)
{
    assert( hour >= 0 && hour <= 23 );
    assert( minute >= 0 && minute <= 59 );
    setIntSettingParameter( ctx, 0x903E, ((hour << 8) | minute ) );
}

//------------------------------------------------------------------------------
void    setWorkingTimeLength2 (modbus_t *ctx, int hour, int minute)
{
    assert( hour >= 0 && hour <= 23 );
    assert( minute >= 0 && minute <= 59 );
    setIntSettingParameter( ctx, 0x903F, ((hour << 8) | minute ) );
}

//------------------------------------------------------------------------------
void    setTurnOnTiming1 (modbus_t *ctx, int hour, int minute, int second)
{
    assert( hour >= 0 && hour <= 23 );
    assert( minute >= 0 && minute <= 59 );
    assert( second >= 0 && second <= 59 );
    setIntSettingParameter( ctx, 0x9042, second );
    setIntSettingParameter( ctx, 0x9043, minute );
    setIntSettingParameter( ctx, 0x9044, hour );
}

//------------------------------------------------------------------------------
void    setTurnOffTiming1 (modbus_t *ctx, int hour, int minute, int second)
{
    assert( hour >= 0 && hour <= 23 );
    assert( minute >= 0 && minute <= 59 );
    assert( second >= 0 && second <= 59 );
    setIntSettingParameter( ctx, 0x9045, second );
    setIntSettingParameter( ctx, 0x9046, minute );
    setIntSettingParameter( ctx, 0x9047, hour );
}

//------------------------------------------------------------------------------
void    setTurnOnTiming2 (modbus_t *ctx, int hour, int minute, int second)
{
    assert( hour >= 0 && hour <= 23 );
    assert( minute >= 0 && minute <= 59 );
    assert( second >= 0 && second <= 59 );
    setIntSettingParameter( ctx, 0x9048, second );
    setIntSettingParameter( ctx, 0x9049, minute );
    setIntSettingParameter( ctx, 0x904A, hour );
}

//------------------------------------------------------------------------------
void    setTurnOffTiming2 (modbus_t *ctx, int hour, int minute, int second)
{
    assert( hour >= 0 && hour <= 23 );
    assert( minute >= 0 && minute <= 59 );
    assert( second >= 0 && second <= 59 );
    setIntSettingParameter( ctx, 0x904B, second );
    setIntSettingParameter( ctx, 0x904C, minute );
    setIntSettingParameter( ctx, 0x904D, hour );
}

//------------------------------------------------------------------------------
void    setBacklightTime (modbus_t *ctx, int seconds)
{
    setIntSettingParameter( ctx, 0x9063, seconds );
}

//------------------------------------------------------------------------------
void    setLengthOfNight (modbus_t *ctx, int hour, int minute)
{
    assert( hour >= 0 && hour <= 23 );
    assert( minute >= 0 && minute <= 59 );
    setIntSettingParameter( ctx, 0x9065, ((hour << 8) | minute ) );
}

//------------------------------------------------------------------------------
void    setDeviceConfigureOfMainPowerSupply (modbus_t *ctx, int value)
{
    assert( value >= 0x01 && value <= 0x02 );
    setIntSettingParameter( ctx, 0x9066, value );
}

//------------------------------------------------------------------------------
void    setBatteryRatedVoltageCode (modbus_t *ctx, int value)
{
    assert( value >= 0x00 && value <= 0x09 );
    setIntSettingParameter( ctx, 0x9067, value );
}

//------------------------------------------------------------------------------
void    setDefaultLoadOnOffInManualMode (modbus_t *ctx, int value)
{
    assert( value >= 0x00 && value <= 0x01 );
    setIntSettingParameter( ctx, 0x906A, value );
}

//------------------------------------------------------------------------------
void    setEqualizeDuration (modbus_t *ctx, int value)
{
    assert( value >= 0 && value <= 180 );
    setIntSettingParameter( ctx, 0x906B, value );
}

//------------------------------------------------------------------------------
void    setBoostDuration (modbus_t *ctx, int value)
{
    assert( value >= 10 && value <= 180 );
    setIntSettingParameter( ctx, 0x906C, value );
}

//------------------------------------------------------------------------------
void    setDischargingPercentage (modbus_t *ctx, float value)
{
    assert( value >= 20.0 && value <= 100.0 );
    setFloatSettingParameter( ctx, 0x906D, value );
}

//------------------------------------------------------------------------------
void    setChargingPercentage (modbus_t *ctx, float value)
{
    assert( value >= 0.0 && value <= 100.0 );
    setFloatSettingParameter( ctx, 0x906E, value );
}

//------------------------------------------------------------------------------
void    setManagementModesOfBatteryChargingAndDischarging (modbus_t *ctx, int value)
{
    assert( value >= 0 && value <= 1 );
    setFloatSettingParameter( ctx, 0x9070, value );
}


//------------------------------------------------------------------------------
static
char    *batteryTypeToString (uint16_t batteryType)
{
    switch (batteryType) {
        case    0x00:   return "User Defined";  break;
        case    0x01:   return "Sealed";        break;
        case    0x02:   return "Gel";           break;
        case    0x03:   return "Flooded";       break;
        
        default:        return "Unknown";
    }
}


// -----------------------------------------------------------------------------
int getChargingDeviceStatus (modbus_t *ctx)
{
    int     coilNum = 0;
    return (getCoilValue( ctx, coilNum, "Charging Device Status (Coil 0)" ));
}

// -----------------------------------------------------------------------------
void    setChargingDeviceStatus (modbus_t *ctx, const int value)
{
    int     coilNum = 0;
    setCoilValue( ctx, coilNum, value, "Charging Device Status (Coil 0)" );
}

// -----------------------------------------------------------------------------
int getOutputControlMode (modbus_t *ctx)
{
    int     coilNum = 1;
    return (getCoilValue( ctx, coilNum, "Output Control Mode (Coil 1)" ));
}

// -----------------------------------------------------------------------------
void    setOutputControlMode (modbus_t *ctx, const int value)
{
    int     coilNum = 1;
    setCoilValue( ctx, coilNum, value, "Output Control Mode (Coil 1)" );
}

// -----------------------------------------------------------------------------
int getManualLoadControlMode (modbus_t *ctx)
{
    int     coilNum = 2;
    return (getCoilValue( ctx, coilNum, "Manual Load Control Mode (Coil 2)" ));
}

// -----------------------------------------------------------------------------
void    setManualLoadControlMode (modbus_t *ctx, const int value)
{
    int     coilNum = 2;
    setCoilValue( ctx, coilNum, value, "Manual Load Control Mode (Coil 2)" );
}

// -----------------------------------------------------------------------------
int getDefaultLoadControlMode (modbus_t *ctx)
{
    int     coilNum = 3;
    return (getCoilValue( ctx, coilNum, "Default Load Control Mode (Coil 3)" ));
}

// -----------------------------------------------------------------------------
void    setDefaultLoadControlMode (modbus_t *ctx, const int value)
{
    int     coilNum = 3;
    setCoilValue( ctx, coilNum, value, "Default Load Control Mode (Coil 3)" );
}

// -----------------------------------------------------------------------------
int getEnableLoadTestMode (modbus_t *ctx)
{
    int     coilNum = 5;
    return (getCoilValue( ctx, coilNum, "Enable Load Test Mode (Coil 5)" ));
}

// -----------------------------------------------------------------------------
void    setEnableLoadTestMode (modbus_t *ctx, const int value)
{
    int     coilNum = 5;
    setCoilValue( ctx, coilNum, value, "Enable Load Test Mode (Coil 5)" );
}

// -----------------------------------------------------------------------------
void    forceLoadOnOff (modbus_t *ctx, const int value)
{
    int     coilNum = 6;
    setCoilValue( ctx, coilNum, value, "Force Load (Coil 6)" );
}

// -----------------------------------------------------------------------------
void    restoreSystemDefaults (modbus_t *ctx)
{
    int     coilNum = 0x13;
    setCoilValue( ctx, coilNum, 1, "Restore System Defaults (Coil 0x13)" );
}

// -----------------------------------------------------------------------------
void    clearEnergyGeneratingStatistics (modbus_t *ctx)
{
    int     coilNum = 0x14;
    setCoilValue( ctx, coilNum, 1, "Clear Energy Gen Stats Load (Coil 0x14)" );
}

// -----------------------------------------------------------------------------
int getOverTemperatureInsideDevice (modbus_t *ctx)
{
    int         registerAddress = 0x2000;
    uint8_t     value = 0;

    Logger_LogDebug( "Getting overTemperatureInsideDevice\n" );
    if (modbus_read_input_bits( ctx, registerAddress, 1, &value ) == -1) {
        Logger_LogError( "read_input_bits on register %X failed: %s\n", registerAddress, modbus_strerror( errno ));
        return -1;
    }
    
    //
    //  Not sure if I should have read just one BIT or a full byte
    Logger_LogDebug( "%s - value = %0X (hex)  Bottom bit = %0x\n", "Getting overTemperatureInsideDevice", value, (value & 0b00000001) );
    
    //
    // Mask off the top 7 just in case
    return ( value & 0b00000001 );    
}

// -----------------------------------------------------------------------------
int isNightTime (modbus_t *ctx)
{
    int         registerAddress = 0x200C;
    uint8_t     value = 0;

    Logger_LogDebug( "Getting isNightTime\n" );
    if (modbus_read_input_bits( ctx, registerAddress, 1, &value ) == -1) {
        Logger_LogError( "read_input_bits on register %X failed: %s\n", registerAddress, modbus_strerror( errno ));
        return -1;
    }
    
    //
    //  Not sure if I should have read just one BIT or a full byte
    Logger_LogDebug( "%s - value = %0X (hex)  Bottom bit = %0x\n", "Getting isNightTime", value, (value & 0b00000001) );
    
    //
    // Mask off the top 7 just in case
    return ( (value & 0b00000001) == 1 );    
}


//------------------------------------------------------------------------------
void    setChargingDeviceOn (modbus_t *ctx)
{
    int     coilNum = 0x00;
    setCoilValue( ctx, coilNum, 1, "Control Charging Device - Set On (Coil 0x00)" );    
}


//------------------------------------------------------------------------------
void    setChargingDeviceOff (modbus_t *ctx)
{
    int     coilNum = 0x00;
    setCoilValue( ctx, coilNum, 0, "Control Charging Device - Set Off (Coil 0x00)" );  
}


//------------------------------------------------------------------------------
void    setLoadDeviceOn (modbus_t *ctx)
{
    // Kick it into Manual control
    setCoilValue( ctx, 0x01, 1, "Setting Load Control to Manual (Coil 0x01)" );
    
    // The turn it on
    setCoilValue( ctx, 0x02, 1, "Setting Load Control to On (Coil 0x02)" );    
}


//------------------------------------------------------------------------------
void    setLoadDeviceOff (modbus_t *ctx)
{
    // Kick it into Manual control
    setCoilValue( ctx, 0x01, 1, "Setting Load Control to Manual (Coil 0x01)" );
    
    // The turn it on
    setCoilValue( ctx, 0x02, 0, "Setting Load Control to Off (Coil 0x02)" );    
}


//------------------------------------------------------------------------------
static
char    *chargingModeToString (uint16_t mode)
{
    switch (mode) {
        case    0x00:   return "Connect/Disconnect";    break;
        case    0x01:   return "PWM";                   break;
        case    0x02:   return "MPPT";                  break;
        default:        return "Unknown";
    }
}

// -----------------------------------------------------------------------------
static
int     getCoilValue (modbus_t *ctx, const int coilNum, const char *description)
{
    int         numBits = 1;                  
    uint8_t     value = 0;
    
    
    Logger_LogDebug( "%s\n", description );
    if (modbus_read_bits( ctx, coilNum, numBits, &value ) == -1) {
        Logger_LogError( "%s -- read_bits on coil %d failed: %s\n", description, coilNum, modbus_strerror( errno ));
        return -1;
    }

    //
    //  Not sure if I should have read just one BIT or a full byte
    Logger_LogDebug( "%s - value = %0X (hex)  Bottom bit = %0x\n", description, value, (value & 0b00000001) );
    
    //
    // Mask off the top 7 just in case
    return ( value & 0b00000001 );
}

// -----------------------------------------------------------------------------
static
void    setCoilValue (modbus_t *ctx, const int coilNum, int value, const char *description)
{
    assert( (value == TRUE) || (value == FALSE) );
    
    Logger_LogDebug( "%s - setting %d to %d\n", description, coilNum, value );
    if (modbus_write_bit( ctx, coilNum, value ) == -1) {
        Logger_LogError( "write_bit on coil %d failed: %s\n", coilNum, modbus_strerror( errno ));
    }
}

// -----------------------------------------------------------------------------
static
void    decodeBatteryStatusBits (RealTimeStatus_t *data, const int value) 
{
 /* D3-D0: 01H Overvolt , 00H Normal , 02H Under Volt, 03H Low Volt Disconnect, 04H Fault
    D7-D4: 00H Normal, 01H Over Temp.(Higher than the warning settings), 02H Low Temp.(Lower than the warning settings),
    D8: Battery inner resistance    abnormal 1, normal 0
    D15: 1-Wrong identification for rated voltage*/   
    switch (value & 0b0000000000001111) {
        case    0x00:   data->batteryStatusVoltage = "Normal";      break;
        case    0x01:   data->batteryStatusVoltage = "Over";        break;
        case    0x02:   data->batteryStatusVoltage = "Under";       break;
        case    0x03:   data->batteryStatusVoltage = "Low Voltage Disconnect";  break;
        case    0x04:   data->batteryStatusVoltage = "Fault";       break;
        default:        data->batteryStatusVoltage = "???"; 
    }

    switch ((value & 0b0000000011110000) >> 4) {
        case    0x00:   data->batteryStatusTemperature = "Normal";      break;
        case    0x01:   data->batteryStatusTemperature = "Higher";      break;
        case    0x02:   data->batteryStatusTemperature = "Lower";       break;
        default:        data->batteryStatusTemperature = "???"; 
    }

    data->batteryInnerResistance = ((value & 0b0000000111100000) ? "Abnormal" : "Normal");
    data->batteryCorrectIdentification = ((value & 0b1000000000000) ? "Incorrect" : "Correct");
}

// -----------------------------------------------------------------------------
static
void    decodeChargingStatusBits (RealTimeStatus_t *data, const int value) 
{
    /*
     *  D15-D14: Input volt status. 00 normal, 01 no power connected, 02H Higher volt input, 03H Input volt error.
        D13: Charging MOSFET is short.
        D12: Charging or Anti-reverse MOSFET is short.
        D11: Anti-reverse MOSFET is short.
        D10: Input is over current.
        D9: The load is Over current.
        D8: The load is short.
        D7: Load MOSFET is short.
        D4: PV Input is short.
        D3-2: Charging status. 00 No charging,01 Float,02 Boost, 03 Equalization.
        D1: 0 Normal, 1 Fault.
        D0: 1 Running, 0 Standby.
     */

    switch ((value & 0b1100000000000000) >> 14) {
        case    0x00:   data->chargingInputVoltageStatus = "Normal";      break;
        case    0x01:   data->chargingInputVoltageStatus = "No power connected";        break;
        case    0x02:   data->chargingInputVoltageStatus = "High";       break;
        case    0x03:   data->chargingInputVoltageStatus = "Input Volt Error";  break;
        default:        data->chargingInputVoltageStatus = "???"; 
    }

    data->chargingMOSFETShort = ((value & 0b0010000000000000) ? TRUE : FALSE);
    data->someMOSFETShort = ((value & 0b0001000000000000) ? TRUE : FALSE);
    data->antiReverseMOSFETShort = ((value & 0b0000100000000000) ? TRUE : FALSE);
    data->inputIsOverCurrent = ((value & 0b0000010000000000) ? TRUE : FALSE);
    data->loadIsOverCurrent = ((value & 0b0000001000000000) ? TRUE : FALSE);
    data->loadIsShort = ((value & 0b0000000100000000) ? TRUE : FALSE);
    data->loadMOSFETIsShort = ((value & 0b0000000010000000) ? TRUE : FALSE);
    data->pvInputIsShort = ((value & 0b0000000000010000) ? TRUE : FALSE);

    switch ((data->chargingStatusValue & 0b0000000000001100) >> 2) {
        case    0x00:   data->chargingStatus = "Not Charging";      break;
        case    0x01:   data->chargingStatus = "Floating";        break;
        case    0x02:   data->chargingStatus = "Boosting";        break;
        case    0x03:   data->chargingStatus = "Equalizing";        break;
        default:        data->chargingStatus = "??";        break;
    }
    
    data->chargingStatusNormal = ((value & 0b0000000000000010) ? FALSE : TRUE );
    data->chargingStatusRunning = ((value & 0b0000000000000001) ? TRUE : FALSE);
}

// -----------------------------------------------------------------------------
static
void    decodeDischargingStatusBits (RealTimeStatus_t *data, const int value) 
{
/*
    D15-D14: 00H normal, 01H low,  02H High, 03H no access Input volt error.
    D13-D12: output power:00-light load,01-moderate,02-rated,03-overload
    D11: short circuit
    D10: unable to discharge
    D9: unable to stop discharging
    D8: output voltage abnormal
    D7: input overpressure
    D6: high voltage side short circuit
    D5: boost overpressure
    D4: output overpressure
    D1: 0 Normal, 1 Fault.
    D0: 1 Running, 0 Standby.
 */
    //                 1234567890123456
    switch ((value & 0b1100000000000000) >> 14) {
        case    0x00:   data->dischargingInputVoltageStatus = "Normal";      break;
        case    0x01:   data->dischargingInputVoltageStatus = "Low";        break;
        case    0x02:   data->dischargingInputVoltageStatus = "High";       break;
        case    0x03:   data->dischargingInputVoltageStatus = "No Access - Input Volt Error";  break;
        default:        data->dischargingInputVoltageStatus = "???"; 
    }
    
    //                 1234567890123456
    switch ((value & 0b0011000000000000) >> 12) {
        case    0x00:   data->dischargingOutputPower = "Light Load";      break;
        case    0x01:   data->dischargingOutputPower = "Moderate Load";        break;
        case    0x02:   data->dischargingOutputPower = "Rated Load";       break;
        case    0x03:   data->dischargingOutputPower = "Overload";  break;
        default:        data->dischargingOutputPower = "???"; 
    }
    
    //                                            5432109876543210
    data->dischargingShortCircuit = ( (value &  0b0000100000000000) ? TRUE : FALSE );
    data->unableToDischarge = ( (value &        0b0000010000000000) ? TRUE : FALSE );
    data->unableToStopDischarging = ( (value &  0b0000001000000000) ? TRUE : FALSE );
    data->outputVoltageAbnormal = ( (value &    0b0000000100000000) ? TRUE : FALSE );
    data->inputOverpressure = ( (value &        0b0000000010000000) ? TRUE : FALSE );
    //                                            5432109876543210
    data->highVoltageSideShort = ( (value &     0b0000000001000000) ? TRUE : FALSE );
    data->boostOverpressure = ( (value &        0b0000000000100000) ? TRUE : FALSE );
    data->outputOverpressure = ( (value &       0b0000000000010000) ? TRUE : FALSE );
    data->dischargingStatusNormal = ( (value &  0b0000000000000010) ? FALSE : TRUE );
    data->dischargingStatusRunning = ( (value & 0b0000000000000001) ? TRUE : FALSE );   
}

// -----------------------------------------------------------------------------
static
float   C2F (float tempC)
{
    // T(°F) = T(°C) × 9/5 + 32
    return ((tempC * 9.0 / 5.0) + 32.0);
}

// -----------------------------------------------------------------------------
static
int     setFloatSettingParameter (modbus_t *ctx, int registerAddress, float floatValue)
{
    uint16_t    buffer[ 2 ];
   
    memset( buffer, '\0', sizeof buffer );
    buffer[ 0 ] = (uint16_t) (floatValue * 100.0);
        
    if (modbus_write_registers( ctx, registerAddress, 0x01, buffer ) == -1) {
        fprintf( stderr, "setFloatSettingParameter() - write of value %0.2f to register %X failed: %s\n", floatValue, registerAddress, modbus_strerror( errno ));
        return FALSE;
    }    
    
    return TRUE;
}

// -----------------------------------------------------------------------------
static
int     setIntSettingParameter (modbus_t *ctx, int registerAddress, int intValue)
{
    uint16_t    buffer[ 2 ];

    memset( buffer, '\0', sizeof buffer );
    buffer[ 0 ] = (uint16_t) intValue;
       
    if (modbus_write_registers( ctx, registerAddress, 0x01, buffer ) == -1) {
        fprintf( stderr, "setIntSettingParameter() - write of value %d to register %X failed: %s\n", intValue, registerAddress, modbus_strerror( errno ));
        return FALSE;
    }    
    
    return TRUE;
}



// ----------------------------------------------------------------------------
static
float   float_read_input_register ( modbus_t *ctx,
                                    const int registerAddress,
                                    const int numBytes,
                                    const char *description,
                                    const float badReadValue)
{
    assert( (numBytes == 1) || (numBytes == 2) );
    
    uint16_t    buffer[ 32  ];
    memset( buffer, '\0', sizeof buffer );
    
    if (modbus_read_input_registers( ctx, registerAddress, numBytes, buffer ) == -1) {
        Logger_LogError( "%s - Read of %d bytes at address %X failed: %s\n", description, numBytes, registerAddress, modbus_strerror( errno ));
        return badReadValue;
    }

    if (numBytes == 2) {
        long temp  = buffer[ 0x01 ] << 16;
        temp |= buffer[ 0x00 ];
        return (float) temp / 100.0;
    }
    
    return ((float) buffer[ 0x00 ]) / 100.0;
}


// ----------------------------------------------------------------------------
static
int int_read_input_register ( modbus_t *ctx,
                                    const int registerAddress,
                                    const int numBytes,
                                    const char *description,
                                    const int badReadValue)
{
    assert( (numBytes == 1) || (numBytes == 2) );
    
    uint16_t    buffer[ 32  ];
    memset( buffer, '\0', sizeof buffer );
    
    if (modbus_read_input_registers( ctx, registerAddress, numBytes, buffer ) == -1) {
        Logger_LogError( "%s - Read of %d bytes at address %X failed: %s\n", description, numBytes, registerAddress, modbus_strerror( errno ));
        return badReadValue;
    }

    if (numBytes == 2) {
        long temp  = buffer[ 0x01 ] << 16;
        temp |= buffer[ 0x00 ];
        return (int) temp;
    }
    
    return ((int) buffer[ 0x00 ]);
}


// ----------------------------------------------------------------------------
static
float   float_read_register ( modbus_t *ctx,
                                    const int registerAddress,
                                    const int numBytes,
                                    const char *description,
                                    const float badReadValue)
{
    assert( (numBytes == 1) || (numBytes == 2) );
    
    uint16_t    buffer[ 32  ];
    memset( buffer, '\0', sizeof buffer );
    
    if (modbus_read_registers( ctx, registerAddress, numBytes, buffer ) == -1) {
        Logger_LogError( "%s - Read of %d bytes at address %X failed: %s\n", description, numBytes, registerAddress, modbus_strerror( errno ));
        return badReadValue;
    }

    if (numBytes == 2) {
        long temp  = buffer[ 0x01 ] << 16;
        temp |= buffer[ 0x00 ];
        return (float) temp / 100.0;
    }
    
    return ((float) buffer[ 0x00 ]) / 100.0;
}


// ----------------------------------------------------------------------------
static
int     int_read_register ( modbus_t *ctx,
                                    const int registerAddress,
                                    const int numBytes,
                                    const char *description,
                                    const int badReadValue)
{
    assert( (numBytes == 1) || (numBytes == 2) );
    
    uint16_t    buffer[ 32  ];
    memset( buffer, '\0', sizeof buffer );
    
    if (modbus_read_registers( ctx, registerAddress, numBytes, buffer ) == -1) {
        Logger_LogError( "%s - Read of %d bytes at address %X failed: %s\n", description, numBytes, registerAddress, modbus_strerror( errno ));
        return badReadValue;
    }

    if (numBytes == 2) {
        long temp  = buffer[ 0x01 ] << 16;
        temp |= buffer[ 0x00 ];
        return (int) temp;
    }
    
    return ((int) buffer[ 0x00 ]);
}
