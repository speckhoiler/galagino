#!/usr/bin/env python
import sys
import os

# --- Configurazione Specifica per Donkey Kong Jr. (dkongjrj) ---

# I due file ROM che contengono i dati dei tile (caratteri grafici)
# Ognuno contiene uno dei due bit-plane per la grafica a 2bpp.
INPUT_ROM_FILE_1 = "../roms/v_3na.bin"
INPUT_ROM_FILE_2 = "../roms/v_3pa.bin"

# Nome del file header di output
OUTPUT_HEADER_FILE = "../../source/src/machines/dkongjr/dkongjr_tilemap.h"

# Nome dell'array C che verrà generato
OUTPUT_ARRAY_NAME = "dkongjr_tilemap"

# Numero di tile da processare (4KB per file -> 4096 bytes / 8 bytes/tile = 512)
NUM_TILES = 512


def parse_chr_2(data0, data1):
    """
    Decodifica un singolo tile (8x8 pixel) da due bit-plane.
    data0 e data1 sono chunk di 8 byte ciascuno.
    """
    char = []
    for y in range(8):
        row = []
        for x in range(8):
            # Estrae i due bit che compongono il colore del pixel
            c0 = 1 if data0[7 - x] & (0x80 >> y) else 0
            c1 = 2 if data1[7 - x] & (0x80 >> y) else 0
            row.append(c0 + c1)
        char.append(row)
    return char

def dump_chr(data):
    """
    Converte una matrice di pixel 8x8 in un array di 8 short (16-bit),
    impacchettando 8 pixel a 2-bit in ogni short.
    """
    hexs = []
    for y in range(8):
        val = 0
        for x in range(8):
            # Logica di impacchettamento bit a bit
            val = (val >> 2) | (data[y][x] << (14))
        hexs.append(hex(val))
    return ",".join(hexs)

def convert_dkjr_tilemap():
    """
    Funzione principale che legge le ROM, le processa e scrive il file di output.
    """
    # 1. Controlla che i file ROM necessari esistano
    print("Controllo dei file ROM necessari...")
    for filename in [INPUT_ROM_FILE_1, INPUT_ROM_FILE_2]:
        if not os.path.exists(filename):
            print(f"ERRORE: File ROM non trovato: '{filename}'")
            print("Assicurati che i file siano nella stessa directory dello script.")
            sys.exit(1)
    print("Tutti i file ROM sono stati trovati.")

    # 2. Legge i dati dai due file ROM
    try:
        with open(INPUT_ROM_FILE_1, "rb") as f:
            charmap_data_0 = f.read()
        with open(INPUT_ROM_FILE_2, "rb") as f:
            charmap_data_1 = f.read()
    except Exception as e:
        print(f"ERRORE durante la lettura dei file: {e}")
        sys.exit(1)

    # Verifica le dimensioni
    if len(charmap_data_0) != 4096 or len(charmap_data_1) != 4096:
        print("ERRORE: La dimensione di uno o entrambi i file ROM non è 4096 byte.")
        sys.exit(1)

    # 3. Decodifica tutti i tile
    print(f"Decodifica di {NUM_TILES} tile in corso...")
    chars = []
    for i in range(NUM_TILES):
        # Estrae gli 8 byte per il tile corrente da ciascun file
        chunk0 = charmap_data_0[8 * i : 8 * (i + 1)]
        chunk1 = charmap_data_1[8 * i : 8 * (i + 1)]
        chars.append(parse_chr_2(chunk0, chunk1))
    print("Decodifica completata.")

    # 4. Scrive il file header C di output
    print(f"Scrittura del file di output: '{OUTPUT_HEADER_FILE}'...")
    with open(OUTPUT_HEADER_FILE, "w") as f:
        f.write(f"// File generato automaticamente per la tilemap di Donkey Kong Jr.\n")
        f.write(f"// Dati estratti da: {INPUT_ROM_FILE_1}, {INPUT_ROM_FILE_2}\n\n")
        f.write(f"const unsigned short {OUTPUT_ARRAY_NAME}[{NUM_TILES}][8] = {{\n")
        
        chars_str = []
        for c in chars:
            chars_str.append("  { " + dump_chr(c) + " }")
        
        f.write(",\n".join(chars_str))
        f.write("\n};")

    print(f"\nProcesso completato! Il file '{OUTPUT_HEADER_FILE}' è stato creato.")

if __name__ == "__main__":
    convert_dkjr_tilemap()