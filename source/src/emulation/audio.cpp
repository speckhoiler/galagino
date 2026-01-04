#include "audio.h"

void Audio::init() {
  // 24 kHz @ 16 bit = 48000 bytes/sec = 800 bytes per 60hz game frame =
  // 1600 bytes per 30hz screen update = ~177 bytes every four tile rows
  const i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
    .sample_rate = 24000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
#ifdef SND_DIFF
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
#elif defined(SND_LEFT_CHANNEL) // For devices using the left channel (e.g. CYD)
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
#else
    .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
#endif
    .intr_alloc_flags = 0,
    .dma_buf_count = 4,
    .dma_buf_len = 64,   // 64 samples
    // APLL usage is broken in ESP-IDF 4.4.5
#ifdef WORKAROUND_I2S_APLL_PROBLEM
    .use_apll = false
#else
    .use_apll = true
#endif
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);

#ifdef SND_DIFF
  i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);
#elif defined(SND_LEFT_CHANNEL) // For devices using the left channel (e.g. CYD)
  i2s_set_dac_mode(I2S_DAC_CHANNEL_LEFT_EN); 
#else
  i2s_set_dac_mode(I2S_DAC_CHANNEL_RIGHT_EN);
#endif  
}

void Audio::start(machineBase *machineBase) {
  currentMachine = machineBase;
  
#ifndef WORKAROUND_I2S_APLL_PROBLEM
  // The audio CPU of donkey kong runs at 6Mhz. A full bus
  // cycle needs 15 clocks which results in 400k cycles
  // per second. The sound CPU typically needs 34 instruction
  // cycles to write an updated audio value to the external
  // DAC connected to port 0.

  // The effective sample rate thus is 6M/15/34 = 11764.7 Hz
  signed char machineType = currentMachine->machineType();
  i2s_set_sample_rates(I2S_NUM_0, machineType == MCH_DKONG ? 11765 : 24000);
#endif
}

void Audio::volumeUpDown(bool up, bool down) {
  if (up && !volumeUpLast) {
    if (volumeSetting > 1)
      volumeSetting--;
  }
  volumeUpLast = up;

  if (down && !volumeDownLast) {
    if (volumeSetting < 30)
      volumeSetting++;
  }
  volumeDownLast = down;
}

void Audio::transmit() {
  // (try to) transmit as much audio data as possible. Since we
  // write data in exact the size of the DMA buffers we can be sure
  // that either all or nothing is actually being written
  size_t bytesOut = 0;
  do {
    // copy data in i2s dma buffer if possible
    i2s_write(I2S_NUM_0, snd_buffer, sizeof(snd_buffer), &bytesOut, 0);

    // render the next audio chunk if data has actually been sent
    if(bytesOut) {      
      if (currentMachine->hasNamcoAudio()) {
        namco_waveregs_parse();
        namco_render_buffer();
      }
      else
        ay_render_buffer();
    }
  } while(bytesOut);
}

