#include "Animation.h"
#include "Arduino.h"

class Matrix : public Animation {
    public:
        
        Matrix(const char* name = "matrix") : Animation(name) {}

        void Setup()
        {
        }

        void Loop()
        {
            Serial.println("Matrix show()");
        }

};
