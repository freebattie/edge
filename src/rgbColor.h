#ifndef RGBCOLOR_H
#define RGBCOLOR_H

#include <Adafruit_DotStar.h>
#include "enum.h"





class RgbColor{
    public:
        RgbColor(Adafruit_DotStar& strip);
        RgbColor(Adafruit_DotStar& strip,int brightness,Color color = Color::GREEN );
        //void setIsLightOn(bool on);
        void setup(int brightness = 100);
        void setColor(Color color);
        //void setIntervalColor(Color color,int intervall);
        //void setDefaultColorIntevall(int intervall );
        void resetColor();
        void setState(ColorState state);
        //void setupRgbLight(int brightness);
        void handelLight();
    private:
    Adafruit_DotStar &_strip;
    Color _defaultColor;
    int _defaultLightIntervall = 0;
    int _lightintervall = 0;
    bool _isLightOn =false;
    bool _isAlarm = false;
    ColorState _state = ColorState::NORMAL;
};

#endif