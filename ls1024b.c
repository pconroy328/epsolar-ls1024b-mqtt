#include <stdio.h>
#include <errno.h>
#include <modbus/modbus.h>
#include <string.h>
#include <assert.h>

#include "ls1024b.h"
#include "logger.h"


// -----------------------------------------------------------------------------
void    getRatedData (modbus_t *ctx, RatedData_t *data)
{
    int         registerAddress = 0x3000;
    int         numBytes = 0x09;                  // 0x0A and up gives 'illegal data address' error
    uint16_t    buffer[ 32 ];
    
    memset( buffer, '\0', sizeof buffer );
    
    if (modbus_read_input_registers( ctx, registerAddress, numBytes, buffer ) == -1) {
        fprintf(stderr, "getRatedData() - Read failed: %s\n", modbus_strerror( errno ));
        return;
    }

    
    data->pvArrayRatedVoltage     = ((float) buffer[ 0x00 ]) / 100.0;
    data->pvArrayRatedCurrent     = ((float) buffer[ 0x01 ]) / 100.0;

    long temp  = buffer[ 0x03 ] << 16;
    temp |= buffer[ 0x02 ];
    data->pvArrayRatedPower =  (float) temp / 100.0;

    data->batteryRatedVoltage     = ((float) buffer[ 0x04 ]) / 100.0;
    data->batteryRatedCurrent     = ((float) buffer[ 0x05 ]) / 100.0;
 
    temp  = buffer[ 0x07 ] << 16;
    temp |= buffer[ 0x06 ];
    data->batteryRatedPower =  (float) temp / 100.0;

    data->chargingMode = chargingModeToString( buffer[ 0x08 ] );                  // 0x01 == PWM

    //
    //  Pick up the last value
    registerAddress = 0x300E;
    numBytes = 0x01;
    
    memset( buffer, '\0', sizeof buffer );
    
    if (modbus_read_input_registers( ctx, registerAddress, numBytes, buffer ) == -1) {
        fprintf(stderr, "getRatedData() - Second Read failed of regsiter 0x200E: %s\n", modbus_strerror( errno ));
        return;
    }
    data->ratedCurrentOfLoad = ((float) buffer[ 0 ]) / 100.0;
}


// -----------------------------------------------------------------------------
void    getRealTimeData (modbus_t *ctx, RealTimeData_t *data)
{
    int         registerAddress = 0x3100;
    int         numBytes = 0x12;
    uint16_t    buffer[ 32 ];
    
    memset( buffer, '\0', sizeof buffer );
    
    if (modbus_read_input_registers( ctx, registerAddress, numBytes, buffer ) == -1) {
        fprintf(stderr, "getRealTimeData() - Read failed: %s\n", modbus_strerror( errno ));
        return;
    }
    
    // ---------------------------------------------
    //  Photo Voltaic Values - Volts, Amps and Watts
    data->pvArrayVoltage =  ((float) buffer[ 0x00 ]) / 100.0;
    data->pvArrayCurrent =  ((float) buffer[ 0x01 ]) / 100.0;
    
    //
    // Assemble the Power (watts) value from two words
    long    temp = buffer[ 0x03 ] << 16;
    temp |= buffer[ 0x02 ];
    data->pvArrayPower   =  (float) temp / 100.0;
    
    
    // ---------------------------------------------
    // NB: These two were in the v1.5 spec but are gone from the V2,3 document!
    //  Battery Values - Volts, Amps and Watts
    // data->batteryVoltage =  ((float) buffer[ 0x04 ]) / 100.0;
    // data->batteryCurrent =  ((float) buffer[ 0x05 ]) / 100.0;
    //

    temp = buffer[ 0x07 ] << 16;
    temp |= buffer[ 0x06 ];
    data->batteryPower   =  (float) temp / 100.0;
    
    // ---------------------------------------------
    //  Load Values - Volts, Amps and Watts
    data->loadVoltage =  ((float) buffer[ 0x0C ]) / 100.0;
    data->loadCurrent =  ((float) buffer[ 0x0D ]) / 100.0;

    temp    = buffer[ 0x0F ] << 16;
    temp |= buffer[ 0x0E ];
    data->loadPower   =  (float) temp / 100.0;
    
    
     data->batteryTemp =  C2F( ((float) buffer[ 0x10 ]) / 100.0 );
     data->caseTemp =  C2F( ((float) buffer[ 0x11 ]) / 100.0 );
     
     //
     // According to a newer version of the Protocol (V2.3)
     // the next register is at 0x311A.  So we'll pick those off individually
     data->batterySOC = getBatteryStateOfCharge( ctx );
     data->remoteBatteryTemperature = getRemoteBatteryTemperature( ctx );
     data->batteryRealRatedPower = getBatteryRealRatedPower( ctx );
}


