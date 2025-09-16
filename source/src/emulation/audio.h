#ifndef AUDIO_H
#define AUDIO_H

#include <driver\i2s.h>
#include "..\machines\machineBase.h"
#include "..\machines\dkong\dkong.h"
#include "..\machines\galaga\galaga.h"
#include "..\config.h"

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 4)
// See https://github.com/espressif/arduino-esp32/issues/8467
#define WORKAROUND_I2S_APLL_PROBLEM
// as a workaround we run dkong audio also at 24khz (instead of 11765)
// and fill the buffer at half the rate resulting in 12khz effective
// sample rate
#endif

class Audio {
public:
  void init();
  void start(machineBase *machineBase);
  void transmit();
  void volumeUpDown(bool up, bool down);

private:
  void namco_waveregs_parse(void);
  void namco_render_buffer(void);
  void ay_render_buffer(void);
  void valueToBuffer(int index, short value);
  machineBase *currentMachine;

#ifdef SND_DIFF
  unsigned short snd_buffer[128]; // buffer space for two channels
#else
  unsigned short snd_buffer[64];  // buffer space for a single channel
#endif

  int ay_period[2][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}};
  int ay_volume[2][3] = {{0, 0, 0}, {0, 0, 0}};
  int ay_enable[2][3] = {{0, 0, 0}, {0, 0, 0}};
  int audio_cnt[2][4], audio_toggle[2][4] = {{1, 1, 1, 1}, {1, 1, 1, 1}};
  unsigned long ay_noise_rng[2] = {1, 1};

  unsigned long snd_cnt[3] = {0, 0, 0};
  unsigned long snd_freq[3];
  const signed char *snd_wave[3];
  unsigned char snd_volume[3];

  short volumeSetting = 3;
  bool volumeUpLast;
  bool volumeDownLast;
};

#endif