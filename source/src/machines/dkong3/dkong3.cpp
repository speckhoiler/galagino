#include "dkong3.h"

// ---------------------------------------------------------------------------
// NES APU synthesis tables
// ---------------------------------------------------------------------------

// Length counter lookup table (indexed by bits 7-3 of $4003/$4007/$400B/$400F)
// Values are in 120 Hz ticks (each tick = 1/120 s)
static const uint8_t DK3_LENGTH_TABLE[32] = {
    10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
    12,  16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26,  16, 28, 32, 30
};

static const uint8_t DK3_DUTY[4][8] = {
    {0,1,0,0,0,0,0,0},
    {0,1,1,0,0,0,0,0},
    {0,1,1,1,1,0,0,0},
    {1,0,0,1,1,1,1,1},
};
static const uint8_t DK3_TRI_SEQ[32] = {
    15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,
     0, 1, 2, 3, 4, 5,6,7,8,9,10,11,12,13,14,15
};
static const uint16_t DK3_NOISE_PERIOD[16] = {
    4,8,16,32,64,96,128,160,202,254,380,508,762,1016,2034,4068
};

// Advance APU state by `ticks` CPU cycles and return one mixed sample (range ~±512)
static int dk3_apu_sample(dk3_apu_t *a, int ticks) {
    int val = 0;

    // Pulse channels — symmetric output (±vol) so AC amplitude uses full range
    for (int ch = 0; ch < 2; ch++) {
        if (!a->p_en[ch] || !a->p_len[ch]) continue;
        uint16_t cur_per = a->p_timer[ch] & 0x7FF;
        if (cur_per < 8) continue;  // period too small: hardware mutes
        // Sweep overflow mute: if adding and target > $7FF, channel is silenced
        uint8_t sw_shift = a->p_sweep[ch] & 7;
        if (sw_shift && !((a->p_sweep[ch] >> 3) & 1)) {
            if (cur_per + (cur_per >> sw_shift) > 0x7FF) continue;
        }
        // bit 4 (c): 1=constant volume (bits 3-0), 0=envelope volume
        uint8_t vol = (a->p_ctrl[ch] & 0x10) ? (a->p_ctrl[ch] & 0x0F) : a->p_env_vol[ch];
        if (!vol) continue;
        int period = (int)(cur_per + 1) * 2;
        if (period < 4) continue;
        a->p_cnt[ch] -= ticks;
        while (a->p_cnt[ch] <= 0) {
            a->p_cnt[ch] += period;
            a->p_seq[ch] = (a->p_seq[ch] + 1) & 7;
        }
        uint8_t duty = (a->p_ctrl[ch] >> 6) & 3;
        val += DK3_DUTY[duty][a->p_seq[ch]] ? (int)vol * 6 : -(int)vol * 6;
    }

    // Triangle — clocked at CPU rate (NOT APU/2 like pulse), so period = T+1 (no ×2)
    if (a->t_en && a->t_len && a->t_lin) {
        int period = (int)(a->t_timer & 0x7FF) + 1;
        if (period >= 2) {
            a->t_cnt -= ticks;
            while (a->t_cnt <= 0) {
                a->t_cnt += period;
                a->t_seq = (a->t_seq + 1) & 31;
            }
            val += ((int)DK3_TRI_SEQ[a->t_seq] - 7) * 6;
        }
    }

    // Noise — symmetric output; silenced if length counter = 0
    if (a->n_en && a->n_len) {
        uint8_t nvol = (a->n_ctrl & 0x10) ? (a->n_ctrl & 0x0F) : a->n_env_vol;
        if (nvol) {
            int period = DK3_NOISE_PERIOD[a->n_period_idx & 0x0F];
            a->n_cnt -= ticks;
            while (a->n_cnt <= 0) {
                a->n_cnt += period;
                uint16_t bit = (a->n_lfsr & 1) ^
                               ((a->n_mode ? (a->n_lfsr >> 6) : (a->n_lfsr >> 1)) & 1);
                a->n_lfsr = (a->n_lfsr >> 1) | (bit << 14);
            }
            val += (a->n_lfsr & 1) ? (int)nvol * 3 : -(int)nvol * 3;
        }
    }

    return val;
}

