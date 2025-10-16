#include "MarketingBotDataPackets.h"
#include "avr/pgmspace.h"
#include "wiring_constants.h"

#ifndef radio_h
#define radio_h

#include <SPI.h>
#include <RH_RF95.h>
#include <MarketingBotDataPackets.h>
#include <Optional.h>


namespace Radio{
  #define RF95_FREQ 915.0
  #define RFM95_CS    8
  #define RFM95_INT   3
  #define RFM95_RST   4
  RH_RF95 rf95(RFM95_CS, RFM95_INT);

  const int _watchdogTimeout=300;
  elapsedMillis watchdog;

  //Multipurpose buffer used to convert radio bits into structured data for parsing
  // union RadioBuffer{
  //   uint8_t bytes[128]; //Set to max size bitsize of the radio to avoid potential overflow
  //   PacketType packetType; // All handled packets will have this in the same memory location

  //   CannonControl cannonControl;
  //   CannonTelemetry cannonTelemetry;
  //   // CannonConfig cannonConfig; //TODO this type does not exist yet
  // } buffer;
  RadioBuffer buffer;


  // Keep the last sent version, since we need to track some values
  CannonControl lastCommand={{PacketType::CANNON_CONTROL,0},false,false,false}; 
  // CannonConfig lastConfig={{PacketType::CANNON_CONFIG}; //Does not exist yet 
  
  void initialize(){
    //Do a full reset of things
    pinMode(RFM95_RST, OUTPUT);
    digitalWrite(RFM95_RST, LOW);
    delay(100);
    digitalWrite(RFM95_RST, HIGH);
    delay(100);

    if (!rf95.init()){
      Serial.println("init failed");
    } else {
      Serial.println("Radio online...");
    }

    rf95.setModemConfig(RH_RF95::Bw500Cr45Sf128);  // 500hz data rate; Default is 128
    rf95.setFrequency(915.0);
    rf95.setTxPower(23, false);
  }

  void debugbuffer(){
    // Serial.print("+++++");
    // Serial.printf("?(%i) ",buffer.bytes);
    Serial.printf("?(%i|%2i) ",buffer.metadata.type, buffer.metadata.heartbeat);
    

    for(int i = 0; i < 6; i++){
      for(int j = 0; j < 8; j++){
        Serial.print((buffer.bytes[i]>>(7-j)) &1);
      }
      Serial.print(".");
    }
    Serial.println();
  }


  void sendTelemetry(CannonTelemetry data){
    return; //TODO REMOVE ME 
    // Serial.println();
    // Serial.print("#");
    bool sent=false;

    data.metadata.heartbeat += 1;
    //Clone the data off to our tx buffer.
    buffer.metadata.type=PacketType::CANNON_TELEMETRY;
    buffer.cannonTelemetry=data;
    sent=rf95.send(buffer.bytes, sizeof(CannonTelemetry));
    // Serial.print(sent ? ">>" : "--" );
    bool done = rf95.waitPacketSent(200);
    // Serial.print(done ? "++" : "--" );
  }

  /** Returns the last good command state, disabling the bot should the watchdog time out */
  CannonControl getCommand(){
    if(watchdog > _watchdogTimeout) lastCommand.enable=false;
    return lastCommand;
  }

  void updateBuffer(){
    bool update = updateBuffer(&rf95,&buffer);

    // Avoid handling the same packet twice
    if(lastCommand.metadata.type==buffer.metadata.type && lastCommand.metadata.heartbeat==buffer.metadata.heartbeat){
      return;
    }
    // debugbuffer(); //Print the buffer in a bytewise format for debugging
    switch(buffer.metadata.type){
    case PacketType::CANNON_CONTROL:
      lastCommand=buffer.cannonControl;
      watchdog=0;
    break;
    default:
      // debugbuffer();
    break;
    }
  }

  ///Mock valid command inputs for debug purposes
  void updateBufferTestMode(){
    static CannonControl c;
    c.metadata.type = PacketType::CANNON_CONTROL;
    c.metadata.heartbeat+=1;
    //Move the angle up and down over a few seconds
    c.angle = sin(millis()/3.14/1000)*20+20;
    //Enable the bot after 3 seconds
    c.enable = millis()>3000;
    // Every 10 seconds tap the fire button
    c.fire = millis()%10000 < 250;
    //Not tested
    c.load = false;
    watchdog=0;
    lastCommand=c;
  }


}

#endif