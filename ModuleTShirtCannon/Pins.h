#ifndef PINS_H
#define PINS_H

#include "BoardPins.h"
/// Provide clear pin names to the various ports on the carrier board
namespace Pins{
    struct Quad{
        int a;
        int b;
    };

    struct DoubleSolenoid{
      int a;
      int b;
    };

    int LED = LED_BUILTIN;

    int BarrelMotor = BoardPins::motors.m2;
    Quad BarrelEncoder = {BoardPins::quad1.a,BoardPins::quad1.b};

    int ElevationMotor = BoardPins::motors.m1;
    int ElevationAbsEncoder = BoardPins::quad2.a; //Quad B unused

    int CylinderLock = BoardPins::pnuematics.p1;
    int FiringPlate = BoardPins::pnuematics.p2;
    DoubleSolenoid DumpValve = {BoardPins::pnuematics.p3,BoardPins::pnuematics.p4};
}

#endif