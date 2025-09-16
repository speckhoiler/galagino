#!/usr/bin/env python3
import sys

def bit_permute_step(x, m, shift):
    t = ((x >> shift) ^ x) & m
    x = (x ^ t) ^ (t << shift)
    return x

def decode_Data(data):
    charmap_data = list(data)
    myiter = iter(range(len(charmap_data)))
    for i in myiter:
        swapbuffer = [None] * 8
        for j in range(8):
            index = bit_permute_step(j, 1, 2);
            #print(i, index)
            swapbuffer[j] = charmap_data[i + index]
        for j in range(8):
            value = bit_permute_step(swapbuffer[j], 16, 2);
            charmap_data[i + j] = value
        for j in range(7):
            next(myiter, None)
    return charmap_data

def show_sprite(data):
    for row in data:
        for pix in row:
            print(" .x*"[pix], end="")
        print("")

def show_sprite_4bpp(data):
    for row in data:
        for pix in row:
            print("  ..--++xx**XX##"[pix], end="")
        print("")

def dump_sprite(data, flip_x, flip_y):
    hexs = [ ]
    
    for y in range(16) if not flip_y else reversed(range(16)):
        val = 0
        for x in range(16):
            if not flip_x:
                val = (val >> 2) + (data[y][x] << (32-2))
            else:
                val = (val << 2) + data[y][x]
        hexs.append(hex(val))

    return ",".join(hexs)
    
def dump_sprite_4bpp(data):
    hexs = [ ]
    
    for y in range(16):
        val = 0
        for x in range(16):
            val = (val >> 4) + (data[y][x] << (64-4))
        hexs.append(hex(val & 0xffffffff))
        hexs.append(hex(val >> 32))

    return ",".join(hexs)
    
def parse_sprite_frogger(data):
    # in frogger D0/D1 of first rom are swapped
    d0 = list(data[0])
    for i in range(len(d0)):
        d0[i] = (d0[i] & 0xfc) | ((d0[i] & 1)<<1) | ((d0[i] & 2)>>1)
    data[0] = bytes(d0)
    
    # frogger has parts of each sprite distributed over both roms
    sprite = []    
    for y in range(16):
        row = [ ]
        for x in range(16):
            ym = y & 7 | ((x & 8) ^ 8)
            xm = x & 7 | (y & 8)
            
            c0 = 1 if data[0][(xm^7) + ((ym & 8) << 1)] & (0x80 >> (ym&7)) else 0
            c1 = 2 if data[1][(xm^7) + ((ym & 8) << 1)] & (0x80 >> (ym&7)) else 0
            row.append(c0+c1)
        sprite.append(row)
    return sprite

