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

  generateSinusWave(256, sinusWaveBuffer, sizeof(sinusWaveBuffer)  / 2 );
}

void Audio::start(machineBase *machineBase) {
  currentMachine = machineBase;
  machineType = currentMachine->machineType();
    
#ifndef WORKAROUND_I2S_APLL_PROBLEM
  // The audio CPU of donkey kong runs at 6Mhz. A full bus
  // cycle needs 15 clocks which results in 400k cycles
  // per second. The sound CPU typically needs 34 instruction
  // cycles to write an updated audio value to the external
  // DAC connected to port 0.

  // The effective sample rate thus is 6M/15/34 = 11764.7 Hz
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
      if (currentMachine->hasNamcoAudio())
        namco_render_buffer();
      else if(machineType == MCH_MRDO || machineType == MCH_LADYBUG)
        sn76489_render_buffer();
      else if(machineType == MCH_BAGMAN)
        discrete_render_buffer();
      else if(machineType == MCH_DKONG)
	      i8048_render_buffer();
      else
        ay_render_buffer();
    }
  } while(bytesOut);
}

void Audio::ay_render_buffer(void) {
  char AY = (machineType == MCH_FROGGER) ? 1 : 2;       // frogger has one AY / 1942 has two AYs
  char AY_INC = (machineType == MCH_FROGGER || machineType == MCH_ANTEATER) ? 9 : 8;   // froggger runs at 1.78 MHz -> 223718/24000 = 9,32 / 1942 runs at 1.5 MHz -> 187500/24000 = 7,81
  char AY_VOL = (machineType == MCH_FROGGER) ? 11 : 5;  // frogger min/max = -/+ 3*15*11 = -/+ 495 / 1942 min/max = -/+ 6*15*11 = -/+ 990
  if (machineType == MCH_BOMBJACK) { AY = 3; AY_INC = 8; AY_VOL = 10; }
  if (machineType == MCH_GYRUSS) { AY = 5; AY_INC = 9; AY_VOL = 3; }

  // up to three AY's
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

    if (machineType != MCH_BOMBJACK)
      continue;

    // --- LOGICA INVILUPPO: Leggi registri R11, R12, R13 ---
    ay_envelope_period[ay] = currentMachine->soundregs[ay_off + 11] + 256 * currentMachine->soundregs[ay_off + 12];
    
    // Rileva un cambio di forma d'onda (R13) per triggerare l'inviluppo
    uint8_t new_shape = currentMachine->soundregs[ay_off + 13];
    if (new_shape != ay_envelope_shape[ay]) {
      ay_envelope_shape[ay] = new_shape & 0x0F;
      ay_envelope_counter[ay] = 0; // Reset contatore
      // Imposta lo step iniziale in base alla forma d'onda (attacco: 0, decadimento: 15)
      ay_envelope_step[ay] = (ay_envelope_shape[ay] < 4 || (ay_envelope_shape[ay] >= 8 && ay_envelope_shape[ay] < 12)) ? 0 : 15;
      ay_envelope_holding[ay] = 0; // Non in stato di "hold"
    }
  }

  // render first buffer contents
  for(int i = 0; i < 64; i++) {
    short value = 0; // silence

    if(machineType == MCH_FROGGER || machineType == MCH_1942 || machineType == MCH_ANTEATER || machineType == MCH_GYRUSS) {    
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
    else if (machineType == MCH_BOMBJACK) {
      for(char ay = 0; ay < AY; ay++) {
        // --- LOGICA INVILUPPO: Esegui un passo di emulazione ---
        if (!ay_envelope_holding[ay] && ay_envelope_period[ay] > 0) {
          ay_envelope_counter[ay] += AY_INC;
          if (ay_envelope_counter[ay] >= ay_envelope_period[ay]) {
            ay_envelope_counter[ay] -= ay_envelope_period[ay];
            // Avanza lo step del volume dell'inviluppo in base alla forma
            if (ay_envelope_shape[ay] < 8) { // Forme d'attacco (volume cresce da 0 a 15)
              ay_envelope_step[ay]++;
              if (ay_envelope_step[ay] > 15) {
                // Se la forma è "alternata" (bit 0 settato), riparte da 0, altrimenti rimane a 15
                ay_envelope_step[ay] = (ay_envelope_shape[ay] & 1) ? 0 : 15; 
                // Se la forma è "hold" (bit 1 settato), si ferma qui
                if (ay_envelope_shape[ay] & 2) ay_envelope_holding[ay] = 1; 
              }
            } 
            else { // Forme di decadimento (volume decresce da 15 a 0)
              ay_envelope_step[ay]--;
              if (ay_envelope_step[ay] < 0) {
                // Se la forma è "alternata" (bit 0 settato), riparte da 15, altrimenti rimane a 0
                ay_envelope_step[ay] = (ay_envelope_shape[ay] & 1) ? 15 : 0; 
                // Se la forma è "hold" (bit 1 settato), si ferma qui
                if (ay_envelope_shape[ay] & 2) ay_envelope_holding[ay] = 1; 
              }
            }
          }
        }
      
        // Elabora il generatore di rumore (R6)
        if(ay_period[ay][3]) {
          audio_cnt[ay][3] += AY_INC;
          if(audio_cnt[ay][3] > ay_period[ay][3]) {
            audio_cnt[ay][3] -= ay_period[ay][3];
            // --- CORREZIONE LFSR RUMORE (Standard AY-3-8910 17-bit LFSR) ---
            uint32_t b = (((ay_noise_rng[ay] >> 0) ^ (ay_noise_rng[ay] >> 3)) & 1);
            ay_noise_rng[ay] = (ay_noise_rng[ay] >> 1) | (b << 16);
          }
        }
  
        // Elabora i 3 canali di tono e li mixa con il rumore
        for(char c = 0; c < 3; c++) {
          // Controlla se il canale è abilitato nel mixer (R7) e ha un periodo valido
          if(ay_period[ay][c] && ay_enable[ay][c]) {
            // --- LOGICA INVILUPPO: Scegli il volume corretto ---
            int current_channel_volume = 0;
            if (ay_volume[ay][c] & 0x10) { // Se il bit 4 del registro volume è 1, usa l'inviluppo
              current_channel_volume = ay_envelope_step[ay];
            } 
            else { // Altrimenti, usa il volume fisso (bit 0-3)
              current_channel_volume = ay_volume[ay][c] & 0x0F;
            }

            if (current_channel_volume > 0) { // Solo se il volume non è zero
              short bit = 1;
              // Applica il mixing Tono/Rumore in base ai bit di ay_enable (ottenuti da R7)
              if(ay_enable[ay][c] & 1) bit &= (audio_toggle[ay][c] > 0) ? 1:0; // Bit 0 di ay_enable -> Tono
              if(ay_enable[ay][c] & 2) bit &= (ay_noise_rng[ay] & 1) ? 1:0;     // Bit 1 di ay_enable -> Rumore
    
              // Se il bit risultante è 0, il segnale è invertito per l'onda quadra
              if(bit == 0) bit = -1;
              value += AY_VOL * bit * current_channel_volume;
            }
    
            // Avanza il contatore del tono (R0-R5)
            audio_cnt[ay][c] += AY_INC;
            if(audio_cnt[ay][c] > ay_period[ay][c]) {
              audio_cnt[ay][c] -= ay_period[ay][c];
              audio_toggle[ay][c] = -audio_toggle[ay][c];
            }
          }
        }
      }
      value = value / 3;
    }
    valueToBuffer(i, value);
  }
}

