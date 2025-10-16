#ifndef DebugDisplay_h
#define DebugDisplay_h

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

namespace Display{


  bool display_initialized=false;
  Adafruit_SH1107 display = Adafruit_SH1107(64, 128, &Wire);
  void initDisplay(){
    if(display_initialized){
      display.display();
      display.setCursor(0,0);
      display.clearDisplay();
      return;
    }
    display_initialized=true;
    display.begin(0x3C, true); // Address 0x3C default
    display.setRotation(1);
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0,0);
    display.clearDisplay();
    display.println("sup");
    display.display();
  }

  // #define display Serial
  #define display display
  void printDebug(){
    initDisplay();

    if(display_initialized==false){
      //We may not have the module, so just skip past doing anything
      delay(10000);
      return;
    }

    //Print radio information
    unsigned long int w = Radio::watchdog;
    if(w<5000){
      display.print("R:");
      display.print(w);
      display.print("");
      display.println(); 
    }else{
      display.println("R: :("); 
    }
    
    //Print last command
    CannonControl c = Radio::getCommand();
    FSM::State s = FSM::getState();
    display.print("C:[");
    display.print(c.metadata.heartbeat,HEX);
    display.print("] ");
    display.print(c.enable ? "E":" ");
    display.print(c.fire   ? "F":" ");
    display.print(c.load   ? "L":" ");
    display.print(":");
    display.print(c.angle);
    display.println();
    display.println(FSM::getStateString(s));

    display.println();
    CannonTelemetry t = Bot::getBotState();
    CannonSimpleState ss = FSM::getCannonSimpleState(s);
    display.print("T:[");
    display.print(t.metadata.heartbeat);
    display.print("] ");
    // display.print(FSM::getStateString(s));
    display.print(FSM::getCannonSimpleStateString(ss));
    delay(20);
  }


};
#endif