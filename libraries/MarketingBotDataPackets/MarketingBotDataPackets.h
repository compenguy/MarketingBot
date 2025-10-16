#ifndef DATAPACKETS_H
#define DATAPACKETS_H

enum PacketType{
    EMPTY, //Must be 0th slot: Captures uninitialized buffers in a non-actionable packet state
    CHASSIS_TELEMETRY,
    CHASSIS_CONTROL,
    CHASSIS_CONFIG,
    CANNON_TELEMETRY,
    CANNON_CONTROL,
    CANNON_CONFIG,
    LED_TELEMETRY,
    LED_CONTROL,
    LED_CONFIG
};

///Simple packet identification format
struct Metadata{
    PacketType type: 4;
    uint8_t heartbeat: 4;
};

// Common definitions for sender/recievers

/// Representation of a shifter state
enum class ChassisGear:bool{
    Low=0,High=1
};

/** Representative Wheel Speeds in in/s
 * Holds a maximum of +/-127in/s
 * Not recommended as an intermediary for math due to overflow/underflow issues
 */
struct ChassisSpeeds{
    int8_t left: 8;
    int8_t right: 8;
};

///
/// Chassis packet definitions
/// 
/** Only necessary to ensure a buffer of all zeros is not interpreted as robot data*/
struct Empty{
    Metadata metadata;
};

struct ChassisTelemetry{
    Metadata metadata;
    /** inches/sec*/
    ChassisSpeeds speed;
    /** PSI*/
    uint8_t pressure: 8;
    /** Battery voltage, in deciVolts. Divide by 10 to get normal volts */
    uint8_t batteryVoltage: 8;
    /** Shifter state */
    ChassisGear gear: 1;
    /** If the chassis is operating*/
    boolean enable: 1;
};

struct ChassisControl{
    Metadata metadata;
    ChassisSpeeds speed;
    ChassisGear gear: 1;
    boolean enable: 1;
};

/**Allow remote systems to save some states
* This allows potential for re-configuration in different terrains
* or with different modules on top
*/
struct ChassisConfig{
    Metadata metadata;
    /// The physical gear arrangement expected for this configuration
    ChassisGear gear;
    /// The encoder ticks per inch travelled
    int encoderRatio;
    /// Max velocity in inches per second
    int maxForwardVelocity;
    /// Max velocity in inches per second
    int maxAngularVelocity;
    /// Feed Forward rate gain
    float kf;
    /// Static gain constant
    float ks;
    /// Proportional gain constant
    float kp;
    /// Integral gain constant
    float ki; //Unused in code
    /// Derivitative gain constant
    float kd; //unused in code
};

//Cannon pnuematic helpers
enum class BarrelLock:bool{locked,unlocked};
enum class FiringPlate:bool{open,closed};
enum class DumpValve:bool{open,closed};

//Simpler state representation for telemetry actions
enum class CannonSimpleState{ NOTREADY=0, READY=1, FIRING=2, LOADING=3 };

struct CannonTelemetry{
    Metadata metadata;
    /** State of the internal FSM. Only valid if same code version on both systems*/
    uint8_t stateDebug:8;
    /** Stable, simplified FSM for user feedback*/
    CannonSimpleState state: 4;
    //Selected Barrel Position; Bitfield with the current position given a 1 to match barrelLoaded 
    uint16_t barrelPosition: 10 ;
    ///Angle in degrees
    uint16_t barrelElevation:8;
    /// Bitfield representing each barrel's loaded state
    uint16_t barrelLoaded :10;
    /// Operating pressure (psi)
    uint16_t pressure :10;

    ///Current state of the firing plate
    FiringPlate firingPlate:1;
    /// Current state of the dump valve; This valve is responsible for opening/closing the primary air gate;
    DumpValve dumpValve:1;
    ///Current state of the index pin
    BarrelLock barrelLock:1;
};

struct CannonControl{
    Metadata metadata;
    uint8_t unused:5; //Placeholder bits
    bool load:1;
    bool fire:1;
    bool enable:1;
    int8_t angle:8;
};

struct CannonConfig{
    Metadata metadata;
    int minAngle:8;
    int maxAngle:8;
};


/*//////////////////////////
//Utility Functions
//////////////////////////*/
  #include <RH_RF95.h>

  union RadioBuffer{
    uint8_t bytes[128]; //Set to max size bitsize of the radio to avoid potential overflow\
    //Streamline inspection, as all packets have this in the first memory location
    Metadata metadata;
    //
    ChassisControl chassisControl;
    ChassisTelemetry chassisTelemetry;
    ChassisConfig chassisConfig;

    CannonControl cannonControl;
    CannonTelemetry cannonTelemetry;
    CannonConfig cannonConfig;
    
    // LedsControl ledsControl;
    // LedsTelemetry ledsTelemetry;
    // LedsConfig ledsConfig;
  };

  /**
  Scan a radio for available packet types, and insert them into the buffer.
  Returns true if new, valid data recieved
  */
  bool updateBuffer(RH_RF95 *radio , RadioBuffer *buffer){
    uint8_t radiobufferlen=42; //This gets set by the recv function; Can be initially set to any non-zero value

    //Scan for command
    if( radio->available() ) {
      bool recieveOK=radio->recv(buffer->bytes, &radiobufferlen);
      if( recieveOK ){
        if(
          radiobufferlen==sizeof(Empty) &&
          buffer->metadata.type==PacketType::EMPTY
        ){
          return true;
        }
        if(
          radiobufferlen==sizeof(CannonControl) &&
          buffer->metadata.type==PacketType::CANNON_CONTROL
        ){
          return true;
        }
        if(
          radiobufferlen==sizeof(CannonTelemetry) &&
          buffer->metadata.type==PacketType::CANNON_TELEMETRY
        ){
          return true;
        }
        if(
          radiobufferlen==sizeof(CannonConfig) &&
          buffer->metadata.type==PacketType::CANNON_CONFIG
        ){
          return true;
        }
        if(
          radiobufferlen==sizeof(ChassisControl) &&
          buffer->metadata.type==PacketType::CHASSIS_CONTROL
        ){
          return true;
        }
        if(
          radiobufferlen==sizeof(ChassisTelemetry) &&
          buffer->metadata.type==PacketType::CHASSIS_TELEMETRY
        ){
          return true;
        }
        if(
          radiobufferlen==sizeof(ChassisConfig) &&
          buffer->metadata.type==PacketType::CHASSIS_CONFIG
        ){
          return true;
        }

        // Some other packet type: This helps print the binary info about what we got
        /* //Comment this line to print the info
        Serial.printf("?(%i) ",radiobufferlen);
        for(int i = 0; i < radiobufferlen; i++){
          for(int j = 0; j < 8; j++){
            Serial.print((buffer->bytes[i]>>(7-j)) &1);
          }
          Serial.print(".");
        }
        Serial.println();
        //*/

      }
      // Serial.println(",");
    }
  // Serial.println(".");

  return false;
  }


#endif
