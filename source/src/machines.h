#ifndef _MACHINES_H_
#define _MACHINES_H_

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
#define ENABLE_BOMBJACK
#define ENABLE_MRDO
#define ENABLE_BAGMAN
#define ENABLE_PENGO
#define ENABLE_GYRUSS
#define ENABLE_LADYBUG

#ifdef ENABLE_PACMAN  
  #include "machines/pacman/pacman.h"
#endif

#ifdef ENABLE_GALAGA
  #include "machines/galaga/galaga.h"
#endif

#ifdef ENABLE_DKONG
  #include "machines/dkong/dkong.h"
#endif

#ifdef ENABLE_FROGGER
  #include "machines/frogger/frogger.h"
#endif

#ifdef ENABLE_DIGDUG
  #include "machines/digdug/digdug.h"
#endif

#ifdef ENABLE_1942
  #include "machines/1942/1942.h"
#endif

#ifdef ENABLE_EYES
  #include "machines/eyes/eyes.h"
#endif

#ifdef ENABLE_MRTNT
  #include "machines/mrtnt/mrtnt.h"
#endif

#ifdef ENABLE_LIZWIZ
  #include "machines/lizwiz/lizwiz.h"
#endif

#ifdef ENABLE_THEGLOB
  #include "machines/theglob/theglob.h"
#endif

#ifdef ENABLE_CRUSH 
  #include "machines/crush/crush.h"
#endif

#ifdef ENABLE_ANTEATER 
  #include "machines/anteater/anteater.h"
#endif

#ifdef ENABLE_BOMBJACK 
  #include "machines/bombjack/bombjack.h"
#endif

#ifdef ENABLE_MRDO 
  #include "machines/mrdo/mrdo.h"
#endif

#ifdef ENABLE_BAGMAN 
  #include "machines/bagman/bagman.h"
#endif

#ifdef ENABLE_PENGO 
  #include "machines/pengo/pengo.h"
#endif

#ifdef ENABLE_GYRUSS
  #include "machines/gyruss/gyruss.h"
#endif

#ifdef ENABLE_LADYBUG  
  #include "machines/ladybug/ladybug.h"
#endif

// change machine order is possible here...
machineBase *machines[] = {
#ifdef ENABLE_PACMAN  
  new pacman(),
#endif
#ifdef ENABLE_GALAGA  
  new galaga(), 
#endif  
#ifdef ENABLE_DIGDUG  
  new digdug(), 
#endif  
#ifdef ENABLE_FROGGER  
  new frogger(), 
#endif  
#ifdef ENABLE_DKONG  
  new dkong(), 
#endif  
#ifdef ENABLE_1942  
  new _1942(), 
#endif  
#ifdef ENABLE_LIZWIZ  
  new lizwiz(),
#endif  
#ifdef ENABLE_EYES  
  new eyes(), 
#endif  
#ifdef ENABLE_MRTNT  
  new mrtnt(), 
#endif  
#ifdef ENABLE_THEGLOB 
  new theglob(),
#endif
#ifdef ENABLE_CRUSH 
  new crush(),
#endif
#ifdef ENABLE_ANTEATER 
  new anteater(),
#endif
#ifdef ENABLE_BOMBJACK 
  new bombjack(),
#endif
#ifdef ENABLE_MRDO 
  new mrdo(),
#endif
#ifdef ENABLE_BAGMAN 
  new bagman(),
#endif
#ifdef ENABLE_PENGO 
  new pengo(),
#endif
#ifdef ENABLE_GYRUSS 
  new gyruss(),
#endif
#ifdef ENABLE_LADYBUG  
  new ladybug()
#endif
};

template <std::size_t N, class T>
constexpr std::size_t countof(T(&)[N]) { return N; }
static_assert(countof(machines) >= 1, "At least one machine has to be enabled!");

#endif // _MACHINES_H_