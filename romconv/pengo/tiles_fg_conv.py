#!/usr/bin/env python

import sys
from PIL import Image, ImageDraw, ImageFont

# -----------------------------------------------------------------------------
# CONFIGURAZIONE
# -----------------------------------------------------------------------------
ROM_BANCO1_FILENAME = "../roms/ep1640.92"
ROM_BANCO2_FILENAME = "../roms/ep1695.105"
OUTPUT_TILES_C_FILE = "../../source/src/machines/pengo/pengo_tiles.h"
OUTPUT_TILES_PREVIEW = "pengo_tiles_preview.png"

# -----------------------------------------------------------------------------
# FUNZIONI DI DECODIFICA E MANIPOLAZIONE
# -----------------------------------------------------------------------------

def decode_tile_8x8(raw_tile_data):
    """Decodifica un tile 8x8. QUESTA LOGICA È CONFERMATA CORRETTA."""
    tile_data_2d = [[0] * 8 for _ in range(8)]
    for y in range(8):
        byte_left = raw_tile_data[y + 8]
        byte_right = raw_tile_data[y]
        plane0_left = byte_left & 0x0F
        plane1_left = (byte_left >> 4) & 0x0F
        plane0_right = byte_right & 0x0F
        plane1_right = (byte_right >> 4) & 0x0F
        for i in range(4):
            bit0_l = (plane0_left >> i) & 1; bit1_l = (plane1_left >> i) & 1
            tile_data_2d[y][3 - i] = (bit1_l << 1) | bit0_l
            bit0_r = (plane0_right >> i) & 1; bit1_r = (plane1_right >> i) & 1
            tile_data_2d[y][7 - i] = (bit1_r << 1) | bit0_r
    return tile_data_2d

def decode_sprite_16x16(raw_sprite_data):
    """
    Decodifica uno sprite 16x16 basandosi fedelmente sulla spritelayout di MAME.
    """
    sprite_2d = [[0] * 16 for _ in range(16)]
    
    # Mappature X e Y dalla spritelayout di MAME
    xoffset = [ 8*8,  8*8+1,  8*8+2,  8*8+3, 16*8+0, 16*8+1, 16*8+2, 16*8+3,
               24*8+0, 24*8+1, 24*8+2, 24*8+3, 0, 1, 2, 3 ]
    yoffset = [ 0*8,  1*8,  2*8,  3*8,  4*8,  5*8,  6*8,  7*8,
               32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 ]
    
    for y in range(16):
        for x in range(16):
            # Calcola l'offset del bit di base all'interno del blocco di 64 byte
            bit_offset = yoffset[y] + xoffset[x]
            
            byte_index = bit_offset // 8
            bit_in_byte = bit_offset % 8
            
            byte_val = raw_sprite_data[byte_index]
            
            # Estrae i due bit di colore (bitplane 0 e 1)
            # { 0, 4 } significa che sono nello stesso byte a 4 bit di distanza
            c0 = 1 if (byte_val & (1 << bit_in_byte)) else 0
            c1 = 1 if (byte_val & (1 << (bit_in_byte + 4))) else 0
            
            color = (c1 << 1) | c0
            sprite_2d[y][x] = color
            
    return sprite_2d

def rotate_gfx(gfx_data, width, height):
    """Ruota un elemento grafico di 90 gradi in senso orario."""
    rotated = [[0] * width for _ in range(height)]
    for y in range(height):
        for x in range(width):
            rotated[x][(height - 1) - y] = gfx_data[y][x]
    return rotated

def dump_tile_packed(data):
    """Impacchetta un tile 8x8 per il C."""
    hexs = []; val = 0
    for y in range(8):
        val = 0
        for x in range(8): val = (val >> 2) | (data[y][x] << 14)
        hexs.append(hex(val))
    return ",".join(hexs)

def dump_sprite_packed(sprite_data):
    """Impacchetta uno sprite 16x16 per il C."""
    hexs = []; val = 0
    for y in range(16):
        val = 0
        for x in range(16): val = (val >> 2) | (sprite_data[y][x] << 30)
        hexs.append(hex(val))
    return ",".join(hexs)

def flip_sprite(sprite_data, flip_x, flip_y):
    flipped = [[0]*16 for _ in range(16)]
    for y in range(16):
        for x in range(16):
            src_x = 15 - x if flip_x else x; src_y = 15 - y if flip_y else y
            flipped[y][x] = sprite_data[src_y][src_x]
    return flipped

def create_preview(gfx_list, width, height, output_filename):
    print(f"Creazione anteprima: {output_filename}...")
    # ... (Codice per l'anteprima) ...
    num_elements = len(gfx_list)
    grid_cols = 32 if width == 8 else 16
    grid_rows = (num_elements + grid_cols - 1) // grid_cols
    palette = [(0, 0, 0), (255, 85, 85), (85, 255, 85), (255, 255, 255)]
    scale = 2
    cell_w = width * scale + 4; cell_h = height * scale + 14
    img = Image.new('RGB', (grid_cols * cell_w, grid_rows * cell_h), (60, 60, 80))
    draw = ImageDraw.Draw(img)
    try: font = ImageFont.truetype("arial.ttf", 10)
    except IOError: font = ImageFont.load_default()
    for i, gfx in enumerate(gfx_list):
        gx, gy = i % grid_cols, i // grid_cols
        ox, oy = gx * cell_w, gy * cell_h
        for y in range(height):
            for x in range(width):
                color = gfx[y][x]
                draw.rectangle((ox + x * scale, oy + y * scale, ox + (x+1)*scale -1, oy + (y+1)*scale -1), fill=palette[color])
        text = str(i); text_bbox = draw.textbbox((0, 0), text, font=font); text_w = text_bbox[2] - text_bbox[0]
        draw.text((ox + (cell_w - text_w)/2, oy + height * scale + 1), text, fill=(220, 220, 220), font=font)
    img.save(output_filename)

# -----------------------------------------------------------------------------
# FUNZIONE PRINCIPALE
# -----------------------------------------------------------------------------
def main():
    try:
        with open(ROM_BANCO1_FILENAME, "rb") as f: rom1 = f.read()
        with open(ROM_BANCO2_FILENAME, "rb") as f: rom2 = f.read()
    except Exception as e:
        print(f"ERRORE di lettura file: {e}"); sys.exit(1)

    # --- 1. PROCESSIAMO I TILE ---
    print("--- Inizio elaborazione TILE ---")
    tile_data_raw = rom1[:4096] + rom2[:4096]
    decoded_tiles = [decode_tile_8x8(tile_data_raw[i*16:(i+1)*16]) for i in range(512)]
    final_tiles = [rotate_gfx(t, 8, 8) for t in decoded_tiles]
    with open(OUTPUT_TILES_C_FILE, "w") as f:
        f.write("const unsigned short pengo_tiles[512][8] = {\n")
        f.write(",\n".join([f"  /* T{i} */ {{ {dump_tile_packed(t)} }}" for i, t in enumerate(final_tiles)]))
        f.write("\n};\n")
    create_preview(final_tiles, 8, 8, OUTPUT_TILES_PREVIEW)
    print("--- Elaborazione TILE completata ---\n")

if __name__ == "__main__":
    main()