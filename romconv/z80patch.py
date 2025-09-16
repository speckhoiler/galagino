#!/usr/bin/env python3
import zipfile

ZIPLOC="../romszip/"
ZIP="Z80-081707.zip"
DEST="../source/src/cpus/z80/"

FILES2COPY=[ "CodesCB.h", "Codes.h", "CodesXX.h", "CodesED.h", "CodesXCB.h", "Tables.h" ]

Z80H_EXTRA=b"""void JumpZ80(word PC);
#endif

void StepZ80(register Z80 *R);
unsigned char OpZ80_INL(unsigned short Addr);

#ifdef __cplusplus
}
#endif
#endif /* Z80_H */

#include "..\..\emulation\emulation.h"
"""

Z80H_EXTRA2=b"""#undef word 
#define word unsigned short
//"""

Z80C_EXTRA=b"""
void StepZ80(Z80 *R)
{
  register byte I;
  register pair J;

  I=OpZ80_INL(R->PC.W++);

  switch(I)
  {
#include "Codes.h"
    case PFX_CB: CodesCB(R);break;
    case PFX_ED: CodesED(R);break;
    case PFX_FD: CodesFD(R);break;
    case PFX_DD: CodesDD(R);break;
  }

  if(R->IFF&IFF_EI)
    R->IFF=(R->IFF&~IFF_EI)|IFF_1; /* Done with AfterEI state */
}
"""

def unpack_z80(name):
    with zipfile.ZipFile(name, 'r') as zip:
        # most files are just unpacked
        for i in FILES2COPY:
            print("Copying", i)
            code = zip.read("Z80/"+i)
            with open(DEST+i, "wb") as of:
                of.write(code)

        # Z80.h patch
        print("Patching Z80.h")
        code = zip.read("Z80/Z80.h")
        with open(DEST+"Z80.h", "wb") as of:
            # read file line by line and uncomment this line:
            STR1 = b"/* #define LSB_FIRST */        /* Compile for low-endian CPU */"
            STR2 = b"typedef unsigned short word;"
            STR3 = b"void JumpZ80(word PC);"

            for i in code.split(b"\r\n"):
                if i == STR3:
                    of.write(Z80H_EXTRA)
                    break

                if i == STR2:
                    of.write(Z80H_EXTRA2)

                if i == STR1:
                    of.write(b"#define LSB_FIRST              /* Compile for low-endian CPU */\r\n")
                else:
                    of.write(i + b"\r\n")
    
        # Z80.c gets an additional function
        print("Patching Z80.c")
        code = zip.read("Z80/Z80.c")
        with open(DEST+"Z80.c", "wb") as of:
            of.write(code)
            of.write(Z80C_EXTRA)

unpack_z80(ZIPLOC + ZIP)

