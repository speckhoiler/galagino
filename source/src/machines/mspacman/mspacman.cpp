#include "mspacman.h"

static inline bool mspacman_is_disable_trap(unsigned short Addr) {
  return (Addr >= 0x0038 && Addr <= 0x003F) ||
         (Addr >= 0x03B0 && Addr <= 0x03B7) ||
         (Addr >= 0x1600 && Addr <= 0x1607) ||
         (Addr >= 0x2120 && Addr <= 0x2127) ||
         (Addr >= 0x3FF0 && Addr <= 0x3FF7) ||
         (Addr >= 0x8000 && Addr <= 0x8007) ||
         (Addr >= 0x97F0 && Addr <= 0x97F7);
}

void mspacman::reset() {
  machineBase::reset();
  decode = false;
}

unsigned char mspacman::opZ80(unsigned short Addr) {
  // ENABLE trap: lettura di 0x3FF8-0x3FFF attiva Ms.Pac-Man mode
  if(Addr >= 0x3FF8 && Addr <= 0x3FFF) {
    decode = true;
    return mspacman_pacrom[Addr];
  }
  
  // DISABLE trap: lettura di alcune zone attiva Pac-Man mode
  if(mspacman_is_disable_trap(Addr)) {
    decode = false;
    return (Addr < 0x4000) ? pacman_rom[Addr] : pacman_rom[Addr & 0x3FFF];
  }
  
  // Lettura normale in base allo stato decode
  if(Addr < 0x4000)
    return decode ? mspacman_pacrom[Addr] : pacman_rom[Addr];
  
  if(Addr >= 0x8000 && Addr < 0xA000)
    return decode ? mspacman_auxrom[Addr - 0x8000] : pacman_rom[Addr & 0x3FFF];
  
  if(Addr >= 0xA000)
    return decode ? mspacman_pacrom[Addr - 0x8000] : pacman_rom[Addr & 0x3FFF];
  
  return 0xFF;
}

unsigned char mspacman::rdZ80(unsigned short Addr) {
  // ENABLE trap
  if(Addr >= 0x3FF8 && Addr <= 0x3FFF) {
    decode = true;
    return mspacman_pacrom[Addr];
  }
  
  // DISABLE trap
  if(mspacman_is_disable_trap(Addr)) {
    decode = false;
    return (Addr < 0x4000) ? pacman_rom[Addr] : pacman_rom[Addr & 0x3FFF];
  }
  
  // ROM
  if(Addr < 0x4000)
    return decode ? mspacman_pacrom[Addr] : pacman_rom[Addr];
  
  if(Addr >= 0x8000 && Addr < 0xA000)
    return decode ? mspacman_auxrom[Addr - 0x8000] : pacman_rom[Addr & 0x3FFF];
  
  if(Addr >= 0xA000)
    return decode ? mspacman_pacrom[Addr - 0x8000] : pacman_rom[Addr & 0x3FFF];
  
  // VRAM/RAM
  if((Addr & 0xF000) == 0x4000)
    return memory[Addr - 0x4000];
  
  // I/O
  if((Addr & 0xF000) == 0x5000) {
    unsigned char keymask = input->buttons_get();
    if(Addr == 0x5080) 
      return PACMAN_DIP;
    
    if(Addr == 0x5000) {
      unsigned char retval = 0xFF;
      if(keymask & BUTTON_UP)    retval &= ~0x01;
      if(keymask & BUTTON_LEFT)  retval &= ~0x02;
      if(keymask & BUTTON_RIGHT) retval &= ~0x04;
      if(keymask & BUTTON_DOWN)  retval &= ~0x08;
      if(keymask & BUTTON_COIN)  retval &= ~0x20;
      return retval;
    }
    
    if(Addr == 0x5040) {
      unsigned char retval = 0xFF;
      if(keymask & BUTTON_START) retval &= ~0x20;
      return retval;
    }
  }
  return 0xFF;
}

void mspacman::wrZ80(unsigned short Addr, unsigned char Value) {
  // ENABLE trap
  if(Addr >= 0x3FF8 && Addr <= 0x3FFF) {
    decode = true;
    return;
  }
  
  // DISABLE trap
  if(mspacman_is_disable_trap(Addr)) {
    decode = false;
    return;
  }
  
  // Mirror A15=1: 0xC000-0xCFFF → 0x4000-0x4FFF, 0xD000-0xD0FF → 0x5000-0x50FF
  // (come Pac-Man hardware dove il bit A15 non è usato nel decode)
  if(Addr >= 0xC000)
    Addr &= 0x7FFF;
  
  // VRAM/RAM (0x4000-0x4FFF)
  if((Addr & 0xF000) == 0x4000) {
    if(!game_started && Addr == 0x4000 + 985 && Value == 85)
      game_started = 1;
    
    memory[Addr - 0x4000] = Value;
    return;
  }

  // I/O (0x5000-0x50FF)
  if((Addr & 0xFF00) == 0x5000) {
    if((Addr & 0xFFF0) == 0x5060)
      memory[Addr - 0x4000] = Value;

    if(Addr == 0x5000)
      irq_enable[0] = Value & 1;
    
    if((Addr & 0xFFE0) == 0x5040) {
      if(soundregs[Addr - 0x5040] != (Value & 0x0F))
        soundregs[Addr - 0x5040] = Value & 0x0F;
    }
    return;
  }
}

const unsigned short *mspacman::tileRom(unsigned short addr) {
  return mspacman_tilemap[memory[addr]];
}

const unsigned long *mspacman::spriteRom(unsigned char flags, unsigned char code) {
  return mspacman_sprites[flags][code];
}

const unsigned short *mspacman::logo(void) {
  return mspacman_logo;
}