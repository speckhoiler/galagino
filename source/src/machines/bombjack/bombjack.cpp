#include "bombjack.h"

void bombjack::reset() {
  machineBase::reset();
  if (background_bufferAllocated) {
    background_bufferAllocated = false;

    for (int i = 0; i < 36; i++) {
      currentBackground[i] = 0;
      free(background_buffer[i]);
    }
  }
}

unsigned char bombjack::opZ80(unsigned short Addr) {
  if (current_cpu == 0)
    return bombjack_rom_cpu1[Addr];
  else
    return bombjack_rom_cpu2[Addr];
}

unsigned char bombjack::rdZ80(unsigned short Addr) {
  if (current_cpu == 0) {
    // --- Lettura dalle aree ROM mappate ---
    if (Addr >= 0x0000 && Addr <= 0x7FFF) { return bombjack_rom_cpu1[Addr]; }
    if (Addr >= 0xC000 && Addr <= 0xDFFF) { return bombjack_rom_cpu1[Addr]; }

    // --- Gestione delle aree RAM mappate ---
    if (Addr >= 0x8000 && Addr <= 0x8FFF) { return memory[Addr - 0x8000]; } // 4096 bytes
    if (Addr >= 0x9000 && Addr <= 0x93FF) { return memory[Addr - 0x9000 + 0x1000]; } // 1024 bytes
    if (Addr >= 0x9400 && Addr <= 0x97FF) { return memory[Addr - 0x9400 + 0x1000 + 0x400]; } // 1024 bytes
    if (Addr >= 0x9800 && Addr <= 0x987F) { return memory[Addr - 0x9800 + 0x1000 + 0x400 + 0x400]; } // 128 bytes
    if (Addr >= 0x9A00 && Addr <= 0x9A01) { return memory[Addr - 0x9A00 + 0x1000 + 0x400 + 0x400 + 0x80]; } // 2 bytes

    // --- Gestione delle porte di I/O (Input e DIP Switches) ---
    if ((Addr & 0xFFF8) == 0xB000) { // Controlla l'indirizzo base 0xB000 con mirror
      switch (Addr & 0xB007) { // Maschera per i mirror
      case PORT_P1_IN: {
        unsigned char keymask = input->buttons_get();
        unsigned char retval = 0x00; // Inizia con tutti i bit a 0 (rilasciato)

        // Il gioco si aspetta IP_ACTIVE_HIGH, quindi mettiamo il bit a 1 se il pulsante è premuto
        if (keymask & BUTTON_RIGHT)
          retval |= 0x01;
        if (keymask & BUTTON_LEFT)
          retval |= 0x02;
        if (keymask & BUTTON_UP)
          retval |= 0x04; // NOTA: Joystick UP e DOWN sono scambiati nel tuo codice originale
        if (keymask & BUTTON_DOWN)
          retval |= 0x08;
        if (keymask & BUTTON_FIRE && game_started) //skip service when fire is pressed
          retval |= 0x10;

        // Gli input di servizio (START/COIN) sono su un'altra porta, quindi qui non servono.
        return retval;
      }
      case PORT_P2_IN:
        return 0x00;
      case PORT_SYSTEM_IN: {
        unsigned char keymask1 = input->buttons_get();
        unsigned char retval1 = 0x00; // Inizia con tutti i bit a 0

        if (keymask1 & BUTTON_COIN)
          retval1 |= 0x01;
        if (keymask1 & BUTTON_START)
          retval1 |= 0x04;
        return retval1;
      }
      case PORT_WATCHDOG:
        return 0xFF; // Valore non importante
      case PORT_DSW1_IN:
        return BOMBJACK_DIP1 | (input->demoSoundsOff() ? BOMBJACK_DIP_DEMOSOUNDS_OFF : BOMBJACK_DIP_DEMOSOUNDS_ON);
      case PORT_DSW2_IN:
        return BOMBJACK_DIP2;
      }
    }
  }
  else if (current_cpu == 1) {
    Addr &= 0x7fff;   // a15 is unused

    // --- Audio CPU Read Logic ---
    // Handle ROM reads
    if (Addr <= 0x3FFF) { return bombjack_rom_cpu2[Addr]; }

    // Handle RAM reads
    if (Addr >= 0x4000 && Addr <= 0x47FF) { return memory[(Addr - 0x4000 + 0x1000 + 0x400 + 0x400 + 0x80 + 2)]; }

    // Handle Sound Latch read
    if (Addr == 0x6000) {
      unsigned char value = sound_latch;
      if (sound_latch_backup != sound_latch)
        m_mmi_skip_audio_cpu = true;

      sound_latch_backup = sound_latch;
      sound_latch = 0;
      return value;
    }
  }

  // If no case matches, return a default value for an open bus
  return 0xFF;
}

