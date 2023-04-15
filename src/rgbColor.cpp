#include "rgbColor.h"
#define NUMPIXELS 1
#define DATAPIN 33
#define CLOCKPIN 21
RgbColor::RgbColor(Adafruit_DotStar &strip) : _strip(strip)
{
    setup();
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
        _strip.setPixelColor(0, 255, 0, 0);
        break;
    case YELLOW: // GROW LIGTH
        _strip.setPixelColor(0, 255, 255, 0);
        break;
    case BLUE: // FIND ME LIGHT
        _strip.setPixelColor(0, 0, 0, 255);
        break;
    case GREEN: // ALL OK LIGTH
        _strip.setPixelColor(0, 0, 255, 0);
        break;
    default:
        break;
    }
    _strip.show();
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
    case ColorState::ALARM:
        if (state != ColorState::ALARMOFF)
            break;
        else
        {
            _state = state;
        }
        break;
    case ColorState::ALARMOFF:
    case ColorState::FINDOFF:
    case ColorState::GROWOFF:
        _state = state;
        break;
    case ColorState::NORMAL:
        if (state == ColorState::ALARMOFF)
            break;
        else
        {
            _state = state;
        }
        break;
    case ColorState::FIND:
        if (state != ColorState::ALARM && state != ColorState::FINDOFF)
            break;
        else
        {
            _state = state;
        }
        break;

    case ColorState::GROW:
        if (state != ColorState::ALARM && state != ColorState::GROWOFF)
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

void RgbColor::handelLight()
{
    switch (_state)
    {
    case ColorState::ALARM:
        setColor(Color::RED);
        break;
    case ColorState::NORMAL:
    case ColorState::ALARMOFF:
    case ColorState::FINDOFF:
    case ColorState::GROWOFF:
        _strip.clear();
        break;
    case ColorState::FIND:
        setColor(Color::BLUE);
        break;
    case ColorState::GROW:
        setColor(Color::YELLOW);
        break;
    default:
        break;
    }
}
