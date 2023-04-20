#ifndef TIMER_H
#define TIMER_H

#include <Arduino.h>
// #include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <Wire.h>
#include "enum.h"

class Timer
{
public:
    Timer(resolution_t resolution = MILLIS);
    void start();
    void stop();
    uint32_t readElaspedTime();
    void setInterval(int interval);
    status_t checkInterval();
    void reset();

private:
    resolution_t _resolution;
    uint32_t _started;
    uint32_t _current;
    uint32_t _prev;
    uint32_t _interval;
    bool _is_stopped = false;
};
#endif