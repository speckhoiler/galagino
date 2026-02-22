#!/usr/bin/env python
import sys
import os

# --- Configurazione Specifica per Bomb Jack (Sprites) ---
INPUT_ROM_FILES = ["../roms/16_m07b.bin", "../roms/15_l07b.bin", "../roms/14_j07b.bin"]
OUTPUT_HEADER_FILE = "../../source/src/machines/bombjack/bombjack_sprites.h"

# Imposta a False per usare il tuo blitter che fa la rotazione al volo.
# Imposta a True se vuoi che lo script ruoti i dati e il tuo blitter li disegni direttamente.
ROTATE_SPRITES = False 

def rotate_matrix_90_cw(matrix):
    """Ruota una matrice 2D di 90 gradi in senso orario."""
    h, w = len(matrix), len(matrix[0])
    return [[matrix[h - 1 - x][y] for x in range(h)] for y in range(w)]

def decode_sprite(rom_planes, width, height, base_offset):
    """
    Decodifica un singolo sprite, simulando la logica di GATHER di floooh.
    """
    sprite_matrix = [[0] * width for _ in range(height)]
    
    # Simula il contatore 'off' del codice C
    off = base_offset
    
    for y in range(height):
        # Simula le macro GATHER
        if width == 16:
            bm0 = (rom_planes[2][off] << 8) | rom_planes[2][off + 8]
            bm1 = (rom_planes[1][off] << 8) | rom_planes[1][off + 8]
            bm2 = (rom_planes[0][off] << 8) | rom_planes[0][off + 8]
        elif width == 32:
            bm0 = (rom_planes[2][off] << 24) | (rom_planes[2][off+8] << 16) | (rom_planes[2][off+32] << 8) | rom_planes[2][off+40]
            bm1 = (rom_planes[1][off] << 24) | (rom_planes[1][off+8] << 16) | (rom_planes[1][off+32] << 8) | rom_planes[1][off+40]
            bm2 = (rom_planes[0][off] << 24) | (rom_planes[0][off+8] << 16) | (rom_planes[0][off+32] << 8) | rom_planes[0][off+40]
        else:
            raise ValueError("Dimensione non supportata")

        # Estrai i pixel per la riga corrente
        for x in range(width):
            bit_pos = width - 1 - x
            pen = (((bm2 >> bit_pos) & 1) << 0) | \
                  (((bm1 >> bit_pos) & 1) << 1) | \
                  (((bm0 >> bit_pos) & 1) << 2)
            sprite_matrix[y][x] = pen
            
        # Aggiorna l'offset 'off' come farebbe il codice C
        off += 1
        if width == 16:
            if y == 7: off += 8
        elif width == 32:
            if (y & 7) == 7: off += 8
            if (y & 15) == 15: off += 32
            
    return sprite_matrix

def dump_sprite_to_long(data):
    """Converte una matrice 2D di pixel in una stringa di 'unsigned long'."""
    hexs = []
    width = len(data[0])
    for y_row in data:
        # Impacchetta 8 pixel alla volta
        for chunk_start in range(0, width, 8):
            chunk = y_row[chunk_start : chunk_start + 8]
            val = 0
            for pixel in chunk:
                val = (val << 3) | pixel
            hexs.append(f"0x{val:06X}")
    return ",".join(hexs)

def process_and_write(rom_data, width, height, num_sprites, array_name, f_out):
    """Funzione generica per processare e scrivere un set di sprite."""
    print(f"Processando {num_sprites} sprite {width}x{height}...")
    sprite_byte_size = (width * height) // 8
    
    f_out.write(f"const unsigned long {array_name}[{num_sprites}][{width*height//8}] = {{\n")
    
    all_sprites_str = []
    for i in range(num_sprites):
        base_offset = i * sprite_byte_size
        sprite_matrix = decode_sprite(rom_data, width, height, base_offset)
        
        if ROTATE_SPRITES:
            sprite_matrix = rotate_matrix_90_cw(sprite_matrix)
        
        all_sprites_str.append(f"  {{ {dump_sprite_to_long(sprite_matrix)} }} /* Sprite {i} */")
        
    f_out.write(",\n".join(all_sprites_str))
    f_out.write("\n};\n\n")

def main():
    print("--- Conversione Sprite per Bomb Jack (Logica Corretta) ---")
    for filename in INPUT_ROM_FILES:
        if not os.path.exists(filename):
            print(f"ERRORE: File ROM non trovato: '{filename}'")
            sys.exit(1)

    # L'ordine è importante per la funzione di decodifica: LSB, bit1, MSB
    rom_data = [
        open(INPUT_ROM_FILES[2], "rb").read(), # 14_j07b.bin (LSB)
        open(INPUT_ROM_FILES[1], "rb").read(), # 15_l07b.bin (bit 1)
        open(INPUT_ROM_FILES[0], "rb").read()  # 16_m07b.bin (MSB)
    ]

    with open(OUTPUT_HEADER_FILE, "w") as f:
        f.write(f"// File generato automaticamente per gli sprite di Bomb Jack.\n")
        f.write(f"// Logica di decodifica basata sull'emulatore di floooh.\n")
        if ROTATE_SPRITES:
            f.write(f"// Gli sprite sono stati ruotati di 90 gradi in senso orario.\n\n")
        else:
            f.write(f"// Gli sprite NON sono stati ruotati. Il blitter deve gestire la rotazione.\n\n")
        
        # Processa e scrivi sprite 16x16
        process_and_write(rom_data, 16, 16, 256, "bombjack_sprites_16x16", f)
        
        # Processa e scrivi sprite 32x32
        process_and_write(rom_data, 32, 32, 64, "bombjack_sprites_32x32", f)

    print(f"\nProcesso completato! Il file '{OUTPUT_HEADER_FILE}' è stato creato.")

if __name__ == "__main__":
    main()