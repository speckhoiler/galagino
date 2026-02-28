#include "mrdo.h"

void mrdo::reset() {
  machineBase::reset();
  ignoreFireButton = 1;
}

unsigned char mrdo::opZ80(unsigned short Addr) {
  return mrdo_rom1[Addr];
}

unsigned char mrdo::rdZ80(unsigned short Addr) {
  if (Addr < 0x8000)
    return mrdo_rom1[Addr];

  if (Addr >= 0x8000 && Addr <= 0x87FF) {
    if (Addr < 0x8400)
      return memory[Addr - 0x8000];
    else
      return memory[Addr - 0x8400 + 0x400];
  }

  if (Addr >= 0x8800 && Addr <= 0x8FFF) {
    if (Addr < 0x8C00)
      return memory[Addr - 0x8800 + 0x800];
    else
      return memory[Addr - 0x8C00 + 0xc00];
  }

  if (Addr >= 0xE000 && Addr <= 0xEFFF)
    return memory[Addr - 0xE000 + 0x1100];

  // Logica di gestione delle porte di input/registri
  switch (Addr) {
    case 0x9803: // --- PROTEZIONE MR. DO! ---
    {
      return protection_r();
    }
    case 0xA000: // Player 1
    {
      // Il gioco si aspetta input IP_ACTIVE_LOW.
      // Iniziamo con tutti i bit a 1 (pulsanti non premuti).
      unsigned char retval = 0xFF;

      // Legge lo stato attuale dei pulsanti fisici.
      unsigned char keymask = input->buttons_get();

      // Se un pulsante è premuto (bit a 1 in keymask), imposta il bit corrispondente a 0.
      if (keymask & BUTTON_LEFT)
        retval &= ~0x01;
      if (keymask & BUTTON_DOWN)
        retval &= ~0x02;
      if (keymask & BUTTON_RIGHT)
        retval &= ~0x04;
      if (keymask & BUTTON_UP)
        retval &= ~0x08;
      // if game started with fire button, do not jump to service mode...
      if (ignoreFireButton & !(keymask & BUTTON_FIRE))
        ignoreFireButton = 0;

      if(!ignoreFireButton && (keymask & BUTTON_FIRE)) {
        retval &= ~0x10;
    }

    // Player 1 START è sulla porta A000, bit 5 (0x20)
    if (keymask & BUTTON_START)
      retval &= ~0x20;
    return retval;
  }
  case 0xA001: // Player 2 / Coin
  {
    // Anche questa porta è IP_ACTIVE_LOW.
    unsigned char retval = 0xFF;
    unsigned char keymask = input->buttons_get();

    if (keymask & BUTTON_COIN)
      retval &= ~0x40;
    return retval;
  }
  case 0xA002: // DSW1
    return MRDO_DIP1;

  case 0xA003: // DSW2
    return MRDO_DIP2;
  }
  // Se l'indirizzo non corrisponde a nessuna mappatura, restituisci 0xFF
  return 0xff;
}

void mrdo::wrZ80(unsigned short Addr, unsigned char Value) {
  if (Addr >= 0x8000 && Addr <= 0x87FF) {
    if (Addr < 0x8400) { // Indirizzi 0x8000-0x83FF -> bg_colorram
      memory[Addr - 0x8000] = Value;
      protection_w(Value);
    }
    else { // Indirizzi 0x8400-0x87FF -> bg_videoram
      memory[Addr - 0x8400 + 0x400] = Value;
      protection_w(Value);
    }
    return;
  }

  if (Addr >= 0x8800 && Addr <= 0x8FFF) {
    if (Addr < 0x8C00) { // Indirizzi 0x8800-0x8BFF -> fg_colorram
      memory[Addr - 0x8800 + 0x800] = Value;
      protection_w(Value);
    }
    else { // Indirizzi 0x8C00-0x8FFF -> fg_videoram
      memory[Addr - 0x8C00 + 0xc00] = Value;
      protection_w(Value);
    }
    return;
  }

  if (Addr >= 0x9000 && Addr <= 0x90FF) {
    memory[Addr - 0x9000 + 0x1000] = Value;
    return;
  }

  if (Addr >= 0xE000 && Addr <= 0xEFFF) {
    memory[Addr - 0xE000 + 0x1100] = Value;
    return;
  }

  // Scritture su registri hardware
  if (Addr == 0x9800) {
    flipscreen_w = Value & 0x01;
    return;
  }
  // ...
  if (Addr == 0x9801) {
    // audio_trigger_chip0 = Value; // Non più necessario
    SN76489_Write_2chip(0, Value); // Chiama direttamente la funzione di scrittura
    return;
  }

  if (Addr == 0x9802) {
    // audio_trigger_chip1 = Value; // Non più necessario
    SN76489_Write_2chip(1, Value); // Chiama direttamente la funzione di scrittura
    return;
  }
  // ...
  if (Addr >= 0xF000 && Addr <= 0xF7FF) {
    scrollx_w = Value;
    return;
  }

  if (Addr >= 0xF800 && Addr <= 0xFFFF) {
    scrolly_w = Value;
    return;
  }
}

