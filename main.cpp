#include "mbed.h"
#include "arm_book_lib.h"
#include "display.h"

//=====[Defines]=============================================================

//Millisecond delays for wiper control - my edits to test branch - edit #2 CT
#define RAMP_TIME_HIGH      279
#define RAMP_TIME_LOW       372
#define INT_DELAY1          2256     //3000 - 744
#define INT_DELAY2          5256
#define INT_DELAY3          7256

//Servo PWM settings
#define DUTY_MIN 0.025
#define DUTY_MAX 0.120
#define STEP_LOW  0.00285
#define STEP_HIGH 0.0038
#define PERIOD 0.02

// Mode control thresholds
#define LOW_SPEED   0.25
#define INT_MODE    0.5
#define HIGH_SPEED  0.75

// Interval delay control thresholds
#define INT3   0.33
#define INT5   0.66
#define INT8   1.0


//#define NUMBER_OF_AVG_SAMPLES    100   //skip averaging - loop too slow
#define TIME_INCREMENT_MS        10 //Need to be longer than servo period
#define DEBOUNCE_BUTTON_TIME_MS  20


//=====[Declaration and initialization of public global objects]===============

DigitalIn ignition(BUTTON1);  
DigitalIn driverSeat(PE_8); 

DigitalOut ignitionLight(LED2);

AnalogIn wiperPot(A0);
AnalogIn intPot(A1);

PwmOut servo(PF_9);

//=====[Declaration and initialization of public global variables]=============

 
bool engineOn = false;
int accumulatedDebounceButtonTime     = 0;
enum buttonState_t {
    BUTTON_UP,
    BUTTON_DOWN,
    BUTTON_FALLING,
    BUTTON_RISING
};
buttonState_t ignitionState;

enum wiperMode_t {
    HSmode,
    LSmode,
    OFFmode,
    INTmode
};

wiperMode_t modeSetting;

enum intMode_t {
    int1,
    int2,
    int3
};
intMode_t intSetting;

float wiperPos; //position of wiper
//=====[Declarations (prototypes) of public functions]=========================

void inputsInit();
void outputsInit();
void ignitionButtonInit();

void ignitionUpdate();
void setWiperMode();
void wiperControl();
void displayMode();

bool debounceIgnitionUpdate();

//=====[Main function, the program entry point after power on or reset]========

int main()
{
    inputsInit();
    outputsInit();
    while (true) {
        ignitionUpdate();  //set ignition state 
        setWiperMode();   //check wiper controls to set mode
        wiperControl(); //based on wiper mode, control servo
        displayMode(); //display appropriate mode
        delay(TIME_INCREMENT_MS);
    }
}

//=====[Implementations of public functions]===================================

void inputsInit()
{
    driverSeat.mode(PullDown);  //No initialization required for two analog inputs
    ignitionButtonInit();  //Set initial state BUTTON_DOWN for actice high button
}

void outputsInit()
{
    displayInit();
    servo.period(PERIOD);
    ignitionLight = OFF;
}

void ignitionButtonInit()
{
    if( ! ignition ) {
        ignitionState = BUTTON_DOWN;
    } else {
        ignitionState = BUTTON_UP;
    }
}
void ignitionUpdate()
{
    bool ignitionReleasedEvent = debounceIgnitionUpdate();
    
    if ( !engineOn ) {  // Turn engine on if it is off
        if ( driverSeat && ignitionReleasedEvent) {
            engineOn = true;
            ignitionLight = ON;
        }
    }else{             // Turn engine off if it is on
        if ( ignitionReleasedEvent) {
            engineOn = false;
            ignitionLight = OFF;
        }
    }
}

void setWiperMode() {
    char str[100];
    float wiperRead = wiperPot.read();
//    sprintf ( str, "Wiper Pot: %.3f    ", wiperRead);
//    displayCharPositionWrite( 0, 1 );
//    displayStringWrite( str );
    float intervalRead = intPot.read();
//    sprintf ( str, "Int Pot: %.3f    ", intervalRead);
 //   displayCharPositionWrite( 0, 1 );
//    displayStringWrite( str );
  if (engineOn) {
    if (wiperRead <= LOW_SPEED ) {
        modeSetting = OFFmode;
    }else{
        if (wiperRead <= INT_MODE ){
            modeSetting = LSmode;
        }else{
            if (wiperRead <= HIGH_SPEED) {
            modeSetting = INTmode;
            }
            else {
                modeSetting = HSmode;
            }
        }
    }
     if (intervalRead <= INT3 ) {
        intSetting = int1;
    }else{
        if (intervalRead <= INT5 ){
            intSetting = int2;
        }else{
            if (intervalRead <= INT8) {
            intSetting = int3;
            }
        }
    }
    } else {
        modeSetting = OFFmode;  //when engine is off
    }
}