// -----------------------------------------------------------------------------
void    getRealTimeStatus (modbus_t *ctx, RealTimeStatus_t *data)
{
    int         registerAddress = 0x3200;
    int         numBytes = 0x3;
    uint16_t    buffer[ 32 ];
    
    memset( buffer, '\0', sizeof buffer );
    
    if (modbus_read_input_registers( ctx, registerAddress, numBytes, buffer ) == -1) {
        fprintf(stderr, "getRealTimeStatus() - Read failed: %s\n", modbus_strerror( errno ));
        return;
    }
    
    data->batteryStatusValue =  buffer[ 0x00 ];
    decodeBatteryStatusBits( data, data->batteryStatusValue );
    
    data->chargingStatusValue =  buffer[ 0x01 ];
    decodeChargingStatusBits( data, data->chargingStatusValue );

    data->dischargingStatusValue =  buffer[ 0x02 ];
    decodeDischargingStatusBits( data, data->dischargingStatusValue );
}

// -----------------------------------------------------------------------------
void    getStatisticalParameters (modbus_t *ctx, StatisticalParameters_t *data)
{
    int         registerAddress = 0x3300;
    int         numBytes = 0x1E;                  
    uint16_t    buffer[ 32 ];
    
    memset( buffer, '\0', sizeof buffer );
    
    if (modbus_read_input_registers( ctx, registerAddress, numBytes, buffer ) == -1) {
        fprintf(stderr, "getStatisicalParameters() - Read failed: %s\n", modbus_strerror( errno ));
        return;
    }

     data->maximumInputVoltageToday = ((float) buffer[ 0x00 ]) / 100.0;
     data->minimumInputVoltageToday = ((float) buffer[ 0x01 ]) / 100.0;
     data->maximumBatteryVoltageToday = ((float) buffer[ 0x02 ]) / 100.0;
     data->minimumBatteryVoltageToday = ((float) buffer[ 0x03 ]) / 100.0;

    long temp  = buffer[ 0x05 ] << 16;
    temp |= buffer[ 0x04 ];
    data->consumedEnergyToday =   (float) temp / 100.0;

    temp  = buffer[ 0x07 ] << 16;
    temp |= buffer[ 0x06 ];
    data->consumedEnergyMonth =   (float) temp / 100.0;

    temp  = buffer[ 0x09 ] << 16;
    temp |= buffer[ 0x08 ];
    data->consumedEnergyYear =   (float) temp / 100.0;
    
    temp  = buffer[ 0x0B ] << 16;
    temp |= buffer[ 0x0A ];
    data->totalConsumedEnergy =   (float) temp / 100.0;
    
    temp  = buffer[ 0x0D ] << 16;
    temp |= buffer[ 0x0C ];
    data->generatedEnergyToday =   (float) temp / 100.0;
    
    temp  = buffer[ 0x0F ] << 16;
    temp |= buffer[ 0x0E ];
    data->generatedEnergyMonth =   (float) temp / 100.0;

    temp  = buffer[ 0x11 ] << 16;
    temp |= buffer[ 0x10 ];
    data->generatedEnergyYear =   (float) temp / 100.0;

    temp  = buffer[ 0x13 ] << 16;
    temp |= buffer[ 0x12 ];
    data->totalGeneratedEnergy =   (float) temp / 100.0;
    
\
    //
    // Another difference between v1.5 and v2.3 of the spec
    //  skip ahead to more data
    registerAddress = 0x331A;
    numBytes = 0x03;
    memset( buffer, '\0', sizeof buffer );
    
    if (modbus_read_input_registers( ctx, registerAddress, numBytes, buffer ) == -1) {
        fprintf(stderr, "getStatisicalParameters() - Second Read or register 0x331A failed: %s\n", modbus_strerror( errno ));
        return;
    }
   
    data->batteryVoltage =   ((float) buffer[ 0x00 ]) / 100.0;
            
    temp  = buffer[ 0x02 ] << 16;
    temp |= buffer[ 0x01 ];
    data->batteryCurrent = (float) temp / 100.0;
}

