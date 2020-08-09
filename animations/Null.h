#include "Animation.h"
#include "Arduino.h"

class Null : public Animation {
    public:
        
        Null(const char* name = "null") : Animation(name) {}

        void Setup()
        {
            FastLED.clear();
        }

        void Loop()
        {
        }

};
