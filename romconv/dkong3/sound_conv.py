#!/usr/bin/env python
import sys
import os

# --- Configurazione per Donkey Kong 3 (Japanese) ---
# Lista dei file ROM per le sound CPU e dei nomi per gli array di output.
SOUND_ROMS = [
    {
        "input_file": "../roms/dk3c.5l",
        "array_name": "dkong3_rom_sound_a",
        "comment": "Sound CPU #1 (RP2A03A)"
    },
    {
        "input_file": "../roms/dk3c.6h",
        "array_name": "dkong3_rom_sound_b",
        "comment": "Sound CPU #2 (RP2A03B)"
    }
]

# File di output unico per tutte le ROM sonore
OUTPUT_HEADER_FILE = "../../source/src/machines/dkong3/dkong3_sound_roms.h"

def write_array_to_file(file_handle, rom_data, array_name, input_filename, comment):
    """
    Scrive un singolo array C nel file di output già aperto.
    """
    file_handle.write(f"// --- {comment} ---\n")
    file_handle.write(f"// Dati dal file: {input_filename}\n")
    file_handle.write(f"const unsigned char {array_name}[{len(rom_data)}] = {{\n  ")
    
    for i, byte in enumerate(rom_data):
        # Scrive il byte in formato esadecimale (es. 0x4A)
        file_handle.write(f"0x{byte:02X}")
        
        # Aggiunge una virgola se non è l'ultimo byte
        if i < len(rom_data) - 1:
            file_handle.write(",")
            # Va a capo ogni 16 byte per una migliore leggibilità
            if (i + 1) % 16 == 0:
                file_handle.write("\n  ")
        else:
            file_handle.write("\n") # Nuova riga alla fine dell'array
    
    file_handle.write("};\n\n") # Aggiunge due nuove righe per separare gli array

def convert_dkong3j_sound_roms():
    """
    Legge i file ROM delle CPU sonore di Donkey Kong 3 e li converte
    in un singolo file header C contenente più array.
    """
    # 1. Controlla che tutti i file ROM esistano
    print("Controllo dei file ROM necessari...")
    all_files_found = True
    for rom_info in SOUND_ROMS:
        if not os.path.exists(rom_info["input_file"]):
            print(f"ERRORE: File ROM non trovato: '{rom_info['input_file']}'")
            all_files_found = False
    
    if not all_files_found:
        print("Assicurati che tutti i file ROM siano nella stessa directory dello script.")
        sys.exit(1)
    print("Tutti i file ROM sonori sono stati trovati.")

    # 2. Apre il file di output in scrittura e scrive l'intestazione
    print(f"Inizio scrittura del file di output: '{OUTPUT_HEADER_FILE}'...")
    with open(OUTPUT_HEADER_FILE, "w") as of:
        of.write(f"// File header generato automaticamente per le Sound CPU di dkong3j\n")
        of.write(f"// Contiene le ROM per entrambe le CPU sonore.\n\n")

        # 3. Itera su ogni ROM, la legge e scrive l'array corrispondente
        for rom_info in SOUND_ROMS:
            input_file = rom_info["input_file"]
            array_name = rom_info["array_name"]
            comment = rom_info["comment"]
            
            print(f" elaborazione di '{input_file}'...")
            try:
                with open(input_file, "rb") as f:
                    rom_data = f.read()
                print(f"  Letti {len(rom_data)} byte.")
                
                # Scrive l'array nel file di output
                write_array_to_file(of, rom_data, array_name, input_file, comment)

            except Exception as e:
                print(f"ERRORE: Impossibile processare il file '{input_file}'. Causa: {e}")
                sys.exit(1)

    print("\nConversione completata con successo!")
    print(f"Il file '{OUTPUT_HEADER_FILE}' è stato creato con entrambi gli array.")


if __name__ == "__main__":
    convert_dkong3j_sound_roms()