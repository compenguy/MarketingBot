#ifndef BOT_H
#define BOT_H

#include "Pins.h"
#include <Servo.h>
#include <Encoder.h>
#include "ElevationEncoder.h"
#include "MiniPID.h"
#include "RelativeToAbs.h"
#include "MarketingBotDataPackets.h"

namespace Bot{

  Encoder barrelEncoder(Pins::BarrelEncoder.a, Pins::BarrelEncoder.b);
  int barrelEncoderTicksPerBarrel = ((4096*30)/24);

  Servo barrelServo;
  struct {
    int min=1000;
    int neutral=1500;
    int max=2000;
    int forward=110; //nominal forward speed
  } BarrelMicros;

  MiniPID elevationPID = MiniPID(10,0.0,0.0);

  Servo elevationServo;
  struct {
    int min=1000;
    int neutral=1500;
    int max=2000;
  } ElevationMicros;

  CannonTelemetry _telemetry;

  /** Initialize all the hardware to proper states*/
  void initialize(){
    pinMode(Pins::LED, OUTPUT);
    pinMode(Pins::CylinderLock, OUTPUT);
    pinMode(Pins::FiringPlate,OUTPUT);
    pinMode(Pins::DumpValve.a,OUTPUT);
    pinMode(Pins::DumpValve.b,OUTPUT);

    //Configure the motors, set default stall speed
    elevationServo.attach(Pins::ElevationMotor);
    elevationServo.writeMicroseconds(BarrelMicros.neutral);
    barrelServo.attach(Pins::BarrelMotor);
    barrelServo.writeMicroseconds(ElevationMicros.neutral);

    //Configure Elevation handling
    elevationPID.setOutputLimits(-175,175);
    elevationPID.setMaxIOutput(20);
    ElevationEncoder::ConfigElevationPWM(Pins::ElevationAbsEncoder);

    //Arbitrary delay to make sure all the hardware is fully set up.
    delay(30);
  }

  void led(boolean output){
    digitalWrite(Pins::LED, output);
    // analogWrite(Pins::LED,output?64:0);
  }

  void setBarrelLock(BarrelLock state){
    _telemetry.barrelLock=state;
    if(state==BarrelLock::locked){
      digitalWrite(Pins::CylinderLock,LOW);
    }else{
      digitalWrite(Pins::CylinderLock,HIGH);
    }
  }

  void setDumpValve(DumpValve state){
    _telemetry.dumpValve=state;
    if(state==DumpValve::open){
      digitalWrite(Pins::DumpValve.a,LOW);
      digitalWrite(Pins::DumpValve.b,HIGH);
    }else{
      digitalWrite(Pins::DumpValve.a,HIGH);
      digitalWrite(Pins::DumpValve.b,LOW);
    }
  }

  /// Takes relative speed with zero being neutral
  void setBarrelRotation(int speed){
      // Serial.print("speed ");
      // Serial.println(speed);
      //TODO: feed forwards, feedback, and other stuff
      //TODO: Make units something sensible
      int range=(BarrelMicros.max-BarrelMicros.min)/2;
      speed = map(speed,-range,range,BarrelMicros.min,BarrelMicros.max);
      speed = constrain(speed,BarrelMicros.min,BarrelMicros.max);
      analogWrite(Pins::BarrelMotor,speed);
  }

  void setFiringPlate(FiringPlate state){
    _telemetry.firingPlate=state;
    if(state==FiringPlate::open){
      digitalWrite(Pins::FiringPlate,LOW);
    }else{
      digitalWrite(Pins::FiringPlate,HIGH);
    }
  }

  /** Set all FSM oriented actuators at once,
  *  making sure nothing can be omitted
  */
  void setActuators(
    BarrelLock barrelLock,
    FiringPlate firingPlate,
    DumpValve dumpValve,
    int rotation
  ){
    setBarrelLock(barrelLock);
    setFiringPlate(firingPlate);
    setDumpValve(dumpValve);
    setBarrelRotation(rotation);
  }


  RelativeToAbs cannonAngleFilter(10,40);
  void setElevation(double targetElevation){
    double currentAngle=ElevationEncoder::ReadElevationDegrees();
    //TODO apply feed-forward?
    // double kcos = 0.0*Math.cos(currentAngle);
    // targetElevation = constrain(targetElevation,10,40); //Old, straightforward method
    targetElevation = cannonAngleFilter.get(targetElevation); // TODO test, hould handle relative inputs better
    double pidoutput= elevationPID.getOutput(currentAngle,targetElevation);
    elevationServo.writeMicroseconds(ElevationMicros.neutral+pidoutput);
  }

  /** Set the filter so current input matches current system state, meaning no motion*/
  void resetElevationTargets(double value){
    cannonAngleFilter.set(value,ElevationEncoder::ReadElevationDegrees());
  }


  ///Generate a partially complete telemetry object with some of the bot actuation details
  CannonTelemetry getBotState(){
    _telemetry.barrelElevation=ElevationEncoder::ReadElevationDegrees();
    _telemetry.pressure=0; //TODO: do not have electronic pressure sensor
    _telemetry.barrelPosition=0; //TODO: Get barrel rotation from encoder
    _telemetry.barrelLoaded=0; //TODO: Once laser is installed, we can check for barrel positions during rotation
    return _telemetry;
  }

}

#endif