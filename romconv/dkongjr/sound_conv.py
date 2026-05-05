#!/usr/bin/env python
import sys
import os

# --- Configurazione ---
# File ROM di input per la Sound CPU di dkongjrj
INPUT_ROM_FILE = "../roms/c_3h.bin"

# File di output
OUTPUT_HEADER_FILE = "../../source/src/machines/dkongjr/dkongjr_rom2.h"

# Nome dell'array C che verrà generato nel file di output
OUTPUT_ARRAY_NAME = "dkongjr_rom2"

def convert_sound_rom():
    """
    Legge un singolo file ROM binario e lo converte in un file header C
    contenente un array di byte.
    """
    # 1. Controlla se il file ROM esiste
    print(f"Controllo della presenza del file ROM: '{INPUT_ROM_FILE}'...")
    if not os.path.exists(INPUT_ROM_FILE):
        print(f"ERRORE: File ROM non trovato: '{INPUT_ROM_FILE}'")
        print("Assicurati che il file sia nella stessa directory dello script.")
        sys.exit(1)
    print("File ROM trovato.")

    # 2. Legge i dati binari dal file ROM
    try:
        with open(INPUT_ROM_FILE, "rb") as f:
            rom_data = f.read()
        print(f"Letti {len(rom_data)} byte dal file.")
    except Exception as e:
        print(f"ERRORE: Impossibile leggere il file ROM. Causa: {e}")
        sys.exit(1)

    # 3. Scrive il file header (.h) in formato C
    print(f"Inizio scrittura del file di output: '{OUTPUT_HEADER_FILE}'...")
    with open(OUTPUT_HEADER_FILE, "w") as of:
        of.write(f"// File header generato automaticamente per la Sound CPU di dkongjrj\n")
        of.write(f"// Contiene i dati del file: {INPUT_ROM_FILE}\n\n")
        of.write(f"const unsigned char {OUTPUT_ARRAY_NAME}[{len(rom_data)}] = {{\n  ")
        
        for i, byte in enumerate(rom_data):
            # Scrive il byte in formato esadecimale (es. 0x4A)
            of.write(f"0x{byte:02X}")
            
            # Aggiunge una virgola se non è l'ultimo byte
            if i < len(rom_data) - 1:
                of.write(",")
                # Va a capo ogni 16 byte per una migliore leggibilità
                if (i + 1) % 16 == 0:
                    of.write("\n  ")
            else:
                of.write("\n") # Nuova riga alla fine dell'array
        
        of.write("};")
    
    print("\nConversione completata con successo!")
    print(f"Il file '{OUTPUT_HEADER_FILE}' è stato creato.")


if __name__ == "__main__":
    convert_sound_rom()