void bombjack::wrZ80(unsigned short Addr, unsigned char Value) {
  if (current_cpu == 0) {
    // --- DEBUG MIRATO ---
    // Monitoriamo l'area di memoria dove dovrebbe apparire la scritta "SIDE-ONE".
    // L'hexdump mostra che è intorno all'offset 0x140.
    if (Addr == 0x9144 && Value == 83) {
      if (game_started == 0) {
        // Trovato il trigger! Imposta game_started a 1.
        // Questo sbloccherà la sincronizzazione in emulate_frame.
        game_started = 1;
        printf("game_started\n");
      }
    }
    // --- Fase 1: Scrittura nelle aree RAM mappate ---
    if (Addr >= 0x8000 && Addr <= 0x8FFF) {
      memory[Addr - 0x8000] = Value;
      return;
    }

    if (Addr >= 0x9000 && Addr <= 0x93FF) {
      memory[Addr - 0x9000 + 0x1000] = Value;
      return;
    }

    if (Addr >= 0x9400 && Addr <= 0x97FF) {
      memory[Addr - 0x9400 + 0x1000 + 0x400] = Value;
      return;
    }

    if (Addr >= 0x9800 && Addr <= 0x987F) {
      memory[Addr - 0x9800 + 0x1000 + 0x400 + 0x400] = Value;
      return;
    }

    if (Addr >= 0x9A00 && Addr <= 0x9A01) {
      memory[Addr - 0x9A00 + 0x1000 + 0x400 + 0x400 + 0x80] = Value;
      return;
    }

    if (Addr >= 0x9C00 && Addr <= 0x9CFF) {
      palette[Addr - 0x9C00] = Value;
      uint8_t b_p, b_d;
      if (Addr & 1) {
        b_p = palette[Addr - 0x9C01];
        b_d = Value;
      }
      else {
        b_p = Value;
        b_d = palette[Addr - 0x9C00 + 1];
      }

      int r = (b_p & 15) | (b_p << 4), g = (b_p & 240) | (b_p >> 4), b = (b_d & 15) | (b_d << 4);
      uint16_t c = ((r & 248) << 8) | ((g & 252) << 3) | (b >> 3);

      int i = (Addr - 0x9C00) / 2;
      bombjack_palette[i] = (c >> 8) | (c << 8);
      return;
    }

    if (Addr == 0x9E00) {
      m_bg_image = Value;
      return;
    }

    if (Addr == 0xB000) {
      m_nmi_on = (Value & 0x01);
      return;
    }

    if (Addr == 0xB003) {
      return;
    }

    if (Addr == 0xB004) {
      m_flip = (Value & 0x01);
      return;
    }

    if (Addr == 0xB800) {
      sound_latch = Value;
      return;
    }

    return; // Se l'indirizzo non corrisponde a nulla, non facciamo niente.
  }
  else if (current_cpu == 1) {
    Addr &= 0x7fff;   // a15 is unused

    // Scrittura nella RAM audio
    if (Addr >= 0x4000 && Addr <= 0x47ff) {
      memory[(Addr - 0x4000 + 0x1000 + 0x400 + 0x400 + 0x80 + 2)] = Value; //& 0x07FF
      return;
    }
  }
}

