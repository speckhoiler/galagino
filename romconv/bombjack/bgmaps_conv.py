#!/usr/bin/env python
import sys
import os

# --- Configurazione per la Mappa del Background di Bomb Jack ---
INPUT_ROM_FILE = "../roms/02_p04t.bin"
OUTPUT_HEADER_FILE = "../../source/src/machines/bombjack/bombjack_bg_maps.h"
OUTPUT_ARRAY_NAME = "bombjack_bg_maps"

def create_output_file(rom_data, outfile, id_name):
    """Scrive i dati della ROM in un file header C."""
    print(f"Scrittura del file di output: {outfile}...")
    with open(outfile, "w") as of:
        of.write(f"// File generato automaticamente per la mappa del background di Bomb Jack.\n")
        of.write(f"// Dati estratti da: {INPUT_ROM_FILE}\n\n")
        of.write(f"const unsigned char {id_name}[{len(rom_data)}] = {{\n  ")
        
        for i, byte in enumerate(rom_data):
            of.write(f"0x{byte:02X}")
            if i < len(rom_data) - 1:
                of.write(",")
                if (i + 1) % 16 == 0:
                    of.write("\n  ")
            else:
                of.write("\n")
        
        of.write("};")
    print("Completato.")

def convert_bg_maps():
    """
    Legge il file ROM della mappa di background e genera il file header C.
    """
    print("--- Conversione Mappa Background per Bomb Jack ---")
    if not os.path.exists(INPUT_ROM_FILE):
        print(f"ERRORE: File ROM non trovato: '{INPUT_ROM_FILE}'")
        sys.exit(1)
    
    print(f"File ROM '{INPUT_ROM_FILE}' trovato.")

    try:
        with open(INPUT_ROM_FILE, "rb") as f:
            rom_data = f.read()
        
        # Il driver MAME fa un RELOAD, che duplica i dati.
        # La ROM è 0x1000 (4KB), ma la regione è 0x2000 (8KB).
        # Duplichiamo i dati per simulare il reload.
        final_rom_data = rom_data + rom_data
        print(f"Dati duplicati per simulare ROM_RELOAD. Dimensione finale: {len(final_rom_data)} bytes.")

        create_output_file(final_rom_data, OUTPUT_HEADER_FILE, OUTPUT_ARRAY_NAME)

    except Exception as e:
        print(f"Si è verificato un errore imprevisto: {e}")
        sys.exit(1)

if __name__ == "__main__":
    convert_bg_maps()