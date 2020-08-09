// https://github.com/bblanchon/ArduinoJson
#include <ArduinoJson.h>

// 
// LED Driver Setup
//

// Requires FastLED >= 3.3.0
#include "FastLED.h"
#if FASTLED_VERSION < 3003000
#error "Requires FastLED 3.3.0 or greater"
#endif

// This sketch is specific to the Teensy 4.0 platform and makes use of the
// parallel output support implemented as of FastLED 3.3.0
// 
// https://github.com/FastLED/FastLED/wiki/Parallel-Output#parallel-output-on-the-teensy-4
//
// In particular, there are specific sets of sequential mapped pins which must
// be used when writing parallel output for the Teensy 4:
//
// First:   1,  0, 24, 25, 19, 18, 14, 15, 17, 16, 22, 23, 20, 21, 26, 27
// Second: 10, 12, 11, 13,  6,  9, 32,  8,  7
// Third:  37, 36, 35, 34, 39, 38, 28, 31, 30
//
// For this project, we want four parallel lines so we pick the consecutive
// group of pins 14, 15, 17, 16
//
#define FASTLED_ANCHOR_PIN 14

// 60x16 matrix broken into 4 distinct "strips"
#define LED_COLS 60
#define LED_ROWS 16
#define NUM_STRIPS 4

#define NUM_LEDS LED_COLS * LED_ROWS 
#define NUM_LEDS_PER_STRIP NUM_LEDS / NUM_STRIPS

CRGB leds[NUM_LEDS];

//
// Bluetooth UART Setup
//

#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_UART.h"

#define BLUEFRUIT_HWSERIAL_NAME Serial1 // Rx: pin 0, Tx: pin 1
#define BLUEFRUIT_UART_MODE_PIN 2

Adafruit_BluefruitLE_UART ble(BLUEFRUIT_HWSERIAL_NAME, BLUEFRUIT_UART_MODE_PIN);

//
// Current Sensor Setup
//

#define CURRENT_SENSOR_PIN A9 // pin 23

//
// OLED Display Setup
//

// Using the Adafruit 128x32 SSD1306
#include "Adafruit_SSD1306.h"

#define OLED_SCREEN_WIDTH 128
#define OLED_SCREEN_HEIGHT 32

// These are user-defined
#define OLED_DC_PIN 8
#define OLED_RESET_PIN 9
#define OLED_CS_PIN 10
// These are implicit for Teensy 4.0 hardware SPI
#define OLED_MOSI_PIN 11
#define OLED_SCK_PIN 13

Adafruit_SSD1306 display(OLED_SCREEN_WIDTH, OLED_SCREEN_HEIGHT, &SPI, OLED_DC_PIN, OLED_RESET_PIN, OLED_CS_PIN);

//
// Sketch globals
//

char gBuffer[256];
int gIndex = 0;

int gOffsetLookup[NUM_LEDS];
#define XY(x,y) gOffsetLookup[(y * LED_COLS) + x]

float gPeakCurrent = 0.0;

//
// Animation objects
//

#include "animations/Starfield.h"
#include "animations/Matrix.h"
#include "animations/TwinkleFOX.h"
#include "animations/FauxTV.h"
#include "animations/Pacifica.h"

Animation* gAnimations[] = {
    new TwinkleFOX(),
    new FauxTV(),
    new Pacifica()
};
Animation* gCurrentAnimation;

//
// Sketch functions
//

//
// Scan the names of each animation in the animation array
// Return a pointer to the animation which matches the requested
// name (or the first animation in the list as a fail-safe)
//
Animation* findAnimation(char* name)
{
    for( unsigned int i = 0; i < sizeof(gAnimations) / sizeof(gAnimations[0]); i++) {
        if( !strcmp(gAnimations[i]->Name(), name) ) {
            return gAnimations[i];
        }
    }
    return gAnimations[0];
}

