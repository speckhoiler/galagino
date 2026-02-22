#!/usr/bin/env python
import sys
import os

# --- Configurazione per Bomb Jack (Background) ---
INPUT_ROM_FILES = ["../roms/06_l08t.bin", "../roms/07_n08t.bin", "../roms/08_r08t.bin"]
OUTPUT_HEADER_FILE = "../../source/src/machines/bombjack/bombjack_bg_tiles.h"
OUTPUT_ARRAY_NAME = "bombjack_bg_tiles"
NUM_TILES = 256
TILE_WIDTH = 16
TILE_HEIGHT = 16

# --- OPZIONE DI ROTAZIONE ---
# La tua funzione C fa la rotazione al volo, quindi i dati devono essere NON ruotati.
ROTATE_TILES = False

def parse_chr_3bpp_16x16_from_c_logic(data0, data1, data2):
    """
    Decodifica un tile 16x16 traducendo LETTERALMENTE la logica
    della tua funzione C funzionante, incluso l'ordine dei bit per il colore.
    data0 corrisponde a 06_l08t.bin (bm0)
    data1 corrisponde a 07_n08t.bin (bm1)
    data2 corrisponde a 08_r08t.bin (bm2)
    """
    matrix = [[0] * TILE_WIDTH for _ in range(TILE_HEIGHT)]
    
    for y in range(TILE_HEIGHT):
        row_byte_offset = (y % 8) + (16 if y >= 8 else 0)
        
        bm0 = (data0[row_byte_offset] << 8) | data0[row_byte_offset + 8]
        bm1 = (data1[row_byte_offset] << 8) | data1[row_byte_offset + 8]
        bm2 = (data2[row_byte_offset] << 8) | data2[row_byte_offset + 8]
        
        for x in range(TILE_WIDTH):
            bit_pos = 15 - x
            
            # Estrai i bit dai rispettivi piani
            bit_from_bm0 = 1 if (bm0 & (1 << bit_pos)) else 0
            bit_from_bm1 = 1 if (bm1 & (1 << bit_pos)) else 0
            bit_from_bm2 = 1 if (bm2 & (1 << bit_pos)) else 0
            
            # Combina i bit nell'ordine corretto, come da formula C:
            # pen = (bit_from_bm0 << 2) | (bit_from_bm1 << 1) | (bit_from_bm2 << 0)
            pen = (bit_from_bm0 * 4) + (bit_from_bm1 * 2) + bit_from_bm2
            
            matrix[y][x] = pen
            
    return matrix

def dump_row_to_ulong_pair(row_data):
    """Converte una riga di 16 pixel in una coppia di 'unsigned long'."""
    val1 = 0
    for x in range(8): val1 = (val1 << 3) | row_data[x]
    val2 = 0
    for x in range(8, 16): val2 = (val2 << 3) | row_data[x]
    return f"0x{val1:06X}, 0x{val2:06X}"

def convert_bombjack_bg_tiles():
    rotation_status = "abilitata" if ROTATE_TILES else "disabilitata"
    print(f"--- Conversione Tilemap Background (Logica da C, Rotazione {rotation_status}) ---")
    
    for filename in INPUT_ROM_FILES:
        if not os.path.exists(filename):
            print(f"ERRORE: File ROM non trovato: '{filename}'")
            sys.exit(1)

    # Assicurati che l'ordine dei file corrisponda a bm0, bm1, bm2
    with open(INPUT_ROM_FILES[0], "rb") as f: plane_bm0 = f.read()
    with open(INPUT_ROM_FILES[1], "rb") as f: plane_bm1 = f.read()
    with open(INPUT_ROM_FILES[2], "rb") as f: plane_bm2 = f.read()

    chars = []
    tile_byte_size = (TILE_WIDTH * TILE_HEIGHT) // 8
    for i in range(NUM_TILES):
        base_offset = i * tile_byte_size
        chunk0 = plane_bm0[base_offset : base_offset + tile_byte_size]
        chunk1 = plane_bm1[base_offset : base_offset + tile_byte_size]
        chunk2 = plane_bm2[base_offset : base_offset + tile_byte_size]
        
        decoded_char = parse_chr_3bpp_16x16_from_c_logic(chunk0, chunk1, chunk2)
        
        if ROTATE_TILES:
            # Non dovrebbe essere usato se la logica C già ruota
            print("ATTENZIONE: La rotazione è abilitata, ma potrebbe non essere necessaria.")
            from PIL import Image
            # Esempio di come ruotare se necessario
            # final_char = ...
        
        chars.append(decoded_char)

    with open(OUTPUT_HEADER_FILE, "w") as f:
        f.write(f"// File generato automaticamente per il background di Bomb Jack.\n")
        f.write(f"// Dati decodificati seguendo la logica C funzionante.\n")
        f.write(f"const unsigned long {OUTPUT_ARRAY_NAME}[{NUM_TILES}][{TILE_HEIGHT * 2}] = {{\n")
        
        for i, c in enumerate(chars):
            f.write(f"  {{ // Tile {i}\n")
            rows_str = []
            for y in range(TILE_HEIGHT):
                rows_str.append("    " + dump_row_to_ulong_pair(c[y]))
            f.write(",\n".join(rows_str))
            f.write("\n  }")
            if i < NUM_TILES - 1:
                f.write(",\n")
        
        f.write("\n};")

    print(f"\nProcesso completato! Il file '{OUTPUT_HEADER_FILE}' è stato creato.")

if __name__ == "__main__":
    convert_bombjack_bg_tiles()