#!/usr/bin/env python3
import sys
import os
import numpy as np

try:
    import imageio.v2 as iio
except:
    print("Unable to import imageio")
    print("try e.g. 'pip3 install imageio'")
    sys.exit(-1)
    
def parse_logo(inname, outname):
    # read image
    img = iio.imread(inname)

    # expect a 224x96 logo
    if len(img) != 96:
        raise ValueError("Logo height mismatch")
    if len(img[0]) != 224:
        raise ValueError("Logo width mismatch")

    colset = set()
    # get colorset
    for row in img:
        for pix in row:
            pix0 = np.divide(pix[0], 255)
            r = np.uint16(np.multiply(31, pix0))
            pix1 = np.divide(pix[1], 255)
            g = np.uint16(np.multiply(63, pix1))
            pix2 = np.divide(pix[2], 255)
            b = np.uint16(np.multiply(31, pix2))
            rgb = (r << 11) + (g << 5) + b
            rgbs = ((rgb & 0xff00) >> 8) + ((rgb & 0xff) << 8)
            colset.add(rgbs)

    # we can cope with up to 255 colors (index 0 to 254)
    print("Colors:", len(colset))
            
    # convert all pixels to 16 bit 565 rgb
    rgb565 = []
    for row in img:
        for pix in row:
            pix0 = np.divide(pix[0], 255)
            r = np.uint16(np.multiply(31, pix0))
            pix1 = np.divide(pix[1], 255)
            g = np.uint16(np.multiply(63, pix1))
            pix2 = np.divide(pix[2], 255)
            b = np.uint16(np.multiply(31, pix2))
            rgb = (r << 11) + (g << 5) + b
            rgbs = ((rgb & 0xff00) >> 8) + ((rgb & 0xff) << 8)
            rgb565.append(rgbs)

    # find a color that is not in this image
    marker = 0
    while marker in colset:
        marker += 1

    print("Marker:", marker)
        
    # do some simple rle encoding
    enc16= [ ]
    c, l = None, 0
    for pix in rgb565:
        if c == pix:    # same pixel as before or stream > 255 pixels
            l += 1
        else:
            if c != None:
                if l == 0:  # just one pixel, encode directly
                    enc16.append(c)
                else:
                    enc16.append(marker)
                    enc16.append(l)
                    enc16.append(c)                
            c = pix
            l = 0

    # encode last pixel
    if l == 0:  # just one pixel, encode directly
        enc16.append(c)
    else:
        enc16.append(marker)
        enc16.append(l)
        enc16.append(c)

    print("Encoded from", 2*len(rgb565), "to", 2*len(enc16))

    c_name = outname.split("/")[-1].split(".")[0]
    if c_name[0].isnumeric(): c_name = "_" + c_name
    
    f = open(outname, "w")
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
    f.close()
    
if len(sys.argv) != 3:
    print("Invalid arguments")
    exit(-1)

parse_logo(sys.argv[1], sys.argv[2])
