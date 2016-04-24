/*
   Robotic Handling System - RHS04

   Description:

   Components:
    Analog-Pins:
      - A0: Piezo
      - A1: IR1
      - A2: IR2
      - A3: EndSwitch
      - A4: StartButton
      - A5: ResetButton
    Digital-Pins:
      - D0/1/2: ShiftRegister Claw Motor
        - D0: DS - dataPin
        - D1: ST_CP - latchPin
        - D2: SH_CP - clockPin
      - D3/4/5: ShiftRegister Tower Motor
        - D3: DS - dataPin
        - D4: ST_CP - latchPin
        - D5: SH_CP - clockPin
      - D6: STEP-Pin Tower Motor
      - D7: DIR-Pin Tower Motor
      - D8: STEP-Pin Claw Motor
      - D9: DIR-Pin Claw Motor
      - D10/11/12: ShiftRegister LED
        - D10: DS - dataPin
        - D11: ST_CP - latchPin
        - D12: SH_CP - clockPin
      - D13: unused; (!internal 20k pull-up!)


   Created 22.03.16
   David Fleischlin

   Modified 22.04.16
   David Fleischlin

   Version: 0.8

   Group Project PDP1/2, Team04
    - Marc Bichsel
    - David Fleischlin
    - Yannick Hilpert
    - Christoph Stich
    - Michelle Suter
*/

/*
   Variable-Declaration
*/

//Sensors
#define RESET 19
#define START 18
#define IR1 1
#define IR2 2
#define EndSwitch 3
#define PIEZO 0

//Claw
#define ClawSTEP 8
#define ClawDIR 9
//Shiftregister Claw
#define dataPinClaw 0
#define latchPinClaw 1
#define clockPinClaw 2

//Tower
#define TowerSTEP 6
#define TowerDIR 7
//Shiftregister Tower
#define dataPinTower 3
#define latchPinTower 4
#define clockPinTower 5

//Shiftregister LED
#define dataPinLed 10
#define latchPinLed 11
#define clockPinLed 12

//Claw
byte clawCondition = 0; //0 = open, 1 = closed
const int clawSteps = 30;  //always the same amount of steps to open/close

//stepcount for every route
const int eToC = -20;
const int cToA1 = -25;
const int cToA2 = -15;
const int a1ToB = 50;
const int a2ToB = 40;

//Piezo sensor
int piezoValue = 0;
int piezoCounter = 0;

//IR variable
byte ballPosition = 0; //0 == no ball; 1 == ball at A1; 2 == ball at A2

//LED
byte flashLedState = 0;
unsigned long lastLedFlash = 0;
const unsigned long flashDelay = 500;

//Shiftregisters
byte activeLedBitMask = B10101010;
byte activeClawBitMask = B00001100;
byte activeTowerBitMask = B00001100;

//debouncing Reset-Button
unsigned long lastResetDebounceTime = 0;
const unsigned long resetDebounceDelay = 40;
byte resetButtonState = 0;
byte lastResetButtonState = 0;
byte resetButtonValue;

//debouncing Start-Button
unsigned long lastStartDebounceTime = 0;
const unsigned long startDebounceDelay = 40;
byte startButtonState = 0;
byte lastStartButtonState = 0;
byte startButtonValue;

//debouncing EndSwitch-Button
//byte lastEndReading = LOW;
unsigned long lastEndDebounceTime = 0;
const unsigned long endDebounceDelay = 20;
int endButtonState = 0;
int lastEndButtonState = 0;

/*
   Setup
*/
void setup() {
  //setup analog pins
  pinMode(RESET, INPUT);
  pinMode(START, INPUT);
  pinMode(IR1, INPUT);
  pinMode(IR2, INPUT);
  pinMode(EndSwitch, INPUT);
  pinMode(PIEZO, INPUT);

  //setup digital pins
  pinMode(ClawSTEP, OUTPUT);
  pinMode(ClawDIR, OUTPUT);
  pinMode(dataPinClaw, OUTPUT);
  pinMode(latchPinClaw, OUTPUT);
  pinMode(clockPinClaw, OUTPUT);
  pinMode(TowerSTEP, OUTPUT);
  pinMode(TowerDIR, OUTPUT);
  pinMode(dataPinTower, OUTPUT);
  pinMode(latchPinTower, OUTPUT);
  pinMode(clockPinTower, OUTPUT);
  pinMode(dataPinLed, OUTPUT);
  pinMode(latchPinLed, OUTPUT);
  pinMode(clockPinLed, OUTPUT);

  //setup shiftregisters
  updateShiftRegister(dataPinLed, clockPinLed, latchPinLed, activeLedBitMask);
  updateShiftRegister(dataPinClaw, clockPinClaw, latchPinClaw, activeClawBitMask);
  updateShiftRegister(dataPinTower, clockPinTower, latchPinTower, activeTowerBitMask);

  //first state setup other pins
  digitalWrite(ClawSTEP, LOW);
  digitalWrite(ClawDIR, LOW);
  digitalWrite(TowerSTEP, LOW);
  digitalWrite(TowerDIR, LOW);
}

