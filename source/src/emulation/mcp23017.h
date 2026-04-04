#ifndef _MCP23017_H_
#define _MCP23017_H_

#include <Arduino.h>
#include "../config.h"

#ifdef MCP23017_INPUT
#include <Adafruit_MCP23X17.h>

class MCP23017 {
public:
  void setup();
  void enable();
  void disable();
  unsigned char getInput();
 
private:
  Adafruit_MCP23X17 mcp;
  bool enabled;
  unsigned long timeout;
  unsigned char lastValue;
};

#endif

#endif //_MCP23017_H_
