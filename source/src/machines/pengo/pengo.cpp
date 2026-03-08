#include "pengo.h"

unsigned char pengo::opZ80(unsigned short Addr) {
  if (Addr < 0x8000)
    return pengo_rom[Addr];
  else
    return memory[Addr - 0x8000];
}

unsigned char pengo::rdZ80(unsigned short Addr) {
  if (Addr < 0x8000) return pengo_rom[Addr];
    
  if (Addr >= 0x8000 && Addr <= 0x8FFF) return memory[Addr - 0x8000];

  if (Addr >= 0x9000) {
    unsigned char keymask;
    unsigned char retval = 0xFF;
    switch(Addr) {
      case 0x9000: return PENGO_DSW1;

      case 0x9040: return PENGO_DSW0 | (input->demoSoundsOff() ? PENGO_DIP_DEMOSOUNDS_OFF : PENGO_DIP_DEMOSOUNDS_ON);
 
      case 0x9080: // IN1
        keymask = input->buttons_get();
        if(keymask & BUTTON_START) retval &= ~0x20;
        return retval;

      case 0x90C0: // IN0
        keymask = input->buttons_get();
        if(keymask & BUTTON_UP)    retval &= ~0x01;
        if(keymask & BUTTON_DOWN)  retval &= ~0x02;
        if(keymask & BUTTON_LEFT)  retval &= ~0x04;
        if(keymask & BUTTON_RIGHT) retval &= ~0x08;
        if(keymask & BUTTON_FIRE)  retval &= ~0x80;
        if(keymask & BUTTON_COIN && !coinBackup) {
          coinFrameCounter = 2;
          coinBackup = 1;
        }
        
        //coin pulse for min 2 and max 9 frames...
        if (coinFrameCounter > 0) {
          retval &= ~0x10; 
        }
        else {
          if ((keymask & BUTTON_COIN) == 0)
            coinBackup = 0;
        }
        return retval;
    }
  }
  return 0xFF;
}

void pengo::wrZ80(unsigned short Addr, unsigned char Value) {  
  // --- NUOVA GESTIONE SCRITTURA IN RAM ---
  if (Addr >= 0x8000 && Addr <= 0x8FFF) {
    memory[Addr - 0x8000] = Value;
    return;
  }

  // Audio
  if (Addr >= 0x9000 && Addr <= 0x901F) {
    if (soundregs[Addr - 0x9000] != Value & 0x0f) {
      soundregs[Addr - 0x9000] = Value & 0x0f;
    }
    return;
  }

  // --- CORREZIONE COORDINATE SPRITE ---
  // Scrivi nell'array separato, come fa namco.h
  // L'indirizzo base è 0x9020, non 0x9022.
  if (Addr >= 0x9020 && Addr <= 0x902F) {
    sprite_coords[Addr & 0x0F] = Value;
    return;
  }

  // Latch Hardware
  if (Addr >= 0x9040 && Addr <= 0x9047) {
    unsigned char latch_bit = Addr & 7;
    unsigned char data = Value & 1;
    switch(latch_bit) {
      case 0: irq_enable[0] = data; break;
      case 1: /* Sound Enable */ break;
      case 2: palette_bank = data; break;
      case 3: flipscreen = data; break;
      case 4: /* Coin Counter */ break;
      case 5: /* Coin Counter */ break;
      case 6: colortable_bank = data; break;
      case 7: gfx_bank = data; break;
    }
    return;
  }

  // Watchdog
  if (Addr == 0x9070)
    return;
} 

void pengo::run_frame(void) {
  for(int i=0;i<INST_PER_FRAME;i++) {
    StepZ80(cpu); StepZ80(cpu); StepZ80(cpu); StepZ80(cpu);
  }

  if(irq_enable[0]) {
    IntZ80(cpu, INT_RST38); 
  }

  if (coinFrameCounter > 0)
    coinFrameCounter--;

  if (!game_started)
    game_started = 1; 
}

