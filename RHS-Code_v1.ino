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

   Modified 29.03.16
   David Fleischlin

   Version: 0.1

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

#define RESET 5
#define START 4
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

//Schieberegister LED
#define dataPinLed 10
#define latchPinLed 11
#define clockPinLed 12


byte clawCondition = 0; //0 = open, 1 = closed
int clawSteps = 200;  //always the same amount of steps to open/close

int eToC;
int cToA1;
int cToA2;
int a1ToB;
int a2ToB;

int piezoValue = 0;
int piezoCounter = 0;

byte ballPosition = 0; //0 == no ball; 1 == ball at A1; 2 == ball at A2

byte flashLedState = 0;
byte activeLedBitMask = B10101010;

byte activeClawBitMask = B10000001;
byte activeTowerBitMask = B10000001;

//debouncing Reset-Button
byte lastResetReading = LOW;
long lastResetDebounceTime = 0;
long resetDebounceDelay = 50;
int resetButtonState;
int lastResetButtonState = LOW;

//debouncing Start-Button
byte lastStartReading = LOW;
long lastStartDebounceTime = 0;
long startDebounceDelay = 50;
int startButtonState;
int lastStartButtonState = LOW;

//debouncing EndSwitch-Button
byte lastEndReading = LOW;
long lastEndDebounceTime = 0;
long endDebounceDelay = 50;
int endButtonState;
int lastEndButtonState = LOW;

/*
   Setup
*/
void setup() {
  //pinMode();

  // + LED-Setup
  updateShiftRegister(dataPinClaw, clockPinClaw, latchPinClaw, activeClawBitMask);
  updateShiftRegister(dataPinTower, clockPinTower, latchPinTower, activeClawBitMask);
}

