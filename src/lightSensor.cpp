#include "lightSensor.h"

LightSensor::LightSensor(RgbColor &rgb) : _rgb(rgb)
{
    configTime(_gmtOffset_sec, _daylightOffset_sec, _ntpServer);

    _ltr = Adafruit_LTR329();
}

void LightSensor::setup()
{
    if (!_ltr.begin())
    {
        Serial.println("Couldn't find LTR sensor!");
         _rgb.setState(ALARM);
        while (1)
            delay(10);
    }
    Serial.println("Found LTR sensor!");

    _ltr.setGain(LTR3XX_GAIN_96);
    _ltr.setIntegrationTime(LTR3XX_INTEGTIME_100);
    _ltr.setMeasurementRate(LTR3XX_MEASRATE_200);
    delay(200);
    calculateNoise();

    Serial.println("Lux sensor setud done");
    Serial.println("Getting the day and time ");

    if (!getLocalTime(&_timeinfo))
    {
        configTime(_gmtOffset_sec, _daylightOffset_sec, "pool.ntp.org");
        if (!getLocalTime(&_timeinfo))
        {
             Serial.println("Failed to obtain time");
             return;
        }
       
        
    }

    Serial.println("Time variables");
    delay(1000);
    _savedDay = readDay();
    _lastHours = readhour();
}

void LightSensor::handelSensor()
{
    readSensor();
    calculateLux();
    handelSunlightLogging();
    delay(30);
}

void LightSensor::setLuxLevel(int lux)
{
    _LuxLevel = lux;
}

void LightSensor::setMinHours(int hours)
{
    _minHours = hours;
}

bool LightSensor::getSendData()
{
    return _isSendData;
}

void LightSensor::setSendData(bool send)
{
    _isSendData = send;
    if (!_isSendData)
    {
        _lightHours.lampLightHours = 0;
        _lightHours.sunLightHours = 0;
    }
}

light_data_t LightSensor::getLightData()
{
    return _lightHours;
}

/// @brief get value at startup and find how much nois +/- sensor has like +240 and -140 that way i can use it to find if value changed
/// @param ch0
/// @param ch1
void LightSensor::setNoise(uint16_t ch0, uint16_t ch1)
{
    if (ch0 > _CH0_OFFSET_VAL && (int)(ch0 - _CH0_OFFSET_VAL > _CH0_MAX_VAL))
    {
        _CH0_MAX_VAL = (int)(ch0 - _CH0_OFFSET_VAL);
    }
    if (ch1 > _CH1_OFFSET_VAL && (int)(ch1 - _CH1_OFFSET_VAL > _CH1_MAX_VAL))
    {
        _CH1_MAX_VAL = (int)(ch1 - _CH1_OFFSET_VAL);
    }

    if (ch0 < _CH0_OFFSET_VAL && ch0 - _CH0_OFFSET_VAL < _CH0_MIN_VAL)
    {
        _CH0_MIN_VAL = (int)(ch0 - _CH0_OFFSET_VAL);
    }
    if (ch1 < _CH1_OFFSET_VAL && ch1 - _CH1_OFFSET_VAL < _CH1_MIN_VAL)
    {
        _CH1_MIN_VAL = (int)(ch1 - _CH1_OFFSET_VAL);
    }
    Serial.print("Ch0 min val: ");
    Serial.print(_CH0_MIN_VAL);
    Serial.print("  Ch0 max val: ");
    Serial.print(_CH0_MAX_VAL);
    Serial.print("  Ch1 min val: ");
    Serial.print(_CH1_MIN_VAL);
    Serial.print("  Ch1 max val: ");
    Serial.println(_CH1_MAX_VAL);
}

void LightSensor::calculateNoise()
{
    int start = millis();
    while (millis() < start + 10000)
    {
        if (_ltr.newDataAvailable())
        {
            _valid = _ltr.readBothChannels(_visible_plus_ir, _infrared);
            if (_valid)
            {
                Serial.println("Inisilizing lux sensor");
            }
        }
        delay(100);
    }
    _valid = false;

    while (!_valid)
    {
        if (_ltr.newDataAvailable())
        {
            _valid = _ltr.readBothChannels(_visible_plus_ir, _infrared);
            if (_valid)
            {
                Serial.println("Getting base line");
                _CH0_OFFSET_VAL = _visible_plus_ir;
                _CH1_OFFSET_VAL = _infrared;
                // know from thesting that it will at least be this.
                _CH0_MIN_VAL = -5;
                _CH1_MIN_VAL = -5;
            }
        }
        delay(100);
    }

    Serial.print("ch0 offset: ");
    Serial.print(_CH0_OFFSET_VAL);
    Serial.print("  ch1 offset : ");
    Serial.println(_CH1_OFFSET_VAL);
    start = millis();
    _valid = false;
    Serial.println("Messuring max and min noice vs base line");
    while (millis() < start + 5000)
    {
        if (_ltr.newDataAvailable())
        {
            _valid = _ltr.readBothChannels(_visible_plus_ir, _infrared);
            if (_valid)
            {
                Serial.print("CH0 Visible + IR: ");
                Serial.print(_visible_plus_ir);
                Serial.print("\t\tCH1 Infrared: ");
                Serial.println(_infrared);
                setNoise(_visible_plus_ir, _infrared);
            }
        }
        delay(20);
    }
}

