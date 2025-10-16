#ifndef radio_h
#define radio_h

#include <MarketingBotDataPackets.h>
#include <elapsedMillis.h>
namespace Radio {

  #define RF95_FREQ 915.0
  #define RFM95_CS 8
  #define RFM95_INT 3
  #define RFM95_RST 4
  RH_RF95 rf95(RFM95_CS, RFM95_INT);


  /** 
  // This buffer is a simple union type containing all marketing bot packet types
  // This allows easy unpacking and packing of the arrays into the byte array format
  // needed for 
  // union RadioBuffer {
  //   ChassisControl chassisControl;
  //   ChassisTelemetry chassisTelemetry;
  //   CannonControl cannonControl;
  //   CannonTelemetry cannonTelemetry;
  //   uint8_t buffer[128];  //Set to max size bitsize of the radio to avoid potential overflow
  // } radioBuffer;
  //*/
  RadioBuffer buffer;


  elapsedMillis chassisTelemetryTimer = 10000;
  elapsedMillis cannonTelemetryTimer = 10000;
  ChassisTelemetry chassisTelemetry;
  CannonTelemetry cannonTelemetry;

  void initialize() {
    //Do radio bringup
    pinMode(RFM95_RST, OUTPUT);
    digitalWrite(RFM95_RST, LOW);
    delay(100);
    digitalWrite(RFM95_RST, HIGH);
    delay(100);

    if (!rf95.init()) {
      Serial.println("init failed");
    } else {
      Serial.println("Radio online...");
    }

    rf95.setModemConfig(RH_RF95::Bw500Cr45Sf128);  // 500hz data rate; Default is 128
    rf95.setFrequency(915.0);
    rf95.setTxPower(23, false);
  }

  void debugRadioBuffer(){
    Serial.print("D ");

    Serial.printf("<%2i>",buffer.metadata.heartbeat);

    // Serial.printf(" %2s ",buffer.chassisTelemetry.enable?"EN":"--");

    //Print a bitfield of the data as it would be sent
    //Useful to troubleshoot length and offset issues
    Serial.print(" ");
    int size=6;
    // size=sizeof(radioBuffer.buffer);
    // size=sizeof(12);
    for(int i = 0; i < size; i++){
      for(int j = 0; j < 8; j++){
        Serial.print((buffer.bytes[i]>>(7-j)) &1);
      }
      Serial.print(".");
    }
    Serial.print("\n");
  }


  void updateBuffer(){
    //This function in MarketingBotDataPackets.h
    bool update = updateBuffer(&rf95,&buffer);
    if(update==false)return;

    switch(buffer.metadata.type){
    case PacketType::CANNON_TELEMETRY:
      cannonTelemetry=buffer.cannonTelemetry;
      break;
    case PacketType::CHASSIS_TELEMETRY:
      chassisTelemetry=buffer.chassisTelemetry;
      break;
    }
  }

  /// Print a binary representation of the buffer for diagnostic purposes
  void debugbuffer(){
    Serial.printf("?(%i) ",buffer.bytes);
    for(int i = 0; i < 6; i++){
      for(int j = 0; j < 8; j++){
        Serial.print((buffer.bytes[i]>>(7-j)) &1);
      }
      Serial.print(".");
    }
    Serial.println();
  }

  ///Actually transmit a prepared radio buffer
  bool flush() {
    uint waitcounter=0;
    //If we're mid-recieve, let the radio do it's thing.
    while (rf95.mode() == RHGenericDriver::RHModeRx && rf95.isChannelActive() && waitcounter++<5){
      delayMicroseconds(100);
    }
    bool sent = false;
    int length=12;
    switch(buffer.metadata.type){
    case PacketType::CHASSIS_CONTROL:
      length=sizeof(ChassisControl);
      break;
    case PacketType::CANNON_CONTROL:
      length=sizeof(CannonControl);
      break;
    case PacketType::CANNON_CONFIG:
      length=sizeof(CannonConfig);
      break;
    case PacketType::CHASSIS_CONFIG:
      length=sizeof(ChassisConfig);
      break;
    }      

    // debugbuffer();
    sent = rf95.send(buffer.bytes, length);
    if(sent==false) return false;
    bool done = rf95.waitPacketSent(200);
    return done;
  }

  bool sendChassisControl(ChassisControl data) {
    static uint8_t heartbeat=0;
    buffer.chassisControl = data;
    buffer.chassisControl.metadata.heartbeat = heartbeat;
    buffer.chassisControl.metadata.type = PacketType::CHASSIS_CONTROL;
    heartbeat+=1;
    return flush();
  }

  bool sendCannonControl(CannonControl data) {
    static int heartbeat=0;
    buffer.cannonControl = data;
    buffer.cannonControl.metadata.type = PacketType::CANNON_CONTROL;
    buffer.chassisControl.metadata.heartbeat = heartbeat;
    heartbeat+=1;
    debugRadioBuffer();
    return flush();
  }

  ChassisTelemetry getChassisTelemetry() {
    return chassisTelemetry;
  }
  CannonTelemetry getCannonTelemetry() {
    return cannonTelemetry;
  }

}

#endif