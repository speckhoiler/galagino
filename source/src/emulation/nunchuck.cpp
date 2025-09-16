#include "Nunchuck.h"

#ifdef NUNCHUCK_INPUT
void Nunchuck::setup() {
  Wire.begin(NUNCHUCK_SDA, NUNCHUCK_SCL);
  if (!nchuk.connect()) {
    Serial.println("Nunchuk on bus #1 not detected!");
    delay(1000);
  }

  timeout = millis();
  enable();
}

void Nunchuck::enable() {
  enabled = true;
}

void Nunchuck::disable() {
  enabled = false;
}

unsigned char Nunchuck::getInput() {
  // update every 100ms only
  if(!enabled || millis() - timeout < 100)
    return lastValue;
  else
    timeout = millis();

  bool success = nchuk.update();  // Get new data from the controller

  if (!success) {  // Ruh roh
    Serial.println("Nunchuck disconnected!");
    return 0;
  }
  else {
    // Read a joystick axis (0-255, X and Y)
    // Roughly 127 will be the axis centered
    int joyY = nchuk.joyY();
    int joyX = nchuk.joyX();
    lastValue = ((joyX < 127 - NUNCHUCK_MOVE_THRESHOLD) ? BUTTON_LEFT : 0) | //Move Left
           ((joyX > 127 + NUNCHUCK_MOVE_THRESHOLD) ? BUTTON_RIGHT : 0) | //Move Right
           ((joyY > 127 + NUNCHUCK_MOVE_THRESHOLD) ? BUTTON_UP : 0) | //Move Up
           ((joyY < 127 - NUNCHUCK_MOVE_THRESHOLD) ? BUTTON_DOWN : 0) | //Move Down
           (nchuk.buttonZ() ? BUTTON_FIRE : 0) |
           (nchuk.buttonC() ? BUTTON_EXTRA : 0) ;
    return lastValue;
  }
}
#endif