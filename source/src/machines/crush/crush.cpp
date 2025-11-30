#include "crush.h"

void crush::maketrax_protection_w(uint8_t data)
{
  // disable protection / reset?
	if (data == 0) {
		m_maketrax_counter = 0;
		m_maketrax_offset = 0;
		m_maketrax_disable_protection = 1;
		return;
	}

	if (data == 1) {
		m_maketrax_disable_protection = 0;

		m_maketrax_counter++;
		if (m_maketrax_counter == 0x3c)
		{
			m_maketrax_counter = 0;
			m_maketrax_offset++;

			if (m_maketrax_offset == 0x1e)
				m_maketrax_offset = 0;
		}
	}
}

uint8_t crush::maketrax_special_port2_r(unsigned short offset)
{
	const uint8_t protdata[0x1e] = { // table at $ebd (odd entries)
		0x00, 0xc0, 0x00, 0x40, 0xc0, 0x40, 0x00, 0xc0, 0x00, 0x40, 0x00, 0xc0, 0x00, 0x40, 0xc0, 0x40,
		0x00, 0xc0, 0x00, 0x40, 0x00, 0xc0, 0x00, 0x40, 0xc0, 0x40, 0x00, 0xc0, 0x00, 0x40
	};

	uint8_t data = CRUSH_DIP & 0x3f;
  
  if (m_maketrax_disable_protection == 0)
		return protdata[m_maketrax_offset] | data;

	switch (offset)
	{
		case 0x01:
		case 0x04:
			data |= 0x40; break;
		case 0x05:
		case 0x0e: // korosuke
		case 0x10: // korosuke
			data |= 0xc0; break;
		default:
			data &= 0x3f; break;
	}
	return data;
}

uint8_t crush::maketrax_special_port3_r(unsigned short offset)
{
	const uint8_t protdata[0x1e] = { // table at $ebd (even entries)
		0x1f, 0x3f, 0x2f, 0x2f, 0x0f, 0x0f, 0x0f, 0x3f, 0x0f, 0x0f, 0x1c, 0x3c, 0x2c, 0x2c, 0x0c, 0x0c,
		0x0c, 0x3c, 0x0c, 0x0c, 0x11, 0x31, 0x21, 0x21, 0x01, 0x01, 0x01, 0x31, 0x01, 0x01
	};

	if (m_maketrax_disable_protection == 0)
		return protdata[m_maketrax_offset];

	switch (offset)
	{
		case 0x00:
			return 0x1f;
		case 0x09:
			return 0x30;
		case 0x0c:
			return 0x00;
		default:
			return 0x20;
	}
}

unsigned char crush::opZ80(unsigned short Addr) {
  return crush_rom[Addr];
}

unsigned char crush::rdZ80(unsigned short Addr) {
  Addr &= 0x7fff;   // a15 is unused

  if(Addr < 16384)
    return crush_rom[Addr];

  if((Addr & 0xf000) == 0x4000) {    
    // this includes spriteram 1
    return memory[Addr - 0x4000];
  }   

  if((Addr & 0xf000) == 0x5000) {
    game_started = 1;
  
    // get a mask of currently pressed keys
    unsigned char keymask = input->buttons_get();
        
    if(Addr == 0x5000) {
      unsigned char retval = 0xff; // 0xef for service
      if(keymask & BUTTON_UP)    retval &= ~0x01;
      if(keymask & BUTTON_LEFT)  retval &= ~0x02;
      if(keymask & BUTTON_RIGHT) retval &= ~0x04;
      if(keymask & BUTTON_DOWN)  retval &= ~0x08;
      if(keymask & BUTTON_COIN)  retval &= ~0x20;
      return retval;
    }
    
    if(Addr == 0x5040) {
      unsigned char retval = 0xff; 
      if(keymask & BUTTON_START)  retval &= ~0x20;  
      return retval;
    }

    if (Addr >= 0x5080 && Addr <= 0x50bf)
      return maketrax_special_port2_r(Addr - 0x5080);
    else if (Addr >= 0x50c0 && Addr <= 0x50ff)
      return maketrax_special_port3_r(Addr - 0x50c0);
  }
  return 0xff;
}

void crush::wrZ80(unsigned short Addr, unsigned char Value) {
  Addr &= 0x7fff;   // a15 is unused

  if((Addr & 0xf000) == 0x4000) {
    
    memory[Addr - 0x4000] = Value;
    return;
  }

  if((Addr & 0xff00) == 0x5000) {
    // 0x5060 to 0x506f writes through to ram (spriteram2)
    if((Addr & 0xfff0) == 0x5060)
      memory[Addr - 0x4000] = Value;
    
    if(Addr == 0x5000) 
      irq_enable[0] = Value & 1;

    if (Addr == 0x5004)
      maketrax_protection_w(Value);

    if ((Addr & 0xffe0) == 0x5040) {
      if (soundregs[Addr - 0x5040] != Value & 0x0f)
	      soundregs[Addr - 0x5040] = Value & 0x0f;

      if (Addr == 0x505f)
        timerSoundChanged = millis();

      //printf("%x;%x;%x\n", soundregs[0x15], soundregs[0x1a], soundregs[0x1f]);
    }
  }

  // Remove ugly sound (protection not to use namco audio hardware?) when game over...
  if (((soundregs[0x15] == 0xc && soundregs[0x1a] == 0x3 && soundregs[0x1f] == 0x6) ||
    (soundregs[0x15] == 0xf && soundregs[0x1a] == 0x5 && soundregs[0x1f] == 0x9)) && ((millis() - timerSoundChanged) > 40)) {
    printf("%s\n", "Game over - silence");
    soundregs[0x15] = 0;
    soundregs[0x1a] = 0;
    soundregs[0x1f] = 0;
  }
} 

void crush::outZ80(unsigned short Port, unsigned char Value) {
  irq_ptr = Value;
}

void crush::run_frame(void) {
  for(int i=0;i<INST_PER_FRAME;i++) {
    StepZ80(cpu); StepZ80(cpu); StepZ80(cpu); StepZ80(cpu);
  }
      
  if(irq_enable[0])
    IntZ80(cpu, irq_ptr);     
}

const unsigned short *crush::tileRom(unsigned short addr) {
  return crush_tilemap[memory[addr]];
}

const unsigned short *crush::colorRom(unsigned short addr) {
  return crush_colormap[addr];
}

const unsigned long *crush::spriteRom(unsigned char flags, unsigned char code) {
  return crush_sprites[flags][code];
}

const signed char * crush::waveRom(unsigned char value) {
  return crush_wavetable[value]; 
}

const unsigned short *crush::logo(void) {
  return crush_logo;
}