void pengo::prepare_frame(void) {
  active_sprites = 0;
  
  for (int idx = 0; idx < 6; idx++) {
    unsigned char *attrib_ptr = &memory[0x0FF2  + idx * 2];
    //unsigned char *coord_ptr  = &sprite_coords[idx * 2];
        
    // Leggi le coordinate hardware originali
    unsigned char sy_hw = sprite_coords[idx * 2 + 2];
    unsigned char sx_hw = sprite_coords[idx * 2 + 3];
    
    // Il gioco nasconde gli sprite impostando la loro coordinata Y hardware >= 248.
    // Ignoriamo questi sprite.
    if (sy_hw >= 248) {
      continue;
    }

    struct sprite_S spr;     
    spr.code  = attrib_ptr[0] >> 2;
    // L'attributo colore usa i 6 bit più bassi
    spr.color = attrib_ptr[1] & 0x3F;
    // Leggi i flag originali
    unsigned char original_flags = attrib_ptr[0] & 3; // Bit 0: FlipY, Bit 1: FlipX

    // --- CORREZIONE FINALE DELLA SPECCHIATURA ---
    // La nostra trasformazione delle coordinate inverte implicitamente l'asse X.
    // Per compensare, dobbiamo invertire il flag di flip orizzontale (FlipX).
    // Il bit di FlipX è il bit 1 (valore 2).
    spr.flags = original_flags ^ 2; // Inverte solo il bit di FlipX

    // --- FORMULA DI ROTAZIONE E TRASFORMAZIONE CORRETTA ---
    // Dopo la rotazione di 90 gradi, la X dell'hardware diventa la Y dello schermo
    // e la Y dell'hardware diventa la X dello schermo.
    // I tuoi test hanno dimostrato che entrambi gli assi risultano invertiti.

    // 1. Trasformazione asse X dello schermo:
    // La Y dell'hardware (sy_hw) mappa la X dello schermo.
    // Dobbiamo invertire l'asse per correggere il movimento destra->sinistra.
    spr.x = 224 - sy_hw;

    // 2. Trasformazione asse Y dello schermo:
    // La X dell'hardware (sx_hw) mappa la Y dello schermo.
    // Dobbiamo invertire anche questo asse per correggere il posizionamento sopra/sotto.
    spr.y = 288  - sx_hw;
    
    // 3. Gestione del flipscreen (modalità cocktail)
    // Se il flipscreen è attivo, invertiamo di nuovo le coordinate finali
    // e invertiamo entrambi i flag di flip dello sprite.
    if (flipscreen) {
        spr.x = 224 - 16 - spr.x;
        spr.y = 288 - 16 - spr.y;
        spr.flags ^= 3; // Inverte sia FlipX che FlipY
    }
    
    // Aggiungi lo sprite alla lista solo se il codice è valido e non superiamo il limite
    if ((spr.code < 64) && (active_sprites < 6)) {      
      sprite[active_sprites] = spr;
      active_sprites++;
    }
  }
}

// render one of 36 tile rows (8 x 224 pixel lines)
void pengo::render_row(short row) {
  for(char col=0;col<28;col++){
    blit_tile(row, col);
  }

  // render sprites
  for(unsigned char s=0;s<active_sprites;s++) {
    blit_sprite(row, s);
  }
}

