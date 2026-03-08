#!/usr/bin/env python

import sys
from PIL import Image, ImageDraw, ImageFont

# -----------------------------------------------------------------------------
# CONFIGURAZIONE
# -----------------------------------------------------------------------------
ROM_BANCO1_FILENAME = "../roms/ep1640.92"
ROM_BANCO2_FILENAME = "../roms/ep1695.105"
OUTPUT_C_FILE = "../../source/src/machines/pengo/pengo_spritemap.h"
OUTPUT_PREVIEW = "pengo_sprites_preview.png"

# -----------------------------------------------------------------------------
# FUNZIONI DI DECODIFICA E MANIPOLAZIONE
# -----------------------------------------------------------------------------

def decode_sprite_16x16_from_mame(raw_sprite_data):
    """
    Traduzione diretta dell'algoritmo di MAME, CON LA CORREZIONE
    SULL'ORDINE DEI BITPLANE per i colori di Pengo.
    """
    sprite_2d = [[0] * 16 for _ in range(16)]
    
    plane_offsets = [0, 4]
    x_offsets = [ 8*8, 8*8+1, 8*8+2, 8*8+3, 16*8+0, 16*8+1, 16*8+2, 16*8+3,
                 24*8+0, 24*8+1, 24*8+2, 24*8+3, 0, 1, 2, 3 ]
    y_offsets = [ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
                 32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 ]

    for y in range(16):
        for x in range(16):
            color = 0
            for plane in range(2):
                bit_offset = y_offsets[y] + x_offsets[x]
                plane_bit_offset = bit_offset + plane_offsets[plane]
                byte_index = plane_bit_offset // 8
                bit_index = 7 - (plane_bit_offset % 8)
                
                if (raw_sprite_data[byte_index] >> bit_index) & 1:
                    # --- CORREZIONE CHIAVE QUI ---
                    # Invertiamo l'ordine di assegnazione dei bit.
                    # Il primo bit (plane 0) diventa il bit 1 del colore (valore 2).
                    # Il secondo bit (plane 1) diventa il bit 0 del colore (valore 1).
                    color |= (1 << (1 - plane))
            
            sprite_2d[y][x] = color
            
    return sprite_2d

# ... (tutto il resto dello script, da rotate_gfx in poi, rimane identico) ...

def rotate_gfx(gfx_data, width, height):
    rotated = [[0] * width for _ in range(height)]
    for y in range(height):
        for x in range(width):
            rotated[x][(height - 1) - y] = gfx_data[y][x]
    return rotated

def flip_sprite(sprite_data, flip_x, flip_y):
    flipped = [[0]*16 for _ in range(16)]
    for y in range(16):
        for x in range(16):
            src_x = 15 - x if flip_x else x; src_y = 15 - y if flip_y else y
            flipped[y][x] = sprite_data[src_y][src_x]
    return flipped

def dump_sprite_packed(sprite_data):
    hexs = []; val = 0
    for y in range(16):
        val = 0
        for x in range(16): val = (val >> 2) | (sprite_data[y][x] << 30)
        hexs.append(hex(val))
    return ",".join(hexs)

def create_preview(gfx_list, width, height, output_filename):
    print(f"Creazione anteprima: {output_filename}...")
    num_elements = len(gfx_list)
    grid_cols = 16
    grid_rows = (num_elements + grid_cols - 1) // grid_cols
    # Corretto anche qui per coerenza
    palette = [(0, 0, 0), (85, 255, 85), (255, 85, 85), (255, 255, 255)]
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

def main():
    try:
        with open(ROM_BANCO1_FILENAME, "rb") as f: rom1 = f.read()
        with open(ROM_BANCO2_FILENAME, "rb") as f: rom2 = f.read()
    except Exception as e:
        print(f"ERRORE di lettura file: {e}"); sys.exit(1)
    print("--- Inizio elaborazione SPRITE ---")
    sprite_data_raw = rom1[4096:] + rom2[4096:]
    num_sprites = len(sprite_data_raw) // 64
    print(f"Dati combinati, processando {num_sprites} sprite con la logica di MAME corretta...")
    decoded_sprites = [decode_sprite_16x16_from_mame(sprite_data_raw[i*64:(i+1)*64]) for i in range(num_sprites)]
    print("Rotazione degli sprite...")
    final_sprites_unflipped = [rotate_gfx(s, 16, 16) for s in decoded_sprites]
    print(f"Scrittura file C per gli sprite: {OUTPUT_C_FILE}...")
    with open(OUTPUT_C_FILE, "w") as f:
        f.write("const unsigned long pengo_sprites[2][4][64][16] = {\n")
        bank_lines = []
        for bank in range(2):
            flip_lines = []
            for flip_val in range(4):
                flip_y = (flip_val & 1) != 0; flip_x = (flip_val & 2) != 0
                sprite_lines = []
                for i in range(64):
                    sprite_idx = bank * 64 + i
                    flipped = flip_sprite(final_sprites_unflipped[sprite_idx], flip_x, flip_y)
                    sprite_lines.append(f"    /* S{i} F{flip_val} */ {{ {dump_sprite_packed(flipped)} }}")
                flip_lines.append("  {\n" + ",\n".join(sprite_lines) + "\n  }")
            bank_lines.append(" {\n" + ",\n".join(flip_lines) + "\n }")
        f.write(",\n".join(bank_lines) + "\n};\n")
    create_preview(final_sprites_unflipped, 16, 16, OUTPUT_PREVIEW)
    print("--- Elaborazione SPRITE completata ---")

if __name__ == "__main__":
    main()