#include "input.h"

#ifdef NUNCHUCK_INPUT
  Nunchuck nunchuck;
#endif

void Input::init() {
#ifndef NUNCHUCK_INPUT
  pinMode(BTN_START_PIN, INPUT_PULLUP);
  #ifdef BTN_COIN_PIN
    pinMode(BTN_COIN_PIN, INPUT_PULLUP);
  #endif
  pinMode(BTN_LEFT_PIN, INPUT_PULLUP);
  pinMode(BTN_RIGHT_PIN, INPUT_PULLUP);
  pinMode(BTN_DOWN_PIN, INPUT_PULLUP);
  pinMode(BTN_UP_PIN, INPUT_PULLUP);
  pinMode(BTN_FIRE_PIN, INPUT_PULLUP);
#else
  nunchuck.setup();
#endif
}

void Input::enable() {
#ifdef NUNCHUCK_INPUT
  nunchuck.enable();
#endif
}

void Input::disable() {
#ifdef NUNCHUCK_INPUT
  nunchuck.disable();
  vTaskDelay(100);
#endif
}
unsigned char Input::buttons_get(void) {
  // galagino can be compiled without coin button. This will then
  // be implemented by the start button. Whenever the start button 
  // is pressed, a virtual coin button will be sent first 
  unsigned char input_states = 0;
#ifdef NUNCHUCK_INPUT
  input_states = nunchuck.getInput();  
#else
  #ifdef BTN_COIN_PIN
    input_states = (!digitalRead(BTN_COIN_PIN)) ? BUTTON_EXTRA : 0;
  #else
    input_states = (!digitalRead(BTN_START_PIN)) ? BUTTON_EXTRA : 0;
  #endif
  input_states |=
    (digitalRead(BTN_LEFT_PIN) ? 0 : BUTTON_LEFT) |
    (digitalRead(BTN_RIGHT_PIN) ? 0 : BUTTON_RIGHT) |
    (digitalRead(BTN_UP_PIN) ? 0 : BUTTON_UP) |
    (digitalRead(BTN_DOWN_PIN) ? 0 : BUTTON_DOWN) |
    (digitalRead(BTN_FIRE_PIN) ? 0 : BUTTON_FIRE);
#endif
  
  unsigned char startAndCoinState = 0;
#ifdef BTN_COIN_PIN
  // there is a coin pin -> coin and start work normal
  startAndCoinState = (digitalRead(BTN_START_PIN) ? 0 : BUTTON_START) |
    (digitalRead(BTN_COIN_PIN) ? 0 : BUTTON_COIN);
#else
  switch(virtual_coin_state)  {
    case 0:  // idle state
      if(input_states & BUTTON_EXTRA) {
        virtual_coin_state = 1;   // virtual coin pressed
        virtual_coin_timer = millis();
      }
      break;
    case 1:  // start was just pressed
      // check if 100 milliseconds have passed
      if(millis() - virtual_coin_timer > 100) {
        virtual_coin_state = 2;   // virtual coin released
        virtual_coin_timer = millis();        
      }
      break;
    case 2:  // virtual coin was released
      // check if 500 milliseconds have passed
      if(millis() - virtual_coin_timer > 500) {
        virtual_coin_state = 3;   // pause between virtual coin an start ended
        virtual_coin_timer = millis();        
      }
      break;
    case 3:  // pause ended
      // check if 100 milliseconds have passed
      if(millis() - virtual_coin_timer > 100) {
        virtual_coin_state = 4;   // virtual start ended
        virtual_coin_timer = millis();        
      }
      break;
    case 4:  // virtual start has ended
      // check if start button is actually still pressed
      if(!(input_states & BUTTON_EXTRA))
        virtual_coin_state = 0;   // button has been released, return to idle
      break;
  }
  startAndCoinState = ((virtual_coin_state != 1) ? 0 : BUTTON_COIN) | (((virtual_coin_state != 3) && (virtual_coin_state != 4)) ? 0 : BUTTON_START); 
#endif

  // volume control
  if ((input_states & BUTTON_EXTRA) && _volume_callback) {
    _volume_callback(input_states & BUTTON_UP, input_states & BUTTON_DOWN);
    
    if ((input_states & BUTTON_UP) | (input_states & BUTTON_DOWN))
      reset_timer = 0;
  }

#ifndef SINGLE_MACHINE
  bool buttonExtraRisingEdge = (input_states && BUTTON_EXTRA) && !(input_states_last && BUTTON_EXTRA); 
  bool buttonUpRisingEdge = (input_states && BUTTON_UP) && !(input_states_last && BUTTON_UP); 
  bool buttonDownRisingEdge = (input_states && BUTTON_DOWN) && !(input_states_last && BUTTON_DOWN); 

  // joystick up/down (menu) or extra disables attract mode
  if (buttonUpRisingEdge | buttonDownRisingEdge | buttonExtraRisingEdge ) {
    if (_doAttractReset_callback)
      _doAttractReset_callback();  
  }

  // reset control
  if(input_states & BUTTON_EXTRA) {
    if(!reset_timer)
      reset_timer = millis();
    
    // reset if coin (or start if no coin is configured) is held for more than 3 seconds
    if(millis() - reset_timer > 3000) {
      if (_doReset_callback)
        _doReset_callback();
    }
  } 
  else
    reset_timer = 0;

  input_states_last = input_states;
#endif
  return input_states | startAndCoinState;
}

Input &Input::onVolumeUpDown(THandlerVolume fn) {
  _volume_callback = fn;
  return *this;
}

Input &Input::onDoReset(THandlerDoReset fn) {
  _doReset_callback = fn;
  return *this;
}

Input &Input::onDoAttractReset(THandlerDoAttractReset fn) {
  _doAttractReset_callback = fn;
  return *this;
}