def parse_sprite_dkong(data):
    # dkong has parts of each sprite distributed over all four roms
    sprite = []    
    for y in range(16):
        row = [ ]
        for x in range(16):
            c0 = 1 if data[y//8][15-x] & (0x80 >> (y&7)) else 0
            c1 = 2 if data[y//8+2][15-x] & (0x80 >> (y&7)) else 0
            row.append(c0+c1)
        sprite.append(row)
    return sprite

def parse_sprite_1942(data_low, data_high):
    sprite = []    

    for y in range(16):
        row = [ ]
        for x in range(16):
            byte = 2*x + (((15-y)&4)>>2) + (((15-y)&8)<<2)
            bit = (15-y)&3

            c0 = 1 if data_low[byte] & (0x80 >> bit) else 0
            c1 = 2 if data_low[byte] & (0x08 >> bit) else 0
            c2 = 4 if data_high[byte] & (0x80 >> bit) else 0
            c3 = 8 if data_high[byte] & (0x08 >> bit) else 0
            row.append(c0 + c1 + c2 + c3)
        sprite.append(row)

    return sprite

def parse_sprite(data, pacman_fmt, decode):
    # the pacman sprite format differs from the galaga
    # one. The top 4 pixels are in fact the bottom four
    # ones for pacman
    if decode:
     data = decode_Data(data)
 
    # sprites are 16x16 pixels
    sprite = []    
    for y in range(16):
        row = []
        for x in range(16):
            idx = ((y&8)<<1) + (((x&8)^8)<<2) + (7-(x&7)) + 2*(y&4)
            c0 = 1 if data[idx] & (0x08 >> (y&3)) else 0
            c1 = 2 if data[idx] & (0x80 >> (y&3)) else 0
            row.append(c0+c1)
        sprite.append(row)

    if pacman_fmt:
        sprite = sprite[4:] + sprite[:4]
    return sprite

def dump_c_source(sprites, flip_x, flip_y, f):
    # write as c source
    print(" {" ,file=f)
    sprites_str = []
    for s in sprites:
        sprites_str.append("  { " + dump_sprite(s, flip_x, flip_y) + " }")
    print(",\n".join(sprites_str), file=f)
    if flip_x and flip_y: print(" }", file=f)
    else:                 print(" },", file=f)

def dump_c_source_4bpp(sprites, f):
    # write as c source
    sprites_str = []
    for s in sprites:
        sprites_str.append("  { " + dump_sprite_4bpp(s) + " }")
    print(",\n".join(sprites_str), file=f)

def parse_spritemap(id, fmt, infiles, outfile):
    sprites = []

    if fmt == "frogger":
        # frogger uses the tilemap roms for sprites as well
        spritemap_data = []
        for file in infiles:        
            f = open(file, "rb")
            spritemap_data.append(f.read())
            f.close()
            
            if len(spritemap_data[-1]) != 2048:
                raise ValueError("Missing spritemap data")

        # most of these aren't sprites but tiles. Converting them all
        # won't hurt as flash memory is no the limit
        for sprite in range(64):
            data = []
            for i in range(2):
                data.append(spritemap_data[i][32*sprite:32*(sprite+1)])
            
            sprites.append(parse_sprite_frogger(data))

    # pacman, galaga and digdug, eyes
    elif fmt == "pacman" or fmt == "galaga" or fmt == "digdug" or fmt == "eyes" or fmt == "lizwiz" or fmt == "mrtnt":
        for name in infiles:    
            f = open(name, "rb")
            spritemap_data = f.read()
            f.close()

            if len(spritemap_data) != 4096:
                raise ValueError("Missing spritemap data")

            # read and parse all 64 sprites
            for sprite in range(64):
                sprites.append(parse_sprite(spritemap_data[64*sprite:64*(sprite+1)], fmt == "pacman" or fmt == "eyes" or fmt == "lizwiz" or fmt == "mrtnt", fmt == "eyes" or fmt == "mrtnt"))
            #for s in range(len(sprites)): 
            #   print(s)
            #   show_sprite(sprites[s])
                
    elif fmt == "1942":
        for i in range(len(infiles)//2):
            # 1942 uses 4bpp sprites
            # roms are used in pairs
            f = open(infiles[i], "rb")
            spritemap_data_low = f.read()
            f.close()
            
            f = open(infiles[i+len(infiles)//2], "rb")
            spritemap_data_high = f.read()
            f.close()

            # read and parse all 256 sprites
            for sprite in range(256):
                sprites.append(parse_sprite_1942(
                    spritemap_data_low[64*sprite:64*(sprite+1)],
                    spritemap_data_high[64*sprite:64*(sprite+1)]
                ))
                
            # for s in range(len(sprites)): print(s); show_sprite_4bpp(sprites[s])
    
    else: # dkong
        spritemap_data = []
        for file in infiles:        
            f = open(file, "rb")
            spritemap_data.append(f.read())
            f.close()
            
            if len(spritemap_data[-1]) != 2048:
                raise ValueError("Missing spritemap data")
            
        for sprite in range(128):
            data = []
            for i in range(4):
                data.append(spritemap_data[i][16*sprite:16*(sprite+1)])
            
            sprites.append(parse_sprite_dkong(data))

    f=open(outfile, "w")

    if fmt == "1942":
        # write 4 bpp
        print("const unsigned long "+id+"[][32] = {", file=f)    
        dump_c_source_4bpp(sprites, f)
    else:
        # write 2 bpp    
        print("const unsigned long "+id+"[]["+str(len(sprites))+"][16] = {", file=f)    
        dump_c_source(sprites, False, False, f)
    
        # we have plenty of flash space, so we simply pre-compute x/y flipped
        # versions of all sprites
        dump_c_source(sprites, False,  True, f)
        dump_c_source(sprites,  True, False, f)
        dump_c_source(sprites,  True,  True, f)
        
    print("};", file=f)

if len(sys.argv) < 5:
    print("Invalid arguments")
    exit(-1)

if sys.argv[1] == "-d":       
    parse_spritemap(sys.argv[2], sys.argv[3], sys.argv[4:-1], sys.argv[-1], True)
else:
    parse_spritemap(sys.argv[1], sys.argv[2], sys.argv[3:-1], sys.argv[-1])