void Audio::ay_render_buffer(void) {
  signed char machineType = currentMachine->machineType();

  char AY = (machineType == MCH_FROGGER) ? 1 : 2;       // frogger has one AY / 1942 has two AYs
  char AY_INC = (machineType == MCH_FROGGER || machineType == MCH_ANTEATER) ? 9 : 8;   // froggger runs at 1.78 MHz -> 223718/24000 = 9,32 / 1942 runs at 1.5 MHz -> 187500/24000 = 7,81
  char AY_VOL = (machineType == MCH_FROGGER) ? 11 : 5;  // frogger min/max = -/+ 3*15*11 = -/+ 495 / 1942 min/max = -/+ 6*15*11 = -/+ 990

  if(machineType == MCH_FROGGER || machineType == MCH_1942 || machineType == MCH_ANTEATER) {
    // up to two AY's
    for(char ay = 0; ay < AY; ay++) {
      int ay_off = 16 * ay;

      // three tone channels
      for(char c = 0; c < 3; c++) {
	      ay_period[ay][c] = currentMachine->soundregs[ay_off + 2 * c] + 256 * (currentMachine->soundregs[ay_off + 2 * c + 1] & 15);
	      ay_enable[ay][c] = (((currentMachine->soundregs[ay_off + 7] >> c) & 1) | ((currentMachine->soundregs[ay_off + 7] >> (c + 2)) & 2)) ^ 3;
	      ay_volume[ay][c] = currentMachine->soundregs[ay_off + 8 + c] & 0x0f;
      }
      // noise channel
      ay_period[ay][3] = currentMachine->soundregs[ay_off + 6] & 0x1f;
    }
  }

  // render first buffer contents
  for(int i = 0; i < 64; i++) {
    short value = 0; // silence

    if(machineType == MCH_DKONG) {
      dkong *dkongMachine = dynamic_cast<dkong*>(currentMachine);
      
      // no buffer available
      if(dkongMachine->dkong_audio_rptr != dkongMachine->dkong_audio_wptr)
        // copy data from dkong buffer into tx buffer
        // 8048 sounds gets 50% of the available volume range
#ifdef WORKAROUND_I2S_APLL_PROBLEM
        value = dkongMachine->dkong_audio_transfer_buffer[dkongMachine->dkong_audio_rptr][(dkongMachine->dkong_obuf_toggle ? 32 : 0) + (i / 2)];
#else
        value = dkongMachine->dkong_audio_transfer_buffer[dkongMachine->dkong_audio_rptr][i];
#endif

      // include sample sounds
      // walk is 6.25% volume, jump is at 12.5% volume and, stomp is at 25%
      for(char j = 0; j < 3; j++) {
        if(dkongMachine->dkong_sample_cnt[j]) {
#ifdef WORKAROUND_I2S_APLL_PROBLEM
          value += *dkongMachine->dkong_sample_ptr[j] >> (2 - j); 
          if(i & 1) { // advance read pointer every second sample
            dkongMachine->dkong_sample_ptr[j]++;
            dkongMachine->dkong_sample_cnt[j]--;
          }
#else
          volume += *dkongMachine->dkong_sample_ptr[j]++ >> (2 - j); 
          dkongMachine->dkong_sample_cnt[j]--;
#endif
        }
      }
#ifdef WORKAROUND_I2S_APLL_PROBLEM
      if (i == 63) {
        // advance write pointer. The buffer is a ring
        if(dkongMachine->dkong_obuf_toggle)
          dkongMachine->dkong_audio_rptr = (dkongMachine->dkong_audio_rptr + 1) & DKONG_AUDIO_QUEUE_MASK;
        
        dkongMachine->dkong_obuf_toggle = !dkongMachine->dkong_obuf_toggle;
      }
#endif
    }
    else if(machineType == MCH_FROGGER || machineType == MCH_1942 || machineType == MCH_ANTEATER) {    
      for(char ay = 0; ay < AY; ay++) {
        // frogger can acually skip the noise generator as
        // it doesn't use it      
        if(ay_period[ay][3]) {
	        // process noise generator
	        audio_cnt[ay][3] += AY_INC; // for 24 khz
	        if(audio_cnt[ay][3] > ay_period[ay][3]) {
	          audio_cnt[ay][3] -= ay_period[ay][3];
	          // progress rng
	          ay_noise_rng[ay] ^= (((ay_noise_rng[ay] & 1) ^ ((ay_noise_rng[ay] >> 3) & 1)) << 17);
	          ay_noise_rng[ay] >>= 1;
	        }
        }
	
        for(char c = 0; c < 3; c++) {
	        // a channel is on if period != 0, vol != 0 and tone bit == 0
	        if(ay_period[ay][c] && ay_volume[ay][c] && ay_enable[ay][c]) {
	          short bit = 1;
	          if(ay_enable[ay][c] & 1) bit &= (audio_toggle[ay][c] > 0) ? 1 : 0;  // tone
	          if(ay_enable[ay][c] & 2) bit &= (ay_noise_rng[ay] & 1) ? 1 : 0;     // noise
	  
	          if(bit == 0) bit = -1;
	          value += AY_VOL * bit * ay_volume[ay][c];
	  
	          audio_cnt[ay][c] += AY_INC; // for 24 khz
	          if(audio_cnt[ay][c] > ay_period[ay][c]) {
	            audio_cnt[ay][c] -= ay_period[ay][c];
	            audio_toggle[ay][c] = -audio_toggle[ay][c];
	          }
	        }
        }
      }
    }  
    valueToBuffer(i, value);
  }
}

void Audio::namco_waveregs_parse(void) {  
  // parse all three wsg channels
  for(char ch = 0; ch < 3; ch++) {  
    snd_wave[ch] = currentMachine->waveRom(currentMachine->soundregs[ch * 5 + 0x05] & 0x07);
    snd_freq[ch] = (ch == 0) ? currentMachine->soundregs[0x10] : 0; //5050-5054, 5056-5059, 505b-505e
    snd_freq[ch] += currentMachine->soundregs[ch * 5 + 0x11] << 4;
    snd_freq[ch] += currentMachine->soundregs[ch * 5 + 0x12] << 8;
    snd_freq[ch] += currentMachine->soundregs[ch * 5 + 0x13] << 12;
    snd_freq[ch] += currentMachine->soundregs[ch * 5 + 0x14] << 16;        
    snd_volume[ch] = currentMachine->soundregs[ch * 5 + 0x15]; //5055, 505a, 505f
  }
}

void Audio::namco_render_buffer(void) {
  signed char machineType = currentMachine->machineType();

  // render first buffer contents
  for(int i = 0; i < 64; i++) {
    short value = 0;

    // add up to three wave signals
    if(snd_volume[0]) value += snd_volume[0] * snd_wave[0][(snd_cnt[0] >> 13) & 0x1f];
    if(snd_volume[1]) value += snd_volume[1] * snd_wave[1][(snd_cnt[1] >> 13) & 0x1f];
    if(snd_volume[2]) value += snd_volume[2] * snd_wave[2][(snd_cnt[2] >> 13) & 0x1f];

    snd_cnt[0] += snd_freq[0];
    snd_cnt[1] += snd_freq[1];
    snd_cnt[2] += snd_freq[2];
    
    if(machineType == MCH_GALAGA) {
      galaga *galagaMachine = dynamic_cast<galaga*>(currentMachine);

      if(galagaMachine->snd_boom_cnt) {
        value += *galagaMachine->snd_boom_ptr * 3;
        
        if(galagaMachine->snd_boom_cnt & 1) 
          galagaMachine->snd_boom_ptr++;
        
        galagaMachine->snd_boom_cnt--;
      }
    }
    valueToBuffer(i, value);
  }
}

void Audio::valueToBuffer(int index, short value) {
    // value is now in the range of +/- 512, so expand to +/- 15 bit
    value = value * 64;

#ifdef SND_DIFF
    // generate differential output
    snd_buffer[2 * index]   = 0x8000 + (volume / volumeSetting);    // positive signal on GPIO26
    snd_buffer[2 * index + 1] = 0x8000 - (volume / volumeSetting);    // negatve signal on GPIO25 
#else
    // work-around weird byte order bug, see 
    // https://github.com/espressif/arduino-esp32/issues/8467#issuecomment-1656616015
    snd_buffer[index ^ 1]   = 0x8000 + (value / volumeSetting); 
#endif
}
