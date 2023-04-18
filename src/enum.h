#ifndef ENUM_H
#define ENUM_H

enum Color
{
    RED,
    YELLOW,
    BLUE,
    GREEN
};
enum ColorState
{
    ALARM,
    ALARMOFF,
    NORMAL,
    FIND,
    FINDOFF,
    GROW,
    GROWOFF
};
enum status_t {STOPPED, RUNNING, PAUSED,RUNCODE};
enum resolution_t
{
    MICROS,
    MILLIS
};
#endif