void mrdo::protection_w(unsigned char data) {
  const unsigned char i9 = (data >> 0) & 1;
  const unsigned char i8 = (data >> 1) & 1;
  // const unsigned char i7 = (data >> 2) & 1; // non usato nelle equazioni
  const unsigned char i6 = (data >> 3) & 1;
  const unsigned char i5 = (data >> 4) & 1;
  const unsigned char i4 = (data >> 5) & 1;
  const unsigned char i3 = (data >> 6) & 1;
  const unsigned char i2 = (data >> 7) & 1;

  // Equazioni booleane della PAL
  const unsigned char t1 = i2 & !i3 & i4 & !i5 & !i6 & !i8 & i9;
  const unsigned char t2 = !i2 & !i3 & i4 & i5 & !i6 & i8 & !i9;
  const unsigned char t3 = i2 & i3 & !i4 & !i5 & i6 & !i8 & i9;
  const unsigned char t4 = !i2 & i3 & i4 & !i5 & i6 & i8 & i9;

  const unsigned char r13 = (t1) << 1;
  const unsigned char r14 = (t1 | t2) << 2;
  const unsigned char r15 = (t1 | t3) << 3;
  const unsigned char r16 = (t1) << 4;
  const unsigned char r17 = (t1 | t3) << 5;
  const unsigned char r18 = (t3 | t4) << 6;

  m_pal_u001 = ~(r18 | r17 | r16 | r15 | r14 | r13);
}

// Questa funzione simula la lettura dalla PAL di protezione.
unsigned char mrdo::protection_r() {
  return m_pal_u001;
}

void mrdo::SN76489_Write_2chip(int chip, unsigned char data) {
  if (data & 0x80) {               // Latch
    int reg = (data >> 5) & 0x03;  // Canale 0-3
    int type = (data >> 4) & 0x01; // 0=Freq, 1=Volume
    sn_last_register[chip] = (reg * 2) + type;

    if (reg < 4) {
      if (type == 0) { // Freq (4 bit bassi)
        sn_period[chip][reg] = (sn_period[chip][reg] & 0x3F0) | (data & 0x0F);
      }
      else { // Volume
        sn_volume[chip][reg] = data & 0x0F;
      }
    }
  }
  else { // Dati
    int reg = sn_last_register[chip] / 2;
    int type = sn_last_register[chip] % 2;

    if (reg < 4 && type == 0) { // Se il latch era per una Frequenza
      if (reg == 3) {   // Canale di rumore
        sn_period[chip][3] = data & 0x07; // 3 bit per il controllo del rumore
      }
      else { // Canali di tono
        sn_period[chip][reg] = (sn_period[chip][reg] & 0x00F) | ((data & 0x3F) << 4);
      }
    }
  }
}

void mrdo::run_frame(void) {
  for (int i = 0; i < INST_PER_FRAME; i++) {
    StepZ80(&cpu[0]); StepZ80(&cpu[0]); StepZ80(&cpu[0]); StepZ80(&cpu[0]);
  }
  
  // GENERA IRQ al termine del frame
  IntZ80(&cpu[0], INT_IRQ); // INT_IRQ = 0x00FF

  // hold interrupt line until acknowledged
  while (cpu[0].IFF == IFF_IM1)
    StepZ80(&cpu[0]);

  if (!game_started)
    game_started = 1;
}

