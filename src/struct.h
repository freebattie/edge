#ifndef STRUCT_H
#define STRUCT_H

#include <Arduino.h>
typedef struct
{
    String deviceName;
    String mqtt_pass;
    String mqtt_username;
    String location;
    String city;
    int fw;
    bool isAutoUpdateOn;
    String build;
    int version; // 0 == latest else version

} profile_t;

typedef struct
{
    int sunLightHours;
    int lampLightHours;
    int totalHours()
    {
        return sunLightHours + lampLightHours;
    }
} light_data_t;

typedef struct
{
    float temp;
    float humidity;
    float lux;
} live_values_t;
typedef struct
{
    bool isHighTempAlarm;
    bool isLowTempAlarm;
    bool isHighTempWarning;
    bool isLowTempWarning;
    bool isLogging;
    bool isHighHumidityAlarm;
    bool isLowHumidityAlarm;
    bool isHighHumidityWarning;
    bool isLowHumidityWarning;
    bool isHeater;
    bool isHeaterFailed;
    bool isDoorOpen;
    bool isWindowOpen;
    bool isWindowOpenAlarm;
    bool isWindowClosedAndHotOutside;
    bool isDeviceAlarm;
    bool isFindMe;
} sensor_flag_t;

typedef struct
{
    bool alarmsNO;
    bool warningON;
    bool sendLightData;
    bool sendliveData;
    bool statusON;
    bool debug;
} mqtt_testing_t;

#endif