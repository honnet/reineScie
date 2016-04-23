# reineScie

Mozzi + non-blocking sonar measure experiment.

## What ?

Concept : A sculpture half-man, half-dolphin. From abyssal sounds to machine
entrails, it communicates algorithmically with visitors.

Artistic Technique : unrolled poplar wood (1), elastomer (2) laminated wood
on tire.

Digital Technique : ultrasonic distance sensor (3), micro-controller for sensor
and sound synthesis(4), speakers.

(1) crate, (2) latex, (3) sonar, (4) using Sensorium/Mozzi library.


## How ?

This project was developed with the HC-SR04 ultrasonic range sensor.
The common version has several problems, but it can be improved:

    http://uglyduck.ath.cx/ep/archive/2014/01/Making_a_better_HC_SR04_Echo_Locator.html
    http://wiki.jack.tf/hc-sr04

Once our sonar was fixed, we needed to make it work in parallel with mozzi
which means that the classic NewPing driver for arduino would not work as
it uses delays and/or interruptions.
This code thus implements a non-blocking driver with a state machine, see
the comments + text-base graph in the code.