void pengo::blit_tile(short row, char col) {
  // --- 1. Recupero dati tile (questa parte era già corretta) ---
  unsigned short addr = tileaddr[row][col];
  unsigned char tile_code = memory[VIDEORAM_BASE + addr];
  unsigned char color_code = memory[COLORRAM_BASE + addr] & 0x1F; // Legge l'attributo a 6 bit (0-63)
  //if (gfx_bank !=0) printf("blit_tile gfx_bank %d\n", gfx_bank);

  // Seleziona il puntatore ai dati grafici del tile
  const unsigned short *tile = pengo_tiles[(gfx_bank << 8) | tile_code];

  // --- 2. LOGICA DI SELEZIONE PALETTE CORRETTA (identica a blit_sprite) ---
  // Combina i 2 bit del banco tabella con i 6 bit dell'attributo colore
  // per creare l'indice finale a 8 bit (0-255) del gruppo di colori.
  unsigned int color_group_index = (colortable_bank << 6) | color_code;

  // Seleziona la palette di 4 colori corretta dall'array, usando ANCHE il palette_bank.
  const uint16_t *colors = pengo_colormap[palette_bank][color_group_index];
  // -------------------------------------------------------------------------

  // --- 3. Loop di disegno (questa parte era già corretta) ---
  unsigned short *ptr = frame_buffer + 8 * col;

  for (char r = 0; r < 8; r++, ptr += (224 - 8)) {
    unsigned short pix_row = *tile++;
    for (char c = 0; c < 8; c++, pix_row >>= 2) {
      unsigned char pen = pix_row & 3; // Estrae il valore 0, 1, 2, o 3
      // if (pen) { // Disegna solo se non è il colore 0 (trasparente)
      *ptr = colors[pen];
      // }
      ptr++;
    }
  }
}

void pengo::blit_sprite(short current_strip_row, unsigned char s) {
  // --- 1. Recupera i dati dello sprite dalla struttura ---
  const short sx = sprite[s].x + 16;
  const short sy = sprite[s].y - 16;
  const unsigned char sprite_code = sprite[s].code;
  const unsigned char flags = sprite[s].flags;
  const unsigned char color_attr = sprite[s].color;

  // --- 2. Clipping verticale veloce ---
  short strip_start_y = current_strip_row * 8;
  if (sy >= strip_start_y + 8 || (sy + 16) <= strip_start_y) {
    return;
  }

  // --- 3. Seleziona i dati grafici e la palette corretti ---
  const unsigned long *spr_data = pengo_sprites[gfx_bank][flags][sprite_code];
    
  // *** LOGICA DEI COLORI CORRETTA ***
  // Combina il banco della tabella colori (1 bit) con l'attributo colore dello sprite (6 bit)
  // per creare un indice finale a 7 bit (0-127).
  unsigned int final_color_index = (colortable_bank << 6) | (color_attr & 0x3F);
  const uint16_t *colors = pengo_colormap[palette_bank][final_color_index];
  // *******************************

  // --- 4. Calcola il clipping orizzontale ---
  int start_col = 0;
  if (sx < 0) start_col = -sx;
    
  int end_col = 16;
  if (sx + 16 > 224) end_col = 224 - sx;

  // --- 5. Calcola la sovrapposizione verticale ---
  int start_row_in_sprite = 0;
  if (sy < strip_start_y) start_row_in_sprite = strip_start_y - sy;
    
  int rows_to_draw = 16 - start_row_in_sprite;
  if (sy + 16 > strip_start_y + 8) {
    rows_to_draw -= (sy + 16) - (strip_start_y + 8);
  }
    
  // --- 6. Calcola il puntatore di partenza nel framebuffer ---
  int start_row_in_strip = (sy > strip_start_y) ? (sy - strip_start_y) : 0;
  uint16_t* ptr = frame_buffer + (start_row_in_strip * 224) + sx + start_col;

  // --- 7. Loop di disegno principale ---
  for (int r = 0; r < rows_to_draw; r++) {
    unsigned long pix_row = spr_data[start_row_in_sprite + r];
    pix_row <<= (start_col * 2);

    for (int c = start_col; c < end_col; c++) {
      uint8_t color_index = (pix_row >> 30) & 3;
      if (color_index) {
        *ptr = colors[color_index];
      }
      ptr++;
      pix_row <<= 2;
    }
    ptr += (224 - (end_col - start_col));
  }
}

const signed char * pengo::waveRom(unsigned char value) {
  return pengo_wavetable[value]; 
}

const unsigned short *pengo::logo(void) {
  return pengo_logo;
}