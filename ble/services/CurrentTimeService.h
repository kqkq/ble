/* 
 * BLE Current Time Service (subset)
 *
 * by ohneta/ Oct. 2015
 */
#ifndef __BLE_CURRENT_TIME_SERVICE_H__
#define __BLE_CURRENT_TIME_SERVICE_H__
 
#include "ble/BLE.h"
#include <time.h>
 
//extern Serial  pc;
 
 
enum BLE_DayofWeek {
    notknown = 0,
    Monday = 1,
    Tuesday,
    Wednesday,
    Thursday,
    Friday,
    Saturday,
    Sunday
};
 
typedef struct {
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  hours;
    uint8_t  minutes;
    uint8_t  seconds;
} BLE_DateTime;
 
typedef struct : BLE_DateTime {
    BLE_DayofWeek   dayOfWeek;
} BLE_DayDateTime;
 
typedef struct BLE_ExactTime256 : BLE_DayDateTime {
    uint8_t fractions256;
} BLE_ExactTime256;
 
typedef struct BLE_CurrentTime : BLE_ExactTime256 {
    uint8_t adjustReason;
} BLE_CurrentTime;
 
#define     BLE_CURRENT_TIME_CHAR_VALUE_SIZE      10
 
/**
 *
 */
class CurrentTimeService {
 
protected:
    Ticker  ticker; 
 
    /**
     * ticker callback.
     * interval = 1sec
     */
    void epochtimePeriodicCallback(void)
    {
        time_t tmpEpochTime = epochTimeByDateTimeBuffer();
        tmpEpochTime++;
        dataTimeBufferByEpochTime(&tmpEpochTime);
    }
 
    void dataTimeBufferByEpochTime(time_t *epochTime)
    {
        struct tm *tmPtr = localtime(epochTime);
 
        *(uint16_t *)&valueBytes[0] = tmPtr->tm_year + 1900;
        valueBytes[2] = tmPtr->tm_mon + 1;
        valueBytes[3] = tmPtr->tm_mday;
        valueBytes[4] = tmPtr->tm_hour;
        valueBytes[5] = tmPtr->tm_min;
        valueBytes[6] = tmPtr->tm_sec;
        valueBytes[7] = (BLE_DayofWeek)((tmPtr->tm_wday == 0) ? 7 : tmPtr->tm_wday);
        valueBytes[8] = 0x00;
        valueBytes[9] = 0x00;
 
        ble.gattServer().write(currentTimeCharacteristic.getValueHandle(), valueBytes, BLE_CURRENT_TIME_CHAR_VALUE_SIZE);
    }
    
    time_t epochTimeByDateTimeBuffer()
    {
        struct tm  timep;
        {
            timep.tm_year  = *(uint16_t *)&valueBytes[0] - 1900;
            timep.tm_mon   = valueBytes[2] - 1;
            timep.tm_mday  = valueBytes[3];
            timep.tm_hour  = valueBytes[4];
            timep.tm_min   = valueBytes[5];
            timep.tm_sec   = valueBytes[6];
            timep.tm_isdst = 0;
        }
        time_t epochTime = mktime(&timep);
    
        return epochTime;
    }
 
public:
    //------------------------------------------------------------------------------------
    /**
     *
     */
    CurrentTimeService(BLE &_ble, BLE_DateTime &initialDateTime) :
        ble(_ble),
        currentTimeCharacteristic(  GattCharacteristic::UUID_CURRENT_TIME_CHAR,
                                    valueBytes, BLE_CURRENT_TIME_CHAR_VALUE_SIZE, BLE_CURRENT_TIME_CHAR_VALUE_SIZE,
                                    GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ
                                    | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY
                                    | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE
                                )                       
    {
        writeDateTime(initialDateTime);
        ticker.attach(this, &CurrentTimeService::epochtimePeriodicCallback, 1.0);
 
        GattCharacteristic *charsTable[] = {&currentTimeCharacteristic};
        GattService  currentTimeService(GattService::UUID_CURRENT_TIME_SERVICE, charsTable, sizeof(charsTable) / sizeof(GattCharacteristic *)   );
 
        ble.addService(currentTimeService);
        ble.onDataWritten(this, &CurrentTimeService::onDataWritten);
    }
 
    /**
     */
    void writeDateTime(BLE_DateTime &dateTime)
    {
        *(uint16_t *)&valueBytes[0] = dateTime.year;
        valueBytes[2] = dateTime.month;
        valueBytes[3] = dateTime.day;
        valueBytes[4] = dateTime.hours;
        valueBytes[5] = dateTime.minutes;
        valueBytes[6] = dateTime.seconds;
 
        // not support
        valueBytes[7] = 0x00;   // day of week
        valueBytes[8] = 0x00;   // Fractions256
        valueBytes[9] = 0x00;   // Adjust Reason
    }
 
    void writeEpochTime(time_t et)
    {
        dataTimeBufferByEpochTime(&et);
    }

    /**
     */
    void readDateTime(BLE_DateTime &dateTime)
    {
        dateTime.year     = *(uint16_t *)&valueBytes[0];
        dateTime.month    = valueBytes[2];
        dateTime.day      = valueBytes[3];
        dateTime.hours    = valueBytes[4];
        dateTime.minutes  = valueBytes[5];
        dateTime.seconds  = valueBytes[6];
    }
 
    time_t readEpochTime()
    {
        return epochTimeByDateTimeBuffer();
    }
 
 
    // for BLE GATT callback (optional)
    virtual void onDataWritten(const GattWriteCallbackParams *params)
    {
        if (params->handle == currentTimeCharacteristic.getValueHandle()) {
            memcpy((void *)&valueBytes, params->data, params->len);
        }
    }
 
protected:
    BLE &ble;
    uint8_t   valueBytes[BLE_CURRENT_TIME_CHAR_VALUE_SIZE];    
    GattCharacteristic  currentTimeCharacteristic;

};
 
#endif /* #ifndef __BLE_CURRENT_TIME_SERVICE_H__*/
 