// -----------------------------------------------------------------------------
void    getSettings (modbus_t *ctx, Settings_t *data)
{
    int         registerAddress = 0x9000;
    int         numBytes = 0x0F;                    // 0x10 and up gives 'illegal data address' error
    uint16_t    buffer[ 32 ];

    memset( buffer, '\0', sizeof buffer );
    
    if (modbus_read_registers( ctx, registerAddress, numBytes, buffer ) == -1) {
        fprintf(stderr, "getSettings() - Read failed: %s\n", modbus_strerror( errno ));
        return;
    }
    
    data->batteryType =  batteryTypeToString( buffer[ 0x00 ] );
    data->batteryCapacity =  buffer[ 0x01 ];
    
    data->tempCompensationCoeff   = ((float) buffer[ 0x02 ]) / 100.0;
    data->highVoltageDisconnect   = ((float) buffer[ 0x03 ]) / 100.0;
    data->chargingLimitVoltage    = ((float) buffer[ 0x04 ]) / 100.0;
    data->overVoltageReconnect    = ((float) buffer[ 0x05 ]) / 100.0;
    data->equalizationVoltage     = ((float) buffer[ 0x06 ]) / 100.0;
    data->boostVoltage            = ((float) buffer[ 0x07 ]) / 100.0;
    data->floatVoltage            = ((float) buffer[ 0x08 ]) / 100.0;
    data->boostReconnectVoltage   = ((float) buffer[ 0x09 ]) / 100.0;

    //  Our LS1024B controller doesn't seem to support any register data above 0x0A
    data->lowVoltageReconnect     = ((float) buffer[ 0x0A ]) / 100.0;
    data->underVoltageRecover     = ((float) buffer[ 0x0B ]) / 100.0;
    data->underVoltageWarning     = ((float) buffer[ 0x0C ]) / 100.0;
    data->lowVoltageDisconnect    = ((float) buffer[ 0x0D ]) / 100.0;
    data->dischargingLimitVoltage = ((float) buffer[ 0x0E ]) / 100.0;
}



// -----------------------------------------------------------------------------
int     getBatteryStateOfCharge (modbus_t *ctx)
{
    int         registerAddress = 0x311A;
    int         numBytes = 1; 
    uint16_t    buffer[ 32 ];
    
    memset( buffer, '\0', sizeof buffer );
    
    if (modbus_read_input_registers( ctx, registerAddress, numBytes, buffer ) == -1) {
        fprintf(stderr, "getRealTimeData() - Read failed: %s\n", modbus_strerror( errno ));
        return -1;
    }
    
    return buffer[ 0 ];
}

// -----------------------------------------------------------------------------
float   getRemoteBatteryTemperature (modbus_t *ctx)
{
    int         registerAddress = 0x311B;
    int         numBytes = 1; 
    uint16_t    buffer[ 32 ];
    
    memset( buffer, '\0', sizeof buffer );
    
    if (modbus_read_input_registers( ctx, registerAddress, numBytes, buffer ) == -1) {
        fprintf(stderr, "getRemoteBatteryTemp() - Read failed: %s\n", modbus_strerror( errno ));
        return -1;
    }
    //printf( "       remote battery temp sent back: %0x (hex)  %d\n", buffer[ 0 ], buffer[ 0 ] );
    return C2F( (float) buffer[ 0 ] / 100.0 );
}

// -----------------------------------------------------------------------------
float   getBatteryRealRatedPower (modbus_t *ctx)
{
    int         registerAddress = 0x311D;
    int         numBytes = 1; 
    uint16_t    buffer[ 32 ];
    
    memset( buffer, '\0', sizeof buffer );
    
    if (modbus_read_input_registers( ctx, registerAddress, numBytes, buffer ) == -1) {
        fprintf(stderr, "getBatteryRealRatedPower() - Read failed: %s\n", modbus_strerror( errno ));
        return -1;
    }
    
    return ((float) buffer[ 0 ] / 100.0);
}

