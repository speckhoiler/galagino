#include "starforce.h"

unsigned char starforce::opZ80(unsigned short Addr) {
  if (current_cpu == 0)
    return starforce_main_cpu_rom[Addr];
  else 
    return starforce_sub_cpu_rom[Addr];
}

unsigned char starforce::rdZ80(unsigned short Addr) {
  if (current_cpu == 0) {
    // --- Letture dalla CPU Principale ---
    // 0x0000 - 0x7FFF: ROM principale (32 KB)
    if (Addr < 0x8000)
      return starforce_main_cpu_rom[Addr];

    // 0x8000 - 0x8FFF: RAM Generica (4 KB)
    if (Addr >= 0x8000 && Addr <= 0x8FFF)
      return memory[STARFORCE_GENERAL_RAM + (Addr - 0x8000)];

    // 0x9000 - 0x93FF: Video RAM Foreground (1 KB)
    if (Addr >= 0x9000 && Addr <= 0x93FF)
      return memory[STARFORCE_FG_VIDEO_RAM + (Addr - 0x9000)];

    // 0x9400 - 0x97FF: Color RAM Foreground (1 KB)
    if (Addr >= 0x9400 && Addr <= 0x97FF)
      return memory[STARFORCE_FG_COLOR_RAM + (Addr - 0x9400)];

    // 0x9800 - 0x987F: Sprite RAM (128 bytes)
    if (Addr >= 0x9800 && Addr <= 0x987F)
      return memory[STARFORCE_SPRITE_RAM + (Addr - 0x9800)];

    // 0x9C00 - 0x9DFF: Palette RAM (512 bytes)
    if (Addr >= 0x9C00 && Addr <= 0x9DFF)
      return memory[STARFORCE_PALETTE_RAM + (Addr - 0x9C00)];

    // 0x9E00 - 0x9E3F: Registri di controllo (64 bytes)
    if (Addr >= 0x9E00 && Addr <= 0x9E3F)
      return memory[STARFORCE_HW_CONTROL_RAM + (Addr - 0x9E00)];

    // 0xA000 - 0xA7FF: Video RAM Background 3 (2 KB)
    if (Addr >= 0xA000 && Addr <= 0xA7FF)
      return memory[STARFORCE_BG3_VIDEO_RAM + (Addr - 0xA000)];

    // 0xA800 - 0xAFFF: Video RAM Background 2 (2 KB)
    if (Addr >= 0xA800 && Addr <= 0xAFFF)
       return memory[STARFORCE_BG2_VIDEO_RAM + (Addr - 0xA800)];

    // 0xB000 - 0xB7FF: Video RAM Background 1 (2 KB)
    if (Addr >= 0xB000 && Addr <= 0xB7FF)
       return memory[STARFORCE_BG1_VIDEO_RAM + (Addr - 0xB000)];

    // 0xB800 - 0xBBFF: Radar RAM (1 KB)
    if (Addr >= 0xB800 && Addr <= 0xBBFF)
      return memory[STARFORCE_RADAR_RAM + (Addr - 0xB800)];

    // 0xD000 - 0xD005: Input e DSW
    if (Addr >= 0xD000 && Addr <= 0xD005) {
      unsigned char retval = 0x00;
      unsigned char keymask = 0x00;
      switch (Addr)
      {
      case 0xD000:
        keymask = input->buttons_get();
	      if(keymask & BUTTON_RIGHT) retval |= 0x01;
	      if(keymask & BUTTON_LEFT)  retval |= 0x02;
	      if(keymask & BUTTON_UP)    retval |= 0x04;
	      if(keymask & BUTTON_DOWN)  retval |= 0x08;
        if(keymask & BUTTON_FIRE)  retval |= 0x10;
        return retval;
      case 0xD001:
        return 0x00; // Input Giocatore 2
      case 0xD002:
        keymask = input->buttons_get();
        if(keymask & BUTTON_START) retval |= 0x04;       
        if(keymask & BUTTON_COIN && !coinBackup) {
          coinFrameCounter = 2;
          coinBackup = 1;
        }
        
        //coin pulse for min 2 and max 9 frames...
        if (coinFrameCounter > 0) {
          retval |= 0x01; 
        }
        else {
          if ((keymask & BUTTON_COIN) == 0)
            coinBackup = 0;
        }
        return retval;
      case 0xD003:
        return 0x00; // NOPR in MAME (Not Read)
      case 0xD004:
        game_started = 1;
        return STARFORCE_DSW1 | (input->demoSoundsOff() ? STARFORCE_DSW2_DEMO_SOUND_OFF : STARFORCE_DSW2_DEMO_SOUND_ON);
      case 0xD005:
        return STARFORCE_DSW2;
      }
    }
  }
  else {
    // --- Letture dalla CPU Audio ---
    // 0x0000 - 0x1FFF: ROM Audio (8 KB)
    if (Addr < 0x2000)
      return starforce_sub_cpu_rom[Addr];

    // 0x4000 - 0x43FF: RAM Audio (1 KB)
    if (Addr >= 0x4000 && Addr <= 0x43FF)
      return memory[STARFORCE_SOUND_RAM + (Addr - 0x4000)];
  }
  return 0xFF;
}