void mrdo::prepare_frame(void) {
  active_sprites = 0;

  for (int i = 63; i >= 0; i--) {
    if (active_sprites >= 64)
      break;

    const uint8_t *sprite_data = memory + 0x1000 + (i * 4);

    uint8_t y_raw = sprite_data[1];
    if (y_raw != 0) {
      struct sprite_S *s = &sprite[active_sprites];

      // Coordinate originali del gioco (verticale)
      short x_raw = sprite_data[3];
      short y_bottom = 256 - y_raw;

      // Calcolo di s->x (asse orizzontale dello schermo)
      s->x = 224 - 1 - y_bottom;

      // Calcolo di s->y (asse verticale dello schermo)
      s->y = x_raw;

      // Applica l'offset di -16 pixel sull'asse verticale
      s->y += 16;

      // Carica codice e colore
      s->code = sprite_data[0];
      uint8_t attributes = sprite_data[2];
      s->color = attributes & 0x0F;

      s->flags = 0;

      // Trasformazione dei flag (questa logica è corretta)
      if (attributes & 0x10)
        s->flags |= SPRITE_FLIP_Y; // diventa FLIP Y nostro
  
      if (attributes & 0x20)
        s->flags |= SPRITE_FLIP_X; // diventa FLIP X nostro
  
      active_sprites++;
    }
  }
}

void mrdo::render_background_strip(short screen_strip_row) {
  const int screen_width = 224;
  const int game_width = 192;
  const int border_width = (screen_width - game_width) / 2;

  short buffer_y_start = (screen_strip_row - 2) * 8;

  for (int y_in_strip = 0; y_in_strip < 8; y_in_strip++) {
    short screen_y = buffer_y_start + y_in_strip;
    uint16_t *ptr_row_start = frame_buffer + (y_in_strip * 224);

    for (int screen_x = border_width; screen_x < screen_width - border_width; screen_x++) {
      int game_coord_x = screen_y;
      int game_coord_y = (game_width - 1) - (screen_x - border_width);

      int world_x = (game_coord_x + scrollx_w) % 256;

      // Applica lo scroll orizzontale
      int scrolled_y = game_coord_y + scrolly_w;

      // Il gioco inizia a disegnare dalla colonna di tile 2.
      // Questo significa che c'è un offset hardware fisso di 16 pixel.
      // Dobbiamo sottrarre questo offset per allineare correttamente il mondo.
      int world_y = (scrolled_y + 32) % 256;

      // Gestione del modulo per numeri negativi in C
      if (world_y < 0)
        world_y += 256;

      int tile_col_x = world_x / 8;
      int tile_row_y = world_y / 8;
      int pixel_col_in_tile_world = world_x % 8;
      int pixel_row_in_tile_world = world_y % 8;

      unsigned short addr = (tile_row_y * 32) + tile_col_x;

      uint8_t attr = memory[addr];
      uint16_t tile_code = memory[addr + 0x400];
      if (attr & 0x80)
        tile_code += 256;

      int tile_x = pixel_row_in_tile_world;
      int tile_y = 7 - pixel_col_in_tile_world;
      uint8_t pixel_index = mrdo_bg_tiles[tile_code][tile_y][tile_x];

      uint8_t color_group_index = attr & 0x3F;
      uint16_t palette_base_offset = color_group_index * 4;
      const uint16_t *colors = &mrdo_master_palette[palette_base_offset];

      uint16_t color_to_draw = colors[pixel_index];

      *(ptr_row_start + screen_x) = color_to_draw;
      *(ptr_row_start + screen_x) = color_to_draw;
    }
  }
}

