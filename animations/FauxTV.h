#include "Animation.h"
#include "Arduino.h"

// Courtesy of Mark Kriegsman and FastLED
// https://github.com/FastLED/FastLED/blob/master/examples/XYMatrix/XYMatrix.ino

void DrawOneFrame( byte startHue8, int8_t yHueDelta8, int8_t xHueDelta8)
{
    byte lineStartHue = startHue8;
    for( byte y = 0; y < LED_ROWS; y++) {
        lineStartHue += yHueDelta8;
        byte pixelHue = lineStartHue;      
        for( byte x = 0; x < LED_COLS; x++) {
            pixelHue += xHueDelta8;
            leds[ XY(x,y) ] = CHSV( pixelHue, 255, 255);
        }
    }
}


class FauxTV : public Animation {
    public:
        
        FauxTV(const char* name = "fauxtv") : Animation(name) {}

        void Setup()
        {
        }

        void Loop()
        {
            uint32_t ms = millis();
            int32_t yHueDelta32 = ((int32_t)cos16( ms * 27 ) * (350 / LED_COLS));
            int32_t xHueDelta32 = ((int32_t)cos16( ms * 39 ) * (310 / LED_ROWS));
            DrawOneFrame( ms / 65536, yHueDelta32 / 32768, xHueDelta32 / 32768);
        }

};