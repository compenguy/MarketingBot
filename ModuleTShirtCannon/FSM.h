#ifndef fsm_h
#define fsm_h

#include "Bot.h"
#include <elapsedMillis.h>

namespace FSM{

  enum State{
    STARTUP,
    PRESSURIZING,
    IDLE,
    IDLE_READY,
    MANUAL_LOADING,
    MANUAL_RELOAD_UNLOCKED,//TODO: Probably unnecessary and can be removed, see note below
    MANUAL_RELOAD_LOCKED,
    FIRING,
    RECOVERY,
    RELOAD_UNLOCKED,
    RELOAD_LOCKED,
    RESET
  };

  enum State state = STARTUP;
  enum State last_state=STARTUP;


  State getState(){
    return state;
  }

  String getStateString(State s){
    switch(s){
    case STARTUP :        return "Startup" ;
    case PRESSURIZING :   return "Pressurizing" ;
    case IDLE :           return "Idle" ;
    case IDLE_READY :     return "Idle_ready" ;
    case MANUAL_LOADING : return "Manual Loading" ;
    case MANUAL_RELOAD_UNLOCKED : return "Manual Reload Unlocked" ;
    case MANUAL_RELOAD_LOCKED : return "Manual Reload Locked" ;
    case FIRING :         return "Firing" ;
    case RECOVERY :       return "Recovery" ;
    case RELOAD_UNLOCKED : return "Reload_unlocked" ;
    case RELOAD_LOCKED :  return "Reload_locked" ;
    case RESET :          return "Reset" ;
    default:              return "Forgot one :(";
    }
  }

  elapsedMillis stateTimer;
  //These are just to track how long the two inputs are held for some transitions
  elapsedMillis triggerTimer;
  elapsedMillis reloadTriggerTimer;

  void run(bool enable, bool cannonTrigger, bool reloadTrigger){
    // Drop these two triggers if we're disabled.
    cannonTrigger &= enable;
    reloadTrigger &= enable; 
    
    //Count hold times
    if(cannonTrigger==false)triggerTimer=0;
    if(reloadTriggerTimer==false)reloadTriggerTimer=0;

    switch(state){
    case STARTUP:
      // When the bot is initialized we don't know what the state is, but don't want to move it beyond the hardware
      // defaults. Wait until the bot is enabled, and then do a reload cycle to ensure everything's all good.
      //TODO: Test.
      Bot::setActuators(BarrelLock::unlocked, FiringPlate::closed, DumpValve::closed, Bot::BarrelMicros.neutral);
      if(enable){
        state=RELOAD_UNLOCKED;
      }
    break;
    case PRESSURIZING:
      Bot::setActuators(BarrelLock::locked, FiringPlate::closed, DumpValve::closed, Bot::BarrelMicros.neutral);

      if (cannonTrigger == false && stateTimer>1000){
        state=IDLE;
      }
    break;
    case IDLE:
      Bot::setActuators(BarrelLock::locked, FiringPlate::closed, DumpValve::closed, Bot::BarrelMicros.neutral);

      //For safety: Always ensure users release the fire button before attempting to fire again.
      // This should be the _only_ transition out of idle
      if (enable==true && cannonTrigger==false){
        state = IDLE_READY;
      }
    break;
    case IDLE_READY:
      Bot::setActuators(BarrelLock::locked, FiringPlate::closed, DumpValve::closed, Bot::BarrelMicros.neutral);

      if (cannonTrigger){
        state = State::FIRING;
      }
      else if(reloadTrigger == true){
        state=MANUAL_LOADING;
      }
    break;
    case MANUAL_LOADING:
      Bot::setActuators(BarrelLock::unlocked, FiringPlate::open, DumpValve::closed, Bot::BarrelMicros.neutral);

      if (reloadTrigger == true && reloadTriggerTimer > 200){
        state = MANUAL_RELOAD_UNLOCKED;
        reloadTriggerTimer = 0;
      }
    break;
    case MANUAL_RELOAD_UNLOCKED:
      //TODO: This state is probably unnecessary
      //Legacy behaviour that it had to spin while unlocked to build up inertia due to 
      //barrel lock mechanical issues. Resolved in 2023.
      Bot::setActuators(BarrelLock::unlocked, FiringPlate::open, DumpValve::closed, Bot::BarrelMicros.neutral);
      if (stateTimer>200){
        state=MANUAL_RELOAD_LOCKED;
      }
    break;
    case MANUAL_RELOAD_LOCKED:
      Bot::setActuators(BarrelLock::locked, FiringPlate::open, DumpValve::closed, Bot::BarrelMicros.forward);
      if (stateTimer>1000){
        state = IDLE;
      }
    break;
    case FIRING:
      Bot::setActuators(BarrelLock::locked, FiringPlate::closed, DumpValve::open, Bot::BarrelMicros.neutral);

      if (stateTimer > 1000){
        state = RECOVERY;
      }
    break;
    case RECOVERY:
      Bot::setActuators(BarrelLock::unlocked, FiringPlate::closed, DumpValve::closed, Bot::BarrelMicros.neutral);

      if (stateTimer > 1000){
        Bot::barrelEncoder.readAndReset();
        state=RELOAD_UNLOCKED;
      }
    break;
    case RELOAD_UNLOCKED:
      Bot::setActuators(BarrelLock::unlocked, FiringPlate::open, DumpValve::closed, Bot::BarrelMicros.forward);

      // if timer expires advance to RELOAD_LOCKED
      if (Bot::barrelEncoder.read() > Bot::barrelEncoderTicksPerBarrel/6){
        state=RELOAD_LOCKED;
      }
      //TODO: This is just a failsafe. Remove?
      if (stateTimer > 2000){
        state=RELOAD_LOCKED;
      }
    break;
    case RELOAD_LOCKED:
      Bot::setActuators(BarrelLock::locked, FiringPlate::open, DumpValve::closed, Bot::BarrelMicros.forward);

      if(stateTimer>1000){
        state=PRESSURIZING;
      }
    break;
    default:
      //This shouldn't happen, but just in case restart the state machine.
      state=STARTUP;
    break;
    }//end switch

    //Check our state transition timer
    if (last_state != state){
      Serial.print("State Transition: ");
      Serial.print(getStateString(last_state));
      Serial.print("-->");
      Serial.print(getStateString(state));
      Serial.println();
      stateTimer=0;
      last_state=state;
      triggerTimer=0;
      reloadTriggerTimer=0;
    }
  delay(10);
  }//function

  CannonSimpleState getCannonSimpleState(State s){
    switch(s){
    case FSM::State::IDLE_READY:
    case FSM::State::IDLE:
      return CannonSimpleState::READY;
    case FSM::State::MANUAL_LOADING:
      return CannonSimpleState::LOADING;
    case FSM::State::FIRING:
    case FSM::State::RECOVERY:
      return CannonSimpleState::FIRING;
    default:
      return CannonSimpleState::NOTREADY;
    }
  }

  String getCannonSimpleStateString(CannonSimpleState s){
    switch(s){
    case CannonSimpleState::READY:
      return "READY";
    case CannonSimpleState::FIRING:
      return "FIRING";
    case CannonSimpleState::LOADING:
      return "LOADING";
    case CannonSimpleState::NOTREADY:
      return "NOTREADY";
    default:
      return "NOTREADY";
    break;
    }
  }

};
#endif