void Audio::i8048_render_buffer(void) {
  dkong *dkongMachine = dynamic_cast<dkong*>(currentMachine);

  // render first buffer contents
  for(int i = 0; i < 64; i++) {
    short value = 0; // silence

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
        value += *dkongMachine->dkong_sample_ptr[j]++ >> (2 - j); 
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

    valueToBuffer(i, value);
  }
}

void Audio::sn76489_render_buffer(void) {
  const int sn_inc = 11;  // SN_CLOCK / SAMPLE_RATE
  
  // Volumi con hold
  int vol[2][4];
  for (int chip = 0; chip < 2; chip++) {
    for (int c = 0; c < 4; c++) {
      if (currentMachine->sn_hold[chip][c] > 0) {
        vol[chip][c] = currentMachine->sn_min_volume[chip][c];
        currentMachine->sn_hold[chip][c]--;
        if (currentMachine->sn_hold[chip][c] == 0)
          currentMachine->sn_min_volume[chip][c] = currentMachine->sn_volume[chip][c];
        } 
        else {
          vol[chip][c] = currentMachine->sn_volume[chip][c];
          currentMachine->sn_min_volume[chip][c] = currentMachine->sn_volume[chip][c];
        }
      }
    }

    for (int i = 0; i < 64; i++) {
      short sample = 0;

      for (int chip = 0; chip < 2; chip++) {
        for (int c = 0; c < 4; c++) {
          int period = currentMachine->sn_period[chip][c];

          if (vol[chip][c] < 15 && period > 0) {
            sn_counter[chip][c] -= sn_inc;

          while (sn_counter[chip][c] <= 0) {
            if (c == 3) {  // Noise channel
              uint32_t feedback = (noise_lfsr[chip] & 0x01) ^ ((noise_lfsr[chip] & 0x02) >> 1);
              noise_lfsr[chip] = (noise_lfsr[chip] >> 1) | (feedback << 14);
              sn_toggle[chip][c] = (noise_lfsr[chip] & 0x01) ? 1 : -1;
              sn_counter[chip][c] += (period << 3);  // Rallenta noise
            } 
            else {  // Tone
              sn_counter[chip][c] += period;
              sn_toggle[chip][c] = -sn_toggle[chip][c];
            }
          }
          sample += sn_toggle[chip][c] * (15 - vol[chip][c]) * 6;
        }
      }
    }
    valueToBuffer(i, sample);
  }
}

