#include "rgbColor.h"

RgbColor::RgbColor(Adafruit_DotStar &strip) : _strip(strip)
{
}

RgbColor::RgbColor(Adafruit_DotStar &strip, int brightness, Color defaultColor) : _strip(strip)
{
    setup(brightness);
    _defaultColor = defaultColor;
    resetColor();
}

void RgbColor::setup(int brightness)
{
    _strip.begin(); // Initialize pins for output
    _strip.setBrightness(brightness);
    _strip.show();
}

void RgbColor::setColor(Color color)
{
    switch (color)
    {
    case RED: // ALARM LIGHT
        Serial.println("alarm");
        _strip.setPixelColor(0, 255, 0, 0);
        _strip.show();
        break;
    case YELLOW: // GROW LIGTH
        _strip.setPixelColor(0, 255, 255, 0);
        _strip.show();
        break;
    case BLUE: // FIND ME LIGHT
        _strip.setPixelColor(0, 0, 0, 255);
        _strip.show();
        break;
    case GREEN: // ALL OK LIGTH
        _strip.setPixelColor(0, 0, 255, 0);
        _strip.show();
        break;
    default:
        break;
    }
}

void RgbColor::resetColor()
{
    setColor(_defaultColor);
    _strip.show();
}

void RgbColor::setState(ColorState state)
{
    switch (_state)
    {
    case ALARM:
        if (state != ALARMOFF)
            break;
        else
        {
            _state = state;
        }
        break;
    case ALARMOFF:
    case FINDOFF:
    case GROWOFF:
        _state = state;
        break;
    case NORMAL:
        if (state == ALARM)
            break;
        else
        {
            _state = state;
        }
        break;
    case FIND:
        if (state != ALARM && state != FINDOFF)
            break;
        else
        {
            _state = state;
        }
        break;

    case GROW:
        if (state != ALARM && state != GROWOFF)
            break;
        else
        {
            _state = state;
        }
        break;
    default:
        break;
    }
}

ColorState RgbColor::getState()
{
    return _state;
}

void RgbColor::handelLight()
{
    switch (_state)
    {
    case ALARM:
        setColor(RED);
        break;
    case NORMAL:
    case ALARMOFF:
    case FINDOFF:
    case GROWOFF:
        _strip.clear();
        break;
    case FIND:
        setColor(BLUE);
        break;
    case GROW:
        setColor(YELLOW);
        break;
    default:
        break;
    }
    _strip.show();
}