// ---------------------------------------------------------------------------
// 6502 memory map for sound CPU #0  (ROM: dkong3_rom_sound_a)
// ---------------------------------------------------------------------------
IRAM_ATTR uint8_t dkong3::snd0_read(m6502_t *cpu, uint16_t addr) {
    if (addr >= 0x8000) return dkong3_rom_sound_a[addr & 0x1FFF]; // fast path: ~85% of reads
    dkong3 *s = (dkong3*)cpu->user;
    if (addr < 0x0800) return s->snd_ram[0][addr & 0x07FF];
    if (addr == 0x4016) return s->sound_latch[0];  // latch1 ($7C00)
    if (addr == 0x4017) return s->sound_latch[1];  // latch2 ($7C80)
    if (addr == 0x4015) {
        dk3_apu_t &a = s->apu[0];
        return (a.p_len[0] ? 0x01 : 0) | (a.p_len[1] ? 0x02 : 0) |
               (a.t_len    ? 0x04 : 0) | (a.n_len    ? 0x08 : 0);
    }
    return 0;
}

IRAM_ATTR void dkong3::snd0_write(m6502_t *cpu, uint16_t addr, uint8_t val) {
    dkong3 *s = (dkong3*)cpu->user;
    if (addr < 0x0800) { s->snd_ram[0][addr] = val; return; }
    dk3_apu_t &a = s->apu[0];
    switch (addr) {
        case 0x4000: a.p_ctrl[0] = val; break;
        case 0x4001: a.p_sweep[0] = val; a.p_sweep_rst[0] = true; break;
        case 0x4002: a.p_timer[0] = (a.p_timer[0] & 0x700) | val; break;
        case 0x4003: a.p_timer[0] = (a.p_timer[0] & 0x0FF) | ((val & 7) << 8);
                     a.p_seq[0] = 0; a.p_cnt[0] = 0; a.p_env_rst[0] = true;
                     if (a.p_en[0]) a.p_len[0] = DK3_LENGTH_TABLE[val >> 3]; break;
        case 0x4004: a.p_ctrl[1] = val; break;
        case 0x4005: a.p_sweep[1] = val; a.p_sweep_rst[1] = true; break;
        case 0x4006: a.p_timer[1] = (a.p_timer[1] & 0x700) | val; break;
        case 0x4007: a.p_timer[1] = (a.p_timer[1] & 0x0FF) | ((val & 7) << 8);
                     a.p_seq[1] = 0; a.p_cnt[1] = 0; a.p_env_rst[1] = true;
                     if (a.p_en[1]) a.p_len[1] = DK3_LENGTH_TABLE[val >> 3]; break;
        case 0x4008: a.t_lin_ctrl = val; break;
        case 0x400A: a.t_timer = (a.t_timer & 0x700) | val; break;
        case 0x400B: a.t_timer = (a.t_timer & 0x0FF) | ((val & 7) << 8);
                     a.t_cnt = 0; a.t_lin_reload = true;
                     if (a.t_en) a.t_len = DK3_LENGTH_TABLE[val >> 3]; break;
        case 0x400C: a.n_ctrl = val; break;
        case 0x400E: a.n_period_idx = val & 0x0F; a.n_mode = (val >> 7) & 1; break;
        case 0x400F: a.n_env_rst = true;
                     if (a.n_en) a.n_len = DK3_LENGTH_TABLE[val >> 3]; break;
        case 0x4015:
            if (!((val >> 0) & 1)) a.p_len[0] = 0;
            if (!((val >> 1) & 1)) a.p_len[1] = 0;
            if (!((val >> 2) & 1)) a.t_len = 0;
            if (!((val >> 3) & 1)) a.n_len = 0;
            a.p_en[0] = (val >> 0) & 1;
            a.p_en[1] = (val >> 1) & 1;
            a.t_en    = (val >> 2) & 1;
            a.n_en    = (val >> 3) & 1;
            break;
        default: break;
    }
}

