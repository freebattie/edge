
#include "roomStatus.h"
RoomStatus::RoomStatus(RgbColor &rgb) : _rgb(rgb)
{
    _lis = Adafruit_LIS3DH();
    pinMode(HALL_SENSOR_PIN, INPUT);
    _doorTimer = Timer();
}

void RoomStatus::setup()
{
    _startTimer = millis();
    _doorTimer.setInterval(5000);
    _doorTimer.start();
    if (!_lis.begin(0x18))
    { // change this to 0x19 for alternative i2c address

        Serial.println("Couldnt start acc sensor");
        _rgb.setState(ALARM);
        while (1)
            yield();
    }

    _rgb.handelLight();

    _lis.setRange(LIS3DH_RANGE_2_G); // 2, 4, 8 or 16 G!
    // Adjust how often the board should read the sensor with this command
    _lis.setDataRate(LIS3DH_DATARATE_100_HZ);
    _lis.read();
    _lis.getEvent(&_event);
    while (millis() < _startTimer + 2000)
    {
        Serial.print(_startTimer + 2000 - millis());
        Serial.println(" ms untill Window Offsett is set please dont move device");
        delay(100);
    }
    _lis.read();
    _lis.getEvent(&_event);
    _windowOffsett = _event.acceleration.x;
}

void RoomStatus::handelSensors()
{

    readWindowAngelSensor();
    readDoorSensor();
}

float RoomStatus::getWindowAngel()
{
    return _windowAngel;
}

bool RoomStatus::getIsWindowOpen()
{
    Serial.print("window angel");
    Serial.println(_windowAngel);
    if (_windowAngel > 3)
        return true;

    return false;
}

bool RoomStatus::getIsDoorOpen()
{

    return _isDoorOpen;
}

void RoomStatus::readWindowAngelSensor()
{
    _lis.read();
    _lis.getEvent(&_event);
    // device 90 by 10 instead of 9.81 since the sensor shows 10 when at 90 degreees and not the ccorrect value of 9.81
    // this is not the perfect way to calculat but its ok for testing if window is open or closed;
    _windowAngel = (_event.acceleration.x - _windowOffsett) * (float)((float)90.0 / (float)10);
}

void RoomStatus::readDoorSensor()
{
    bool tmp = digitalRead(HALL_SENSOR_PIN);
    if (!tmp)
    {
        _isDoorOpen = digitalRead(HALL_SENSOR_PIN);
        _startDoorTimer = false;
        _doorTimer.stop();
    }
    else if (_doorTimer.checkInterval() == RUNCODE || _doorTimer.checkInterval() == STOPPED)
    {
        _isDoorOpen = digitalRead(HALL_SENSOR_PIN);
        _doorTimer.stop();
    }
    else
    {
        if (!_startDoorTimer)
        {
            _doorTimer.reset();
            _doorTimer.start();
            _startDoorTimer = true;
        }
    }
}