/*
   Main Loop
*/
void loop() {

  if (getResetSignal() == 1) {
    checkIR();
    openClaw(clawSteps);
    //resetTower();     //turnTower() and checkEndSwitch() simultaneous
    turnTower(50);
  }

  if (getStartSignal() == 1) {
    //getting the ball
    if (ballPosition == 1) {
      turnTower(cToA1);      //stands at C, turns to A1
    }
    else if (ballPosition == 2) {
      turnTower(cToA2);      //stands at C, turns to A2
    }
    closeClaw(clawSteps);
    //updateShiftRegister(dataPinLed, clockPinLed, latchPinLed, activeLedBitMask);

    //delivering the ball
    if (ballPosition == 1) {
      turnTower(a1ToB);      //stands at A1, turns to B
    }
    else if (ballPosition == 2) {
      turnTower(a2ToB);      //stands at A2, turns to B
    }
    //deliverBall();    //openClaw() and checkPiezo() simultaneous
    openClaw(clawSteps);
    //updateShiftRegister(dataPinLed, clockPinLed, latchPinLed, activeLedBitMask);
  }

  flashLED();
}


/*
   Method-Declaration
*/

/*
   Description: checks the Reset-Button's status
   Parameter: -
   Return: returns '1' if the button is being pressed
*/
byte getResetSignal() {
  resetButtonValue = 0;  //button is reseted
  byte resetReading = digitalRead(RESET); //saves the current state

  //did the state change since last check?
  if (resetReading != lastResetButtonState) {
    lastResetDebounceTime = millis(); //resets debouncing timer
  }

  //is the debounce delay time reached?
  if ((millis() - lastResetDebounceTime) > resetDebounceDelay) {

    //did the state change since last time?
    if (resetReading != resetButtonState) {
      resetButtonState = resetReading;

      //is the button being pressed?
      if (resetButtonState == HIGH) {
        resetButtonValue = 1;
      }
    }
  }
  
  lastResetButtonState = resetReading;
  //returns 1 = button is pressed, 0 = button is not pressed
  return resetButtonValue;
}


/*
   Description: checks the Start-Button's status
   Parameter: -
   Return: returns 'true' if the button is being pressed
*/
byte getStartSignal() {
  startButtonValue = 0; //button is reseted
  byte startReading = digitalRead(START); //saves the current state

  //did the state change since last check?
  if (startReading != lastStartButtonState) {
    lastStartDebounceTime = millis(); //resets debouncing timer
  }

  //is the debounce delay time reached?
  if ((millis() - lastStartDebounceTime) > startDebounceDelay) {

    //did the state change since last time?
    if (startReading != startButtonState) {
      startButtonState = startReading;

      //is the button being pressed?
      if (startButtonState == 1) {
        startButtonValue = 1;
      }
    }
  }

  lastStartButtonState = startReading;
  //returns 1 = button is pressed, 0 = button is not pressed
  return startButtonValue;
}


/*
  Description: shifts the bits in the 'updateBitMsk'-byte into the
  defined shiftregister
  Parameter: the shiftregister's data, clock and latch Pin; a bitmask
  Return: -
*/
void updateShiftRegister(byte dataPin, byte clockPin, byte latchPin, byte updateBitMask) {
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, LSBFIRST, updateBitMask);
  digitalWrite(latchPin, HIGH);
}

/*
   LED-Methods
*/

/*
   Description: changes the state of 1 LED; changes only the corresponding
   bit in the 'activeLedBitMask'-variable.
   Parameter:
   Return:
*/
void flashLED() {
  //Led-state is only changed every 'flashDelay'-seconds
  if ((millis() - lastLedFlash) > flashDelay) {
    if (flashLedState == 0) {
      activeLedBitMask = activeLedBitMask | B00000001;
      flashLedState = 1;
    }
    else if (flashLedState == 1) {
      activeLedBitMask = activeLedBitMask & B11111110;
      flashLedState = 0;
    }
    updateShiftRegister(dataPinLed, clockPinLed, latchPinLed, activeLedBitMask);
    lastLedFlash = millis();
  }
}


/*
   Movement-Methods
*/