void bombjack::outZ80(unsigned short Port, unsigned char Value) {
  int chip_id = -1;
  if ((Port & 0xFF) <= 0x01)
    chip_id = 0;
  else if ((Port & 0xFF) >= 0x10 && (Port & 0xFF) <= 0x11)
    chip_id = 1;
  else if ((Port & 0xFF) >= 0x80 && (Port & 0xFF) <= 0x81)
    chip_id = 2;
  else
    return;

  if ((Port & 1) == 0) { // Scrittura all'indirizzo del registro
    ay_address[chip_id] = Value & 0x0F;
  }
  else { // Scrittura ai dati del registro
    soundregs[(chip_id * 16) + ay_address[chip_id]] = Value;
  }
}

unsigned char bombjack::inZ80(unsigned short Port) {
  if ((Port & 0xFF) == 0x01)
    return soundregs[ay_address[0]];
  else if ((Port & 0xFF) == 0x11)
    return soundregs[(1 * 16) + ay_address[1]];
  else if ((Port & 0xFF) == 0x81)
    return soundregs[(2 * 16) + ay_address[2]];
  else
    return 0xff;
}

void bombjack::run_frame(void) {
  for (int i = 0; i < INST_PER_FRAME; i++) {
    current_cpu = 0; StepZ80(&cpu[0]); StepZ80(&cpu[0]); StepZ80(&cpu[0]); StepZ80(&cpu[0]);
    current_cpu = 1; StepZ80(&cpu[1]); StepZ80(&cpu[1]); StepZ80(&cpu[1]);
  }

  // NMI per la CPU Principale (già corretto, è GATED)
  if (m_nmi_on) {
    current_cpu = 0;
    IntZ80(&cpu[0], INT_NMI);
  }

  if (m_mmi_skip_audio_cpu) {
    m_mmi_skip_audio_cpu = false;
    return;
  }

  // NMI per la CPU Audio (DEVE ESSERE INCONDIZIONATO)
  current_cpu = 1;
  IntZ80(&cpu[1], INT_NMI);
}

void bombjack::prepare_frame(void) {
  uint8_t large_sprite_rev = (memory[0 + 0x1000 + 0x400 + 0x400 + 0x80] > memory[1 + 0x1000 + 0x400 + 0x400 + 0x80]) ? 1 : 0;
  uint8_t large_sprite_start_idx = memory[large_sprite_rev + 0x1000 + 0x400 + 0x400 + 0x80];
  uint8_t large_sprite_end_idx = memory[(large_sprite_rev + 0x1000 + 0x400 + 0x400 + 0x80) ^ 1];
  active_sprites = 0;

  // Itera sui 32 slot degli sprite hardware, in ordine inverso (da 31 a 0)
  for (int idx = 31; idx >= 0; idx--) {
    if (active_sprites >= 32)
      break;

    unsigned char *sprite_base_ptr = (memory + 0x1000 + 0x400 + 0x400) + 4 * (31 - idx);
    uint8_t code_low = sprite_base_ptr[0];

    // Salta gli sprite inattivi
    if (code_low == 0 && sprite_base_ptr[2] == 0 && sprite_base_ptr[3] == 0)
      continue;

    struct sprite_S spr;

    spr.is_32x32 = (((31 - idx) / 2) > large_sprite_start_idx) && (((31 - idx) / 2) <= large_sprite_end_idx);

    if (spr.is_32x32) {
      if (((31 - idx) % 2) != 0)
        continue;
      spr.code = code_low & 0x3F;
    }
    else {
      spr.code = code_low;
    }

    spr.color_block = (sprite_base_ptr[1] & 0x0F) << 3;
    spr.flip_x = (sprite_base_ptr[1] & 0x40) != 0;
    spr.flip_y = (sprite_base_ptr[1] & 0x80) != 0;

    spr.x = sprite_base_ptr[2] - 16;
    spr.y = sprite_base_ptr[3];

    if (spr.is_32x32)
      spr.x -= 1;
   
    // printf("Sprite code=0x%02X, size=%s, x=%4d, y=%4d, color=%d, flip(X:%d,Y:%d)\n",spr.code,(spr.is_32x32 ? "32x32" : "16x16"),spr.x,spr.y,spr.color_block >> 3, spr.flip_x,spr.flip_y);
    if ((spr.y > -16) && (spr.y < 288) && (spr.x > -16) && (spr.x < 224))
      sprite[active_sprites++] = spr;
  }
}