// ---------------------------------------------------------------------------
// 6502 memory map for sound CPU #1  (ROM: dkong3_rom_sound_b)
// ---------------------------------------------------------------------------
IRAM_ATTR uint8_t dkong3::snd1_read(m6502_t *cpu, uint16_t addr) {
    if (addr >= 0x8000) return dkong3_rom_sound_b[addr & 0x1FFF]; // fast path
    dkong3 *s = (dkong3*)cpu->user;
    if (addr < 0x0800) return s->snd_ram[1][addr & 0x07FF];
    if (addr == 0x4016) return s->sound_latch[2];  // latch3 ($7D00)
    if (addr == 0x4017) return 0xFF;               // nopr (not connected)
    if (addr == 0x4015) {
        dk3_apu_t &a = s->apu[1];
        return (a.p_len[0] ? 0x01 : 0) | (a.p_len[1] ? 0x02 : 0) |
               (a.t_len    ? 0x04 : 0) | (a.n_len    ? 0x08 : 0);
    }
    return 0;
}

IRAM_ATTR void dkong3::snd1_write(m6502_t *cpu, uint16_t addr, uint8_t val) {
    dkong3 *s = (dkong3*)cpu->user;
    if (addr < 0x0800) { s->snd_ram[1][addr] = val; return; }
    dk3_apu_t &a = s->apu[1];
    switch (addr) {
        case 0x4000: a.p_ctrl[0] = val; break;
        case 0x4001: a.p_sweep[0] = val; a.p_sweep_rst[0] = true; break;
        case 0x4002: a.p_timer[0] = (a.p_timer[0] & 0x700) | val; break;
        case 0x4003: a.p_timer[0] = (a.p_timer[0] & 0x0FF) | ((val & 7) << 8);
                     a.p_seq[0] = 0; a.p_cnt[0] = 0; a.p_env_rst[0] = true;
                     if (a.p_en[0]) a.p_len[0] = DK3_LENGTH_TABLE[val >> 3]; break;
        case 0x4004: a.p_ctrl[1] = val; break;
        case 0x4005: a.p_sweep[1] = val; a.p_sweep_rst[1] = true; break;
        case 0x4006: a.p_timer[1] = (a.p_timer[1] & 0x700) | val; break;
        case 0x4007: a.p_timer[1] = (a.p_timer[1] & 0x0FF) | ((val & 7) << 8);
                     a.p_seq[1] = 0; a.p_cnt[1] = 0; a.p_env_rst[1] = true;
                     if (a.p_en[1]) a.p_len[1] = DK3_LENGTH_TABLE[val >> 3]; break;
        case 0x4008: a.t_lin_ctrl = val; break;
        case 0x400A: a.t_timer = (a.t_timer & 0x700) | val; break;
        case 0x400B: a.t_timer = (a.t_timer & 0x0FF) | ((val & 7) << 8);
                     a.t_cnt = 0; a.t_lin_reload = true;
                     if (a.t_en) a.t_len = DK3_LENGTH_TABLE[val >> 3]; break;
        case 0x400C: a.n_ctrl = val; break;
        case 0x400E: a.n_period_idx = val & 0x0F; a.n_mode = (val >> 7) & 1; break;
        case 0x400F: a.n_env_rst = true;
                     if (a.n_en) a.n_len = DK3_LENGTH_TABLE[val >> 3]; break;
        case 0x4015:
            if (!((val >> 0) & 1)) a.p_len[0] = 0;
            if (!((val >> 1) & 1)) a.p_len[1] = 0;
            if (!((val >> 2) & 1)) a.t_len = 0;
            if (!((val >> 3) & 1)) a.n_len = 0;
            a.p_en[0] = (val >> 0) & 1;
            a.p_en[1] = (val >> 1) & 1;
            a.t_en    = (val >> 2) & 1;
            a.n_en    = (val >> 3) & 1;
            break;
        default: break;
    }
}

