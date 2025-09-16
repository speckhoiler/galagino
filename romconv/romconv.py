#!/usr/bin/env python3
import sys

PATCHES = {
    "galaga_rom_cpu1":
    [
        # jump over tile ram test
        ( 0x3382, 0x06, 0xc3 ),     # 06, jp
        ( 0x3383, 0x0a, 0x35 ),     # xx35
	( 0x3384, 0xd9, 0x34 ),     # 34xx
	# only one ramtest round, instead of 30
	( 0x348a, 0x1e, 0x01 ),
        # skip rom test
	( 0x352b, 0xe5, 0xc9 )      # ret
    ]    
}

def bit_permute_step(x, m, shift):
    t = ((x >> shift) ^ x) & m
    x = (x ^ t) ^ (t << shift)
    return x

def parse_rom(id, infiles, outfile, apply_patches = False, decode = False):
    offset = 0
    of = open(outfile, "w")

    print("const unsigned char "+id+"[] = {\n  ", end="", file=of)
    
    for name_idx in range(len(infiles)):
        f = open(infiles[name_idx], "rb")
        rom_data = f.read()
        f.close()

        # the first frogger audio cpu rom has bits
        # d0 and d1 swapped. Fix this
        if rom_data[:8] == bytes([0x05,0x00,0x22,0x00,0x40,0xc3,0x0b,0x02]):
            rom_data = list(rom_data)
            for i in range(len(rom_data)):
                rom_data[i] = (rom_data[i] & 0xfc) | ((rom_data[i] & 2)>>1) | ((rom_data[i] & 1)<<1)
            rom_data = bytes(rom_data)
            
        # apply patches
        if apply_patches:
            rom_data = list(rom_data)
            for i in PATCHES:
                if i == id:
                    for p in PATCHES[i]:
                        if p[0] - offset < len(rom_data):
                            if rom_data[p[0] - offset] == p[1]:
                                print("Patching", hex(p[0]), ":", p[1], "->", p[2])
                                rom_data[p[0] - offset] = p[2]
                            else:
                                raise ValueError("Unexpected patchdata")
        
        offset += len(rom_data)
        rom_data = list(rom_data)
        for i in range(len(rom_data)):
            # value, bitMask, shift left
            if decode:
             rom_data[i] = bit_permute_step(rom_data[i], 8, 2)
            
            print("0x{:02X}".format(rom_data[i]), end="", file=of)
            if i != len(rom_data)-1 or name_idx != len(infiles)-1:
                print(",", end="", file=of)
                if i&15 == 15:
                    print("\n  ", end="", file=of)
            else:
                print("", file=of)
        
    print("};", file=of)
    of.close()

if len(sys.argv) < 3:
    print("Invalid arguments")
    exit(-1)

if sys.argv[1] == "-p":       
    parse_rom(sys.argv[2], sys.argv[3:-1], sys.argv[-1], True)
elif sys.argv[1] == "-d":       
    parse_rom(sys.argv[2], sys.argv[3:-1], sys.argv[-1], False, True)
else:
    parse_rom(sys.argv[1], sys.argv[2:-1], sys.argv[-1])