void starforce::wrZ80(unsigned short Addr, unsigned char Value) {
  if (current_cpu == 0) {
    // 0x8000 - 0x8FFF: RAM Generica (4 KB)
    if (Addr >= 0x8000 && Addr <= 0x8FFF) {
      memory[STARFORCE_GENERAL_RAM + (Addr - 0x8000)] = Value;
      return;
    }

    // 0x9000 - 0x93FF: Video RAM Foreground (1 KB)
    if (Addr >= 0x9000 && Addr <= 0x93FF) {
      memory[STARFORCE_FG_VIDEO_RAM + (Addr - 0x9000)] = Value;
      return;
    }

    // 0x9400 - 0x97FF: Color RAM Foreground (1 KB)
    if (Addr >= 0x9400 && Addr <= 0x97FF) {
      memory[STARFORCE_FG_COLOR_RAM + (Addr - 0x9400)] = Value;
      return;
    }

    // 0x9800 - 0x987F: Sprite RAM (128 bytes)
    if (Addr >= 0x9800 && Addr <= 0x987F) {
      memory[STARFORCE_SPRITE_RAM + (Addr - 0x9800)] = Value;
      return;
    }

    // 0x9C00 - 0x9DFF: Palette RAM (512 bytes)
    if (Addr >= 0x9C00 && Addr <= 0x9DFF) {
      memory[STARFORCE_PALETTE_RAM + (Addr - 0x9C00)] = Value;
      starforce_palette[Addr - 0x9C00] = calculate_color_starforce(Value);
      return;
    }

    // 0x9E00 - 0x9E3F: Registri di controllo (64 bytes)
    if (Addr >= 0x9E00 && Addr <= 0x9E3F) {
      memory[STARFORCE_HW_CONTROL_RAM + (Addr - 0x9E00)] = Value;
      return;
    }

    // 0xA000 - 0xA7FF: Video RAM Background 3 (2 KB)
    if (Addr >= 0xA000 && Addr <= 0xA7FF) {
      memory[STARFORCE_BG3_VIDEO_RAM + (Addr - 0xA000)] = Value;
      return;
    }

    // 0xA800 - 0xAFFF: Video RAM Background 2 (2 KB)
    if (Addr >= 0xA800 && Addr <= 0xAFFF) {
      memory[STARFORCE_BG2_VIDEO_RAM + (Addr - 0xA800)] = Value;
      return;
    }

    // 0xB000 - 0xB7FF: Video RAM Background 1 (2 KB)
    if (Addr >= 0xB000 && Addr <= 0xB7FF) {
      memory[STARFORCE_BG1_VIDEO_RAM + (Addr - 0xB000)] = Value;
      return;
    }

    // 0xB800 - 0xBBFF: Radar RAM (1 KB)
    if (Addr >= 0xB800 && Addr <= 0xBBFF) {
      memory[STARFORCE_RADAR_RAM + (Addr - 0xB800)] = Value;
      return;
    }

    // 0xD000 - 0xD005: Controllo hardware
    if (Addr >= 0xD000 && Addr <= 0xD005) {
      switch (Addr)
      {
      case 0xD000: // flip_screen_w
        return;
      case 0xD002:
        //irq_ctrl_w: Used during power on selft test from main cpu (skip interrupts)
        return;
      case 0xD004: // Latch per il comando audio
        sound_latch = Value;
        sound_latch_pending = 1;
        return;
      }
    }
  }
  else {
    // --- Scritture dalla CPU Audio ---
    // 0x4000 - 0x43FF: RAM Audio (1 KB)
    if (Addr >= 0x4000 && Addr <= 0x43FF) {
      memory[STARFORCE_SOUND_RAM + (Addr - 0x4000)] = Value;
      return;
    }

    // 0x8000, 0x9000, 0xA000: Scrittura ai chip audio SN76489
    if (Addr == 0x8000) {
      SN76489_Write_3chip(0, Value); // Chiama direttamente la funzione di scrittura
      return;
    }
    if (Addr == 0x9000) {
      SN76489_Write_3chip(1, Value); // Chiama direttamente la funzione di scrittura
      return;
    }
    if (Addr == 0xA000) {
      SN76489_Write_3chip(2, Value); // Chiama direttamente la funzione di scrittura
      return;
    }

    // Aggiunta per il DAC (non essenziale per ora, ma completo)
    if (Addr == 0xD000) { /* DAC Volume */
      return;
    }

    if (Addr == 0xE000) { /* DAC Enable */
      return;
    }
  }
}

