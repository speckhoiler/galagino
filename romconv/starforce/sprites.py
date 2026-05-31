import os
try:
    from PIL import Image, ImageDraw, ImageFont
    PIL_AVAILABLE = True
except ImportError:
    PIL_AVAILABLE = False

# --- Configurazione ---
ROM_FILES = ["../roms/6.10lm", "../roms/5.9lm", "../roms/4.8lm"]
ROM_FILE_SIZE = 16384
OUTPUT_C_FILE = "../../source/src/machines/starforce/starforce_sprites.h"
ROTATE_TILES = True
GENERATE_PREVIEW = True
BPP = 3

# Parametri Sprite 16x16
NUM_SPRITES_16, SPRITE_WIDTH_16, SPRITE_HEIGHT_16 = 512, 16, 16
C_ARRAY_NAME_16 = "starforce_sprites_16x16"
PREVIEW_PNG_16 = "starforce_sprites_16x16_preview.png"

# Parametri Sprite 32x32
NUM_SPRITES_32, SPRITE_WIDTH_32, SPRITE_HEIGHT_32 = 128, 32, 32
C_ARRAY_NAME_32 = "starforce_sprites_32x32"
PREVIEW_PNG_32 = "starforce_sprites_32x32_preview.png"

def rotate_matrix_90_cw(matrix, width, height):
    new_matrix = [[0] * height for _ in range(width)]
    for y in range(height):
        for x in range(width):
            new_matrix[x][(height - 1) - y] = matrix[y][x]
    return new_matrix

