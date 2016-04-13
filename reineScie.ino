#include <ADC.h>  // for Teensy 3.1: get it from http://github.com/pedvide/ADC
#include <MozziGuts.h>
#include <Oscil.h> // oscillator template
#include <tables/sin2048_int8.h> // sine table for oscillator

// use: Oscil <table_size, update_rate> oscilName (wavetable)
// look in .h file of table #included above
Oscil <SIN2048_NUM_CELLS, AUDIO_RATE> aSin(SIN2048_DATA);

// use #define for CONTROL_RATE, not a constant
#define CONTROL_RATE 64 // powers of 2 please

int freq = 440;

///////////////////////////////////////////////////////////////////////////////
const int trigPin = 22;
const int echoPin = 23;
const int maxDistance = 150; // cm
int distance = 0;            // cm
typedef enum state {ready, pulseStarted, pulseSent, waitForEchoEnd, wait_30ms} state_t;
state_t sonarState = ready;


///////////////////////////////////////////////////////////////////////////////

void setup(){
    Serial.begin(9600);         // 9600 is strongly recommended with Mozzi
    startMozzi(CONTROL_RATE);   // set a control rate of 64 (powers of 2 please)

    pinMode(echoPin, INPUT);
    pinMode(trigPin, OUTPUT);
    digitalWrite(trigPin, LOW);
}


void updateControl(){ // runs @ 64 Hz // every 15ms
    if (distance) {
        freq = map(distance, 0, maxDistance, 150, 15000);
        aSin.setFreq(freq); // set the frequency
    }
}


int updateAudio(){ // runs @ 16 KHz // every ~60us
    nonBlockingPing();

    return aSin.next(); // return an int signal centred around 0
}


void loop(){
    audioHook(); // required here
}


///////////////////////////////////////////////////////////////////////////////
/*
Sonar principle: http://www.accudiy.com/download/HC-SR04_Manual.pdf

Summary:
- send a 10 us pulse (yes, microsecs, not milis)
- wait ~450 ms till the sonar acknowledges the trigger request (echo raises)
- measure the echo pulse width, it will determine the distance
- wait ~30 ms before sending another pulse

       -> 10us <-   . <-       wait 30ms for new pulse         ->.
         .____.     .                                            .____
trigger _|    |_____.____________________________________________|    |__...
              .
            ->.450ms.<-
              .     .______________
echo    ______._____|  represents  |_____________________________________...
                    .<- distance ->.
*/

inline void nonBlockingPing(void) {
    static long trigTime = 0; // us
    long elapsed = mozziMicros() - trigTime;

    switch (sonarState) {
        case ready : // start triggering a pulse
            {
                digitalWrite(trigPin, HIGH);
                sonarState = pulseStarted;
            }
            break;

        case pulseStarted : // finish the pulse
            {
                if (elapsed > 10) {
                    digitalWrite(trigPin, LOW);
                    sonarState = pulseSent;
                }
            }
            break;

        case pulseSent : // start counting when echo raises
            {
                if (1 == digitalRead(echoPin)) {
                    trigTime = mozziMicros();
                    sonarState = waitForEchoEnd;
                }
            }
            break;

        case waitForEchoEnd : // stop counting when echo falls
            {
                if (0 == digitalRead(echoPin)) {
                    distance = us2cm(elapsed);

                    Serial.println(distance);       // TODO REMOVE !!!

                    sonarState = wait_30ms;
                }

                // did a time-out occur ?
                if (elapsed > cm2us(maxDistance)) {
                    distance = 0; // timeout
                    sonarState = ready;             // TODO : wait 30ms ?
                }

            }
            break;

        case wait_30ms : // wait 30ms before new pulse
            {
                if (elapsed > 30000) {
                    sonarState = ready;
                }
            }
            break;

        default:
            // nothing for now
            break;
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