unsigned char starforce::inZ80(unsigned short Port) {
  // La CPU audio sta leggendo dalla porta dati
  if ((Port & 0xFF) == 0x00) {
    sound_latch_pending = 0; 
    return sound_latch;
  }
  return 0x00;
}

void starforce::outZ80(unsigned short Port, unsigned char Value) {
  // Initialize Z80pio and Z80ctc
  //09: z80ctc-ch2  tempo interrupt 88.778Hz
  //Port &= 0xFF;
  //printf("Out Port: %X, Value: %d\n", Port, Value);
}

void starforce::run_frame(void) {
  for (int i = 0; i < 1666; i++) {
    current_cpu = 0; StepZ80(&cpu[0]); StepZ80(&cpu[0]); StepZ80(&cpu[0]); StepZ80(&cpu[0]);
    current_cpu = 1; StepZ80(&cpu[1]); StepZ80(&cpu[1]);
    if (sound_irq_toggle && i == 832) {
      IntZ80(&cpu[1], sound_latch_pending ? 0x00 : 0x0A); // 30Hz; Vector 0x0A = daisy chain
    }
  }

  // 60Hz + 30Hz = ~88.778Hz
  sound_irq_toggle ^= 1;
  IntZ80(&cpu[1], sound_latch_pending ? 0x00 : 0x0A); // 60Hz; Vector 0x0A = daisy chain

  current_cpu = 0;
  IntZ80(&cpu[0], INT_IRQ);

  if (coinFrameCounter > 0)
    coinFrameCounter--;
}

void starforce::SN76489_Write_3chip(int chip, unsigned char data) {
  if (data & 0x80) { // Latch
    int reg = (data >> 5) & 0x03; 
    int type = (data >> 4) & 0x01;
    sn_last_register[chip] = (reg * 2) + type;
        
    if (reg < 4) { 
      if (type == 0) { // Freq (4 bit bassi)
        sn_period[chip][reg] = (sn_period[chip][reg] & 0x3F0) | (data & 0x0F);
      } 
      else { // Volume
        sn_volume[chip][reg] = (data & 0x0F);
      }
    }
  } 
  else { // Dati
    int reg = sn_last_register[chip] / 2;
    int type = sn_last_register[chip] % 2;

    if (reg < 4 && type == 0) { // Se il latch era per una Frequenza
      if (reg == 3) { // Canale di rumore
        sn_period[chip][3] = data & 0x07;
      } 
      else { // Canali di tono
        sn_period[chip][reg] = (sn_period[chip][reg] & 0x00F) | ((data & 0x3F) << 4);
      }
    }
  }
}