int LightSensor::readDay()
{
    strftime(_getDay, 3, "%d", &_timeinfo);
    Serial.print("day: ");
    Serial.print(_getDay);
    int days = atoi(_getDay);
    Serial.print("/");
    Serial.println(days);
    return days;
}

int LightSensor::readhour()
{
    strftime(_getHour, 3, "%H", &_timeinfo);
    Serial.print("  Hour: ");
    Serial.println(_getHour);
    int hours = atoi(_getHour);
    Serial.print("/");
    Serial.println(hours);
    return hours;
}

void LightSensor::readSensor()
{
    uint16_t channel0, channel1;
    if (_ltr.newDataAvailable())
    {
        _valid = _ltr.readBothChannels(channel0, channel1);
        if (_valid)
        {
            if (channel0 > _visible_plus_ir + _CH0_MAX_VAL)
            {
                _visible_plus_ir = channel0;
            }
            if (channel0 < _visible_plus_ir + _CH0_MIN_VAL)
            {
                _visible_plus_ir = channel0;
            }

            if (channel1 > _infrared + _CH1_MAX_VAL)
            {
                _infrared = channel1;
            }
            if (channel1 < _infrared + _CH1_MIN_VAL)
            {
                _infrared = channel1;
            }
        }
    }
    // Serial.print("ch0 are: ");
    // Serial.print(_visible_plus_ir);
    // Serial.print("  ch1 are: ");
    // Serial.println(_infrared);
}

bool LightSensor::getIsAlarm()
{
    return _isAlarm;
}

double LightSensor::getLux()
{
    return _lux;
}

bool LightSensor::isSunny()
{

    return _lux >= _LuxLevel;
}

void LightSensor::handelSunlightLogging()
{
    int hours = readhour();
    if(_lastHours <= hours)
    return;
    ColorState state = _rgb.getState();
    if (state == GROW)
    {
        _rgb.setState(GROWOFF);
        int start = millis();
        while (millis() < start + 4000)
        {
            readSensor();
        }
    }

    
    if (!_isCurrentSun &&
        state == GROW &&
        _lightHours.totalHours() + (_lastHours - hours) >= MIN_LIGH_HOURS &&
        !isSunny())
    {
        _lightHours.lampLightHours = _lastHours - hours;
        _lastHours = hours;
    }

    else if (_isCurrentSun != isSunny() &&
             _savedDay == readDay() &&
             _lastHours >= SUN_UP_HOUR)
    {
        calculateTotalLight(hours);
    }
    else if (_savedDay != readDay())
    {
        _isSendData = true;

        _savedDay = readDay();
    }

    if (!isSunny() && _lastHours >= SUN_UP_HOUR && _lightHours.totalHours() < MIN_LIGH_HOURS)
        _rgb.setState(GROW);
    else
        _rgb.setState(NORMAL);
}

void LightSensor::calculateLux()
{
    _ch0 = _visible_plus_ir;
    _ch1 = _infrared;
    uint ALS_GAIN[8] = {1, 2, 4, 8, 1, 1, 48, 96};
    float ALS_INT[8] = {1.0, 0.5, 2.0, 4.0, 1.5, 2.5, 3.0, 3.5};
    ratio = _ch1 / (_ch0 + _ch1);
    uint8_t intg = _ltr.getIntegrationTime();
    uint8_t gain = _ltr.getGain();

    // Determine lux per datasheet equations:
    if (ratio < 0.45)
    {
        _lux = ((1.7743 * _ch0) + (1.1059 * _ch1)) / ALS_GAIN[gain] / ALS_INT[intg];
    }

    else if (ratio < 0.64)
    {
        _lux = ((4.2785 * _ch0) - (1.9548 * _ch1)) / ALS_GAIN[gain] / ALS_INT[intg];
    }

    else if (ratio < 0.85)
    {
        _lux = ((0.5926 * _ch0) + (0.1185 * _ch1)) / ALS_GAIN[gain] / ALS_INT[intg];
    }

    else
        _lux = 0;

    Serial.print("Lux is: ");
    Serial.println(_lux);
}

void LightSensor::calculateTotalLight(int hours)
{

    if (_lightHours.totalHours() >= MIN_LIGH_HOURS && !isSunny())
    {
        return;
    }
    else if (_isCurrentSun)
    {
        if (_lastHours < hours)
        {
            _lightHours.sunLightHours += hours - _lastHours;
            _lastHours = hours;
            _isCurrentSun = isSunny();
        }
    }
    else
    {
        if (_lastHours < hours)
        {
            _lightHours.lampLightHours += hours - _lastHours;
            _lastHours = hours;
            _isCurrentSun = isSunny();
        }
    }
}
