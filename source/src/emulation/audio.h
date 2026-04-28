#ifndef AUDIO_H
#define AUDIO_H

#include <driver/i2s.h>
#include "../machines/machineBase.h"
#include "../machines/dkong/dkong.h"
#include "../machines/galaga/galaga.h"
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
  void discrete_render_buffer(void);
  void generateSinusWave(int32_t amplitude, short* buffer, uint16_t length);
 
  machineBase *currentMachine;
  signed char machineType;

#ifdef SND_DIFF
  unsigned short snd_buffer[128]; // buffer space for two channels
#else
  unsigned short snd_buffer[64];  // buffer space for a single channel
#endif

  int ay_period[5][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}};
  int ay_volume[5][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
  int ay_enable[5][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
  int audio_cnt[5][4], audio_toggle[5][4] = {{1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 1, 1}};
  unsigned long ay_noise_rng[5] = {1, 1, 1, 1, 1};

  unsigned long snd_cnt[3] = {0, 0, 0};
  unsigned long snd_freq[3];
  const signed char *snd_wave[3];
  unsigned char snd_volume[3];

  short volumeSetting = 3;
  bool volumeUpLast;
  bool volumeDownLast;

  // Bombjack
  int ay_envelope_period[3] = {0,0,0};
  uint8_t ay_envelope_shape[3] = {0,0,0};
  int ay_envelope_counter[3] = {0,0,0};
  int ay_envelope_step[3] = {0,0,0};
  int ay_envelope_holding[3] = {0,0,0};

  // MrDo!
  int sn_counter[2][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}};
  int sn_toggle[2][4] = {{1, 1, 1, 1}, {1, 1, 1, 1}};

  // Labdybug
  uint32_t noise_lfsr[2] = {0x4000, 0x4000};

  // Bagman 
  unsigned short positionLast;
  short sinusWaveBuffer[256];
};

#endif