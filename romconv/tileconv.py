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
def show_chr(data):
    for row in data:
        for pix in row:
            print(" .x*"[pix], end="")
        print("")

def show_chr_3bpp(data):
    # ascii display for 3bpp (8 values)
    for row in data:
        for pix in row:
            print(" .-+x*X#"[pix], end="")
        print("")

def dump_chr(data):
    hexs = [ ]
    
    for y in range(8):
        val = 0
        for x in range(8):
            val = (val >> 2) + (data[y][x] << (16-2))
        hexs.append(hex(val))

    return ",".join(hexs)

# 1942 can flip background tiles
def dump_tile_1942(data, hflip=False, vflip=False):
    hexs = [ ]
    
    for my in range(16):
        y = my if not vflip else 15-my
        val = 0
        for mx in range(16):
            x = mx if not hflip else 15-mx            
            val = (val >> 4) + (data[y][x] << (64-4))
        hexs.append(hex(val & 0xffffffff))
        hexs.append(hex(val >> 32))

    return ",".join(hexs)
    
def parse_chr_2(data0, data1):
    # characters are 8x8 pixels
    char = []    
    for y in range(8):
        row = []
        for x in range(8):
            c0 = 1 if data0[7-x] & (0x80>>y) else 0
            c1 = 2 if data1[7-x] & (0x80>>y) else 0
            row.append(c0 + c1)
        char.append(row)
    return char

def parse_chr(data):
    # characters are 8x8 pixels
    char = []    
    for y in range(8):
        row = []
        for x in range(8):
            byte = data[15 - x - 2*(y&4)]
            c0 = 1 if byte & (0x08 >> (y&3)) else 0
            c1 = 2 if byte & (0x80 >> (y&3)) else 0
            row.append(c0+c1)
        char.append(row)
    return char
            
def parse_chr_dd(data):
    # characters are 8x8 pixels
    char = []    
    for y in range(8):
        row = []
        for x in range(8):
            byte = data[7 - x]
            c = 3 if byte & (0x01 << y) else 0
            row.append(c)
        char.append(row)
    return char

def parse_chr_1942(data):
    # characters are 8x8 pixels
    char = []    
    for y in range(8):
        row = []
        for x in range(8):
            if y < 4: byte = data[2*x+1]
            else:     byte = data[2*x]
            
            c0 = 1 if byte & (0x10 << (y&3)) else 0
            c1 = 2 if byte & (0x01 << (y&3)) else 0
            row.append(c0+c1)
        char.append(row)
    return char
            
def parse_charmap(id, inname, outname):
    # The character map rom contains the same set of 128 characters
    # two times. The second set is upside down for cocktail mode. We
    # ignore that.

    f = open(inname, "rb")
    charmap_data = f.read()
    f.close()

    chars = []
    if len(charmap_data) == 8192:
        # 1942 2bpp format
        for chr in range(512):
            chars.append(parse_chr_1942(charmap_data[16*chr:16*(chr+1)]))
        
    elif len(charmap_data) == 4096:
        # galaga and pacman 2bpp format

        if id == "eyes_tilemap" or id == "mrtnt_tilemap":    
         charmap_data = decode_Data(charmap_data)
        
        # read and parse all 256 characters
        for chr in range(256):
            chars.append(parse_chr(charmap_data[16*chr:16*(chr+1)]))

    else:
        # digdug format
        for chr in range(128):
            chars.append(parse_chr_dd(charmap_data[8*chr:8*(chr+1)]))
            
    #for c in chars: show_chr(c)

    # write as c source
    f = open(outname, "w")
    
    print("const unsigned short "+id+"[][8] = {", file=f )
    chars_str = []
    for c in chars:
        chars_str.append(" { " + dump_chr(c) + " }")
    print(",\n".join(chars_str), file=f)
    print("};", file=f)
    
    f.close()

