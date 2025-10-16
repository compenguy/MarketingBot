
#include <MarketingBotDataPackets.h>
#include <elapsedMillis.h>
#include <Encoder.h>
#include <SPI.h>
#include <RH_RF95.h>
#include <Scheduler.h>
#include <Adafruit_SleepyDog.h>
#include "Radio.h"

#include "Buttons.h"
#include "Chassis.h"


#if defined(ARDUINO_SAMD_ZERO) && defined(SERIAL_PORT_USBVIRTUAL)
// Required for Serial on Zero based boards
#define Serial SERIAL_PORT_USBVIRTUAL
#endif

CannonControl cannonControl;
ChassisControl chassisControl;

ChassisTelemetry chassisTelemetry;
CannonTelemetry cannonTelemetry;

struct SleepTime {
  int enabled;
  int disabled;
};
SleepTime sleepChassisControl = { .enabled = 20, .disabled = 100 };
SleepTime sleepCannonControl = { .enabled = 20, .disabled = 100 };
SleepTime sleepTelemetry = {.enabled = 200, .disabled = 20 };

void setup() {
  // put your setup code here, to run once:

  while (!Serial && millis() < 2000) {
    delay(10);
  }
  Serial.println("============");
  Serial.println("==New Boot==");
  Serial.println("============");


  //Do our init steps
  Radio::initialize();
  Buttons::init();

  // Core operating tasks
  // Outside of debugging leave disabled; Radio switching unfortunately causes surprising 
  // jitter on send times. Need additional radio on hardware side
  // Scheduler.startLoop(recieveTelemetry);

  Scheduler.startLoop(updateControls);

  Scheduler.startLoop(sendChassisControl);
  Scheduler.startLoop(sendCannonControl);

  //Show useful info
  // Scheduler.startLoop(printChassisControl);
  // Scheduler.startLoop(printChassisTelemetry);

  //Helpful for debugging hardware
  // Scheduler.startLoop(Buttons::printDebug);
  // Scheduler.startLoop(print_status);
  // Scheduler.startLoop(print);

  // Scheduler.startLoop(debugControlPackets);
}

//For temp code and the like
void loop() {
  delay(2000);
}

/** Monitor hardware + do the things. */
void updateControls(){
  //Hold the Home button to enable the bot
  if( Buttons::home2()>1500 && chassisControl.enable==false ){
    chassisControl.enable=true;
    cannonControl.enable = true;
  }
  else if( Buttons::home2()>1500 && chassisControl.enable==true ){
    //wait to release button
  }
  //Tap to turn off again
  else if( Buttons::home2() && chassisControl.enable==true ){
    chassisControl.enable=false;
    cannonControl.enable =false;
  }
  //Disable if controller is not touched for a while
  //TODO: Not all buttons properly reset the idle timer
  if( Buttons::idleTimer() > 30*1000 ){
    //Handle a controller being forgotten
    chassisControl.enable = false;
    cannonControl.enable = false;
  }

  //Auto power off the controller
  // TODO: This sleeps, but doesn't seem to wake properly on buttons. Leave off for now.
  // if(Buttons::idleTimer() > 2*60*1000 || Buttons::home2() >5000 ){
  //   while(Buttons::home2()); //wait til home button is released
  //   //do a low power mode
  //   unsigned long int sleepms=0;
  //   Serial.println("\n zzz....");
  //   rf95.sleep();
  //   while(Buttons::home2()==false){
  //     sleepms += Watchdog.sleep(1000);
  //     delay(2);
  //   }
  //   while(Buttons::home2()); //wait til home button is released
  //   Serial.printf("...yawn %s\n",sleepms);
  // }

  //CANNOT USE: Some idiot mentor wired this pin to the wheel encoder, so there's a pin conflict
  // analogWrite(LED_BUILTIN, enable ? 255:0);

  Buttons::JoystickReadings stick = Buttons::LeftStick();
  stick.x*=-1; //Convert x to the right to CCW notation for arcade drive
  stick.y*=-1; //Covert y from HID standard of negative-is-away to positive robot forward
  chassisControl.speed = Chassis::arcadeDrive( stick.y, stick.x*.25 );


  //Update the Cannon Module, which is mostly trivial state machine triggers
  cannonControl.angle = Buttons::rollerWheel.read();
  cannonControl.fire = Buttons::a();
  cannonControl.load = Buttons::x();


  delay(10);
}


void printControllerStatus() {
  Serial.println();
  Serial.print("<3 ");
  Serial.print(Radio::rf95.lastRssi());

  delay(2000);
}


elapsedMillis chassisConnectionTimer;
void sendChassisControl() {
  bool ok = Radio::sendChassisControl(chassisControl);
  if( ok ){ chassisConnectionTimer=0; };
  delay(chassisControl.enable? sleepChassisControl.enabled : sleepChassisControl.disabled);
}

elapsedMillis cannonConnectionTimer;
void sendCannonControl(){
  bool ok = Radio::sendCannonControl(cannonControl);
  if( ok ){ cannonConnectionTimer=0; };
  delay(cannonControl.enable? sleepCannonControl.enabled : sleepCannonControl.disabled);
}


/// Handle Poll the system for telemetry information
void recieveTelemetry(){
  Radio::updateBuffer();
  chassisTelemetry = Radio::getChassisTelemetry();
  cannonTelemetry = Radio::getCannonTelemetry();
  delay(chassisControl.enable ? sleepTelemetry.enabled : sleepTelemetry.disabled);
}


void printChassisControl(){
  
  Serial.println();

  Serial.print("> Chassis ");

  Serial.printf(
    "%s",chassisControl.enable?"EN":".."
  );

  Serial.printf(
    "[L%4i R%4i]",
    chassisControl.speed.left,
    chassisControl.speed.right
  );

  Serial.printf(
    "[%s]",
    chassisControl.gear==ChassisGear::High?"HG":"LG"
  );
  Serial.printf(
    "[<3 %i]",
    chassisControl.metadata.heartbeat
  );

  delay(200);
}

void printChassisTelemetry(){
  ChassisTelemetry chassisTelemetry = Radio::getChassisTelemetry();
  Serial.println();

  Serial.print(" <Chassis ");

  Serial.printf(
    "%2s ",chassisTelemetry.enable?"EN":"--."
  );

  Serial.printf(
    "[L%4i R%4i] ",
    chassisTelemetry.speed.left,
    chassisTelemetry.speed.right
  );

  Serial.printf(
    "[%s] ",
    chassisTelemetry.gear==ChassisGear::High?"HG":"LG"
  );

  Serial.printf(
    "[%2i psi] ",
    chassisTelemetry.pressure
  );
  Serial.printf(
    "[%2.1fv]",
    chassisTelemetry.batteryVoltage/10.0
  );

  delay(200);
}

/** Print some very detailed information about the Control packets
* May be necessary to troubleshoot communication related issues
*/
void debugControlPackets(){
  // Serial.print("D ");

  // Serial.printf("<%2i>",chassisControl.metadata.heartbeat);

  // Serial.printf(" %2s ",chassisControl.enable?"EN":"--");

  // //Print a bitfield of the data as it would be sent
  // //Useful to troubleshoot length and offset issues
  // Serial.print(" ");
  // for(int i = 0; i < sizeof(chassisControlData); i++){
  //   for(int j = 0; j < 8; j++){
  //     Serial.print((chassisControlData.buffer[i]>>(7-j)) &1);
  //   }
  //   Serial.print(".");
  // }


  // Serial.println();
  delay(200);
}