/*
   Main Loop
*/
void loop() {

  if (getResetSignal() == true) {
    checkIR();
    openClaw();
    resetTower();     //turnTower() and checkEndSwitch() simultaneous
  }

  if (getStartSignal() == true) {
    //getting the ball
    if (ballPosition = 1) {
      turnTower(cToA1);      //stands at C, turns to A1
    }
    else if (ballPosition = 2) {
      turnTower(cToA2);      //stands at C, turns to A2
    }
    closeClaw();
    //updateShiftRegister(dataPinLed, clockPinLed, latchPinLed, activeLedBitMask);

    //delivering the ball
    if (ballPosition = 1) {
      turnTower(a1ToB);      //stands at A1, turns to B
    }
    else if (ballPosition = 2) {
      turnTower(a2ToB);      //stands at A2, turns to B
    }
    deliverBall();    //openClaw() and checkPiezo() simultaneous
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
   Return: returns 'true' if the button is being pressed
*/
boolean getResetSignal() {
  int resetReading = analogRead(RESET); //saves the current state

  //did the state change since last check?
  if (resetReading != lastResetButtonState) {
    lastResetDebounceTime = millis(); //resets debouncing timer
  }

  //is the debounce delay time reached?
  if ((millis() - lastResetDebounceTime) > resetDebounceDelay) {

    //did the state change since last time?
    if (resetReading != resetButtonState) {
      resetButtonState = resetReading;

      if (resetButtonState == HIGH) {
        return true;  //returns if the button is pressed
      }
    }
  }

  lastResetButtonState = resetReading;
}

/*
   Description: checks the Start-Button's status
   Parameter: -
   Return: returns 'true' if the button is being pressed
*/
boolean getStartSignal() {
  int startReading = analogRead(START); //saves the current state

  //did the state change since last check?
  if (startReading != lastStartButtonState) {
    lastStartDebounceTime = millis(); //resets debouncing timer
  }

  //is the debounce delay time reached?
  if ((millis() - lastStartDebounceTime) > startDebounceDelay) {

    //did the state change since last time?
    if (startReading != startButtonState) {
      startButtonState = startReading;

      if (startButtonState == HIGH) {
        return true;  //returns if the button is pressed
      }
    }
  }

  lastStartButtonState = startReading;
}


/*
  Description: shifts the bits in the 'updateBitMsk'-byte into the
  defined shiftregister
  Parameter: the shiftregister's data, clock and latch Pin; a bitmask
  Return: -
*/
void updateShiftRegister(byte dataPin, byte clockPin, byte latchPin, byte updateBitMask) {
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, updateBitMask);
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
  if (flashLedState = 0) {
    activeLedBitMask = activeLedBitMask | B00000001;
    flashLedState = 1;
  }
  else if (flashLedState = 1) {
    activeLedBitMask = activeLedBitMask & B11111110;
    flashLedState = 0;
  }
  updateShiftRegister(dataPinLed, clockPinLed, latchPinLed, activeLedBitMask);
}


/*
   Movement-Methods
*/

/*
   Description:
   Parameter:
   Return:
*/
void turnTower(int stepsToTarget) {
  if (stepsToTarget > 0) {  //algebraic sign positive == ccw
    digitalWrite(TowerDIR, HIGH);  //direction high == ccw
  }
  else {  //algebraic sign negative == cw
    digitalWrite(TowerDIR, LOW);  //direction low == cw
  }
  stepsToTarget = abs(stepsToTarget); //deleting algebraic sign
  delay(1); //step-flank at least 1ms after direction change

  // rotate; one high-low-flank per step
  for (int i = 0; i < (stepsToTarget + 52); i++) {  //+52 steps for de- and acceleration

    if (i <= 16) {
      //activeTowerBitMask = (16tel Schritte)
    }
    if ((i > 16) && (i <= 24)) {
      //activeTowerBitMask = (8tel Schritte)
    }
    if ((i > 24) && (i <= 28)) {
      //activeTowerBitMask = (4tel Schritte)
    }
    if ((i > 28) && (i <= 30)) {
      //activeTowerBitMask = (2tel Schritte)
    }
    if ((i > 30) && (i <= (stepsToTarget - 30))) {
      //activeTowerBitMask = (ganze Schritte)
    }

    if ((i > (stepsToTarget - 30)) && (i <= (stepsToTarget - 28))) {
      //(2tel Schritte)
    }
    if ((i > (stepsToTarget - 28)) && (i <= (stepsToTarget - 24))) {
      //(4tel Schritte)
    }
    if ((i > (stepsToTarget - 24)) && (i <= (stepsToTarget - 16))) {
      //(8tel Schritte)
    }
    if ((i > (stepsToTarget - 16)) && (i <= stepsToTarget)) {
      //(16tel Schritte)
    }

    updateShiftRegister(dataPinTower, clockPinTower, latchPinTower, activeTowerBitMask);

    //minmal flank of 1ms; motor possibly needs more
    digitalWrite(TowerSTEP, HIGH);
    delay(1);
    digitalWrite(TowerSTEP, LOW);
    delay(1);
  }
}

/*
   Description:
   Parameter:
   Return:
*/
void openClaw() {
  if (clawCondition = 1) {
    digitalWrite(ClawDIR, HIGH);  //direction high == ccw
    delay(1); //step-flank at least 1ms after direction change

    // rotate; one high-low-flank per step
    for (int i = 0; i < (clawSteps + 52); i++) {  //+52 steps for de- and acceleration

      if (i <= 16) {
        //activeClawBitMask = (16tel Schritte)
      }
      if ((i > 16) && (i <= 24)) {
        //activeClawBitMask = (8tel Schritte)
      }
      if ((i > 24) && (i <= 28)) {
        //activeClawBitMask = (4tel Schritte)
      }
      if ((i > 28) && (i <= 30)) {
        //activeClawBitMask = (2tel Schritte)
      }
      if ((i > 30) && (i <= (clawSteps - 30))) {
        //activeClawBitMask = (ganze Schritte)
      }

      if ((i > (clawSteps - 30)) && (i <= (clawSteps - 28))) {
        //(2tel Schritte)
      }
      if ((i > (clawSteps - 28)) && (i <= (clawSteps - 24))) {
        //(4tel Schritte)
      }
      if ((i > (clawSteps - 24)) && (i <= (clawSteps - 16))) {
        //(8tel Schritte)
      }
      if ((i > (clawSteps - 16)) && (i <= clawSteps)) {
        //(16tel Schritte)
      }

      updateShiftRegister(dataPinClaw, clockPinClaw, latchPinClaw, activeClawBitMask);

      //minmal flank of 1ms; motor possibly needs more
      digitalWrite(ClawSTEP, HIGH);
      delay(1);
      digitalWrite(ClawSTEP, LOW);
      delay(1);
    }
  }
}

/*
   Description:
   Parameter:
   Return:
*/
void closeClaw() {
  if (clawCondition = 1) {
    digitalWrite(ClawDIR, LOW);  //direction low == cw
    delay(1); //step-flank at least 1ms after direction change

    // rotate; one high-low-flank per step
    for (int i = 0; i < (clawSteps + 52); i++) { //+52 steps for de- and acceleration

      if (i <= 16) {
        //activeClawBitMask = (16tel Schritte)
      }
      if ((i > 16) && (i <= 24)) {
        //activeClawBitMask = (8tel Schritte)
      }
      if ((i > 24) && (i <= 28)) {
        //activeClawBitMask = (4tel Schritte)
      }
      if ((i > 28) && (i <= 30)) {
        //activeClawBitMask = (2tel Schritte)
      }
      if ((i > 30) && (i <= (clawSteps - 30))) {
        //activeClawBitMask = (ganze Schritte)
      }

      if ((i > (clawSteps - 30)) && (i <= (clawSteps - 28))) {
        //(2tel Schritte)
      }
      if ((i > (clawSteps - 28)) && (i <= (clawSteps - 24))) {
        //(4tel Schritte)
      }
      if ((i > (clawSteps - 24)) && (i <= (clawSteps - 16))) {
        //(8tel Schritte)
      }
      if ((i > (clawSteps - 16)) && (i <= clawSteps)) {
        //(16tel Schritte)
      }

      updateShiftRegister(dataPinClaw, clockPinClaw, latchPinClaw, activeClawBitMask);

      //minmal flank of 1ms; motor possibly needs more
      digitalWrite(ClawSTEP, HIGH);
      delay(1);
      digitalWrite(ClawSTEP, LOW);
      delay(1);
    }
  }
}

/*
   Description:
   Parameter:
   Return:
*/
void resetTower() {
  while (checkEndSwitch == false) {
    turnTower(1);
  }
}

/*
   Description:
   Parameter:
   Return:
*/
void deliverBall() { }


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
  if (analogRead(IR1) > 1000) {
    //activeLedBitMask = B10000001;
    ballPosition = 1;
  }
  else if (analogRead(IR2) > 1000) {
    //activeLedBitMask = B00011000;
    ballPosition = 2;
  }
  else {
    //activeLedBitMask = B00000000;
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
  int limit = 200;
  if (piezoCounter < counter) {
    piezoValue += analogRead(PIEZO);
    piezoCounter += 1;
  }
  else if (piezoCounter == counter) {
    piezoValue = piezoValue / counter;
    if (piezoValue > limit) {
      //activeLedBitMask = B10000001;
    }
    piezoValue = 0;
    piezoCounter = 0;
  }
  else {
    //activeLedBitMask = B00011000;
  }
  //updateShiftRegister(dataPinLed, clockPinLed, latchPinLed, activeLedBitMask);
}

/*
   Description: checks the EndSwitch's status
   Parameter: -
   Return: returns 'true' if the button is being pressed
*/
boolean checkEndSwitch() {
  int endReading = analogRead(EndSwitch); //saves the current state

  //did the state change since last check?
  if (endReading != lastEndButtonState) {
    lastEndDebounceTime = millis(); //resets debouncing timer
  }

  //is the debounce delay time reached?
  if ((millis() - lastEndDebounceTime) > endDebounceDelay) {

    //did the state change since last time?
    if (endReading != endButtonState) {
      endButtonState = endReading;

      if (endButtonState == HIGH) {
        return true;  //returns if the button is pressed
      }
    }
  }

  lastEndButtonState = endReading;
}







/*
   Description:
   Parameter:
   Return:
   Precondition:
   Postcondition:
*/

/*
   void toggleClaw() {
  if(clawCondition == 'o'){ //Claw open
    rotateMotor('claw', 'close');
    clawCondition = 'c';
  }
  else if(clawCondition == 'c'){  //Claw closed
    rotateMotor('claw', 'open');
    clawCondition = 'o';
  }
  }


  void setupClaw(byte bitMaskClaw) {

  }


  void setupTower(byte bitMaskTower) {

  }


   Description: writes the 'activeLedPattern'-variable into the
   shiftregister
   Parameter:
   Return:

  void updateLED() {
  digitalWrite(latchPinLed, LOW);
  shiftOut(dataPinLed, clockPinLed, MSBFIRST, activeLedPattern);
  digitalWrite(latchPinLed, HIGH);
  }


   Description:
   Parameter:
   Return:

  char getTowerPosition() { } //unused?


  boolean getResetSignal() {
  if (analogRead(RESET) == true) {
    return true;
  }
  else {
    return false;
  }
  }
*/
