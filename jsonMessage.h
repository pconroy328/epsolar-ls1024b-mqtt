/*
 */

/* 
 * File:   jsonMessage.h
 * Author: pconroy
 *
 * Created on October 3, 2018, 8:34 AM
 */

#ifndef JSONMESSAGE_H
#define JSONMESSAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <modbus/modbus.h>
#include "ls1024b.h"
   

extern char *createJSONMessage( modbus_t *ctx, const char *topic, const RatedData_t *ratedData, 
        const RealTimeData_t *rtData, const RealTimeStatus_t *rtStatusData, 
        const Settings_t *setData, const StatisticalParameters_t *stats );


#ifdef __cplusplus
}
#endif

#endif /* JSONMESSAGE_H */