// ---------------------------------------------------------------------------
// Machine lifecycle
// ---------------------------------------------------------------------------
void dkong3::start() {
    general_ram = memory + 0x0000;
    sprite_ram  = memory + 0x1000;
    video_ram   = memory + 0x1400;
    memset(memory, 0, 0x1800);

    memset(snd_ram,    0, sizeof(snd_ram));
    memset(dk3_samples,0, sizeof(dk3_samples));
    sound_latch[0] = sound_latch[1] = sound_latch[2] = 0;
    dk3_rptr = dk3_wptr = 0;
    coin_latch = 0;
    coin_prev  = false;

    // Init APU states
    memset(&apu[0], 0, sizeof(dk3_apu_t)); apu[0].n_lfsr = 1;
    memset(&apu[1], 0, sizeof(dk3_apu_t)); apu[1].n_lfsr = 1;

    // Sound CPU #0
    memset(&snd_cpu[0], 0, sizeof(m6502_t));
    snd_cpu[0].read  = snd0_read;
    snd_cpu[0].write = snd0_write;
    snd_cpu[0].user  = this;
    m6502_reset(&snd_cpu[0]);

    // Sound CPU #1
    memset(&snd_cpu[1], 0, sizeof(m6502_t));
    snd_cpu[1].read  = snd1_read;
    snd_cpu[1].write = snd1_write;
    snd_cpu[1].user  = this;
    m6502_reset(&snd_cpu[1]);
}

void dkong3::reset() {
    machineBase::reset();
    general_ram = memory + 0x0000;
    sprite_ram  = memory + 0x1000;
    video_ram   = memory + 0x1400;

    memset(snd_ram,     0, sizeof(snd_ram));
    memset(dk3_samples, 0, sizeof(dk3_samples));
    memset(&apu[0], 0, sizeof(dk3_apu_t)); apu[0].n_lfsr = 1;
    memset(&apu[1], 0, sizeof(dk3_apu_t)); apu[1].n_lfsr = 1;
    sound_latch[0] = sound_latch[1] = sound_latch[2] = 0;
    dk3_rptr = dk3_wptr = 0;
    coin_latch = 0;
    coin_prev  = false;

    if (snd_cpu[0].read) m6502_reset(&snd_cpu[0]);
    if (snd_cpu[1].read) m6502_reset(&snd_cpu[1]);
}

// ---------------------------------------------------------------------------
// Z80 main CPU memory map
// ---------------------------------------------------------------------------
unsigned char dkong3::opZ80(unsigned short Addr) {
    return dkong3_rom_cpu[Addr];
}

unsigned char dkong3::rdZ80(unsigned short Addr) {
    if (Addr < 0x6000)                         return dkong3_rom_cpu[Addr];
    if (Addr >= 0x8000 && Addr <= 0x9FFF)      return dkong3_rom_cpu[Addr];
    if (Addr >= 0x6000 && Addr <= 0x6FFF)      return general_ram[Addr - 0x6000];
    if (Addr >= 0x7000 && Addr <= 0x73FF)      return sprite_ram[Addr - 0x7000];
    if (Addr >= 0x7400 && Addr <= 0x77FF)      return video_ram[Addr - 0x7400];

    if (Addr >= 0x7C00 && Addr <= 0x7DFF) {
        unsigned char keymask = input->buttons_get();
        unsigned char retval  = 0x00;
        switch (Addr) {
          case 0x7C00:
            if (keymask & BUTTON_RIGHT) retval |= 0x01;
            if (keymask & BUTTON_LEFT)  retval |= 0x02;
            if (keymask & BUTTON_UP)    retval |= 0x04;
            if (keymask & BUTTON_DOWN)  retval |= 0x08;
            if (keymask & BUTTON_FIRE)  retval |= 0x10;
            if (keymask & BUTTON_START) retval |= 0x20;
            return retval;
          case 0x7C80: {
            bool coin_now = (keymask & BUTTON_COIN) != 0;
            if (coin_now && !coin_prev) coin_latch = 20; // rising edge only
            coin_prev = coin_now;
            retval = (coin_latch > 0) ? 0x20 : 0;
            if (coin_latch) coin_latch--;
            return retval;
          }
          case 0x7D00:
            game_started = 1;
            return DK3_DSW2;
          case 0x7D80:
            return DK3_DSW1;
        }
    }
    return 0xFF;
}