def parse_tile_1942(b0,b1,b2):
    # 1942 tiles are 16*16 pixel
    tile = []    
    for y in range(16):
        row = []
        for x in range(16):
            bit = 1<<(y&7)
            byte = x+(0 if (y//8) else 16)
            p0 = 4 if (b0[byte] & bit) else 0
            p1 = 2 if (b1[byte] & bit) else 0
            p2 = 1 if (b2[byte] & bit) else 0
            row.append(p0 + p1 + p2)
        tile.append(row)
        
    # show_chr_3bpp(tile)
    return tile
    
def parse_tilemap_1942(id, files, outname):
    # in 1942 the tiles use 3 bit color indices with one bit in a serperate file each
    data = [ [],[],[] ]
    for fidx in range(len(files)):
        for fname in files[fidx]:        
            f = open(fname, "rb")
            data[fidx].extend(f.read())
            f.close()

    tiles = [ ]
            
    # we now have 3 * 16k bytes = 128k pixel @ 3bpp. Each tiles has 16x16 = 256 pixel
    # Thus there are 512 tiles
    for tidx in range(len(data[0]) * 8 // 256):
        # data for all three color index bits
        p0 = data[0][tidx*32:32+tidx*32]
        p1 = data[1][tidx*32:32+tidx*32]
        p2 = data[2][tidx*32:32+tidx*32]

        tiles.append(parse_tile_1942(p0,p1,p2))

    # write as c source
    f = open(outname, "w")
    
    print("const unsigned long "+id+"[]["+str(len(tiles))+"][32] = {", file=f )
    tiles_maps_str = []
    for xflip in [ False, True ]:
        for yflip in [ False, True ]:
            tiles_str = []
            for t in tiles:
                tiles_str.append(" { " + dump_tile_1942(t,xflip,yflip) + " }")
            tiles_maps_str.append("{\n" + ",\n".join(tiles_str) +"\n}")
    print(",\n".join(tiles_maps_str), file=f)
    print("};", file=f)
    
    f.close()
        
def parse_charmap_2(id, innames, outname):

    # swap bits 0 and 1 in an integer
    def bit01_swap(a):
        return (a & 0xfffc) | ((a & 2)>>1) | ((a & 1)<<1) 
    
    f = open(innames[0], "rb")
    charmap_data_0 = f.read()
    f.close()
    
    f = open(innames[1], "rb")
    charmap_data_1 = f.read()
    f.close()

    if len(charmap_data_0) != 2048 or len(charmap_data_1) != 2048:
        raise ValueError("Missing charmap data")

    # in frogger the second gfx rom has D0 and D1 swapped ... wtf ...
    # check first 16 bytes / two characters as they should be bitswapped identical in frogger
    d01_swap = True
    for i in range(16):
        if bit01_swap(charmap_data_0[i]) != charmap_data_1[i]:
            d01_swap = False

    if d01_swap:
        charmap_data_0 = list(charmap_data_0)
        for i in range(len(charmap_data_0)):
            charmap_data_0[i] = bit01_swap(charmap_data_0[i])
        charmap_data_0 = bytes(charmap_data_0)
    
    # read and parse all 256 characters
    chars = []
    for chr in range(256):
        chars.append(parse_chr_2(charmap_data_0[8*chr:8*(chr+1)],
                     charmap_data_1[8*chr:8*(chr+1)]))

#    for c in range(len(chars)):
#        print("---", c)
#        show_chr(chars[c])
#     for c in chars: show_chr(c)

    # write as c source
    f = open(outname, "w")
    
    print("const unsigned short "+id+"[][8] = {", file=f )
    chars_str = []
    for c in chars:
        chars_str.append(" { " + dump_chr(c) + " }")
    print(",\n".join(chars_str), file=f)
    print("};", file=f)
    
    f.close()

if len(sys.argv) != 4 and len(sys.argv) != 5 and len(sys.argv) != 9:
    print("Invalid arguments")
    exit(-1)

if len(sys.argv) == 9:
    # 6 files are 1942 tiles which have a very different format
    parse_tilemap_1942(sys.argv[1], (sys.argv[2:4], sys.argv[4:6], sys.argv[6:8] ), sys.argv[8])
elif len(sys.argv) == 4:
    parse_charmap(sys.argv[1], sys.argv[2], sys.argv[3])
else:
    # len == 5
    parse_charmap_2(sys.argv[1], sys.argv[2:4], sys.argv[4])
