#ifndef _CONFIG_H_
#define _CONFIG_H_

// disable e.g. if roms are missing
#define ENABLE_PACMAN
#define ENABLE_GALAGA
#define ENABLE_DKONG
#define ENABLE_FROGGER
#define ENABLE_DIGDUG
#define ENABLE_1942
#define ENABLE_EYES
#define ENABLE_MRTNT
#define ENABLE_LIZWIZ
#define ENABLE_THEGLOB
#define ENABLE_CRUSH
#define ENABLE_ANTEATER

#if !defined(ENABLE_PACMAN) && !defined(ENABLE_GALAGA) && !defined(ENABLE_DKONG) && !defined(ENABLE_FROGGER) && !defined(ENABLE_DIGDUG) && !defined(ENABLE_1942) && !defined(ENABLE_EYES) && !defined(ENABLE_MRTNT) && !defined(ENABLE_LIZWIZ) && !defined(ENABLE_THEGLOB) && !defined(ENABLE_CRUSH) && !defined(ENABLE_ANTEATER)
  #error "At least one machine has to be enabled!"
#endif

// check if only one machine is enabled
#if (( defined(ENABLE_PACMAN) && !defined(ENABLE_GALAGA) && !defined(ENABLE_DKONG) && !defined(ENABLE_FROGGER) && !defined(ENABLE_DIGDUG) && !defined(ENABLE_1942) && !defined(ENABLE_EYES) && !defined(ENABLE_MRTNT) && !defined(ENABLE_LIZWIZ) && !defined(ENABLE_THEGLOB) && !defined(ENABLE_CRUSH) && !defined(ENABLE_ANTEATER)) || \
     (!defined(ENABLE_PACMAN) &&  defined(ENABLE_GALAGA) && !defined(ENABLE_DKONG) && !defined(ENABLE_FROGGER) && !defined(ENABLE_DIGDUG) && !defined(ENABLE_1942) && !defined(ENABLE_EYES) && !defined(ENABLE_MRTNT) && !defined(ENABLE_LIZWIZ) && !defined(ENABLE_THEGLOB) && !defined(ENABLE_CRUSH) && !defined(ENABLE_ANTEATER)) || \
     (!defined(ENABLE_PACMAN) && !defined(ENABLE_GALAGA) &&  defined(ENABLE_DKONG) && !defined(ENABLE_FROGGER) && !defined(ENABLE_DIGDUG) && !defined(ENABLE_1942) && !defined(ENABLE_EYES) && !defined(ENABLE_MRTNT) && !defined(ENABLE_LIZWIZ) && !defined(ENABLE_THEGLOB) && !defined(ENABLE_CRUSH) && !defined(ENABLE_ANTEATER)) || \
     (!defined(ENABLE_PACMAN) && !defined(ENABLE_GALAGA) && !defined(ENABLE_DKONG) &&  defined(ENABLE_FROGGER) && !defined(ENABLE_DIGDUG) && !defined(ENABLE_1942) && !defined(ENABLE_EYES) && !defined(ENABLE_MRTNT) && !defined(ENABLE_LIZWIZ) && !defined(ENABLE_THEGLOB) && !defined(ENABLE_CRUSH) && !defined(ENABLE_ANTEATER)) || \
     (!defined(ENABLE_PACMAN) && !defined(ENABLE_GALAGA) && !defined(ENABLE_DKONG) && !defined(ENABLE_FROGGER) &&  defined(ENABLE_DIGDUG) && !defined(ENABLE_1942) && !defined(ENABLE_EYES) && !defined(ENABLE_MRTNT) && !defined(ENABLE_LIZWIZ) && !defined(ENABLE_THEGLOB) && !defined(ENABLE_CRUSH) && !defined(ENABLE_ANTEATER)) || \
     (!defined(ENABLE_PACMAN) && !defined(ENABLE_GALAGA) && !defined(ENABLE_DKONG) && !defined(ENABLE_FROGGER) && !defined(ENABLE_DIGDUG) &&  defined(ENABLE_1942) && !defined(ENABLE_EYES) && !defined(ENABLE_MRTNT) && !defined(ENABLE_LIZWIZ) && !defined(ENABLE_THEGLOB) && !defined(ENABLE_CRUSH) && !defined(ENABLE_ANTEATER)) || \
     (!defined(ENABLE_PACMAN) && !defined(ENABLE_GALAGA) && !defined(ENABLE_DKONG) && !defined(ENABLE_FROGGER) && !defined(ENABLE_DIGDUG) && !defined(ENABLE_1942) &&  defined(ENABLE_EYES) && !defined(ENABLE_MRTNT) && !defined(ENABLE_LIZWIZ) && !defined(ENABLE_THEGLOB) && !defined(ENABLE_CRUSH) && !defined(ENABLE_ANTEATER)) || \
     (!defined(ENABLE_PACMAN) && !defined(ENABLE_GALAGA) && !defined(ENABLE_DKONG) && !defined(ENABLE_FROGGER) && !defined(ENABLE_DIGDUG) && !defined(ENABLE_1942) && !defined(ENABLE_EYES) &&  defined(ENABLE_MRTNT) && !defined(ENABLE_LIZWIZ) && !defined(ENABLE_THEGLOB) && !defined(ENABLE_CRUSH) && !defined(ENABLE_ANTEATER)) || \
     (!defined(ENABLE_PACMAN) && !defined(ENABLE_GALAGA) && !defined(ENABLE_DKONG) && !defined(ENABLE_FROGGER) && !defined(ENABLE_DIGDUG) && !defined(ENABLE_1942) && !defined(ENABLE_EYES) && !defined(ENABLE_MRTNT) &&  defined(ENABLE_LIZWIZ) && !defined(ENABLE_THEGLOB) && !defined(ENABLE_CRUSH) && !defined(ENABLE_ANTEATER)) || \
     (!defined(ENABLE_PACMAN) && !defined(ENABLE_GALAGA) && !defined(ENABLE_DKONG) && !defined(ENABLE_FROGGER) && !defined(ENABLE_DIGDUG) && !defined(ENABLE_1942) && !defined(ENABLE_EYES) && !defined(ENABLE_MRTNT) && !defined(ENABLE_LIZWIZ) &&  defined(ENABLE_THEGLOB) && !defined(ENABLE_CRUSH) && !defined(ENABLE_ANTEATER)) || \
     (!defined(ENABLE_PACMAN) && !defined(ENABLE_GALAGA) && !defined(ENABLE_DKONG) && !defined(ENABLE_FROGGER) && !defined(ENABLE_DIGDUG) && !defined(ENABLE_1942) && !defined(ENABLE_EYES) && !defined(ENABLE_MRTNT) && !defined(ENABLE_LIZWIZ) && !defined(ENABLE_THEGLOB) &&  defined(ENABLE_CRUSH) && !defined(ENABLE_ANTEATER)) || \
     (!defined(ENABLE_PACMAN) && !defined(ENABLE_GALAGA) && !defined(ENABLE_DKONG) && !defined(ENABLE_FROGGER) && !defined(ENABLE_DIGDUG) && !defined(ENABLE_1942) && !defined(ENABLE_EYES) && !defined(ENABLE_MRTNT) && !defined(ENABLE_LIZWIZ) && !defined(ENABLE_THEGLOB) && !defined(ENABLE_CRUSH) &&  defined(ENABLE_ANTEATER)))
  #define SINGLE_MACHINE
