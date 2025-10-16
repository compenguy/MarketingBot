#include "./Bot.h"
#include "./FSM.h"
#include "./Radio.h"
#include <elapsedMillis.h>
#include <MarketingBotDataPackets.h>
#include <Scheduler.h>
#include "DebugDisplay.h"

void setup() {
  Serial.begin(15200);

  while(!Serial.available() && millis()<2000){delay(1);};
  Serial.println("Booting...");


  Radio::initialize();
  Bot::initialize();
  FSM::run(false,false,false);

  Scheduler.startLoop(scanForCommands);
  Scheduler.startLoop(runRobot);
  Scheduler.startLoop(updateTelemetry);

  //Optional interface to 
  Scheduler.startLoop(Display::printDebug);
  Serial.println("Running...");
}

void loop() {
  Bot::led(millis()%1024<20); //simple heartbeat
  delay(20);
}

void runRobot(){
  CannonControl command = Radio::getCommand();

  if(command.enable){
    FSM::run(command.enable, command.fire, command.load);
    Bot::setElevation(command.angle);
  } else {
    FSM::run(false, false, false);
  }

  delay(10);
}

void scanForCommands(){
  //Scan for packets 
  Radio::updateBuffer();
  delay(1);
}

void updateTelemetry(){
  CannonTelemetry telemetry = Bot::getBotState();

  telemetry.metadata.type = PacketType::CANNON_TELEMETRY;
  telemetry.metadata.heartbeat +=1 ;

  //Clean up the states to be more clear on the telemetry side
  telemetry.stateDebug = FSM::getState();
  telemetry.state = FSM::getCannonSimpleState(FSM::getState());

  Radio::sendTelemetry(telemetry);

  //Send slower when enabled to minimize radio mode switching
  delay(Radio::getCommand().enable ? 2000 : 200);
}



