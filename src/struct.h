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
    String fw;
    String build;
   
} profile_t;

typedef struct
{
    int sunLightHours;
    int lampLightHours;
    int totalHours(){
        return sunLightHours +lampLightHours;
    }
} lightData_t;

#endif