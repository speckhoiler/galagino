#!/usr/bin/env python
import sys
import os

# --- Configurazione Specifica per Bomb Jack (Foreground) ---
INPUT_ROM_FILES = ["../roms/03_e08t.bin", "../roms/04_h08t.bin", "../roms/05_k08t.bin"]
OUTPUT_HEADER_FILE = "../../source/src/machines/bombjack/bombjack_fg_tiles.h"
OUTPUT_ARRAY_NAME = "bombjack_fg_tiles"
NUM_TILES = 512
TILE_WIDTH = 8
TILE_HEIGHT = 8
ROTATE_TILES = True

def rotate_matrix_90_cw(matrix):
    """Ruota una matrice 8x8 di 90 gradi in senso orario."""
    rotated_matrix = [[0] * TILE_WIDTH for _ in range(TILE_HEIGHT)]
    for y in range(TILE_HEIGHT):
        for x in range(TILE_WIDTH):
            rotated_matrix[y][x] = matrix[(TILE_WIDTH - 1) - x][y]
    return rotated_matrix

def parse_chr_3bpp_corrected(plane_data, tile_index):
    """
    Decodifica un singolo tile 8x8 con l'ordine dei bit-plane corretto.
    plane_data è una lista di 3 buffer di byte: [03_e08t, 04_h08t, 05_k08t].
    """
    char_matrix = [[0] * TILE_WIDTH for _ in range(TILE_HEIGHT)]
    offset = tile_index * 8
    
    # Estrai i chunk di 8 byte per il tile corrente
    chunk0 = plane_data[0][offset : offset + 8] # Dati da 03_e08t.bin
    chunk1 = plane_data[1][offset : offset + 8] # Dati da 04_h08t.bin
    chunk2 = plane_data[2][offset : offset + 8] # Dati da 05_k08t.bin

    for y in range(TILE_HEIGHT):
        for x in range(TILE_WIDTH):
            bit_pos = 7 - x
            
            # --- LOGICA DI COMBINAZIONE CORRETTA ---
            # File 0 ('03_e08t.bin') -> Bit 2 (MSB, valore 4)
            # File 1 ('04_h08t.bin') -> Bit 1 (valore 2)
            # File 2 ('05_k08t.bin') -> Bit 0 (LSB, valore 1)
            
            bit2 = 4 if (chunk0[y] & (1 << bit_pos)) else 0
            bit1 = 2 if (chunk1[y] & (1 << bit_pos)) else 0
            bit0 = 1 if (chunk2[y] & (1 << bit_pos)) else 0
            
            char_matrix[y][x] = bit0 + bit1 + bit2
            
    return char_matrix

def dump_chr_to_ulong(data):
    """Converte una matrice 8x8 di pixel in una stringa di 8 'unsigned long'."""
    hexs = []
    for y_row in data:
        val = 0
        for pixel in y_row:
            val = (val << 3) | pixel
        hexs.append(f"0x{val:06X}")
    return ",".join(hexs)

def convert_bombjack_fg_tiles():
    print("--- Conversione Tilemap Foreground per Bomb Jack (Logica Colori Corretta) ---")
    for filename in INPUT_ROM_FILES:
        if not os.path.exists(filename):
            print(f"ERRORE: File ROM non trovato: '{filename}'")
            sys.exit(1)

    # Leggi tutti i file ROM in una lista
    plane_data = [open(f, "rb").read() for f in INPUT_ROM_FILES]

    chars = []
    for i in range(NUM_TILES):
        # Usa la nuova funzione di parsing con la logica corretta
        decoded_char = parse_chr_3bpp_corrected(plane_data, i)
        
        if ROTATE_TILES:
            final_char = rotate_matrix_90_cw(decoded_char)
        else:
            final_char = decoded_char
        chars.append(final_char)

    with open(OUTPUT_HEADER_FILE, "w") as f:
        f.write(f"// File generato automaticamente per Bomb Jack (formato unsigned long).\n")
        f.write(f"// Ordine dei bit-plane corretto per i colori.\n\n")
        f.write(f"const unsigned long {OUTPUT_ARRAY_NAME}[{NUM_TILES}][{TILE_HEIGHT}] = {{\n")
        
        chars_str = []
        for i, c in enumerate(chars):
            comment = f" /* Tile {i} */"
            chars_str.append("  { " + dump_chr_to_ulong(c) + " }" + comment)
        
        f.write(",\n".join(chars_str))
        f.write("\n};")

    print(f"\nProcesso completato! Il file '{OUTPUT_HEADER_FILE}' è stato creato.")

if __name__ == "__main__":
    convert_bombjack_fg_tiles()