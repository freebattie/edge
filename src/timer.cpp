
#include "Arduino.h"
#include "timer.h"

Timer::Timer(resolution_t resolution )
{
    _resolution = MILLIS;
}
void Timer::start()
{
    if (_resolution == MILLIS){
         _started = millis();
         _prev = millis();

    }
       
    if (_resolution == MICROS){
        _started = micros();
        _prev = micros();
    }
       
        
}

uint32_t Timer::readElaspedTime()
{
    if (_resolution == MILLIS)
        return millis() - _started;
    if (_resolution == MICROS)
        return micros() - _started;

        else return 0;
}
void Timer::stop(){
    _is_stopped = true;
}
void Timer::setInterval(int interval)
{
    _interval = interval;
}
status_t Timer::checkInterval()
{
    if(_is_stopped) return STOPPED;
    if (_resolution == MILLIS)
        _current = millis();
    if (_resolution == MICROS)
        _current = micros();

    if (_current - _prev > _interval)
    {
        
        return RUNCODE;
       
    }
    return RUNNING;
}

void Timer::reset()
{
     _prev = _current;
}