void wiperControl() {
 static int ticker = 0; //keep track of 30ms timer
 static int wipeTime = 0; //keep track of ramp and interfal time
 ticker = ticker + TIME_INCREMENT_MS;
   if (engineOn && (ticker >= 30)) {
            ticker = 0;
            switch (modeSetting) {
                case (OFFmode):  //OFF
                wiperPos = DUTY_MIN;
                wipeTime = 0;
                servo.write(wiperPos);
            break;
                case (LSmode):  //low speed  
                wipeTime = wipeTime + 3*TIME_INCREMENT_MS;
                if (wipeTime <= RAMP_TIME_LOW) {
                wiperPos = wiperPos + STEP_LOW;     
                servo.write(wiperPos);
                }else {
                    if (wipeTime <= (2*RAMP_TIME_LOW)) {
                        wiperPos = wiperPos - STEP_LOW;
                        servo.write(wiperPos);
                        } else {
                            wipeTime = 0;
                            wiperPos = DUTY_MIN;
                            servo.write(wiperPos);
                        }
                }
           break;
                case (HSmode):  
                wipeTime = wipeTime + 3*TIME_INCREMENT_MS;
                 if (wipeTime <= RAMP_TIME_HIGH) {
                wiperPos = wiperPos + STEP_HIGH;     
                servo.write(wiperPos);
                }else {
                    if (wipeTime <= (2*RAMP_TIME_HIGH)) {
                        wiperPos = wiperPos - STEP_HIGH;
                        servo.write(wiperPos);
                        } else {
                            wipeTime = 0;
                            wiperPos = DUTY_MIN;
                        }
                }                         
           break;
            case (INTmode):
            wipeTime = wipeTime + 3*TIME_INCREMENT_MS;
            if (wipeTime <= RAMP_TIME_LOW) {
                wiperPos = wiperPos + STEP_LOW;     
                servo.write(wiperPos);
                }else {
                    if (wipeTime <= (2*RAMP_TIME_LOW)) {
                        wiperPos = wiperPos - STEP_LOW;
                        servo.write(wiperPos);
                        } else {
                            wiperPos = DUTY_MIN;
                            servo.write(wiperPos);
                        }
                }
            switch (intSetting){
              case (int1):
              if (wipeTime > 3000) {
                wipeTime = 0;
              } 
              break;
              case (int2):
              if (wipeTime > 6000) {
                wipeTime = 0;
              } 
              break;
              case (int3):
              if (wipeTime > 8000) {
                wipeTime = 0;
              } 
              break;
              }
            



           default:  //indicate something is wrong with OFF/ON

           break;
       
       }
   }
}

void displayMode() {
    float intervalRead = intPot.read();
    char str[100];
    switch(modeSetting) {
        case OFFmode:
        displayCharPositionWrite( 0, 0 ); //first row, first position
        displayStringWrite("Wiper Mode: OFF");
        break;
        case LSmode:
        displayCharPositionWrite( 0, 0 ); //first row, first position
        displayStringWrite("Wiper Mode: LS");
        break;
        case HSmode:
        displayCharPositionWrite( 0, 0 ); //first row, first position
        displayStringWrite("Wiper Mode: HS");   
        break;
        case INTmode:
        displayCharPositionWrite(0, 0); 
        displayStringWrite("Wiper Mode: INT"); 
        sprintf ( str, "Int Pot: %.3f    ", intervalRead);
        displayCharPositionWrite( 0, 1 );
        displayStringWrite( str );   
        break;
        default:
        displayInit();
        break;
    }
}

bool debounceIgnitionUpdate()
{
    bool ignitionReleasedEvent = false;
    switch( ignitionState ) {

    case BUTTON_UP:
        if( !ignition ) {
            ignitionState = BUTTON_FALLING;
            accumulatedDebounceButtonTime = 0;
        }
        break;

    case BUTTON_FALLING:
        if( accumulatedDebounceButtonTime >= DEBOUNCE_BUTTON_TIME_MS ) {
            if( !ignition ) {
                ignitionState = BUTTON_DOWN;
                ignitionReleasedEvent = true;
            } else {
                ignitionState = BUTTON_UP;
            }
        }
        accumulatedDebounceButtonTime = accumulatedDebounceButtonTime +
                                        TIME_INCREMENT_MS;
        break;

    case BUTTON_DOWN:
        if( ignition ) {
            ignitionState = BUTTON_RISING;
            accumulatedDebounceButtonTime = 0;
        }
        break;

    case BUTTON_RISING:
        if( accumulatedDebounceButtonTime >= DEBOUNCE_BUTTON_TIME_MS ) {
            if( ignition ) {
                ignitionState = BUTTON_UP;
            } else {
                ignitionState = BUTTON_DOWN;
            }
        }
        accumulatedDebounceButtonTime = accumulatedDebounceButtonTime +
                                        TIME_INCREMENT_MS;
        break;

    default:
        ignitionButtonInit();
        break;
    }
    return ignitionReleasedEvent;
}