void mrdo::blit_tile_fg(short row, char col) {
  if ((row < 2) || (row >= 34))
    return;

  // --- Sezione 1: Lettura dei dati ---
  unsigned short addr = tileaddr[row][col];
  uint8_t attr = memory[addr + 0x800];
  uint16_t tile_code = memory[addr + 0xc00];

  if (attr & 0x80)
    tile_code += 256;

  bool is_opaque = (attr & 0x40);
  uint8_t color_group_index = attr & 0x3F;
  uint16_t palette_base_offset = (color_group_index * 4);

  const uint8_t(*tile_data)[8] = mrdo_fg_tiles[tile_code];
  const uint16_t *colors = &mrdo_master_palette[palette_base_offset];

  // Calcola il puntatore all'inizio della prima riga del tile
  unsigned short *row_ptr = frame_buffer + 8 * col; // Assumendo che Y sia gestito dal loop chiamante

  // Itera sulle 8 righe del tile
  for (char r = 0; r < 8; r++) {
    // Itera sugli 8 pixel della riga
    for (char c = 0; c < 8; c++) {
      uint8_t pixel_index = tile_data[r][c];

      if (pixel_index || is_opaque)
        row_ptr[c] = colors[pixel_index];
    }
    // Passa alla riga successiva nel framebuffer
    row_ptr += 224;
  }
}

void mrdo::blit_sprite(short row, unsigned char s_idx) {
  const struct sprite_S *s = &sprite[s_idx];

  short strip_y_start = row * 8;
  short strip_y_end = strip_y_start + 7;

  if ((s->y + 15) < strip_y_start || s->y > strip_y_end)
    return;

  const uint16_t *colors = mrdo_sprite_colormap[s->color];

  for (int y_local = 0; y_local < 8; y_local++) {
    short current_screen_y = strip_y_start + y_local;

    if (current_screen_y >= s->y && current_screen_y < (s->y + 16)) {
      int sprite_y = current_screen_y - s->y;

      // Applica il FLIP_Y se richiesto dal gioco
      if (s->flags & SPRITE_FLIP_Y)
        sprite_y = 15 - sprite_y;

      sprite_y = 15 - sprite_y;

      uint16_t *ptr = frame_buffer + (y_local * 224) + s->x;

      for (int x = 0; x < 16; x++) {
        if ((s->x + x) >= 0 && (s->x + x) < 224) {
          int sprite_x = x;
          if (s->flags & SPRITE_FLIP_X)
            sprite_x = 15 - sprite_x;

          uint8_t pixel_index = mrdo_sprites[s->code][sprite_y][sprite_x];

          if (pixel_index != 0)
            *ptr = colors[pixel_index];
        }
        ptr++;
      }
    }
  }
}

void mrdo::render_row(short row) {
  // 1. CONTROLLO DI SICUREZZA: Esegui il rendering solo per le righe visibili.
  //    Le righe di tile visibili vanno dalla 2 alla 33 (incluse).
  //    Questo previene qualsiasi accesso fuori dai limiti della memoria.
  if (row < 2 || row >= 34)
    return; // Non fare nulla per le righe non visibili.

  // Ipotizziamo che il bit 1 (0x02) del registro 0x9800 abiliti il background.
  // Prova con 0x02, se non funziona prova con 0x04.
  bool background_enabled = ((flipscreen_w & 0x02) == 0);

  // Disegna il background SCROLLATO solo se è abilitato e se la riga è visibile.
  if (background_enabled && (row >= 2 && row < 34))
    render_background_strip(row);

  for (char col = 2; col < 26; col++) {
    blit_tile_fg(row, col);
  }

  // render sprites
  for (unsigned char s = 0; s < active_sprites; s++) {
    blit_sprite(row, s);
  }
}

const unsigned short *mrdo::logo(void) {
  return mrdo_logo;
}

#ifdef LED_PIN
void mrdo::gameLeds(CRGB *leds) {
  // mrdo: slow yellow on green "knight rider" ...
  static char sub_cnt = 0;
  if (sub_cnt++ == 32) {
    sub_cnt = 0;

    // and also do the marquee LEDs
    static char led = 0;

    char il = (led < NUM_LEDS) ? led : ((2 * NUM_LEDS - 2) - led);
    for (char c = 0; c < NUM_LEDS; c++) {
      if (c == il)
        leds[c] = LED_YELLOW;
      else
        leds[c] = LED_GREEN;
    }
    led = (led + 1) % (2 * NUM_LEDS - 2);
  }
}

void mrdo::menuLeds(CRGB *leds) {
  memcpy(leds, menu_leds, NUM_LEDS * sizeof(CRGB));
}
#endif