#!/usr/bin/env python3
"""
Time Pilot ROM converter for GALAGINO
Konami 1982 - timeplt ROM set

ROM set:
  Program: tm1 (8K), tm2 (8K), tm3 (8K) = 24KB main CPU
  Sound:   tm7 (4K) = sound CPU
  Tiles:   tm6 (8K) = 8x8 chars, 2bpp
  Sprites: tm4 (8K) + tm5 (8K) = 16x16 sprites, 2bpp
  PROMs:   timeplt.b4 (32), timeplt.b5 (32) = palette
           timeplt.e9 (256) = sprite color lookup
           timeplt.e12 (256) = char color lookup
"""

import os, sys

ROM_SRC = os.path.normpath(os.path.join("..", "roms"))
OUT_DIR = os.path.normpath(os.path.join("..", "..", "source", "src", "machines", "timeplt"))

def load_file(name):
    path = os.path.join(ROM_SRC, name)
    if not os.path.exists(path):
        for alt in [name.lower(), name.upper()]:
            alt_path = os.path.join(ROM_SRC, alt)
            if os.path.exists(alt_path):
                path = alt_path
                break
    if not os.path.exists(path):
        print(f"ERRORE: File '{name}' non trovato in {os.path.abspath(ROM_SRC)}")
        return None
    with open(path, "rb") as f:
        return bytearray(f.read())

def hex8(v):
    return "0x{:02X}".format(v & 0xFF)

def hex16(v):
    return "0x{:04X}".format(v & 0xFFFF)

def hex32(v):
    return "0x{:08X}".format(v & 0xFFFFFFFF)

# ---- Color PROM -> RGB565 palette ----
# Time Pilot uses resistor-weighted DAC:
#   timeplt.b4: bits 7-3=BLUE(5 weights), bits 2-0=GREEN high 3 bits
#   timeplt.b5: bits 7-6=GREEN low 2 bits, bits 5-1=RED(5 weights), bit0=unused
# Resistor weights: 1.2K, 820, 560, 470, 390 ohm
# Normalized weights: 0x19, 0x24, 0x35, 0x40, 0x4D
def convert_palette(prom_b4, prom_b5):
    weights = [0x19, 0x24, 0x35, 0x40, 0x4D]
    rgb565 = []
    for i in range(32):
        b4 = prom_b4[i]
        b5 = prom_b5[i]

        # RED from b5 bits 5-1 (5 resistors)
        r = (weights[0] * ((b5 >> 1) & 1) +
             weights[1] * ((b5 >> 2) & 1) +
             weights[2] * ((b5 >> 3) & 1) +
             weights[3] * ((b5 >> 4) & 1) +
             weights[4] * ((b5 >> 5) & 1))

        # GREEN from b5 bits 7-6 (low) + b4 bits 2-0 (high)
        g = (weights[0] * ((b5 >> 6) & 1) +
             weights[1] * ((b5 >> 7) & 1) +
             weights[2] * ((b4 >> 0) & 1) +
             weights[3] * ((b4 >> 1) & 1) +
             weights[4] * ((b4 >> 2) & 1))

        # BLUE from b4 bits 7-3 (5 resistors)
        b = (weights[0] * ((b4 >> 3) & 1) +
             weights[1] * ((b4 >> 4) & 1) +
             weights[2] * ((b4 >> 5) & 1) +
             weights[3] * ((b4 >> 6) & 1) +
             weights[4] * ((b4 >> 7) & 1))

        # Clamp to 255
        r = min(r, 255)
        g = min(g, 255)
        b = min(b, 255)

        # Convert to RGB565 byte-swapped for ESP32 SPI
        r5 = (r >> 3) & 0x1F
        g6 = (g >> 2) & 0x3F
        b5 = (b >> 3) & 0x1F
        val = (r5 << 11) | (g6 << 5) | b5
        rgb565.append(((val & 0xFF) << 8) | ((val >> 8) & 0xFF))
    return rgb565

# ---- Build full palette using lookup PROMs ----
def build_colormap(base_palette, prom_e12, prom_e9):
    """
    Char palettes: 32 palettes x 4 colors = 128 entries
    Sprite palettes: 64 palettes x 4 colors = 256 entries

    prom_e12: char lookup (256 bytes, but only 32*4=128 used)
    prom_e9:  sprite lookup (256 bytes, 64*4=256 used)

    Char colors use base_palette entries 16-31 (prom & 0x0F) + 0x10
    Sprite colors use base_palette entries 0-15 (prom & 0x0F)
    """
    char_colors = []
    for i in range(32 * 4):
        idx = (prom_e12[i] & 0x0F) + 0x10
        char_colors.append(base_palette[idx])

    sprite_colors = []
    for i in range(64 * 4):
        idx = prom_e9[i] & 0x0F
        sprite_colors.append(base_palette[idx])

    return char_colors, sprite_colors

