#ifndef SPACEINVADERS_H
#define SPACEINVADERS_H

#include "spaceinvaders_logo.h"
#include "spaceinvaders_rom.h"
#include "spaceinvaders_dipswitches.h"
#include "spaceinvaders_samples.h"
#include "../machineBase.h"

// Space Invaders (Midway, 1978)
// CPU: Intel 8080 @ 2MHz (runs on Z80 emulator - 8080 is a subset of Z80)
// Screen: 256x224 bitmap, rotated 90 CCW → player sees 224 wide x 256 tall
// No tile/sprite hardware - pure framebuffer at 0x2400-0x3FFF
//
// Memory map:
//   0x0000-0x1FFF: ROM (8KB = 4 x 2KB)
//   0x2000-0x23FF: RAM (1KB work RAM)
//   0x2400-0x3FFF: Video RAM (7KB, 1bpp framebuffer)
//   0x4000-0x5FFF: RAM mirror
//
// I/O Ports:
//   IN  1: coin, P1 start, P1 fire/left/right
//   IN  2: DIP switches, P2 controls, tilt
//   IN  3: shift register result
//   OUT 2: shift amount
//   OUT 3: sound effects 1
//   OUT 4: shift data
//   OUT 5: sound effects 2
//   OUT 6: watchdog

// RAM mapping into shared memory[] buffer:
// memory[0x0000..0x1FFF] = addresses 0x2000-0x3FFF (RAM + VRAM)
// VRAM starts at memory[0x0400] (address 0x2400)
#define RAM_OFFSET   0x2000
#define VRAM_OFFSET  0x0400

// Color overlay colors (RGB565 byte-swapped for ESP32 SPI TFT)
// White:  0xFFFF
// Green:  0xE007  (0x07E0 byte-swapped)
// Red:    0x00F8  (0xF800 byte-swapped)
#define COL_WHITE  0xFFFF
#define COL_GREEN  0xE007
#define COL_RED    0x00F8

class spaceinvaders : public machineBase
{
public:
    spaceinvaders() { }
    ~spaceinvaders() { }

    signed char machineType() override { return MCH_SPACE; }
    unsigned char rdZ80(unsigned short Addr) override;
    void wrZ80(unsigned short Addr, unsigned char Value) override;
    void outZ80(unsigned short Port, unsigned char Value) override;
    unsigned char opZ80(unsigned short Addr) override;
    unsigned char inZ80(unsigned short Port) override;

    void run_frame(void) override;
    void prepare_frame(void) override;
    void render_row(short row) override;
    const unsigned short *logo(void) override;
    void reset() override;

#ifdef LED_PIN
    void menuLeds(CRGB *leds) override;
    void gameLeds(CRGB *leds) override;
#endif

protected:
    // Space Invaders uses bitmap framebuffer, no tiles/sprites
    void blit_tile(short row, char col) override { }
    void blit_sprite(short row, unsigned char s) override { }

private:
    // Hardware shift register
    uint16_t shift_data;
    uint8_t shift_amount;
    uint8_t last_coin = 0;  // edge detection for coin insert sound

    // Color overlay lookup (RGB565 byte-swapped)
    unsigned short get_pixel_color(int y);

#ifdef LED_PIN
    const CRGB menu_leds[7] = { LED_GREEN, LED_WHITE, LED_GREEN, LED_WHITE, LED_GREEN, LED_WHITE, LED_GREEN };
#endif
};

#endif