void bombjack::blit_tile_bg(short logical_row) {
  // 1. Controlla se il background è visibile.
  if ((m_bg_image & 0x10) == 0) {
    memset(background_buffer[logical_row], 0, 224 * 8 * 2);
    return;
  }

  // Calcola l'indirizzo base dell'immagine di background.
  uint16_t img_base_addr = (m_bg_image & 7) * 0x0200;

  // Offset visuale
  const int visual_x_offset = 16;
  const int visual_y_offset = 24;

  // Itera su ogni pixel della striscia di destinazione.
  for (int y_in_strip = 0; y_in_strip < 8; y_in_strip++) {
    for (int x_in_strip = 0; x_in_strip < 224; x_in_strip++) {
      int dest_x = x_in_strip;
      int dest_y = (logical_row * 8) + y_in_strip;

      int shifted_dest_x = dest_x - visual_x_offset;
      int shifted_dest_y = dest_y - visual_y_offset;

      // Rotazione inversa per trovare le coordinate sorgente
      int source_x = shifted_dest_y;
      int source_y = 223 - shifted_dest_x;

      if (source_x < 0 || source_x >= 224 || source_y < 0 || source_y >= 288) {
        background_buffer[logical_row][(y_in_strip * 224) + x_in_strip] = 0;
        continue;
      }

      // --- CALCOLO DEL TILE E DEL PIXEL ---
      int map_col = source_x / 16;
      int map_row = source_y / 16;

      int pixel_x_in_tile = source_x % 16;
      int pixel_y_in_tile = source_y % 16;

      uint16_t map_addr = img_base_addr + (map_row * 16) + map_col;

      uint8_t tile_code = bombjack_bg_maps[map_addr];
      uint8_t attr = bombjack_bg_maps[map_addr + 0x100];

      uint8_t color_block = (attr & 0x0F) << 3;
      bool flip_y = (attr & 0x80) != 0;

      // Applica il flip Y se necessario.
      int final_pixel_y = flip_y ? (15 - pixel_y_in_tile) : pixel_y_in_tile;
      int final_pixel_x = pixel_x_in_tile;

      // --- ESTRAZIONE DEL PIXEL DAI DATI NON RUOTATI ---
      const unsigned long *tile_gfx = bombjack_bg_tiles[tile_code];

      unsigned long packed_data;
      uint8_t pen;

      // La logica di estrazione rimane la stessa, ma ora opera su dati non ruotati
      if (final_pixel_x < 8) {
        packed_data = tile_gfx[final_pixel_y * 2];
        pen = (packed_data >> (3 * (7 - final_pixel_x))) & 0x07;
      }
      else {
        packed_data = tile_gfx[final_pixel_y * 2 + 1];
        pen = (packed_data >> (3 * (7 - (final_pixel_x - 8)))) & 0x07;
      }

      // Scrittura nel framebuffer.
      if (pen == 0) {
        background_buffer[logical_row][(y_in_strip * 224) + x_in_strip] = 0;
      }
      else {
        background_buffer[logical_row][(y_in_strip * 224) + x_in_strip] = bombjack_palette[color_block | pen];
      }
    }
  }
}

void bombjack::blit_tile_fg(short row, char col) {
  if ((row < 2) || (row >= 34))
    return;

  unsigned short tilemap_index = tileaddr[row][col];

  uint8_t chr = memory[tilemap_index + 0x1000];
  uint8_t clr = memory[tilemap_index + 0x1000 + 0x400];

  uint16_t tile_id = chr | ((clr & 0x10) << 4);
  uint8_t color_block = (clr & 0x0F) << 3;

  const unsigned long *tile_gfx = bombjack_fg_tiles[tile_id];
  unsigned short *ptr = frame_buffer + (col * 8);

  for (char r = 0; r < 8; r++) {
    unsigned long packed_pixels = *tile_gfx++;

    for (char c = 0; c < 8; c++) {
      uint8_t pen = (packed_pixels >> (3 * (7 - c))) & 0x07;

      if (pen != 0)
        ptr[c] = bombjack_palette[color_block | pen];
    }
    ptr += 224;
  }
}

