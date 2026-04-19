#ifndef GYRUSS_H
#define GYRUSS_H

#include "../machineBase.h"
#include "../../cpus/m6809/m6809.h"
#include "gyruss_rom_main.h"
#include "gyruss_rom_sub.h"
#include "gyruss_rom_audio.h"
#include "gyruss_tilemap.h"
#include "gyruss_spritemap.h"
#include "gyruss_palette.h"
#include "gyruss_dipswitches.h"
#include "gyruss_logo.h"


// Gyruss memory layout offsets in shared memory[] buffer
// memory[0x0000..0x03FF] = Color RAM (1KB)
// memory[0x0400..0x07FF] = Video RAM (1KB)
// memory[0x0800..0x17FF] = Main Z80 Work RAM (4KB)
// memory[0x1800..0x1FFF] = Audio Z80 RAM (1KB, at offset 0x1800 from AY perspective mapped at 0x6000)
#define GYR_CRAM_OFF   0x0000
#define GYR_VRAM_OFF   0x0400
#define GYR_WRAM_OFF   0x0800
#define GYR_ARAM_OFF   0x1800

class gyruss : public machineBase
{
public:
    gyruss() { }
    ~gyruss() { }

    void init(Input *input, unsigned short *framebuffer, sprite_S *spritebuffer, unsigned char *memorybuffer) override;
    void start() override;
    void reset() override;

    signed char machineType() override { return MCH_GYRUSS; }
    signed char useVideoHalfRate() override { return 1; }
    signed char videoFlipY() override { return 0; }
    signed char videoFlipX() override { return 1; }
    
    unsigned char rdZ80(unsigned short Addr) override;
    void wrZ80(unsigned short Addr, unsigned char Value) override;
    void outZ80(unsigned short Port, unsigned char Value) override;
    unsigned char opZ80(unsigned short Addr) override;
    unsigned char inZ80(unsigned short Port) override;

    void run_frame(void) override;
    void prepare_frame(void) override;
    void render_row(short row) override;
    const unsigned short *logo(void) override;

#ifdef LED_PIN
    void menuLeds(CRGB *leds) override;
    void gameLeds(CRGB *leds) override;
#endif

    // M6809 sub-CPU memory access (called from C callbacks)
    uint8_t sub_read(uint16_t addr);
    void sub_write(uint16_t addr, uint8_t val);
    uint8_t sub_read_opcode(uint16_t addr);

    // Audio Z80 dual-core support
    void start_audio_task();
    void stop_audio_task();
    void run_audio_batch(int steps);
    inline bool is_audio_cpu();
    volatile uint8_t audio_running;

protected:
    void blit_tile(short row, char col);
    void blit_sprite(short row, unsigned char s_idx);

private:
    void preapre_sprites(unsigned char *sr);
    // M6809 sub-CPU
    m6809_state sub_cpu;
    unsigned char sub_ram[0x800];     // Sub-CPU local RAM (0x4000-0x47FF)
    unsigned char shared_ram[0x800];  // Shared RAM (Z80: 0xA000-0xA7FF, M6809: 0x6000-0x67FF)
    unsigned char multiplexPart1[0xff];

    // Audio
    volatile unsigned char sound_latch;
    unsigned char sound_latch_pending;
    volatile unsigned char sound_irq_pending;   // latched IRQ for audio Z80
    unsigned char ay_address[5];                // 5 AY address latches
    volatile unsigned long audio_cycle_approx;  // approximate audio Z80 cycle counter

    // Audio dual-core
    TaskHandle_t audio_task_handle = NULL;
    char emu_core_id;
    
    // Video
    unsigned char flip_screen;
    unsigned char scanline_counter;
    unsigned char multiplexUsed;
    unsigned char multiplexUsedPart1 = 0;
    
   
#ifdef LED_PIN
    const CRGB menu_leds[7] = { LED_BLUE, LED_CYAN, LED_WHITE, LED_CYAN, LED_WHITE, LED_CYAN, LED_BLUE };
#endif
};

// Global pointer for M6809 callbacks (only one gyruss instance)
extern gyruss *g_gyruss_instance;

#endif
