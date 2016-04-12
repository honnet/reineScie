#include <ADC.h>  // for Teensy 3.1: get it from http://github.com/pedvide/ADC
#include <MozziGuts.h>
#include <Oscil.h> // oscillator template
#include <tables/sin2048_int8.h> // sine table for oscillator

// use: Oscil <table_size, update_rate> oscilName (wavetable)
// look in .h file of table #included above
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin(SIN2048_DATA);

// use #define for CONTROL_RATE, not a constant
#define CONTROL_RATE 64 // powers of 2 please

int freq = 1000;

///////////////////////////////////////////////////////////////////////////////
#include "Timer.h"

const int trigPin = 22;
const int echoPin = 23;
const int maxDistance = 450; // cm
int distance = 0;            // cm


static bool triggered = false;
static long trigTime = 0; // us
static Timer nonBlockingTimer;


///////////////////////////////////////////////////////////////////////////////

void setup(){
    Serial.begin(115200);         // 9600 is strongly recommended with Mozzi
    startMozzi(CONTROL_RATE);   // set a control rate of 64 (powers of 2 please)

    pinMode(echoPin, INPUT);
    pinMode(trigPin, OUTPUT);
    digitalWrite(trigPin, LOW);
}


void updateControl(){ // runs @ 64 Hz // every 15ms
    nonBlockingPing();
    freq = map(distance, 0, maxDistance, 150, 15000);
    aSin.setFreq(freq); // set the frequency
}


int updateAudio(){ // runs @ 16 KHz // every ~60us
    nonBlockingTimer.update(); // manages the ping pulse triggered below

    return aSin.next(); // return an int signal centred around 0
}


void loop(){
    audioHook(); // required here
}


///////////////////////////////////////////////////////////////////////////////

inline void nonBlockingPing(void) {
    trigTime = mozziMicros();

    // trigger ping if needed:
    if (!triggered)
    {
        // send a 10 us trigger pulse:
        nonBlockingTimer.pulseImmediate(trigPin, 10, HIGH);
        trigTime = mozziMicros();
        Serial.println(trigTime);
        triggered = true;
    }
    else // Check if echo came back
    {
        long elapsed = mozziMicros() - trigTime;

        // did the echo come back ?
        if (digitalRead(echoPin))
        {
            distance = us2cm(elapsed);
            triggered = false;  // restart...
        }

        // did a time-out occur ?
        if (elapsed > cm2us(maxDistance))
        {
            Serial.print("\t\t\ttime-out ");
            distance = 0;       // timeout
            triggered = false;  // restart...
        }

    }
}

inline long us2cm(long microseconds) {
    // The speed of sound is 340 m/s or 29 microseconds per centimeter.
    return microseconds / 58; // twice 29 for round trip
}

inline long cm2us(long centimeters) {
    // The speed of sound is 340 m/s or 29 microseconds per centimeter.
    return centimeters * 58; // twice 29 for round trip
}


/*

inline void nonBlockingPing(void) {
    static Timer nonBlockingTimer;

    nonBlockingTimer.update(); // manages the ping pulse triggered below

    // trigger ping if needed:
    if (!triggered)
    {
        // send a 10 us trigger pulse:
        nonBlockingTimer.pulse(trigPin, 10, HIGH);
        nonBlockingTimer.after(10, pulsedCallback);
    }
    else // Check if echo came back
    {
        long elapsed = mozziMicros() - trigTime;    // TODO half elapsed ?

        // did the echo come back ?
        if (digitalRead(echoPin))
        {
            distance = us2cm(elapsed);
            triggered = false;  // restart...
        }

        // did a time out occur ?
        if (elapsed > cm2us(maxDistance))
        {
            distance = 0;       // timeout
            triggered = false;  // restart...
        }
    }
}

void pulsedCallback() {
    trigTime = mozziMicros();  // counting starts at the end of the pulse
    triggered = true;          // TODO: only do after 10ms ?
}



*/