# ---- Tile conversion (8x8, 2bpp) ----
# Konami charlayout (same as Gyruss):
#   planes: { 4, 0 }  (MSB-first: plane[0]=4 is MSB, plane[1]=0 is LSB)
#   xoffset: { 0,1,2,3, 64,65,66,67 }
#   yoffset: { 0*8, 1*8, ..., 7*8 }
#   charsize: 16 bytes per tile
#
# Pixel decode uses Gyruss-identical MSB-first approach (0x80 >> bit)
# PLUS 90 CW rotation because Time Pilot uses tileaddr (Gyruss does NOT)

def convert_tiles(gfx_data):
    num_tiles = len(gfx_data) // 16  # 512 tiles
    tiles = []
    for tile_idx in range(num_tiles):
        base = tile_idx * 16
        # Step 1: decode landscape pixels using Gyruss-identical approach
        landscape = []
        for row in range(8):
            row_pixels = []
            for col in range(8):
                if col < 4:
                    byte = gfx_data[base + row]
                    mame_bit = col
                else:
                    byte = gfx_data[base + 8 + row]
                    mame_bit = col - 4
                p_lsb = 1 if (byte & (0x80 >> mame_bit)) else 0
                p_msb = 1 if (byte & (0x80 >> (mame_bit + 4))) else 0
                pixel = (p_msb << 1) | p_lsb
                row_pixels.append(pixel)
            landscape.append(row_pixels)

        # Step 2: rotate 90 CW for portrait (needed for tileaddr mapping)
        # portrait[py][px] = landscape[7-px][py]
        packed = []
        for py in range(8):
            row_val = 0
            for px in range(8):
                pixel = landscape[7 - px][py]
                row_val |= (pixel << (px * 2))
            packed.append(row_val)
        tiles.append(packed)
    return tiles

# ---- Sprite conversion (16x16, 2bpp) ----
# Gyruss-style MSB-first decode, NO rotation (landscape orientation).
# Transposed rendering at runtime handles the ROT90 display rotation.
# This preserves correct tiling for multi-sprite objects (clouds).

def convert_sprites(gfx_data):
    num_sprites = len(gfx_data) // 64  # 256 sprites
    all_orientations = []

    # Decode all sprites in landscape orientation (NO rotation)
    all_pixels = []
    for s in range(num_sprites):
        base = s * 64
        landscape = []
        for row in range(16):
            if row < 8:
                row_byte_base = row
            else:
                row_byte_base = 24 + row  # rows 8-15 at bytes 32-39
            row_pixels = []
            for col in range(16):
                col_group = col // 4
                bit_in_group = col % 4
                byte_off = row_byte_base + col_group * 8
                byte = gfx_data[base + byte_off]
                p_lsb = 1 if (byte & (0x80 >> bit_in_group)) else 0
                p_msb = 1 if (byte & (0x80 >> (bit_in_group + 4))) else 0
                pixel = (p_msb << 1) | p_lsb
                row_pixels.append(pixel)
            landscape.append(row_pixels)

        # NO rotation — keep landscape pixels as-is
        all_pixels.append(landscape)

    # Generate 4 orientations: [no flip, flipY, flipX, flipXY]
    for flip_x, flip_y in [(False, False), (False, True), (True, False), (True, True)]:
        orientation = []
        for sprite_pixels in all_pixels:
            packed = []
            y_range = list(range(16)) if not flip_y else list(reversed(range(16)))
            for y in y_range:
                val = 0
                for x in range(16):
                    if not flip_x:
                        val |= (sprite_pixels[y][x] << (x * 2))
                    else:
                        val |= (sprite_pixels[y][x] << ((15 - x) * 2))
                packed.append(val)
            orientation.append(packed)
        all_orientations.append(orientation)

    return all_orientations

# ---- Write C header files ----
def write_rom(filename, name, data):
    with open(filename, 'w') as f:
        f.write("// Time Pilot program ROM ({} bytes)\n".format(len(data)))
        f.write("const unsigned char {}[] = {{\n".format(name))
        for i in range(0, len(data), 16):
            line = ", ".join(hex8(data[j]) for j in range(i, min(i+16, len(data))))
            f.write("  " + line)
            if i + 16 < len(data):
                f.write(",")
            f.write("\n")
        f.write("};\n")
    print("Written: {} ({} bytes)".format(filename, len(data)))

def write_sound_rom(filename, name, data):
    with open(filename, 'w') as f:
        f.write("// Time Pilot sound ROM ({} bytes)\n".format(len(data)))
        f.write("const unsigned char {}[] = {{\n".format(name))
        for i in range(0, len(data), 16):
            line = ", ".join(hex8(data[j]) for j in range(i, min(i+16, len(data))))
            f.write("  " + line)
            if i + 16 < len(data):
                f.write(",")
            f.write("\n")
        f.write("};\n")
    print("Written: {} ({} bytes)".format(filename, len(data)))