void parseCommand(char* input) {
    Serial.println(input);
    
    StaticJsonDocument<255> doc;
    DeserializationError err = deserializeJson(doc, input);
    if( err ) {
        Serial.println(err.c_str());
        return;
    }

    for(JsonPair p: doc.as<JsonObject>()) {
        if( p.key() == "brt") {
            FastLED.setBrightness(p.value());
        } else if( p.key() == "fx") {
            gCurrentAnimation = findAnimation(p.value());
            gCurrentAnimation->Setup();
        }
    }
}

//
// Regularly poll the Bluetooth serial port for incoming data.
// Read and buffer from UART until we receive a newline, then
// send the received string for processing.
//
void readBluetooth()
{
    while (Serial1 && Serial1.available() > 0) {
        char byte = Serial1.read();

        // Read and buffer from UART until we receive a newline
        if( byte == '\n' ) {
            parseCommand(gBuffer);
            gIndex = 0;
            memset(gBuffer,0,sizeof(gBuffer));
        } else {
            gBuffer[gIndex] = byte;
            gIndex++;
            if(gIndex >= 255) {
                gIndex = 0;
                memset(gBuffer,0,sizeof(gBuffer));
            }
        }

    }
}


//
// Initial sketch setup
//
void setup()
{
    // Initialize serial console and wait up to 2 seconds to see if console will connect
    // TODO: remove when serial console is no longer necessary for debugging
    Serial.begin(9600);
    while(!Serial && millis() < 2000);

    //
    // LED Driver Setup
    //

    // Calculate the lookup table based on current LED matrix size. We can afford to do
    // this in memory on the Teensy 4.
    for (int x = 0; x < LED_COLS; x++) {
        for (int y = 0; y < LED_ROWS; y++) {
            int offset = (x * LED_ROWS) + y;
            gOffsetLookup[offset] = (x * LED_ROWS) + ((x & 1) == 0 ? y : (LED_ROWS - 1 - y));
        }
    }

    FastLED.addLeds<NUM_STRIPS, WS2812B, FASTLED_ANCHOR_PIN, BRG>(leds, NUM_LEDS_PER_STRIP);
    // .setCorrection(TypicalLEDStrip);

    // WS2812B timings get a little noisy above 60Hz
    FastLED.setMaxRefreshRate(60);

    // Automatically scales down brightness as we near power capacity
    FastLED.setMaxPowerInMilliWatts(15000 * 5); // 15A @ 5V
    pinMode(LED_BUILTIN, OUTPUT);
    set_max_power_indicator_LED(LED_BUILTIN);

    FastLED.setBrightness(128);

    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();

    gCurrentAnimation = findAnimation((char*)"pacifica");
    gCurrentAnimation->Setup();

    //
    // OLED Display Setup
    //

    // Set the CS (slave select) pin as an output and initialize SPI
    pinMode(OLED_CS_PIN, OUTPUT);
    digitalWrite(OLED_CS_PIN, HIGH);
    SPI.begin();

    //
    // Bluetooth UART Setup
    //

    Serial1.begin(9600);
}

//
// Sketch execution loop
//
void loop()
{    

    EVERY_N_MILLISECONDS(10) {
        // TODO: Add voltage divider resistors to avoid frying Teensy under real load
        float reading = analogRead(CURRENT_SENSOR_PIN) * (3.3 / 1023.0);
        // Sensor reads 500mV for zero + 133mV per Amp
        float current = (reading - 0.5) / .133;
        if (current > gPeakCurrent) {
            gPeakCurrent = current;
        }
    }

    EVERY_N_MILLISECONDS(250) {
        StaticJsonDocument<255> doc;
        doc["fps"] = FastLED.getFPS();
        doc["amp"] = gPeakCurrent;
        doc["fx"] = gCurrentAnimation->Name();
        doc["brt"] = FastLED.getBrightness();

        if(Serial1 && Serial1.availableForWrite() > 0) {
            serializeJson(doc, Serial1);
            Serial1.println();
        }

        gPeakCurrent = 0.0;
    }

    readBluetooth();
    gCurrentAnimation->Loop();
    FastLED.show();
}