#endif

// game config
#define MASTER_ATTRACT_MENU_TIMEOUT  20000      // start games while sitting idle in menu for 20 seconds, undefine to disable
#define MASTER_ATTRACT_GAME_TIMEOUT  60000 * 5  // restart after 5 minutes 

// video config
//#define TFT_SPICLK  40000000    // 40 Mhz. Some displays cope with 80 Mhz
//#define TFT_SPICLK	80000000    // 80 Mhz. Some displays cope with 80 Mhz

// max possible video rate:
// 8*224 pixels = 8*224*16 = 28672 bits
// 2790 char rows per sec at 40Mhz = max 38 fps
#if TFT_SPICLK < 80000000
  #define VIDEO_HALF_RATE
#endif

// x and y offset of 224x288 pixels inside the 240x320 screen
#define TFT_X_OFFSET      8
#define TFT_Y_OFFSET      16

// led config
//#define LED_PIN           18 // pin used for optional WS2812 stripe
#define LED_BRIGHTNESS 	  50 // range 0..255

// audio config
//#define SND_DIFF   	 // set to output differential audio on GPIO25 _and_ inverted on GPIO26
#define SND_LEFT_CHANNEL // Use GPIO 26 for audio

// esp32 model config
//#define CHEAP_YELLOW_DISPLAY_CONF

#ifdef CHEAP_YELLOW_DISPLAY_CONF
  #define TFT_CS          15
  #define TFT_DC          2
  #define TFT_RST         -1
  #define TFT_BL          27   // don't set if backlight is hard wired
  #define TFT_BL_LEVEL    HIGH  // backlight on with low or high signal
  //#define TFT_ILI9341 // define for ili9341, otherwise st7789
  //#define TFT_VFLIP   // define for upside down

  #define TFT_MISO 	      12
  #define TFT_MOSI 	      13
  #define TFT_SCLK 	      14
  //#define TFT_MAC  	    0x20  // some CYD need this to rotate properly and have correct colors

  // Pins used for buttons
  #define BTN_START_PIN	  35
  //#define BTN_COIN_PIN    21   // if this is not defined, then start will act as coin & start

  #define BTN_LEFT_PIN    21
  #define BTN_RIGHT_PIN   22
  #define BTN_DOWN_PIN    16
  #define BTN_UP_PIN      17
  #define BTN_FIRE_PIN    4
#endif

#ifndef CHEAP_YELLOW_DISPLAY_CONF
  #define TFT_CS          5
  #define TFT_DC          4
  #define TFT_RST         22
  #define TFT_BL          15      // don't set if backlight is hard wired
  #define TFT_BL_LEVEL    LOW     // backlight on with low or high signal
  #define TFT_ILI9341             // define for ili9341, otherwise st7789
  //#define TFT_VFLIP               // define for upside down

  #define TFT_MISO 	      19
  #define TFT_MOSI 	      23
  #define TFT_SCLK 	      18

  // Pins used for buttons
  //#define BTN_START_PIN   0
  //#define BTN_COIN_PIN    21      // if this is not defined, then start will act as coin & start

  #ifndef NUNCHUCK_INPUT
    #define BTN_LEFT_PIN  33
    #define BTN_RIGHT_PIN 14
    #define BTN_DOWN_PIN  16
    #define BTN_UP_PIN    21
    #define BTN_FIRE_PIN  12
  #else
    #define NUNCHUCK_SDA  33
    #define NUNCHUCK_SCL  32
    #define NUNCHUCK_MOVE_THRESHOLD 30 // This is the dead-zone for where minor movements on the stick will not be considered valid movements
  #endif
#endif

#endif // _CONFIG_H_