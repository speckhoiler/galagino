#!/usr/bin/env python3
import sys
import os
import numpy as np

try:
    import imageio.v2 as iio
except ImportError:
    try:
        import imageio as iio
    except ImportError:
        print("Unable to import imageio")
        print("try e.g. 'pip3 install imageio'")
        sys.exit(-1)
    
def parse_logo(inname, outname):
    # read image
    img = iio.imread(inname)

    # If it's RGBA, discard alpha channel
    if img.ndim == 3 and img.shape[2] == 4:
        img = img[:, :, :3]

    # expect a 224x96 logo
    if img.shape[0] != 96:
        raise ValueError("Logo height mismatch (expected 96, got {})".format(img.shape[0]))
    if img.shape[1] != 224:
        raise ValueError("Logo width mismatch (expected 224, got {})".format(img.shape[1]))

    colset = set()
    # get colorset
    # convert all pixels to 16 bit 565 rgb
    rgb565 = []
    for row in img:
        for pix in row:
            # Cast to int to prevent overflow during multiplication
            r8, g8, b8 = int(pix[0]), int(pix[1]), int(pix[2])
            
            # Scale 8-bit color components to 5-6-5 bits
            r = np.uint16((r8 * 31) // 255)
            g = np.uint16((g8 * 63) // 255)
            b = np.uint16((b8 * 31) // 255)
            
            rgb = (r << 11) + (g << 5) + b
            # Swap bytes for little-endian storage (as sent over SPI)
            rgbs = ((rgb & 0xff00) >> 8) | ((rgb & 0xff) << 8)
            rgb565.append(rgbs)
            colset.add(rgbs)

    # find a color that is not in this image to use as RLE marker
    marker = 0
    while marker in colset:
        marker += 1
        if marker > 0xFFFF:
            raise ValueError("Too many colors in image, cannot find a marker!")

    print("Colors:", len(colset))
    print("Marker:", marker)
        
    # do some simple rle encoding
    enc16 = []
    c, l = None, 0
    for pix in rgb565:
        if c == pix:
            l += 1
            if l >= 0xFFFF: # Max value for unsigned short
                enc16.append(marker)
                enc16.append(l-1)
                enc16.append(c)
                l = 0
                c = None
        else:
            if c is not None:
                if l == 0:  # just one pixel, encode directly
                    enc16.append(c)
                else:
                    enc16.append(marker)
                    enc16.append(l)
                    enc16.append(c)                
            c = pix
            l = 0

    # encode last pixel
    if c is not None:
        if l == 0:  # just one pixel, encode directly
            enc16.append(c)
        else:
            enc16.append(marker)
            enc16.append(l)
            enc16.append(c)

    print("Encoded from", 2*len(rgb565), "to", 2*len(enc16))

    # Use os.path.basename and replace dots for valid C variable names
    base = os.path.basename(outname)
    c_name = os.path.splitext(base)[0]
    if c_name[0].isnumeric(): c_name = "_" + c_name
    
    with open(outname, "w") as f:
        print("const unsigned short "+c_name+"[] = {", file=f )
        # write marker
        print(str(marker)+",",file=f)
        # write data
        for i in range(len(enc16)):
            print(enc16[i], end="", file=f)
            if i != len(enc16)-1:
                print(", ", end="", file=f)
            if i % 16 == 15:
                print("", file=f)            
        
        print("};", file=f )
    
if len(sys.argv) != 3:
    print("Usage: logoconv.py <input.png> <output.h>")
    sys.exit(-1)

parse_logo(sys.argv[1], sys.argv[2])
