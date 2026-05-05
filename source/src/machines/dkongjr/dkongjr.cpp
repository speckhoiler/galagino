#include "dkongjr.h"

unsigned char dkongjr::rdI8048_xdm(struct i8048_state_S *state, unsigned char addr) {
  if(state->p2_state & 0x40)
    return dkong_sfx_index;

  return dkongjr_rom2[2048 + addr + 256 * (state->p2_state & 7)];
}

unsigned char dkongjr::rdI8048_rom(struct i8048_state_S *state, unsigned short addr) {
  return dkongjr_rom2[addr];
}

unsigned char dkongjr::opZ80(unsigned short Addr) {
  return dkongjr_rom1[Addr];
}

unsigned char dkongjr::rdZ80(unsigned short Addr) { 
  // --- Regione ROM: 0x0000 - 0x5FFF (24 KB) --- CPU PRINCIPALE Z80
  if (Addr < 0x6000) {
    return dkongjr_rom1[Addr];
  }

  // 0x6000 - 0x77ff
  if(((Addr & 0xf000) == 0x6000) || ((Addr & 0xf800) == 0x7000)) 
    return memory[Addr - 0x6000];
  
  // --- Registri di Input ---
  // IN0: Controlli Giocatore 1 (Joystick + Salto)
  if (Addr == 0x7c00) {
    // Ottiene una maschera dei tasti premuti
    unsigned char retval = 0x00;
    unsigned char keymask = input->buttons_get();
    
    if (keymask & BUTTON_RIGHT) retval |= 0x01;
    if (keymask & BUTTON_LEFT)  retval |= 0x02;
    if (keymask & BUTTON_UP)    retval |= 0x04;
    if (keymask & BUTTON_DOWN)  retval |= 0x08;
    if (keymask & BUTTON_FIRE)  retval |= 0x10;
    return retval;
  }
  
  // IN1: Controlli Giocatore 2 (per modalità cocktail)
  if (Addr == 0x7c80) {
    // In una implementazione per un solo giocatore, questo può restituire 0
    return 0x00;
  }
  
  if (Addr == 0x7d00) {
    // Ottiene una maschera dei tasti premuti
    unsigned char retval = 0x00;
    unsigned char keymask = input->buttons_get();    
    if (keymask & BUTTON_START) retval |= 0x04; // P1 Start
    if (keymask & BUTTON_COIN)  retval |= 0x80; // Coin
    return retval;
  }
  
  if (Addr == 0x7d80) {
    return DKONG_DIP;
  }

  // Indirizzo non mappato
  return 0xff;
}

void dkongjr::wrZ80(unsigned short Addr, unsigned char Value) {
  if(((Addr & 0xf000) == 0x6000) || ((Addr & 0xf800) == 0x7000)) {
    memory[Addr - 0x6000] = Value;
    return;
  }

  // Ignora scritture ai registri DMA (semplificazione)
  if ((Addr >= 0x7800) && (Addr <= 0x780f)) {
    return;
  }
  
  // 7C00: dkongjr_sh_tuneselect_w
  if (Addr == 0x7c00) {
    dkong_sfx_index = Value; // Imposta il suono/musica da riprodurre
    return;
  }

  // 7C80: GFX Bank select
  if (Addr == 0x7c80) {
    gfx_bank = Value & 1;
    return;
  }

  // { 0x7d00, 0x7d00, dkongjr_sh_climb_w },
	// { 0x7d01, 0x7d01, dkongjr_sh_jump_w  },
	// { 0x7d02, 0x7d02, dkongjr_sh_land_w  },
	// { 0x7d03, 0x7d03, dkongjr_sh_roar_w  },
  // use Dkong sounds here to save flash memory, so the original DkongJr sounds are different...
  if ((Addr >= 0x7d00) && (Addr <= 0x7d03) && Value) {
    trigger_sound(Addr & 7);
    return;
  }

  // { 0x7d04, 0x7d04, dkong_sh_sound4_w },
	if (Addr == 0x7d04) {
	  cpu_8048.T1 = !(Value & 1);
    return;
  }

  // { 0x7d05, 0x7d05, dkong_sh_sound5_w },
  if (Addr == 0x7d05) {
	  cpu_8048.T0 = !(Value & 1);
    return;          
  }
  	
  // { 0x7d06, 0x7d06, dkongjr_sh_snapjaw_w },
  if ((Addr == 0x7d06) && Value) {
    trigger_sound(Addr & 7);
    return;
  }

  // { 0x7d07, 0x7d07, dkongjr_sh_walk_w },	/* controls pitch of the walk/climb? */
  if ((Addr == 0x7d07) && Value) {
    return;
  }

  // Latch per controlli hardware (7D80 - 7D87)
  if ((Addr >= 0x7d80) && (Addr <= 0x7d87)) {
    unsigned char latch_addr = Addr & 0x07;
    unsigned char bit_val = Value & 1;
    static char bit_val_last = 0;

    switch(latch_addr) {
      case 0: // 7D80: Death sound
        cpu_8048.notINT = !bit_val;
        break;
      case 1: // 7D81: Fall sound
        if (bit_val && !bit_val_last)
          trigger_sound(4);

        bit_val_last = bit_val;
        break;
      case 2: // 7D82: Flip screen
        flip_screen = bit_val;
        break;
      case 4: // 7D84: Interrupt (NMI) enable
        irq_enable[0] = bit_val;
        break;
      case 5: // 7D85: Trigger DMA per copiare gli sprite
        if (bit_val)
          memcpy(memory + 0x1000, memory + 0x0900, 1024);          
        break;
      case 6: // 7D86: Palette bank bit 0
        if (bit_val)
          palette_bank |= 1; // Imposta bit 0 a 1
        else
          palette_bank &= ~1; // Azzera bit 0
        break;
      case 7: // 7D87: Palette bank bit 1
        if (bit_val)
          palette_bank |= 2; // Imposta bit 1 a 1 (il valore 2 corrisponde a 0...010 in binario)
        else
          palette_bank &= ~2; // Azzera bit 1
        break;
    }
    return;
  }  
}

