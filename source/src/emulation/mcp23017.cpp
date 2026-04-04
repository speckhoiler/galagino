#include "mcp23017.h"

#ifdef MCP23017_INPUT
#include "input.h"

void MCP23017::setup() {
  Serial.printf("MCP23017 setup: SDA=%d, SCL=%d\n", MCP23017_SDA, MCP23017_SCL);
  Wire.begin(MCP23017_SDA, MCP23017_SCL);
  delay(100);
  
  if (!mcp.begin_I2C(0x20, &Wire)) {
    Serial.println("MCP23017 not detected at 0x20!");
    // Try scanning or something?
  } else {
    Serial.println("MCP23017 detected!");
  }

  // Setup pins as input with pullup
  mcp.pinMode(MCP23017_LEFT_PIN, INPUT_PULLUP);
  mcp.pinMode(MCP23017_RIGHT_PIN, INPUT_PULLUP);
  mcp.pinMode(MCP23017_UP_PIN, INPUT_PULLUP);
  mcp.pinMode(MCP23017_DOWN_PIN, INPUT_PULLUP);
  mcp.pinMode(MCP23017_FIRE_PIN, INPUT_PULLUP);
  mcp.pinMode(MCP23017_START_PIN, INPUT_PULLUP);
  mcp.pinMode(MCP23017_COIN_PIN, INPUT_PULLUP);
  mcp.pinMode(MCP23017_EXTRA_PIN, INPUT_PULLUP);

  timeout = millis() + 200;
  enable();
}

void MCP23017::enable() {
  enabled = true;
}

void MCP23017::disable() {
  enabled = false;
}

unsigned char MCP23017::getInput() {
  unsigned long now = millis();
  if(!enabled || now - timeout < 20)
    return lastValue;
  else
    timeout = now;

  // Read all pins from MCP23017
  // We can read both ports at once if we want to optimize, but let's use individual digitalRead for now as it's cleaner
  // or use readGPIOAB()
  uint16_t gpio = mcp.readGPIOAB();

  lastValue = 
    ((gpio & (1 << MCP23017_LEFT_PIN)) ? 0 : BUTTON_LEFT) |
    ((gpio & (1 << MCP23017_RIGHT_PIN)) ? 0 : BUTTON_RIGHT) |
    ((gpio & (1 << MCP23017_UP_PIN)) ? 0 : BUTTON_UP) |
    ((gpio & (1 << MCP23017_DOWN_PIN)) ? 0 : BUTTON_DOWN) |
    ((gpio & (1 << MCP23017_FIRE_PIN)) ? 0 : BUTTON_FIRE) |
    ((gpio & (1 << MCP23017_START_PIN)) ? 0 : BUTTON_START) |
    ((gpio & (1 << MCP23017_COIN_PIN)) ? 0 : BUTTON_COIN) |
    ((gpio & (1 << MCP23017_EXTRA_PIN)) ? 0 : BUTTON_EXTRA);

  return lastValue;
}
#endif
