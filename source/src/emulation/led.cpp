#include "led.h"

#ifdef LED_PIN
void Led::init() {
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(LED_BRIGHTNESS);
}

void Led::update(machineBase *machines[], signed char machineIndexPreselection, signed char machineSelected) {
  if (machineSelected == MCH_MENU)
    machines[machineIndexPreselection]->menuLeds(leds);
  else
    machines[machineSelected]->gameLeds(leds);
  
  bool changed = false;
  for (int i = 0; i < NUM_LEDS; i++) {
    if (ledsBackup[i].raw[0] != leds[i].raw[0] ||
        ledsBackup[i].raw[1] != leds[i].raw[1] ||
        ledsBackup[i].raw[2] != leds[i].raw[2]) {
      changed = true;
      break;
    }
  }

  if (!changed)
    return;
    
  for (int i = 0; i < NUM_LEDS; i++) {
    ledsBackup[i] = leds[i];
    //printf("%03d%03d%03d;", leds[i].raw[0], leds[i].raw[1], leds[i].raw[2]);
  }
  //printf("\n");
  FastLED.show();
}
#endif