// -----------------------------------------------------------------------------
void    getRealtimeClock (modbus_t *ctx, int *seconds, int *minutes, int *hour, int *day, int *month, int *year)
{
    int         registerAddress = 0x9013;
    int         numBytes = 0x03;
    uint16_t    buffer[ 32 ];

    memset( buffer, '\0', sizeof buffer );
    
    if (modbus_read_registers( ctx, registerAddress, numBytes, buffer ) == -1) {
        fprintf(stderr, "getRealtimeClock() - Read failed: %s\n", modbus_strerror( errno ));
        return;
    }
    
    *seconds = (buffer[ 0 ] & 0x00FF);
    *minutes = ((buffer[ 0 ] & 0xFF00) >> 8);
    *hour = (buffer[ 1 ] & 0x00FF);
    *day = ((buffer[ 1 ] & 0xFF00) >> 8);
    *month = (buffer[ 2 ] & 0x00FF);
    *year = ((buffer[ 2 ] & 0xFF00) >> 8);
    
    printf( "      buffer[0] = %X    secs = %d    mins = %d \n", buffer[0], *seconds, *minutes);
    printf( "      buffer[1] = %X    hour = %d    day  = %d \n", buffer[1], *hour, *day);
    printf( "      buffer[2] = %X    month= %d    year = %d \n", buffer[1], *month, *year);
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

//------------------------------------------------------------------------------
static
char    *chargingModeToString (uint16_t mode)
{
    switch (mode) {
        case    0x01:   return "PWM";        break;
        default:        return "Unknown";
    }
}

// -----------------------------------------------------------------------------
int     setBatteryTypeAndCapacity (const int batteryType, const int batteryCapacity)
{
    // not sure what asserts() range checking to do
    assert( batteryType >= 0x00 && batteryType <= 0x03 );
}

// -----------------------------------------------------------------------------
int getChargingDeviceStatus (modbus_t *ctx)
{
    int         registerAddress = 0;
    int         numBits = 1;                  
    uint8_t     buffer[ 8 ];
    
    memset( buffer, '\0', sizeof buffer );
    
    Logger_LogDebug( "Getting Charging Device Status (Coil 0)\n" );
    if (modbus_read_bits( ctx, registerAddress, numBits, buffer ) == -1) {
        fprintf(stderr, "getChargingDeviceStatus() - Read bits failed: %s\n", modbus_strerror( errno ));
        return -1;
    }

    //
    //  Not sure if I should have read just one BIT or a full byte
    printf( "    getChargingDeviceStatus() - buffer[0] = %0X (hex)  Bottom bit = %0x\n", buffer[ 0 ], (buffer[ 0 ] & 0b00000001) );
    Logger_LogDebug( "     getChargingDeviceStatus() - buffer[0] = %0X (hex)  Bottom bit = %0x\n", buffer[ 0 ], (buffer[ 0 ] & 0b00000001) );
    
    //
    // Mask off the top 7 just in case
    return ( (buffer[ 0 ] & 0b00000001) );
}

// -----------------------------------------------------------------------------
void    setChargingDeviceStatus (modbus_t *ctx, const int value)
{
    int         registerAddress = 0;

    assert( (value == TRUE) || (value == FALSE) );
    
    Logger_LogDebug( "Setting Charging Device Status (Coil 0) to %0X\n", value );
    
    if (modbus_write_bit( ctx, registerAddress, value ) == -1) {
        fprintf(stderr, "setChargingDeviceStatus() - Read bits failed: %s\n", modbus_strerror( errno ));
        return;
    }
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
    int     coilNum = 13;
    setCoilValue( ctx, coilNum, 1, "Restore System Defaults (Coil 13)" );
}

// -----------------------------------------------------------------------------
void    clearEnergyGeneratingStatistics (modbus_t *ctx)
{
    int     coilNum = 14;
    setCoilValue( ctx, coilNum, 1, "Clear Energy Gen Stats Load (Coil 14)" );
}

// -----------------------------------------------------------------------------
int getOverTemperatureInsideDevice (modbus_t *ctx)
{
    int         registerAddress = 0x2000;
    uint8_t     value = 0;

    Logger_LogDebug( "Getting overTemperatureInsideDevice\n" );
    if (modbus_read_input_bits( ctx, registerAddress, 1, &value ) == -1) {
        fprintf(stderr, "read_input_bits on register %X failed: %s\n", registerAddress, modbus_strerror( errno ));
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
        fprintf(stderr, "read_input_bits on register %X failed: %s\n", registerAddress, modbus_strerror( errno ));
        return -1;
    }
    
    //
    //  Not sure if I should have read just one BIT or a full byte
    Logger_LogDebug( "%s - value = %0X (hex)  Bottom bit = %0x\n", "Getting isNightTime", value, (value & 0b00000001) );
    
    //
    // Mask off the top 7 just in case
    return ( (value & 0b00000001) == 1 );    
}


// -----------------------------------------------------------------------------
static
int     getCoilValue (modbus_t *ctx, const int coilNum, const char *description)
{
    int         numBits = 1;                  
    uint8_t     value = 0;
    
    
    Logger_LogDebug( "%s\n", description );
    if (modbus_read_bits( ctx, coilNum, numBits, &value ) == -1) {
        fprintf(stderr, "%s -- read_bits on coil %d failed: %s\n", description, coilNum, modbus_strerror( errno ));
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
        fprintf(stderr, "write_bit on coil %d failed: %s\n", coilNum, modbus_strerror( errno ));
        return;
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

    data->chargingMOSFETShort = ((value & 0b0010000000000000) ? "SHORT" : "OK");
    data->someMOSFETShort = ((value & 0b0001000000000000) ? "SHORT" : "OK");
    data->antiReverseMOSFETShort = ((value & 0b0000100000000000) ? "SHORT" : "OK");
    data->inputIsOverCurrent = ((value & 0b0000010000000000) ? "Yes" : "No");
    data->loadIsOverCurrent = ((value & 0b0000001000000000) ? "Yes" : "No");
    data->loadIsShort = ((value & 0b0000000100000000) ? "Yes" : "No");
    data->loadMOSFETIsShort = ((value & 0b0000000010000000) ? "Yes" : "No");
    data->pvInputIsShort = ((value & 0b0000000000010000) ? "Yes" : "No");

    switch ((data->chargingStatusValue & 0b0000000000001100) >> 2) {
        case    0x00:   data->chargingStatus = "Not Charging";      break;
        case    0x01:   data->chargingStatus = "Float(ing)";        break;
        case    0x02:   data->chargingStatus = "Boost(ing)";        break;
        case    0x03:   data->chargingStatus = "Equalizing";        break;
        default:        data->chargingStatus = "??";        break;
    }
    
    data->chargingStatusNormal = ((value & 0b0000000000000010) ? "Fault" : "Normal");
    data->chargingStatusRunning = ((value & 0b0000000000000001) ? "Running" : "Standby");
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
    data->dischargingShortCircuit = ( (value &  0b0000100000000000) ? "SHORT" : "OK" );
    data->unableToDischarge = ( (value &        0b0000010000000000) ? "Yes" : "No" );
    data->unableToStopDischarging = ( (value &  0b0000001000000000) ? "Yes" : "No" );
    data->outputVoltageAbnormal = ( (value &    0b0000000100000000) ? "Yes" : "No" );
    data->inputOverpressure = ( (value &        0b0000000010000000) ? "Yes" : "No" );
    //                                            5432109876543210
    data->highVoltageSideShort = ( (value &     0b0000000001000000) ? "Yes" : "No" );
    data->boostOverpressure = ( (value &        0b0000000000100000) ? "Yes" : "No" );
    data->outputOverpressure = ( (value &       0b0000000000010000) ? "Yes" : "No" );
    data->dischargingStatusNormal = ( (value &  0b0000000000000010) ? "Fault" : "Normal" );
    data->dischargingStatusRunning = ( (value & 0b0000000000000001) ? "Running" : "Standby" );
    
}

// -----------------------------------------------------------------------------
static
float   C2F (float tempC)
{
    // T(°F) = T(°C) × 9/5 + 32
    return ((tempC * 9.0 / 5.0) + 32.0);
}