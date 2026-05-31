#ifndef AUDIO_H
#define AUDIO_H

#include <driver/i2s.h>
#include "../machines/machineBase.h"
#include "../machines/dkong/dkong.h"
#include "../machines/galaga/galaga.h"
#include "../machines/spaceinvaders/spaceinvaders.h"
#include "../config.h"

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 4)
// See https://github.com/espressif/arduino-esp32/issues/8467
#define WORKAROUND_I2S_APLL_PROBLEM
// as a workaround we run dkong audio also at 24khz (instead of 11765)
// and fill the buffer at half the rate resulting in 12khz effective
// sample rate
#endif

// disctrete notes
#define A5_3      880.00
#define C6_4      1046.50
#define F5_5      739.989   
#define G5_6      783.991
#define E6_7      1318.51
#define B6_8      1975.53 //??
#define XX_B      2100.00 //??
#define D6_E      1174.56
#define B5_F      987.767

#define NUM_AY_CHIPS 5
#define NUM_SN_CHIPS 3

class Audio {
public:
  void init();
  void start(machineBase *machineBase);
  void transmit();
  void volumeUpDown(bool up, bool down);

private:
  void namco_render_buffer(void);
  void ay_render_buffer(void);
  void sn76489_render_buffer(void);
  void i8048_render_buffer(void);
  void valueToBuffer(int index, short value);
  void bagman_render_buffer(void);
  void galaxian_render_buffer(void);
  void spaceinvaders_render_buffer(void);
  void generateSinusWave(int32_t amplitude, short* buffer, uint16_t length);
 
  machineBase *currentMachine;
  signed char machineType;

#ifdef SND_DIFF
  unsigned short snd_buffer[128]; // buffer space for two channels
#else
  unsigned short snd_buffer[64];  // buffer space for a single channel
#endif

  short volumeSetting = 3;
  bool volumeUpLast;
  bool volumeDownLast;

  // Namco
  unsigned long snd_cnt[3] = {0, 0, 0};
  unsigned long snd_freq[3];
  const signed char *snd_wave[3];
  unsigned char snd_volume[3];

  // AY 8910
  char AY; 
  char AY_INC;
  char AY_VOL;

  // up to five AY's
  int ay_volume[NUM_AY_CHIPS][3];
  int ay_enable[NUM_AY_CHIPS][3];
  int audio_toggle[NUM_AY_CHIPS][3];
  int audio_cnt[NUM_AY_CHIPS][4];
  int ay_period[NUM_AY_CHIPS][4];
  unsigned long ay_noise_rng[NUM_AY_CHIPS];

  // AY Envelope
  char ay_envelope[NUM_AY_CHIPS][3];
  int ay_envelope_period[NUM_AY_CHIPS];
  uint8_t ay_envelope_shape[NUM_AY_CHIPS];
  int ay_envelope_counter[NUM_AY_CHIPS];
  int ay_envelope_step[NUM_AY_CHIPS];
  int ay_envelope_holding[NUM_AY_CHIPS];

  // SN 76489
  int sn_counter[NUM_SN_CHIPS][4];
  int sn_toggle[NUM_SN_CHIPS][4];
  // Labdybug
  uint32_t noise_lfsr[NUM_SN_CHIPS] = {0x4000, 0x4000, 0x4000};

  // Bagman 
  unsigned short positionLast;
  short sinusWaveBuffer[256];

  // Galaxian discrete audio
  unsigned char lfo  = 0;
  unsigned short lfo_counter  = 0;
  unsigned char gal_tone_cnt = 0;       // VCO tone phase accumulator
  int gal_tone_toggle = 1;              // VCO square wave state
  unsigned char gal_fs_cnt[3] = {0,0,0};  // FS1/FS2/FS3 phase accumulators
  int gal_fs_toggle[3] = {1,1,1};       // FS1/FS2/FS3 square wave states
  unsigned long gal_noise_rng = 1;      // noise LFSR (HIT)
  int gal_noise_cnt = 0;                // noise clock counter
  unsigned long gal_fire_rng = 0x1234;  // noise LFSR (FIRE)
  int gal_fire_cnt = 0;                 // fire noise clock counter

  // Space Invaders 
  // Variables are placed here, because of performance loss in digdug game when placing it to spaceinvarders.h!?!
  // Discrete audio (MAME mw8080bw_a.cpp reference)
  // Noise LFSR: 17-bit, taps at bits 4+16, clocked at 7515 Hz
  unsigned long si_noise_rng = 0x1FFFF; // 17-bit LFSR
  int si_noise_clock = 0;               // noise clock accumulator
  int si_noise_out = 0;                 // current noise output (bit 12)
  // UFO: SN76477 - SLF triangle ~5.3Hz modulates VCO 1220-3700Hz
  unsigned long si_ufo_sweep = 0;       // SLF position (0..4527)
  unsigned long si_ufo_cnt = 0;         // VCO phase accumulator
  int si_ufo_toggle = 1;                // VCO square wave state 
  // Shot: sample playback
  int si_shot_pos = 0;                  // current sample position
  int si_shot_playing = 0;              // 1 if playing
  // Explosion: noise burst with slow decay (~2s)
  unsigned long si_explo_cnt = 0;       // decay tick counter
  int si_explo_env = 0;                 // envelope level (decays from 120)
  // Invader die: sample playback
  int si_invhit_pos = 0;                // current sample position
  int si_invhit_playing = 0;            // 1 if playing
  // Fleet: 4 low tones (55/40/37/33 Hz, doubled for small speaker)
  unsigned long si_fleet_cnt = 0;       // tone phase accumulator
  int si_fleet_toggle = 1;              // tone square wave state
  // UFO hit: descending warble tone ~2000Hz
  unsigned long si_ufohit_cnt = 0;      // tone phase accumulator
  int si_ufohit_toggle = 1;             // tone square wave state
  int si_ufohit_freq = 0;               // current frequency (descends)
  unsigned long si_ufohit_warble = 0;   // warble phase counter
  // Coin insert: metallic clink sound
  unsigned long si_coin_cnt = 0;        // tone phase accumulator
  unsigned long si_coin_cnt2 = 0;       // overtone phase accumulator
  int si_coin_toggle = 1;               // primary tone square wave
  int si_coin_toggle2 = 1;              // overtone square wave
  int si_coin_timer = 0;                // samples remaining for coin sound
  int si_coin_env = 0;                  // envelope level (decays)
};

#endif