#!/usr/bin/env python
import os
import re
from PIL import Image, ImageDraw, ImageFont

# --- CONFIGURAZIONE ---
TILE_DATA_FILE = "../../source/src/machines/bombjack/bombjack_bg_tiles.h"
OUTPUT_IMAGE_FILE = "bombjack_bg_preview.png"
NUM_TILES = 256
TILES_PER_ROW = 16
TILE_WIDTH = 16
TILE_HEIGHT = 16

def parse_c_array_from_file(filename, array_name, num_tiles, tile_height):
    start_line_pattern = f"{array_name}[{num_tiles}][{tile_height * 2}]"
    all_values = []
    in_array = False
    with open(filename, 'r') as f:
        for line in f:
            line = line.strip()
            if start_line_pattern in line:
                in_array = True
                line = line.split('{', 1)[-1]
            if in_array:
                line = re.sub(r'//.*|\/\*.*?\*\/', '', line)
                line = line.replace('{', '').replace('}', '').replace(';', '')
                values_in_line = re.findall(r'0x[0-9a-fA-F]+|\d+', line)
                all_values.extend([int(v, 0) for v in values_in_line])
                if ';' in line: break
    return all_values

def create_tile_preview():
    tile_data_flat = parse_c_array_from_file(TILE_DATA_FILE, "bombjack_bg_tiles", NUM_TILES, TILE_HEIGHT)
    preview_palette = { 0: (0,0,0), 1: (255,85,85), 2: (85,255,85), 3: (255,255,85), 4: (85,85,255), 5: (255,85,255), 6: (85,255,255), 7: (255,255,255) }
    num_rows = (NUM_TILES + TILES_PER_ROW - 1) // TILES_PER_ROW
    cell_w, cell_h = TILE_WIDTH + 4, TILE_HEIGHT + 14
    img = Image.new('RGB', (cell_w * TILES_PER_ROW, cell_h * num_rows), (30, 30, 30))
    draw = ImageDraw.Draw(img)
    try: font = ImageFont.truetype("arial.ttf", 10)
    except IOError: font = ImageFont.load_default()

    for idx in range(NUM_TILES):
        row_idx, col_idx = divmod(idx, TILES_PER_ROW)
        x_offset, y_offset = col_idx * cell_w + 2, row_idx * cell_h + 2
        for y in range(TILE_HEIGHT):
            base_idx = idx * (TILE_HEIGHT * 2) + (y * 2)
            packed_left = tile_data_flat[base_idx]
            packed_right = tile_data_flat[base_idx + 1]
            
            for x in range(8):
                pixel_value = (packed_left >> (3 * (7 - x))) & 0x07
                img.putpixel((x_offset + x, y_offset + y), preview_palette.get(pixel_value, (0,0,0)))
            for x in range(8):
                pixel_value = (packed_right >> (3 * (7 - x))) & 0x07
                img.putpixel((x_offset + 8 + x, y_offset + y), preview_palette.get(pixel_value, (0,0,0)))
        
        draw.text((x_offset, y_offset + TILE_HEIGHT), f"{idx}", font=font, fill=(200, 200, 200))
        draw.rectangle([x_offset -1, y_offset -1, x_offset + TILE_WIDTH, y_offset + TILE_HEIGHT], outline=(80, 80, 80))

    img.save(OUTPUT_IMAGE_FILE)
    print(f"\nImmagine di anteprima salvata come '{OUTPUT_IMAGE_FILE}'")

if __name__ == "__main__":
    create_tile_preview()