void starforce::prepare_frame(void) {
  active_sprites = 0;

  // MAME itera al contrario, lo manteniamo per accuratezza.
  for (int i = 31; i >= 0 && active_sprites < 128; i--) {
    unsigned char *sprite_base_ptr = memory + STARFORCE_SPRITE_RAM + (i * 4);

    unsigned char code_byte = sprite_base_ptr[0];
    unsigned char attr_byte = sprite_base_ptr[1];
    unsigned char y_byte = sprite_base_ptr[2]; // y originale del gioco verticale
    unsigned char x_byte = sprite_base_ptr[3]; // x originale del gioco verticale

    struct sprite_S *spr = &sprite[active_sprites];
    spr->is_32x32 = (code_byte & 0xC0) == 0xC0;

    if (spr->is_32x32) {
      // L'indice corretto si ottiene mascherando con 0x7F (127)
      spr->code = code_byte & 0x7F;
      spr->x = 224 - y_byte;
    }
    else {
      spr->code = code_byte;
      spr->x = 240 - y_byte;
    }

    spr->y = x_byte;

    short size = spr->is_32x32 ? 32 : 16;
    if (spr->x > 224 || (spr->x + size) < 0 || spr->y > 288 || (spr->y + size) < 0)
      continue;

    spr->color = attr_byte & 0x07;
    spr->flags = ((attr_byte & 0x40) ? 1 : 0) | ((attr_byte & 0x80) ? 2 : 0);
    spr->priority = (attr_byte & 0x30) >> 4;

    active_sprites++;
  }
}

inline unsigned short starforce::calculate_color_starforce(unsigned char raw_palette_byte) {
  // Passaggi 1-4: Calcolo dei componenti, identici a prima
  unsigned char i = (raw_palette_byte >> 6) & 0x03;
  unsigned char b_base = (raw_palette_byte >> 4) & 0x03;
  unsigned char g_base = (raw_palette_byte >> 2) & 0x03;
  unsigned char r_base = (raw_palette_byte >> 0) & 0x03;

  unsigned char r4 = r_base ? ((r_base << 2) | i) : 0;
  unsigned char g4 = g_base ? ((g_base << 2) | i) : 0;
  unsigned char b4 = b_base ? ((b_base << 2) | i) : 0;

  unsigned char r8 = r4 * 17;
  unsigned char g8 = g4 * 17;
  unsigned char b8 = b4 * 17;

  unsigned char r5 = r8 >> 3;
  unsigned char g6 = g8 >> 2;
  unsigned char b5 = b8 >> 3;

  // 5. Impacchettamento nel formato standard Little Endian
  unsigned short rgb565_le = (r5 << 11) | (g6 << 5) | b5;

  // 6. ESEGUI LO SWAP DEI BYTE
  // Questo trasforma 0xABCD in 0xCDAB
  return ((rgb565_le & 0xFF00) >> 8) | ((rgb565_le & 0x00FF) << 8);
}