/*
   Description: distinguishes between negative and positive parameter to set direction, and
   steps as far as defined in the given parameter. de- and accelerates with the drivers
   microstep-function.
   Parameter: needed stepcount
   Return: -
*/
void turnTower(int stepsToTarget) {
  if (stepsToTarget > 0) {  //algebraic sign negative == cw
    digitalWrite(TowerDIR, LOW);  //direction low == cw
  }
  else {  //algebraic sign positive == ccw
    digitalWrite(TowerDIR, HIGH);  //direction high == ccw
  }
  stepsToTarget = abs(stepsToTarget); //deleting algebraic sign
  delay(1); //step-flank at least 1ms after direction change

  stepsToTarget += 52; //compensation for de- and acceleration

  for (int i = 0; i < stepsToTarget; i++) {

    if ((i == 0) || (i == (stepsToTarget - 16))) {
      //16tel Schritte
      activeTowerBitMask = B01111100;
      updateShiftRegister(dataPinTower, clockPinTower, latchPinTower, activeTowerBitMask);
    }
    if ((i == 16) || (i == (stepsToTarget - 24))) {
      //8tel Schritte
      activeTowerBitMask = B01101100;
      updateShiftRegister(dataPinTower, clockPinTower, latchPinTower, activeTowerBitMask);
    }
    if ((i == 24) || (i == (stepsToTarget - 28))) {
      //4tel Schritte
      activeTowerBitMask = B00101100;
      updateShiftRegister(dataPinTower, clockPinTower, latchPinTower, activeTowerBitMask);
    }
    if ((i == 28) || (i == (stepsToTarget - 30))) {
      //2tel Schritte
      activeTowerBitMask = B01001100;
      updateShiftRegister(dataPinTower, clockPinTower, latchPinTower, activeTowerBitMask);
    }
    if (i == 30) {
      //ganze Schritte
      activeTowerBitMask = B00001100;
      updateShiftRegister(dataPinTower, clockPinTower, latchPinTower, activeTowerBitMask);
    }

    //one high-low-flank per step; minmal flank of 1ms; motor possibly needs more
    digitalWrite(TowerSTEP, HIGH);
    delay(1);
    digitalWrite(TowerSTEP, LOW);
    delay(1);

    if (checkEndSwitch() == true) {
      break;
    }
  }
}

/*
   Description: distinguishes between negative and positive parameter to set direction, and
   steps as far as defined in the given parameter. de- and accelerates with the drivers
   microstep-function.
   Parameter: needed stepcount (constant)
   Return: -
*/
void openClaw(int stepsToTarget) {
  if (clawCondition == 1) {
    digitalWrite(ClawDIR, HIGH);  //direction high == ccw
    delay(1); //step-flank at least 1ms after direction change

    stepsToTarget += 52; //compensation for de- and acceleration

    for (int i = 0; i < stepsToTarget; i++) {

      if ((i == 0) || (i == (stepsToTarget - 16))) {
        //16tel Schritte
        activeClawBitMask = B01111100;
        updateShiftRegister(dataPinClaw, clockPinClaw, latchPinClaw, activeClawBitMask);
      }
      if ((i == 16) || (i == (stepsToTarget - 24))) {
        //8tel Schritte
        activeClawBitMask = B01101100;
        updateShiftRegister(dataPinClaw, clockPinClaw, latchPinClaw, activeClawBitMask);
      }
      if ((i == 24) || (i == (stepsToTarget - 28))) {
        //4tel Schritte
        activeClawBitMask = B00101100;
        updateShiftRegister(dataPinClaw, clockPinClaw, latchPinClaw, activeClawBitMask);
      }
      if ((i == 28) || (i == (stepsToTarget - 30))) {
        //2tel Schritte
        activeClawBitMask = B01001100;
        updateShiftRegister(dataPinClaw, clockPinClaw, latchPinClaw, activeClawBitMask);
      }
      if (i == 30) {
        //ganze Schritte
        activeClawBitMask = B00001100;
        updateShiftRegister(dataPinClaw, clockPinClaw, latchPinClaw, activeClawBitMask);
      }

      //one high-low-flank per step; minmal flank of 1ms; motor possibly needs more
      digitalWrite(ClawSTEP, HIGH);
      delay(1);
      digitalWrite(ClawSTEP, LOW);
      delay(1);

      checkPiezo();
    }
    //activeLedBitMask = B10000001;
  }
}

