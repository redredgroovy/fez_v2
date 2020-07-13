#include "Animation.h"
#include "Arduino.h"

class Starfield : public Animation {
    public:
        
        void Setup() {
            Serial.println("[  ] Initializing animation 'starfield'");
        }
        void Show() {
            Serial.println("Starfield show()");
        }

    protected:
        const static int Name = 4;

};