void Audio::namco_render_buffer(void) {
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

void Audio::generateSinusWave(int32_t amplitude, short* buffer, uint16_t length) { 
  for (int i=0; i<length; ++i) {
    buffer[i] = int32_t(float(amplitude) * sin(2.0 * PI * (1.0 / length) * i));
  }
}

void Audio::discrete_render_buffer() {
  unsigned short duration = currentMachine->soundregs[0] + (currentMachine->soundregs[1] << 8);

  if (duration > 0)
    duration--;

  currentMachine->soundregs[0] = duration & 0x00ff;
  currentMachine->soundregs[1] = (duration & 0xff00) > 8;

  float frequency;
  switch (currentMachine->soundregs[2]) {
    case 0x3: frequency = A5_3; break;
    case 0x4: frequency = C6_4; break;
    case 0x5: frequency = F5_5; break;
    case 0x6: frequency = G5_6; break;
    case 0x7: frequency = E6_7; break;
    case 0x8: frequency = B6_8; break;
    case 0xE: frequency = D6_E; break;
    case 0xF: frequency = B5_F; break;
    case 0xB: frequency = XX_B; break;
  }

  unsigned short pause = currentMachine->soundregs[3];
  if (pause > 0)
    currentMachine->soundregs[3]--;

  float delta = 0;
  if (duration != 0 && pause == 0)
    delta = (frequency * (sizeof(sinusWaveBuffer) / 2)) / float(24000);
  
  for(int i = 0; i < 64; i++) {
    uint16_t pos = uint32_t(((i + 1) * delta) + positionLast) % (sizeof(sinusWaveBuffer) / 2);
    short value = sinusWaveBuffer[pos];

    if (i == 63)
      positionLast = pos;

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