/*
   DDescription: distinguishes between negative and positive parameter to set direction, and
   steps as far as defined in the given parameter. de- and accelerates with the drivers
   microstep-function.
   Parameter: needed stepcount (constant)
   Return: -
*/
void closeClaw(int stepsToTarget) {
  if (clawCondition == 1) {
    digitalWrite(ClawDIR, LOW);  //direction low == cw
    delay(1); //step-flank at least 1ms after direction change

    stepsToTarget += 52; //compensation for de- and acceleration

    for (int i = 0; i < stepsToTarget; i++) {

      if ((i == 0) || (i == (stepsToTarget - 16))) {
        //16tel Schritte
        activeClawBitMask = B01111100;
        updateShiftRegister(dataPinClaw, clockPinClaw, latchPinClaw, activeClawBitMask);
      }
      if ((i == 16) || (i == (stepsToTarget - 24))) {
        //8tel Schritte
        activeClawBitMask = B01101100;
        updateShiftRegister(dataPinClaw, clockPinClaw, latchPinClaw, activeClawBitMask);
      }
      if ((i == 24) || (i == (stepsToTarget - 28))) {
        //4tel Schritte
        activeClawBitMask = B00101100;
        updateShiftRegister(dataPinClaw, clockPinClaw, latchPinClaw, activeClawBitMask);
      }
      if ((i == 28) || (i == (stepsToTarget - 30))) {
        //2tel Schritte
        activeClawBitMask = B01001100;
        updateShiftRegister(dataPinClaw, clockPinClaw, latchPinClaw, activeClawBitMask);
      }
      if (i == 30) {
        //ganze Schritte
        activeClawBitMask = B10000000;
        updateShiftRegister(dataPinClaw, clockPinClaw, latchPinClaw, activeClawBitMask);
      }

      //one high-low-flank per step; minmal flank of 1ms; motor possibly needs more
      digitalWrite(ClawSTEP, HIGH);
      delay(1);
      digitalWrite(ClawSTEP, LOW);
      delay(1);
    }
    //activeLedBitMask = B10100110;
  }
}


/*
  void resetTower() {
  while (checkEndSwitch == false) {
    turnTower(1);
  }
  }


  void deliverBall() { }
*/


/*
   Sensor-Methods
*/

/*
   Description: checks the analog Inputs for IR1 and IR2; input is
   either 0V == 0 or 5V == 1024
   Parameter: -
   Return: -
*/
void checkIR() {
  int ir1Value = analogRead(IR1);
  int ir2Value = analogRead(IR2);
  
  if (ir1Value < 1000) {
    //setting LED for position 1
    activeLedBitMask = activeLedBitMask | B10010000;
    activeLedBitMask = activeLedBitMask & B10011111;
    ballPosition = 1;
  }
  if (ir2Value < 1000) {
    //setting LED for position 2
    activeLedBitMask = activeLedBitMask | B01100000;
    activeLedBitMask = activeLedBitMask & B01101111;
    ballPosition = 2;
  }
  if (((ir1Value > 1000) && (ir2Value > 1000)) || ((ir1Value < 1000) && (ir2Value < 1000))) {
    //setting LED for the case that no position of both positions are occupied
    activeLedBitMask = activeLedBitMask | B01010000;
    activeLedBitMask = activeLedBitMask & B01011111;
    ballPosition = 0;
  }
  
  updateShiftRegister(dataPinLed, clockPinLed, latchPinLed, activeLedBitMask);
}

/*
   Description: checks the analog input's value
   and writes it into the 'piezoValue'-variable. adds as many consecutive
   values as defined in the 'counter'-variable. divides the sum with the
   number of times another value was added.
   Parameter: -
   Return: -
*/
void checkPiezo() {
  int counter = 20;
  int limit = 30;
  if (piezoCounter < counter) {
    piezoValue += analogRead(PIEZO);
    piezoCounter += 1;
  }
  else if (piezoCounter == counter) {
    piezoValue = piezoValue / counter;
    if (piezoValue > limit) {
      //activeLedBitMask = activeLedBitMask | B00001000;
      //activeLedBitMask = activeLedBitMask & B11111011;
      //updateShiftRegister(dataPinLed, clockPinLed, latchPinLed, activeLedBitMask);
    }
    piezoValue = 0;
    piezoCounter = 0;
  }
}

/*
   Description: checks the EndSwitch's status
   Parameter: -
   Return: returns 'true' if the button is being pressed
*/
boolean checkEndSwitch() {
  int endReading = analogRead(EndSwitch); //saves the current state

  //digitalize the analog output
  if (endReading > 800) {
    endReading = 1;
  }
  else {
    endReading = 0;
  }

  //did the state change since last check?
  if (endReading != lastEndButtonState) {
    lastEndDebounceTime = millis(); //resets debouncing timer
  }

  //is the debounce delay time reached?
  if ((millis() - lastEndDebounceTime) > endDebounceDelay) {

    //did the state change since last time?
    if (endReading != endButtonState) {
      endButtonState = endReading;

      if (endButtonState == 1) {
        return true;  //returns if the button is pressed
      }
      else {
        return false;
      }
    }
  }

  lastEndButtonState = endReading;
}

