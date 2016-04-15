#include <ADC.h>
#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/cos2048_int8.h> // table for Oscils to play
#include <mozzi_fixmath.h>
#include <EventDelay.h>
#include <mozzi_rand.h>
#include <mozzi_midi.h>


// audio oscils
Oscil<COS2048_NUM_CELLS, AUDIO_RATE> aCarrier(COS2048_DATA);
Oscil<COS2048_NUM_CELLS, AUDIO_RATE> aModulator(COS2048_DATA);
Oscil<COS2048_NUM_CELLS, AUDIO_RATE> aModDepth(COS2048_DATA);

// for scheduling note changes in updateControl()
EventDelay  kNoteChangeDelay;

// synthesis parameters in fixed point formats
Q8n8 ratio; // unsigned int with 8 integer bits and 8 fractional bits
Q24n8 carrier_freq; // unsigned long with 24 integer bits and 8 fractional bits
Q24n8 mod_freq; // unsigned long with 24 integer bits and 8 fractional bits

// for random notes
Q8n0 octave_start_note = 42;


///////////////////////////////////////////////////////////////////////////////
const int trigPin = 22;
const int echoPin = 23;
const int maxDistance = 400.0; // cm
float distance = 0.0;          // cm
typedef enum state {
    ready, pulseStarted, pulseSent, waitForEchoEnd, waitForNewPulse
} state_t;
state_t sonarState = ready;

#define DEBUG_PRINT 1 // 1 is normal, 2 is verbose
///////////////////////////////////////////////////////////////////////////////


void setup() {
#if DEBUG_PRINT > 0
    Serial.begin(9600);         // 9600 is strongly recommended with Teensy & Mozzi
#endif
    pinMode(echoPin, INPUT);
    pinMode(trigPin, OUTPUT);
    digitalWrite(trigPin, LOW);

    ratio = float_to_Q8n8(9.0f);   // define modulation ratio in float and convert to fixed-point
    randSeed(); // reseed the random generator for different results each time the sketch runs
    startMozzi(); // use default CONTROL_RATE 64
}


inline float distMap(float near, float far) {
    return map(distance, 0.0, maxDistance, near, far);
}


void updateControl() {
#if 0 //DEBUG_PRINT > 0
    static int distOld = 0;
    if (distOld != int(distance)) {
        Serial.println(int(distance));
        distOld = int(distance);
    }
#endif

    kNoteChangeDelay.set(distMap(60, 300));   // note duration ms, within resolution of CONTROL_RATE
    aModDepth.setFreq(distMap(6, 60));      // vary mod depth to highlight am effects

    //////////////////////////////////////////////////////////////////////

    static Q16n16 last_note = octave_start_note;

    if (kNoteChangeDelay.ready()) {

        last_note = distMap(127, 1); // TODO Tune

        // change step up or down a semitone occasionally
        if (rand((byte)3) == 0) {
            last_note += 1 - rand((byte)2);
        }

        // change modulation ratio
        ratio = ((Q8n8) 1 + rand((byte)9)) << 8;

        // sometimes add a fraction to the ratio
        if (rand((byte)5) == 0) {
            ratio += rand((byte)127);
        }

        // convert midi to frequency
        Q16n16 midi_note = Q8n0_to_Q16n16(last_note);
        carrier_freq = Q16n16_to_Q24n8(Q16n16_mtof(midi_note));

        // calculate modulation frequency to stay in ratio with carrier
        mod_freq = (carrier_freq * ratio) >> 8; // (Q24n8   Q8n8) >> 8 = Q24n8

        // set frequencies of the oscillators
        aCarrier.setFreq_Q24n8(carrier_freq);
        aModulator.setFreq_Q24n8(mod_freq);

        // reset the note scheduler
        kNoteChangeDelay.start();
    }
}


int updateAudio() {
    nonBlockingPing();

    unsigned int mod = (128u + aModulator.next()) * ((byte)128 + aModDepth.next());
    int out = ((long)mod * aCarrier.next()) >> 10; // 16 bit   8 bit = 24 bit, then >>10 = 14 bit
    return out;
}


void loop() {
    audioHook();
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

#if DEBUG_PRINT > 1
    static int oldState = 0;
    if (oldState != sonarState) {
        Serial.print("\t");
        Serial.println(sonarState);
        oldState  = distance;
    }
#endif

    switch (sonarState) {
        case ready : // start triggering a pulse
            {
                if (0 == digitalRead(echoPin)) {
                    trigTime = mozziMicros();
                    digitalWrite(trigPin, HIGH);
                    sonarState = pulseStarted;
                }
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
                    sonarState = waitForEchoEnd;
                }
            }
            break;

        case waitForEchoEnd : // stop counting when echo falls
            {
                if (0 == digitalRead(echoPin)) {
                    distance = us2cm(elapsed);
                    Serial.print("0 400 ");
                    Serial.println(distance);
                    sonarState = waitForNewPulse;
                }

                // did a time-out occur ?
                if (elapsed > 120000) {
                    sonarState = ready;
                }

            }
            break;

        case waitForNewPulse : // wait before new pulse
            {
                if (elapsed > 120000) { // TODO: find minimum possible
                    sonarState = ready;
                }
            }
            break;

        default:
            // nothing for now
            break;
    }
}

inline float us2cm(float microseconds) {
    // The speed of sound is 340 m/s or 29 microseconds per centimeter.
    return microseconds / 58.0; // twice 29 for round trip
}

inline float cm2us(float centimeters) {
    // The speed of sound is 340 m/s or 29 microseconds per centimeter.
    return centimeters * 58.0; // twice 29 for round trip
}

