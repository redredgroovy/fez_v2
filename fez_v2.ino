#include <ArduinoJson.h>

// 
// LED Driver Setup
//

// Requires FastLED >= 3.3.0
#include "FastLED.h"
#if FASTLED_VERSION < 3003000
#error "Requires FastLED 3.3.0 or greater"
#endif

//
// This sketch is specific to the Teensy 4.0 platform and makes use of the
// parallel output support implemented as of FastLED 3.3.0
// 
// https://github.com/FastLED/FastLED/wiki/Parallel-Output#parallel-output-on-the-teensy-4
//
// In particular, there are specific sets of consecutive mapped pins which must
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
// These are implicit for Teensy hardware SPI
#define OLED_MOSI_PIN 11
#define OLED_SCK_PIN 13

Adafruit_SSD1306 display(OLED_SCREEN_WIDTH, OLED_SCREEN_HEIGHT, &SPI, OLED_DC_PIN, OLED_RESET_PIN, OLED_CS_PIN);

//
// Sketch globals
//
int gOffsetLookup[NUM_LEDS];

#include "animations/Starfield.h"
Animation* starfield = new Starfield();

//
// Sketch functions
//

void setup() {
    // Initialize serial console and wait up to 2 seconds to see if console will connect
    // TODO: remove when serial console is no longer necessary for debugging
    Serial.begin(9600);
    while(!Serial && millis() < 2000);

    // Calculate the lookup table based on current LED matrix size. We can afford to do
    // this in memory on the Teensy 4.
    for (int x = 0; x < LED_COLS; x++) {
        for (int y = 0; y < LED_ROWS; y++) {
            int offset = (x * LED_ROWS) + y;
            gOffsetLookup[offset] = (x * LED_ROWS) + ((x & 1) == 0 ? y : (LED_ROWS - 1 - y));
        }
    }

    starfield->Setup();
    Serial.println(starfield->GetName());

    // Set the CS (slave select) pin as an output and initialize SPI
    pinMode(OLED_CS_PIN, OUTPUT);
    digitalWrite(OLED_CS_PIN, HIGH);
    SPI.begin();

    FastLED.addLeds<NUM_STRIPS, WS2812B, FASTLED_ANCHOR_PIN, GRB>(leds, NUM_LEDS_PER_STRIP);
    FastLED.setBrightness(50);

    // Automatically scales down brightness as we near power capacity
    FastLED.setMaxPowerInMilliWatts(30000 * 5); // 30A @ 5V
    pinMode(LED_BUILTIN, OUTPUT);
    set_max_power_indicator_LED(LED_BUILTIN);
}

int gFrames = 0;

void loop() {
    fill_solid(leds, NUM_LEDS_PER_STRIP * NUM_STRIPS, CRGB(255, 255, 255));
    FastLED.show();
    gFrames++;

    EVERY_N_SECONDS(1) {
        starfield->Show();
    }

    EVERY_N_SECONDS(60) {
        Serial.println(FastLED.getFPS());
        Serial.println(gFrames);
        float voltage = analogRead(CURRENT_SENSOR_PIN) * (5.0 / 1023.0);
        Serial.println(voltage);
        gFrames = 0;
    }
}