void dkong3::wrZ80(unsigned short Addr, unsigned char Value) {
    if (Addr >= 0x6000 && Addr <= 0x6FFF) { general_ram[Addr - 0x6000] = Value; return; }
    if (Addr >= 0x7000 && Addr <= 0x73FF) { sprite_ram [Addr - 0x7000] = Value; return; }
    if (Addr >= 0x7400 && Addr <= 0x77FF) { video_ram  [Addr - 0x7400] = Value; return; }

    if (Addr == 0x7C00) { sound_latch[0] = Value; return; }  // latch1 → CPU A $4016
    if (Addr == 0x7C80) { sound_latch[1] = Value; return; }  // latch2 → CPU A $4017
    if (Addr == 0x7D00) { sound_latch[2] = Value; return; }  // latch3 → CPU B $4016
    if (Addr == 0x7D80) { return; }

    if (Addr >= 0x7E80 && Addr <= 0x7E87) {
        switch (Addr) {
          case 0x7E81: gfx_bank     = (~Value) & 0x01; break;
          case 0x7E82: flip_screen  = Value & 1;        break;
          case 0x7E84: irq_enable[0] = Value & 1; break;
          case 0x7E85:
            if (Value & 1) memcpy(sprite_ram, general_ram + 0x0900, 512);
            break;
          case 0x7E86:
            palette_bank = Value & 1 ? palette_bank | 1 : palette_bank & ~1; break;
          case 0x7E87:
            palette_bank = Value & 1 ? palette_bank | 2 : palette_bank & ~2; break;
        }
    }
}

