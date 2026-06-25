#!/usr/bin/env python
import sys
import os

# --- Configurazione Specifica per Donkey Kong 3 (dkong3j) ---

INPUT_ROM_FILE_1 = "../roms/dk3v.3n"
INPUT_ROM_FILE_2 = "../roms/dk3v.3p"
OUTPUT_HEADER_FILE = "../../source/src/machines/dkong3/dkong3_tilemap.h"
OUTPUT_ARRAY_NAME = "dkong3_tilemap"
NUM_TILES = 512

def parse_chr_2(data0, data1):
    """
    Decodifica un singolo tile (8x8 pixel) da due bit-plane e applica
    una rotazione di 90 gradi antiorario e un flip verticale.
    
    data0 e data1 sono chunk di 8 byte ciascuno.
    """
    # Matrice temporanea per memorizzare il tile originale
    original_char = [[0] * 8 for _ in range(8)]
    
    # Decodifica il tile nella sua forma originale (orizzontale)
    for y in range(8):
        for x in range(8):
            bit0 = 1 if (data0[y] & (1 << (7 - x))) else 0
            bit1 = 2 if (data1[y] & (1 << (7 - x))) else 0
            original_char[y][x] = bit0 + bit1

    # Matrice finale per il tile trasformato
    transformed_char = [[0] * 8 for _ in range(8)]
    
    # Applica la trasformazione: rotazione 90° antiorario + flip verticale
    # Un pixel in (x, y) nell'originale si sposta in (y, x) nel trasformato.
    # Questo equivale a una trasposizione della matrice.
    for y in range(8):
        for x in range(8):
            transformed_char[x][y] = original_char[y][x]
            
    return transformed_char

def dump_chr(data):
    """
    Converte una matrice di pixel 8x8 in un array di 8 short (16-bit),
    impacchettando 8 pixel a 2-bit in ogni short.
    Questa logica di impacchettamento non cambia.
    """
    hexs = []
    for y in range(8):
        val = 0
        for x in range(8):
            val = (val << 2) | data[y][x]
        hexs.append(hex(val))
    return ",".join(hexs)

def convert_dk3_tilemap():
    """
    Funzione principale che legge le ROM, le processa e scrive il file di output.
    """
    print("Controllo dei file ROM necessari per Donkey Kong 3...")
    for filename in [INPUT_ROM_FILE_1, INPUT_ROM_FILE_2]:
        if not os.path.exists(filename):
            print(f"ERRORE: File ROM non trovato: '{filename}'")
            print("Assicurati che i file siano nella stessa directory dello script.")
            sys.exit(1)
    print("Tutti i file ROM sono stati trovati.")

    try:
        with open(INPUT_ROM_FILE_1, "rb") as f:
            charmap_data_0 = f.read()
        with open(INPUT_ROM_FILE_2, "rb") as f:
            charmap_data_1 = f.read()
    except Exception as e:
        print(f"ERRORE durante la lettura dei file: {e}")
        sys.exit(1)

    if len(charmap_data_0) != 4096 or len(charmap_data_1) != 4096:
        print("ERRORE: La dimensione di uno o entrambi i file ROM non è 4096 byte.")
        sys.exit(1)

    print(f"Decodifica e trasformazione di {NUM_TILES} tile in corso...")
    chars = []
    for i in range(NUM_TILES):
        chunk0 = charmap_data_0[i * 8 : (i + 1) * 8]
        chunk1 = charmap_data_1[i * 8 : (i + 1) * 8]
        chars.append(parse_chr_2(chunk0, chunk1))
    print("Decodifica e trasformazione completata.")

    print(f"Scrittura del file di output: '{OUTPUT_HEADER_FILE}'...")
    with open(OUTPUT_HEADER_FILE, "w") as f:
        f.write(f"// File generato automaticamente per la tilemap di Donkey Kong 3.\n")
        f.write(f"// I tile sono stati ruotati di 90° antiorario e flippati verticalmente.\n")
        f.write(f"// Dati estratti da: {INPUT_ROM_FILE_1}, {INPUT_ROM_FILE_2}\n\n")
        f.write(f"const unsigned short {OUTPUT_ARRAY_NAME}[{NUM_TILES}][8] = {{\n")
        
        chars_str = []
        for c in chars:
            chars_str.append("  { " + dump_chr(c) + " }")
        
        f.write(",\n".join(chars_str))
        f.write("\n};")

    print(f"\nProcesso completato! Il file '{OUTPUT_HEADER_FILE}' è stato creato.")

if __name__ == "__main__":
    convert_dk3_tilemap()