void bombjack::blit_sprite(short row, unsigned char s_idx) {
  if ((row < 2) || (row >= 34))
    return;

  if (s_idx >= active_sprites)
    return;

  struct sprite_S *s = &sprite[s_idx];
  int size = s->is_32x32 ? 32 : 16;

  const unsigned long *sprite_gfx_data;
  if (s->is_32x32) {
    if (s->code >= 64)
      return;

    sprite_gfx_data = bombjack_sprites_32x32[s->code];
  }
  else {
    if (s->code >= 256)
      return;

    sprite_gfx_data = bombjack_sprites_16x16[s->code];
  }

  int strip_start_y = (row - 2) * 8;
  int strip_end_y = strip_start_y + 8;

  int sprite_start_y = s->y;
  int sprite_end_y = sprite_start_y + size;

  if (sprite_end_y <= strip_start_y || sprite_start_y >= strip_end_y)
    return;

  // Itera su ogni pixel della bounding box di destinazione sullo schermo
  for (int y_on_screen = strip_start_y; y_on_screen < strip_end_y; y_on_screen++) {
    for (int x_on_screen = 0; x_on_screen < 224; x_on_screen++) {
      // Calcola la posizione del pixel relativo all'angolo dello sprite
      int px_in_sprite_x = x_on_screen - s->x;
      int px_in_sprite_y = y_on_screen - s->y;

      // Se il pixel è fuori dalla bounding box dello sprite, continua
      if (px_in_sprite_x < 0 || px_in_sprite_x >= size || px_in_sprite_y < 0 || px_in_sprite_y >= size)
        continue;

      int sx = px_in_sprite_y;
      int sy = (size - 1) - px_in_sprite_x;

      if (s->flip_x)
        sx = (size - 1) - sx;
 
      if (s->flip_y)
        sy = (size - 1) - sy;
 
      int pixel_index = sy * size + sx;
      int long_index = pixel_index / 8;
      int bit_offset_in_long = pixel_index % 8;

      unsigned long packed_pixels = sprite_gfx_data[long_index];
      uint8_t pen = (packed_pixels >> (3 * (7 - bit_offset_in_long))) & 0x07;

      if (pen != 0) {
        uint16_t color = bombjack_palette[s->color_block | pen];
        int y_in_strip = y_on_screen - strip_start_y;
        frame_buffer[y_in_strip * 224 + x_on_screen] = color;
      }
    }
  }
}

void bombjack::render_row(short row) {
  //use a buffer for the background for better performance...
  if (!background_bufferAllocated) {
    background_bufferAllocated = true;
    for (int i = 0; i < 36; i++) {
      background_buffer[i] = (unsigned short *)malloc(224 * 8 * 2);
      memset(background_buffer[i], 0, 224 * 8 * 2);
    }
  }

  //calculate new background when image changed only...
  if (currentBackground[row] != m_bg_image) {
    currentBackground[row] = m_bg_image;
    blit_tile_bg(row);
  }

  //copy the buffered background...
  memcpy(frame_buffer, background_buffer[row], 224 * 8 * 2);

  for (char col = 0; col < 28; col++) {
    blit_tile_fg(row, col);
  }

  // render sprites
  for (unsigned char s = 0; s < active_sprites; s++) {
    blit_sprite(row, s);
  }
}

const unsigned short *bombjack::logo(void) {
  return bombjack_logo;
}

#ifdef LED_PIN
void bombjack::gameLeds(CRGB *leds)
{
  // bombjack: slow yellow on green "knight rider" ...
  static char sub_cnt = 0;
  if (sub_cnt++ == 32)
  {
    sub_cnt = 0;

    // and also do the marquee LEDs
    static char led = 0;

    char il = (led < NUM_LEDS) ? led : ((2 * NUM_LEDS - 2) - led);
    for (char c = 0; c < NUM_LEDS; c++)
    {
      if (c == il)
        leds[c] = LED_YELLOW;
      else
        leds[c] = LED_GREEN;
    }
    led = (led + 1) % (2 * NUM_LEDS - 2);
  }
}

void bombjack::menuLeds(CRGB *leds)
{
  memcpy(leds, menu_leds, NUM_LEDS * sizeof(CRGB));
}
#endif