void dkongjr::prepare_frame(void) {
  active_sprites = 0;
 
  for (int idx = 0; idx < 96 && active_sprites < 92; idx++) {
    unsigned char *sprite_base_ptr = memory + 0x1000 + 4 * idx;
    if (sprite_base_ptr[0] == 0) continue;

    struct sprite_S spr;    
    spr.x = 224 + 8 - sprite_base_ptr[0] - 1; //-1
    spr.y = 224 - 24 - sprite_base_ptr[3];
    spr.code = sprite_base_ptr[1] & 0x7f;
    spr.color = sprite_base_ptr[2] & 0x0f;

    // Leggi i flag di flip originali dall'hardware.
    int original_flip_x = (sprite_base_ptr[2] & 0x80) ? 1 : 0; // Bit 0 per FLIP_X
    int original_flip_y = (sprite_base_ptr[1] & 0x80) ? 2 : 0; // Bit 1 per FLIP_Y
    
    // Combina i flip originali
    int original_flags = original_flip_x | original_flip_y;

    // Per correggere la rotazione e lo specchiamento, invertiamo SOLO il flip Y.
    // L'operazione XOR con 2 (binario b10) inverte il bit 1 (Flip Y)
    // e lascia invariato il bit 0 (Flip X).
    spr.flags = original_flags ^ 1; // Inverti solo Flip Y

    // Gestione Flip Screen (modalità cocktail)
    if (flip_screen) {
        spr.x = 224 - 16 - spr.x;
        spr.y = 224 - 16 - spr.y;
        spr.flags ^= 3; // Per la modalità cocktail, invertiamo entrambi
    }

    // Aggiungi alla lista
    if ((spr.y > -16) && (spr.y < 288) && (spr.x > -16) && (spr.x < 224)) {
      sprite[active_sprites++] = spr;
    }
  }
}

// draw a single 8x8 tile
void dkongjr::blit_tile(short row, char col) {
  unsigned short addr = tileaddr[row][col];

  if((row < 2) || (row >= 34)) return;    
  // skip blank dkong tiles (0x10) in rendering  
  if(memory[0x1400 + addr] == 0x10) return;  
  unsigned char base_code = memory[0x1400 + addr];
  // 2. Calcola il codice finale aggiungendo l'offset del banco grafico.
  unsigned short final_code = base_code + (256 * gfx_bank);
  const unsigned short *tile = dkongjr_tilemap[final_code];
  // donkey kong has some sort of global color table
  const unsigned short *colors = dkongjr_colormap[palette_bank][row-2 + 32*(col/4)];
  unsigned short *ptr = frame_buffer + 8*col;

  // 8 pixel rows per tile
  for(char r=0;r<8;r++,ptr+=(224-8)) {
    unsigned short pix = *tile++;
    // 8 pixel columns per tile
    for(char c=0;c<8;c++,pix>>=2) {
      if(pix & 3) *ptr = colors[pix&3];
      ptr++;
    }
  } 
}

// dkongjr has its own sprite drawing routine since unlike the other
// games, in dkongjr black is not always transparent. Black pixels
// are instead used for masking
void dkongjr::blit_sprite(short row, unsigned char s) {
  const unsigned long *spr = dkongjr_sprites[sprite[s].flags & 3][sprite[s].code];
  const unsigned short *colors = dkongjr_colormap_sprite[palette_bank][sprite[s].color];
  
  // create mask for sprites that clip left or right
  unsigned long mask = 0xffffffff;
  if(sprite[s].x < 0)      mask <<= -2*sprite[s].x;
  if(sprite[s].x > 224-16) mask >>= 2*(sprite[s].x-224-16);    

  short y_offset = sprite[s].y - 8*row;

  // check if there are less than 8 lines to be drawn in this row
  unsigned char lines2draw = 8;
  if(y_offset < -8) lines2draw = 16+y_offset;

  // check which sprite line to begin with
  unsigned short startline = 0;
  if(y_offset > 0) {
    startline = y_offset;
    lines2draw = 8 - y_offset;
  }

  // if we are not starting to draw with the first line, then
  // skip into the sprite image
  if(y_offset < 0)
    spr -= y_offset;  

  // calculate pixel lines to paint  
  unsigned short *ptr = frame_buffer + sprite[s].x + 224*startline;
  
  // 16 pixel rows per sprite
  for(char r=0;r<lines2draw;r++,ptr+=(224-16)) {
    unsigned long pix = *spr++ & mask;
    // 16 pixel columns per tile
    for(char c=0;c<16;c++,pix>>=2) {
      unsigned short col = colors[pix&3];
      if(pix & 3) *ptr = col;
      ptr++;
    }
  }
}

const unsigned short *dkongjr::logo(void) {
  return dkongjr_logo;
}