def decode_gfx_correct(rom_data, num_sprites, width, height, layout_x, layout_y, bytes_per_sprite_plane):
    print(f"Inizio decodifica per {num_sprites} sprite di {width}x{height}...")
    plane_size = len(rom_data) // BPP
    plane_offsets = [i * plane_size for i in range(BPP)]
    decoded_sprites = []

    for i in range(num_sprites):
        sprite_data = [[0] * width for _ in range(height)]
        char_base_offset = i * bytes_per_sprite_plane

        for y in range(height):
            for x in range(width):
                pixel_value = 0
                bit_offset = layout_y[y] + layout_x[x]
                
                # ================================================================
                # --- INIZIO CORREZIONE LOGICA BITPLANE PER SPRITE ---
                #
                # A differenza del foreground, per gli sprite usiamo l'ordine
                # dei bitplane come definito letteralmente dal driver MAME,
                # senza scambiare LSB e MSB.
                
                for plane_idx in range(BPP):
                    byte_addr = char_base_offset + (bit_offset // 8)
                    bit_pos = 7 - (bit_offset % 8)
                    rom_addr = plane_offsets[plane_idx] + byte_addr
                    
                    pixel_bit = (rom_data[rom_addr] >> bit_pos) & 1
                    
                    # plane_idx 0 (da 6.10lm) è il LSB
                    # plane_idx 1 (da 5.9lm) è il bit centrale
                    # plane_idx 2 (da 4.8lm) è il MSB
                    pixel_value |= (pixel_bit << plane_idx)
                # --- FINE CORREZIONE LOGICA BITPLANE PER SPRITE ---
                # ================================================================

                sprite_data[y][x] = pixel_value
        decoded_sprites.append(sprite_data)
        
    print("Decodifica completata.")
    return decoded_sprites


def write_c_array_packed(filename, array_name, sprites, width, height):
    print(f"Scrittura dell'array C '{array_name}' in '{filename}'...")
    with open(filename, 'a') as f:
        f.write(f"// Dati per {len(sprites)} sprite {width}x{height}, 3bpp")
        if ROTATE_TILES: f.write(", ruotati di 90 gradi CW")
        f.write(".\n")
        
        f.write(f"const uint32_t {array_name}[{len(sprites)}][{height}][{width//8}] = {{\n")
        
        for i, sprite in enumerate(sprites):
            f.write(f"  {{ // Sprite {i:03d} (0x{i:03X})\n")
            for y, row in enumerate(sprite):
                f.write("    { ")
                packed_chunks = []
                for chunk_idx in range(width // 8):
                    packed_int = 0
                    for x_in_chunk in range(8):
                        x = chunk_idx * 8 + x_in_chunk
                        pixel_value = row[x]
                        packed_int |= pixel_value << (BPP * (7 - x_in_chunk))
                    packed_chunks.append(f"0x{packed_int:06X}")
                f.write(", ".join(packed_chunks))
                f.write(" },\n")
            f.write("  },\n")
        f.write("};\n\n")
    print("Scrittura completata.")


def generate_preview(filename, sprites, width, height, grid_cols=32):
    if not PIL_AVAILABLE: return
    print(f"Generazione anteprima PNG in '{filename}'...")
    num_sprites = len(sprites)
    grid_rows = (num_sprites + grid_cols - 1) // grid_cols
    PREVIEW_PALETTE = [(0,0,0), (0,0,255), (0,255,0), (0,255,255), (255,0,0), (255,0,255), (255,255,0), (255,255,255)] # Palette CGA per distinguere meglio
    UPSCALE = 2 if width == 16 else 1
    CELL_W, CELL_H = width * UPSCALE + 4, height * UPSCALE + 16
    IMG_W, IMG_H = grid_cols * CELL_W, grid_rows * CELL_H
    img = Image.new('RGB', (IMG_W, IMG_H), (80, 80, 80))
    draw = ImageDraw.Draw(img)
    try: font = ImageFont.truetype("arial.ttf", 10)
    except IOError: font = ImageFont.load_default()
    for i, sprite in enumerate(sprites):
        gx, gy = i % grid_cols, i // grid_cols
        ox, oy = gx * CELL_W, gy * CELL_H
        for y in range(height):
            for x in range(width):
                color_idx = sprite[y][x]
                draw.rectangle((ox + x * UPSCALE, oy + y * UPSCALE, ox + (x+1)*UPSCALE-1, oy + (y+1)*UPSCALE-1), fill=PREVIEW_PALETTE[color_idx])
        draw.text((ox + CELL_W/2, oy + height*UPSCALE + 2), str(i), font=font, fill=(220,220,220), anchor="mt")
    img.save(filename)
    print(f"Anteprima '{filename}' generata.")


if __name__ == "__main__":
    full_rom_data = bytearray()
    for filename in ROM_FILES:
        if not os.path.exists(filename): exit(f"Errore: File ROM '{filename}' non trovato.")
        with open(filename, 'rb') as f:
            data = f.read()
            if len(data) != ROM_FILE_SIZE: exit(f"Errore: Dimensione file '{filename}' errata.")
            full_rom_data.extend(data)
    print(f"Dati ROM caricati. Dimensione totale: {len(full_rom_data)} bytes.")

    if os.path.exists(OUTPUT_C_FILE): os.remove(OUTPUT_C_FILE)
    with open(OUTPUT_C_FILE, 'w') as f:
        f.write("// File generato da starforce_sprite_conv.py (versione con logica di decodifica corretta)\n#include <stdint.h>\n\n")

    # Decodifica 16x16
    layout_x_16 = [0,1,2,3,4,5,6,7, 8*8+0,8*8+1,8*8+2,8*8+3,8*8+4,8*8+5,8*8+6,8*8+7]
    layout_y_16 = [0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8, 16*8,17*8,18*8,19*8,20*8,21*8,22*8,23*8]
    sprites_16 = decode_gfx_correct(full_rom_data, NUM_SPRITES_16, SPRITE_WIDTH_16, SPRITE_HEIGHT_16, layout_x_16, layout_y_16, 32)
    if ROTATE_TILES:
        sprites_16 = [rotate_matrix_90_cw(s, SPRITE_WIDTH_16, SPRITE_HEIGHT_16) for s in sprites_16]
    if GENERATE_PREVIEW:
        generate_preview(PREVIEW_PNG_16, sprites_16, SPRITE_HEIGHT_16, SPRITE_WIDTH_16, grid_cols=32)
    write_c_array_packed(OUTPUT_C_FILE, C_ARRAY_NAME_16, sprites_16, SPRITE_HEIGHT_16, SPRITE_WIDTH_16)

    # Decodifica 32x32
    layout_x_32 = [0,1,2,3,4,5,6,7, 8*8+0,8*8+1,8*8+2,8*8+3,8*8+4,8*8+5,8*8+6,8*8+7, 32*8+0,32*8+1,32*8+2,32*8+3,32*8+4,32*8+5,32*8+6,32*8+7, 40*8+0,40*8+1,40*8+2,40*8+3,40*8+4,40*8+5,40*8+6,40*8+7]
    layout_y_32 = [0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8, 16*8,17*8,18*8,19*8,20*8,21*8,22*8,23*8, 64*8,65*8,66*8,67*8,68*8,69*8,70*8,71*8, 80*8,81*8,82*8,83*8,84*8,85*8,86*8,87*8]
    sprites_32 = decode_gfx_correct(full_rom_data, NUM_SPRITES_32, SPRITE_WIDTH_32, SPRITE_HEIGHT_32, layout_x_32, layout_y_32, 128)
    if ROTATE_TILES:
        sprites_32 = [rotate_matrix_90_cw(s, SPRITE_WIDTH_32, SPRITE_HEIGHT_32) for s in sprites_32]
    if GENERATE_PREVIEW:
        generate_preview(PREVIEW_PNG_32, sprites_32, SPRITE_HEIGHT_32, SPRITE_WIDTH_32, grid_cols=16)
    write_c_array_packed(OUTPUT_C_FILE, C_ARRAY_NAME_32, sprites_32, SPRITE_HEIGHT_32, SPRITE_WIDTH_32)