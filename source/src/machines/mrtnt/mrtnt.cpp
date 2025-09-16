#include "mrtnt.h"

unsigned char mrtnt::opZ80(unsigned short Addr) {
  return mrtnt_rom[Addr];
}

unsigned char mrtnt::rdZ80(unsigned short Addr) {
  Addr &= 0x7fff;   // a15 is unused
 
  if(Addr < 16384)
    return mrtnt_rom[Addr];

  if((Addr & 0xf000) == 0x4000) {    
    // this includes spriteram 1
    return memory[Addr - 0x4000];
  }   

  if((Addr & 0xf000) == 0x5000) {
    // get a mask of currently pressed keys
    unsigned char keymask = input->buttons_get();
    
    if(Addr == 0x5080)    // dip switch
      return MRTNT_DIP;
    
    if(Addr == 0x5000) {
      unsigned char retval = 0xff;
      if(keymask & BUTTON_UP)    retval &= ~0x01;
      if(keymask & BUTTON_LEFT)  retval &= ~0x02;
      if(keymask & BUTTON_RIGHT) retval &= ~0x04;
      if(keymask & BUTTON_DOWN)  retval &= ~0x08;
      if(keymask & BUTTON_COIN)  retval &= ~0x20;  
      return retval;
    }
    
    if(Addr == 0x5040) {
      unsigned char retval = 0xff; // 0xef for service
      if(keymask & BUTTON_START)  retval &= ~0x20;  
      if(keymask & BUTTON_FIRE)  retval &= ~0x10;      
      return retval;
    }
  }
  return 0xff;
}

void mrtnt::wrZ80(unsigned short Addr, unsigned char Value) {
  Addr &= 0x7fff;   // a15 is unused
    
  if((Addr & 0xf000) == 0x4000) {
    // writing 80 (P, first char of PLAYER 1) to the top left corner
    // is an indication that the game has booted up      
    if(Addr == 0x4000 + 970 && Value == 80)
      game_started = 1;
    
    memory[Addr - 0x4000] = Value;
    return;
  }
  
  if((Addr & 0xff00) == 0x5000) {
    // 0x5060 to 0x506f writes through to ram (spriteram2)
    if((Addr & 0xfff0) == 0x5060)
      memory[Addr - 0x4000] = Value;
    
    if(Addr == 0x5000) 
      irq_enable[0] = Value & 1;
    
    if((Addr & 0xffe0) == 0x5040) {
      if(soundregs[Addr - 0x5040] != Value & 0x0f)
	soundregs[Addr - 0x5040] = Value & 0x0f;
    }    
    return;
  }
} 

void mrtnt::outZ80(unsigned short Port, unsigned char Value) {
  irq_ptr = Value;
}

void mrtnt::run_frame(void) {
  for(int i=0;i<INST_PER_FRAME;i++) {
    StepZ80(cpu); StepZ80(cpu); StepZ80(cpu); StepZ80(cpu);
  }
      
  if(irq_enable[0])
    IntZ80(cpu, irq_ptr);     
}

const unsigned short *mrtnt::tileRom(unsigned short addr) {
  return mrtnt_tilemap[memory[addr]];
}

const unsigned short *mrtnt::colorRom(unsigned short addr) {
  return mrtnt_colormap[addr];
}

const unsigned long *mrtnt::spriteRom(unsigned char flags, unsigned char code) {
  return mrtnt_sprites[flags][code];
}

const signed char * mrtnt::waveRom(unsigned char value) {
  return mrtnt_wavetable[value]; 
}

const unsigned short *mrtnt::logo(void) {
  return mrtnt_logo;
}