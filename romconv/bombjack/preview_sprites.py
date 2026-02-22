#!/usr/bin/env python
import os
import re
from PIL import Image, ImageDraw, ImageFont

# --- CONFIGURAZIONE ---
SPRITE_DATA_FILE = "../../source/src/machines/bombjack/bombjack_sprites.h"
OUTPUT_IMAGE_16 = "bombjack_sprites_16_preview.png"
OUTPUT_IMAGE_32 = "bombjack_sprites_32_preview.png"

def parse_c_array(filename, array_name):
    """Legge un array C e restituisce una lista flat di valori interi."""
    print(f"Parsing di '{filename}' per l'array '{array_name}'...")
    if not os.path.exists(filename):
        raise FileNotFoundError(f"File non trovato: {filename}")
    
    with open(filename, 'r') as f:
        content = f.read()
    
    match = re.search(re.escape(array_name) + r'\[.*?\]\[.*?\]\s*=\s*\{', content)
    if not match:
        raise ValueError(f"Array '{array_name}' non trovato in '{filename}'")
    
    content = content[match.end():]
    brace_level = 1
    array_content = ""
    for char in content:
        if char == '{': brace_level += 1
        elif char == '}': brace_level -= 1
        if brace_level == 0: break
        array_content += char
    
    array_content = re.sub(r'//.*|\/\*.*?\*\/', '', array_content)
    values = re.findall(r'0x[0-9a-fA-F]+|\d+', array_content)
    return [int(v, 0) for v in values]

def create_preview(data_flat, num_sprites, width, height, tiles_per_row, output_file):
    """Genera un'immagine di anteprima per un set di sprite."""
    preview_palette = { 0: (0,0,0), 1: (255,85,85), 2: (85,255,85), 3: (255,255,85), 4: (85,85,255), 5: (255,85,255), 6: (85,255,255), 7: (255,255,255) }
    num_rows = (num_sprites + tiles_per_row - 1) // tiles_per_row
    cell_w, cell_h = width + 4, height + 14
    
    img = Image.new('RGB', (cell_w * tiles_per_row, cell_h * num_rows), (30, 30, 30))
    draw = ImageDraw.Draw(img)
    try: font = ImageFont.truetype("arial.ttf", 10)
    except IOError: font = ImageFont.load_default()

    print(f"Disegno di {num_sprites} sprite {width}x{height}...")
    
    # Calcola quanti 'long' compongono uno sprite intero
    longs_per_sprite = (width * height) // 8

    for idx in range(num_sprites):
        row_idx, col_idx = divmod(idx, tiles_per_row)
        x_offset, y_offset = col_idx * cell_w + 2, row_idx * cell_h + 2
        
        # Estrai i dati per il singolo sprite corrente
        sprite_data = data_flat[idx * longs_per_sprite : (idx + 1) * longs_per_sprite]
        
        pixel_idx = 0
        for y in range(height):
            for x in range(width):
                # Determina in quale 'long' e in quale bit si trova il pixel
                long_index = pixel_idx // 8
                bit_offset_in_long = pixel_idx % 8
                
                packed_pixels = sprite_data[long_index]
                
                # Estrai il pixel
                pen = (packed_pixels >> (3 * (7 - bit_offset_in_long))) & 0x07
                
                # Disegna il pixel
                rgb_color = preview_palette.get(pen, (0, 0, 0))
                img.putpixel((x_offset + x, y_offset + y), rgb_color)
                
                pixel_idx += 1
        
        draw.text((x_offset, y_offset + height), f"{idx}", font=font, fill=(200, 200, 200))
        draw.rectangle([x_offset -1, y_offset -1, x_offset + width, y_offset + height], outline=(80, 80, 80))

    img.save(output_file)
    print(f"Immagine di anteprima salvata come '{output_file}'")

def main():
    # Processa sprite 16x16
    data_16 = parse_c_array(SPRITE_DATA_FILE, "bombjack_sprites_16x16")
    create_preview(data_16, 256, 16, 16, 16, OUTPUT_IMAGE_16)

    print("-" * 20)

    # Processa sprite 32x32
    data_32 = parse_c_array(SPRITE_DATA_FILE, "bombjack_sprites_32x32")
    create_preview(data_32, 64, 32, 32, 8, OUTPUT_IMAGE_32)

if __name__ == "__main__":
    main()