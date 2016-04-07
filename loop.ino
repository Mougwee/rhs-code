void loop() {

  if (getResetSignal() == true) {
    checkIR();
    openClaw(clawSteps);
    //resetTower();     //turnTower() and checkEndSwitch() simultaneous
    turnTower(50);
  }

  if (getStartSignal() == true) {
    //getting the ball
    if (ballPosition = 1) {
      turnTower(cToA1);      //stands at C, turns to A1
    }
    else if (ballPosition = 2) {
      turnTower(cToA2);      //stands at C, turns to A2
    }
    closeClaw(clawSteps);
    //updateShiftRegister(dataPinLed, clockPinLed, latchPinLed, activeLedBitMask);

    //delivering the ball
    if (ballPosition = 1) {
      turnTower(a1ToB);      //stands at A1, turns to B
    }
    else if (ballPosition = 2) {
      turnTower(a2ToB);      //stands at A2, turns to B
    }
    //deliverBall();    //openClaw() and checkPiezo() simultaneous
    openClaw(clawSteps);
    //updateShiftRegister(dataPinLed, clockPinLed, latchPinLed, activeLedBitMask);
  }

  flashLED();
}
