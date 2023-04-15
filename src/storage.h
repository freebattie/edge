#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
// #include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <Preferences.h>
#include "arduino_secrets.h"


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

class Storage
{
public:
    Storage();
    ~Storage();
    void save();
    void start();
    void load();
    profile_t getProfile();
    bool isRoom();
    void saveProfile(profile_t profile);

private:
    
};
#endif