// ---------------------------------------------------------------------------
// Frame execution
// ---------------------------------------------------------------------------
void dkong3::run_frame(void) {
    // Main Z80 (~3 MHz → 1666 × 4 steps)
    for (int i = 0; i < 1666; i++) {
        StepZ80(cpu); StepZ80(cpu); StepZ80(cpu); StepZ80(cpu);
    }
    if (irq_enable[0])
        IntZ80(cpu, INT_NMI);

    // NES frame sequencer: 4 steps per frame (240 Hz envelope, 120 Hz length/sweep)
    // Steps 0-3: envelope + linear counter every step; length counter at steps 1 and 3
    for (int step = 0; step < 4; step++) {
        bool clock_length = (step == 1 || step == 3);
        for (int ai = 0; ai < 2; ai++) {
            dk3_apu_t &a = apu[ai];

            // Envelope clock (240 Hz)
            for (int ch = 0; ch < 2; ch++) {
                if (a.p_env_rst[ch]) {
                    a.p_env_rst[ch] = false;
                    a.p_env_vol[ch] = 15;
                    a.p_env_cnt[ch] = a.p_ctrl[ch] & 0x0F;
                } else if (!(a.p_ctrl[ch] & 0x10)) {
                    uint8_t per = a.p_ctrl[ch] & 0x0F;
                    if (a.p_env_cnt[ch] > 0) {
                        a.p_env_cnt[ch]--;
                    } else {
                        a.p_env_cnt[ch] = per;
                        if (a.p_env_vol[ch] > 0)
                            a.p_env_vol[ch]--;
                        else if (a.p_ctrl[ch] & 0x20)
                            a.p_env_vol[ch] = 15;
                    }
                }
            }
            if (a.n_env_rst) {
                a.n_env_rst = false;
                a.n_env_vol = 15;
                a.n_env_cnt = a.n_ctrl & 0x0F;
            } else if (!(a.n_ctrl & 0x10)) {
                uint8_t per = a.n_ctrl & 0x0F;
                if (a.n_env_cnt > 0) {
                    a.n_env_cnt--;
                } else {
                    a.n_env_cnt = per;
                    if (a.n_env_vol > 0)
                        a.n_env_vol--;
                    else if (a.n_ctrl & 0x20)
                        a.n_env_vol = 15;
                }
            }

            // Triangle linear counter (240 Hz)
            if (a.t_lin_reload) {
                a.t_lin = a.t_lin_ctrl & 0x7F;
            } else if (a.t_lin > 0) {
                a.t_lin--;
            }
            if (!(a.t_lin_ctrl & 0x80))
                a.t_lin_reload = false;

            // Length counters + sweep (120 Hz — only at steps 1 and 3)
            if (clock_length) {
                for (int ch = 0; ch < 2; ch++) {
                    if (!(a.p_ctrl[ch] & 0x20) && a.p_len[ch] > 0) a.p_len[ch]--;

                    // Sweep unit
                    uint8_t sw = a.p_sweep[ch];
                    uint8_t sw_per = (sw >> 4) & 7;
                    if (a.p_sweep_rst[ch]) {
                        a.p_sweep_cnt[ch] = sw_per;
                        a.p_sweep_rst[ch] = false;
                    } else if (a.p_sweep_cnt[ch] > 0) {
                        a.p_sweep_cnt[ch]--;
                    } else {
                        a.p_sweep_cnt[ch] = sw_per;
                        bool sw_en    = (sw >> 7) & 1;
                        bool sw_neg   = (sw >> 3) & 1;
                        uint8_t sw_sh = sw & 7;
                        uint16_t cur  = a.p_timer[ch] & 0x7FF;
                        if (sw_en && sw_sh > 0 && cur >= 8) {
                            uint16_t delta  = cur >> sw_sh;
                            // Pulse 1 negate uses ones' complement (-delta-1); Pulse 2 uses twos' complement (-delta)
                            uint16_t target = sw_neg ? cur - delta - (ch == 0 ? 1 : 0)
                                                     : cur + delta;
                            if (target <= 0x7FF)
                                a.p_timer[ch] = target;
                        }
                    }
                }
                if (!(a.t_lin_ctrl & 0x80) && a.t_len > 0) a.t_len--;
                if (!(a.n_ctrl & 0x20) && a.n_len > 0) a.n_len--;
            }
        }
    }

    // Sound CPUs: NMI fires once per frame from vblank (same as Z80 NMI).
    // Run 12000 cycles — enough for the NMI handler + music update (~1000-4000 cycles);
    // the remaining idle polling loop does no useful work and costs only ESP32 time.
    snd_cpu[0].nmi = 1;
    snd_cpu[1].nmi = 1;
    m6502_exec(&snd_cpu[0], 12000);
    m6502_exec(&snd_cpu[1], 12000);

    // Synthesize 400 samples (24000 Hz / 60 fps)
    // CPU_TICKS = 1789773 / 24000 = 74.6 → 75 cycles/sample matches real RP2A03 pitch
    static const int SAMPLES_PER_FRAME = 400;
    static const int CPU_TICKS = 75;
    static int dk3_lpf = 0;  // IIR low-pass state (persists across frames)

    for (int i = 0; i < SAMPLES_PER_FRAME; i++) {
        uint16_t next = (dk3_wptr + 1) & (DK3_SAMPLES - 1);
        if (next == dk3_rptr) break;

        int val = dk3_apu_sample(&apu[0], CPU_TICKS) +
                  dk3_apu_sample(&apu[1], CPU_TICKS);
        // First-order IIR low-pass (α=3/4): reduces aliasing from high-freq harmonics
        dk3_lpf = (val * 3 + dk3_lpf) >> 2;
        val = dk3_lpf;
        if (val >  500) val =  500;
        if (val < -500) val = -500;
        dk3_samples[dk3_wptr] = (short)val;
        dk3_wptr = next;
    }
}