void starforce::blit_background_line(short start_screen_row, int layer_num) {
  short start_tile_row = start_screen_row / 8;
  if (start_tile_row < 4 || start_tile_row >= 32)
    return;

  // (Tutta la logica di setup iniziale rimane invariata)
  const uint32_t *base_tile_ptr;
  unsigned char *vram_data;
  int scroll_x, scroll_y;
  int num_tiles_in_map = (layer_num == 3) ? 128 : 256;
  int palette_bank_offset;
  unsigned char *hw_control_ram = &memory[STARFORCE_HW_CONTROL_RAM];

  switch (layer_num) {
  // ... (nessuna modifica qui) ...
  case 1:
    base_tile_ptr = (const uint32_t *)starforce_bg1_tilemap;
    vram_data = &memory[STARFORCE_BG1_VIDEO_RAM];
    scroll_x = (hw_control_ram + 0x35)[0];
    scroll_y = (hw_control_ram + 0x30)[0] + ((hw_control_ram + 0x30)[1] << 8);
    palette_bank_offset = 64;
    break;
  case 2:
    base_tile_ptr = (const uint32_t *)starforce_bg2_tilemap;
    vram_data = &memory[STARFORCE_BG2_VIDEO_RAM];
    scroll_x = (hw_control_ram + 0x35)[0];
    scroll_y = (hw_control_ram + 0x30)[0] + ((hw_control_ram + 0x30)[1] << 8);
    palette_bank_offset = 128;
    break;
  case 3:
    base_tile_ptr = (const uint32_t *)starforce_bg3_tilemap;
    vram_data = &memory[STARFORCE_BG3_VIDEO_RAM];
    scroll_x = (hw_control_ram + 0x25)[0];
    scroll_y = (hw_control_ram + 0x20)[0] + ((hw_control_ram + 0x20)[1] << 8);
    palette_bank_offset = 192;
    break;
  default:
    return;
  }

  // La tua struttura di loop originale
  int logical_x_start = start_screen_row + scroll_x;

  for (int x_buffer = 0; x_buffer < 224; x_buffer++) {
    int game_screen_y = x_buffer;
    int logical_y = game_screen_y + scroll_y;
    int source_tile_row = (logical_y / 16) % 32;
    int y_in_tile = logical_y & 15;

    for (int y_in_buffer = 0; y_in_buffer < 8; y_in_buffer++) {
      int logical_x = logical_x_start + y_in_buffer;
      int source_tile_col = (logical_x / 16) & 15;
      int x_in_tile = logical_x & 15;

      unsigned short vram_addr = (source_tile_row * 16) + source_tile_col;
      unsigned char tile_code = vram_data[vram_addr];

      if (tile_code >= num_tiles_in_map)
        continue;

      int tile_offset = (tile_code * 16 * 2) + (y_in_tile * 2);
      const uint32_t *tile_gfx_row = base_tile_ptr + tile_offset;

      uint32_t packed_chunk = tile_gfx_row[x_in_tile / 8];
      unsigned char px = (packed_chunk >> (3 * (7 - (x_in_tile % 8)))) & 0x07;

      if (px != 0) {
        // La logica della palette è corretta
        unsigned char color_group;
        if (layer_num == 1) {
          unsigned char top_bits = (tile_code & 0xE0) >> 5;
          color_group = ((top_bits & 0x02) >> 1) | ((top_bits & 0x01) << 1) | (top_bits & 0x04);
        }
        else {
          color_group = (tile_code & 0xE0) >> 5;
        }
        const unsigned short *colors = &starforce_palette[palette_bank_offset + color_group * 8];

        // ===============================================
        // === LA SOLUZIONE: INVERTI x_buffer IN SCRITTURA ===
        // ===============================================
        // Il pixel calcolato per la colonna 0 va in 223.
        // Il pixel calcolato per la colonna 1 va in 222.
        // ...
        // Il pixel calcolato per la colonna 223 va in 0.
        frame_buffer[y_in_buffer * 224 + (223 - x_buffer)] = colors[px];
      }
    }
  }
}

void starforce::blit_tile_fg(short row, char col) {
  // Il controllo dei limiti è corretto.
  if ((row < 2) || (row >= 34))
    return;

  // L'indirizzamento della VRAM è corretto.
  unsigned short vram_addr = tileaddr[row][col];

  unsigned char base_tile_code = memory[STARFORCE_FG_VIDEO_RAM + vram_addr];
  unsigned char color_attr = memory[STARFORCE_FG_COLOR_RAM + vram_addr];

  unsigned int final_tile_code = base_tile_code;
  if (color_attr & 0x10)
    final_tile_code += 256;

  const unsigned int *tile_data = starforce_fg_tilemap[final_tile_code];

  // --- MODIFICA CHIAVE ---
  // Invece di calcolare un 'offset' numerico, creiamo un puntatore
  // che punta direttamente al primo dei 8 colori corretti nella nostra palette pre-calcolata.
  const unsigned short *colors = &starforce_palette[(color_attr & 0x07) * 8];

  unsigned short *ptr = frame_buffer + 8 * col;

  // Il doppio loop di disegno è corretto per la gestione dei dati pre-ruotati.
  for (char c = 0; c < 8; c++, ptr += (224 - 8)) {
    unsigned int rowdata = tile_data[c];
    for (char r = 0; r < 8; r++) {
      // Estrae l'indice del pixel (0-7) dalla riga di dati.
      unsigned char px = (rowdata >> (3 * (7 - r))) & 0x07;

      if (px != 0) { // Il pixel 0 è trasparente.
        // --- MODIFICA CHIAVE ---
        // Adesso basta una singola, velocissima operazione di lookup!
        // 'colors' punta al primo colore del banco (es. colore 0).
        // 'colors[px]' accede direttamente al colore corretto (es. colore 0 + px).
        *ptr = colors[px];
      }
      ptr++;
    }
  }
}