def write_tilemap(filename, tiles, char_colors):
    with open(filename, 'w') as f:
        f.write("// Time Pilot tilemap: {} tiles, 8x8, 2bpp\n".format(len(tiles)))
        f.write("const unsigned short timeplt_tilemap[][8] = {\n")
        for t, rows in enumerate(tiles):
            f.write("  { " + ", ".join(hex16(r) for r in rows) + " }")
            if t < len(tiles) - 1:
                f.write(",")
            f.write("\n")
        f.write("};\n\n")

        # Char color palettes (32 palettes x 4 colors)
        f.write("// Time Pilot char color palettes: 32 palettes x 4 colors, RGB565 byte-swapped\n")
        f.write("const unsigned short timeplt_char_colormap[][4] = {\n")
        for pal in range(32):
            colors = char_colors[pal*4 : pal*4+4]
            f.write("  { " + ", ".join(hex16(c) for c in colors) + " }")
            if pal < 31:
                f.write(",")
            f.write("  // palette {}\n".format(pal))
        f.write("};\n")
    print("Written: {} ({} tiles, 32 char palettes)".format(filename, len(tiles)))

def write_spritemap(filename, all_orientations, sprite_colors):
    num_sprites = len(all_orientations[0])
    with open(filename, 'w') as f:
        f.write("// Time Pilot spritemap: {} sprites, 16x16, 2bpp, 4 orientations\n".format(num_sprites))
        f.write("const unsigned long timeplt_spritemap[][%d][16] = {\n" % num_sprites)
        for o, sprites in enumerate(all_orientations):
            f.write("  { // orientation %d\n" % o)
            for s, rows in enumerate(sprites):
                f.write("    { " + ", ".join(hex32(r) for r in rows) + " }")
                if s < len(sprites) - 1:
                    f.write(",")
                f.write("\n")
            f.write("  }")
            if o < 3:
                f.write(",")
            f.write("\n")
        f.write("};\n\n")

        # Sprite color palettes (64 palettes x 4 colors)
        f.write("// Time Pilot sprite color palettes: 64 palettes x 4 colors, RGB565 byte-swapped\n")
        f.write("const unsigned short timeplt_sprite_colormap[][4] = {\n")
        for pal in range(64):
            colors = sprite_colors[pal*4 : pal*4+4]
            f.write("  { " + ", ".join(hex16(c) for c in colors) + " }")
            if pal < 63:
                f.write(",")
            f.write("  // palette {}\n".format(pal))
        f.write("};\n")
    print("Written: {} ({} sprites x 4 orientations, 64 sprite palettes)".format(filename, num_sprites))

# ---- Main ----
def main():
    os.makedirs(OUT_DIR, exist_ok=True)

    print(f"Caricamento ROM da: {os.path.abspath(ROM_SRC)}")
    print(f"Destinazione file: {os.path.abspath(OUT_DIR)}")

    # Carica tutti i file
    tm1 = load_file("tm1")
    tm2 = load_file("tm2")
    tm3 = load_file("tm3")
    tm4 = load_file("tm4")
    tm5 = load_file("tm5")
    tm6 = load_file("tm6")
    tm7 = load_file("tm7")
    prom_b4 = load_file("timeplt.b4")
    prom_b5 = load_file("timeplt.b5")
    prom_e9 = load_file("timeplt.e9")
    prom_e12 = load_file("timeplt.e12")

    # Verifica che tutti i file siano stati caricati
    files_ok = all(v is not None for v in [tm1, tm2, tm3, tm4, tm5, tm6, tm7, prom_b4, prom_b5, prom_e9, prom_e12])
    if not files_ok:
        print("ERRORE: Non tutti i file sono stati caricati correttamente.")
        return

    print("Loaded all ROMs from {}".format(ROM_SRC))
    print("  tm1-tm3: {} + {} + {} = {} bytes (main CPU)".format(len(tm1), len(tm2), len(tm3), len(tm1)+len(tm2)+len(tm3)))
    print("  tm7: {} bytes (sound CPU)".format(len(tm7)))
    print("  tm6: {} bytes (tiles)".format(len(tm6)))
    print("  tm4+tm5: {} + {} = {} bytes (sprites)".format(len(tm4), len(tm5), len(tm4)+len(tm5)))

    # Combine program ROM (24KB, pad to 0x6000)
    program = tm1 + tm2 + tm3
    write_rom(os.path.join(OUT_DIR, "timeplt_rom.h"), "timeplt_rom", program)

    # Sound ROM
    write_sound_rom(os.path.join(OUT_DIR, "timeplt_snd_rom.h"), "timeplt_snd_rom", tm7)

    # Convert palette
    base_palette = convert_palette(prom_b4, prom_b5)
    char_colors, sprite_colors = build_colormap(base_palette, prom_e12, prom_e9)

    # Convert tiles (tm6 = 8KB = 512 tiles of 16 bytes each)
    tiles = convert_tiles(tm6)
    write_tilemap(os.path.join(OUT_DIR, "timeplt_tilemap.h"), tiles, char_colors)

    # Convert sprites (tm4+tm5 = 16KB = 256 sprites of 64 bytes each)
    sprite_data = tm4 + tm5
    sprites = convert_sprites(sprite_data)
    write_spritemap(os.path.join(OUT_DIR, "timeplt_spritemap.h"), sprites, sprite_colors)

    # NOTE: timeplt_dipswitches.h is maintained manually, not overwritten here

    print("\n--- OPERAZIONE COMPLETATA ---")
    print(f"Tutti i file sono stati generati in: {os.path.abspath(OUT_DIR)}")

if __name__ == "__main__":
    main()