// ---------------------------------------------------------------------------
// Video
// ---------------------------------------------------------------------------
void dkong3::prepare_frame(void) {
    active_sprites = 0;

    for (int idx = 0; idx < 92 && active_sprites < 92; idx++) {
        unsigned char *b = sprite_ram + 4 * idx;
        if (b[0] == 0) continue;

        struct sprite_S spr;
        spr.x = 224 + 8 - b[0];
        spr.y = 224 - 24 - b[3];

        unsigned char attr = b[2];
        spr.code  = (b[1] & 0x7F) | ((attr & 0x40) << 1);
        spr.color = attr & 0x3F;
        spr.flags = ((b[1] & 0x80) ? 1 : 0) | ((attr & 0x80) ? 2 : 0);

        if (flip_screen) {
            spr.x   = 224 - 16 - spr.x;
            spr.y   = 224 - 16 - spr.y;
            spr.flags ^= 1;
        }
        if (spr.y > -16 && spr.y < 288 && spr.x > -16 && spr.x < 224)
            sprite[active_sprites++] = spr;
    }
}

void dkong3::blit_tile(short row, char col) {
    unsigned short addr = tileaddr[row][col];
    if (row < 2 || row >= 34) return;
    if (video_ram[addr] == 0x10) return;

    unsigned char  base_code  = video_ram[addr];
    unsigned short final_code = base_code + (256 * gfx_bank);
    const unsigned short *tile = dkong3_tilemap[final_code];

    unsigned short color_idx = (addr % 32) + 32 * ((addr / 32) / 4);
    unsigned char  base_pal  = pgm_read_byte(&color_codes_prom[color_idx]) & 0x0F;
    unsigned short final_pal = base_pal + (palette_bank * 16);
    const unsigned short *colors = dkong3_colormap_sprite[final_pal];

    unsigned short *ptr = frame_buffer + 8 * col;
    for (char r = 0; r < 8; r++, ptr += (224 - 8)) {
        unsigned short pix = *tile++;
        for (char c = 0; c < 8; c++, pix >>= 2) {
            *ptr++ = colors[pix & 3];
        }
    }
}

void dkong3::blit_sprite(short row, unsigned char s) {
    const unsigned long *spr    = dkong3_sprites[sprite[s].flags & 3][sprite[s].code];
    const unsigned short *colors = dkong3_colormap_sprite[sprite[s].color];

    unsigned long mask = 0xFFFFFFFF;
    if (sprite[s].x < 0)        mask <<= (-sprite[s].x * 2);
    if (sprite[s].x > 224 - 16) mask >>= ((sprite[s].x - (224 - 16)) * 2);

    short y_off = sprite[s].y - 8 * row;
    unsigned char lines = 8;
    unsigned short startline = 0;

    if (y_off < -8) lines = 16 + y_off;
    if (y_off > 0)  { startline = y_off; lines = 8 - y_off; }
    if (y_off < 0)  spr += (-y_off);

    unsigned short *ptr = frame_buffer + sprite[s].x + 224 * startline;
    for (char r = 0; r < lines; r++, ptr += (224 - 16)) {
        unsigned long pix = *spr++ & mask;
        for (char c = 0; c < 16; c++, pix >>= 2) {
            if (pix & 3) *ptr = colors[pix & 3];
            ptr++;
        }
    }
}

void dkong3::render_row(short row) {
    for (char col = 0; col < 28; col++)
        blit_tile(row, col);
    for (unsigned char s = 0; s < active_sprites; s++) {
        if (sprite[s].y < 8 * (row + 1) && (sprite[s].y + 16) > 8 * row)
            blit_sprite(row, s);
    }
}

const unsigned short *dkong3::logo(void) {
    return dkong3_logo;
}