void starforce::blit_sprite(short row, unsigned char s_idx, unsigned char target_priority) {
  if ((row < 2) || (row >= 34))
    return;

  const struct sprite_S *spr = &sprite[s_idx];

  // Controllo priorità
  if (spr->priority != target_priority)
    return;

  const short size = spr->is_32x32 ? 32 : 16;

  // Calcola le coordinate Y della striscia corrente
  const short y_strip_start = row * 8;

  // Culling verticale (se lo sprite non interseca questa striscia, esci)
  if (spr->y >= y_strip_start + 8 || (spr->y + size) <= y_strip_start)
    return;
 
  const unsigned short *colors = &starforce_palette[320 + (spr->color * 8)];

  // Itera su ogni riga dello sprite (da 0 a size-1)
  for (int y_in_sprite = 0; y_in_sprite < size; y_in_sprite++) {
    int dest_y = spr->y + y_in_sprite;

    // Se questa riga specifica non è nella striscia, salta
    if (dest_y < y_strip_start || dest_y >= y_strip_start + 8)
      continue;

    // Calcola la riga corrispondente nel nostro framebuffer (0-7)
    int y_in_buffer = dest_y - y_strip_start;

    // Applica Flip Y
    int y_read = (spr->flags & 2) ? (size - 1 - y_in_sprite) : y_in_sprite;

    // Itera sui pixel della riga
    for (int x_in_sprite = 0; x_in_sprite < size; x_in_sprite++) {
      int dest_x = spr->x + x_in_sprite;

      // Clipping orizzontale
      if (dest_x < 0 || dest_x >= 224)
        continue;

      // Applica Flip X
      int x_read = (spr->flags & 1) ? (size - 1 - x_in_sprite) : x_in_sprite;

      // --- ESTRAZIONE PIXEL DAI NUOVI DATI ---
      int chunk_idx = x_read / 8;
      int x_in_chunk = x_read % 8;

      unsigned long packed_data;
      if (spr->is_32x32) {
        // Maschera per ottenere solo gli ultimi 6 bit del codice per l'array 32x32 che pero e gia calcolata in prepare_frame
        packed_data = starforce_sprites_32x32[spr->code][y_read][chunk_idx];
      }
      else {
        packed_data = starforce_sprites_16x16[spr->code][y_read][chunk_idx];
      }

      unsigned char px = (packed_data >> (3 * (7 - x_in_chunk))) & 0x07;
      // --- FINE ESTRAZIONE ---

      if (px != 0) {
        // Scrivi il pixel nel buffer
        frame_buffer[y_in_buffer * 224 + (223 - dest_x)] = colors[px];
        // frame_buffer[y_in_buffer * 224 + dest_x] = colors[px];
      }
    }
  }
}

void starforce::render_row(short row) {
  // Sprite con priorità 0
  for (unsigned char s = 0; s < active_sprites; s++) {
    blit_sprite(row, s, 0);
  }

#ifdef ENABLE_BG3
  blit_background_line(row * 8, 3); // BG3
#endif

  // Sprite con priorità 1
  for (unsigned char s = 0; s < active_sprites; s++) {
    blit_sprite(row, s, 1);
  }

  blit_background_line(row * 8, 2); // BG2

  // Sprite con priorità 2
  for (unsigned char s = 0; s < active_sprites; s++) {
    blit_sprite(row, s, 2);
  }

  blit_background_line(row * 8, 1); // BG1

  // Sprite con priorità 3
  for (unsigned char s = 0; s < active_sprites; s++) {
    blit_sprite(row, s, 3);
  }

  for (char col = 0; col < 28; col++) {
    blit_tile_fg(row, col);
  }
}

const unsigned short *starforce::logo(void) {
  return starforce_logo;
}