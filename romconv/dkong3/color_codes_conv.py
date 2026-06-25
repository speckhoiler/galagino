#!/usr/bin/env python
import sys
import os

# --- Configurazione per la PROM di lookup dei colori dei Tile di DK3 ---

# File PROM che contiene la mappatura dei colori per i tile
COLOR_CODES_FILE = "../roms/dkc1-v.2n"

# Nome del file header di output
OUTPUT_HEADER_FILE = "../../source/src/machines/dkong3/dkong3_color_codes.h"

# Nome dell'array C che verrà generato
OUTPUT_ARRAY_NAME = "color_codes_prom"

def generate_color_codes_array():
    """
    Legge la PROM di lookup dei colori e la converte in un array C.
    """
    # 1. Controlla che il file ROM necessario esista
    print(f"Controllo del file ROM '{COLOR_CODES_FILE}'...")
    if not os.path.exists(COLOR_CODES_FILE):
        print(f"ERRORE: File ROM non trovato: '{COLOR_CODES_FILE}'")
        print("Assicurati che il file sia nella stessa directory dello script.")
        sys.exit(1)
    print("File ROM trovato.")

    # 2. Legge i dati dal file ROM
    try:
        with open(COLOR_CODES_FILE, "rb") as f:
            rom_data = f.read()
    except Exception as e:
        print(f"ERRORE durante la lettura del file: {e}")
        sys.exit(1)

    # La PROM è di 256 byte
    if len(rom_data) != 256:
        print(f"ERRORE: La dimensione del file '{COLOR_CODES_FILE}' non è 256 byte.")
        sys.exit(1)

    # 3. Scrive il file header C di output
    print(f"Scrittura del file di output: '{OUTPUT_HEADER_FILE}'...")
    with open(OUTPUT_HEADER_FILE, "w") as f:
        f.write(f"// File generato automaticamente per la PROM dei codici colore dei tile di Donkey Kong 3.\n")
        f.write(f"// Dati estratti da: {COLOR_CODES_FILE}\n\n")
        
        # Usa PROGMEM per salvare l'array nella memoria Flash su piattaforme come Arduino
        f.write(f"#include <pgmspace.h>\n\n")
        f.write(f"const unsigned char PROGMEM {OUTPUT_ARRAY_NAME}[256] = {{\n")
        
        # Formatta i dati in righe da 16 valori per una migliore leggibilità
        lines = []
        for i in range(0, 256, 16):
            chunk = rom_data[i:i+16]
            hex_values = [hex(val) for val in chunk]
            lines.append("  " + ", ".join(hex_values))
        
        f.write(",\n".join(lines))
        f.write("\n};")

    print(f"\nProcesso completato! Il file '{OUTPUT_HEADER_FILE}' è stato creato.")
    print(f"Includi questo file e usa l'array '{OUTPUT_ARRAY_NAME}' nel tuo codice C.")


if __name__ == "__main__":
    generate_color_codes_array()