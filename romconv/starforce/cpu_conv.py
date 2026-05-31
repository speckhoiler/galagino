#!/usr/bin/env python
import sys
import os

# --- Funzione generica di scrittura file ---

def create_output_file(rom_data, outfile, id_name):
    """Scrive i dati della ROM in un file header C."""
    print(f"Scrittura del file di output: {outfile}...")
    try:
        with open(outfile, "w") as of:
            of.write(f"// File generato automaticamente per {id_name}\n")
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
        print(f"File {outfile} creato con successo.")
    except Exception as e:
        print(f"ERRORE durante la scrittura del file {outfile}: {e}")
        sys.exit(1)


# --- Funzione di conversione per la CPU Principale di Star Force ---

def convert_starforc_main_cpu():
    """
    Legge i file ROM della CPU principale di Star Force, li mappa in memoria
    e genera il file header C.
    """
    print("\n--- Inizio conversione ROM CPU Principale (starforc) ---")
    
    # Configurazione per la CPU principale
    ROM_FILES_MAIN = {
        "3.3p":  "../roms/3.3p",
        "2.3mn": "../roms/2.3mn"
    }
    OUTPUT_FILE_MAIN = "../../source/src/machines/starforce/starforce_main_cpu_rom.h"
    ROM_ID_MAIN = "starforce_main_cpu_rom"
    CPU_REGION_SIZE_MAIN = 0x8000  # 32KB

    # Controlla se i file ROM esistono
    print("Controllo dei file ROM necessari per la CPU principale...")
    rom_path = "."
    for fname in ROM_FILES_MAIN.values():
        if not os.path.exists(os.path.join(rom_path, fname)):
            print(f"ERRORE: File ROM non trovato: '{fname}'")
            print("Assicurati che i file ROM siano nella stessa directory dello script.")
            sys.exit(1)
    print("Tutti i file ROM della CPU principale sono stati trovati.")

    # Crea un buffer per l'intera regione di memoria, inizializzato a 0x00
    cpu_memory = bytearray([0x00] * CPU_REGION_SIZE_MAIN)
    
    print("Inizio della mappatura delle ROM in memoria...")

    try:
        # File 1: 3.3p
        # ROM_LOAD( "3.3p", 0x0000, 0x4000, ... )
        with open(os.path.join(rom_path, ROM_FILES_MAIN["3.3p"]), "rb") as f:
            rom_data = f.read()
        cpu_memory[0x0000:0x4000] = rom_data
        print(f"- {ROM_FILES_MAIN['3.3p']} mappato correttamente a 0x0000.")

        # File 2: 2.3mn
        # ROM_LOAD( "2.3mn", 0x4000, 0x4000, ... )
        with open(os.path.join(rom_path, ROM_FILES_MAIN["2.3mn"]), "rb") as f:
            rom_data = f.read()
        cpu_memory[0x4000:0x8000] = rom_data
        print(f"- {ROM_FILES_MAIN['2.3mn']} mappato correttamente a 0x4000.")
        
        # Genera il file di output per la CPU principale
        create_output_file(cpu_memory, OUTPUT_FILE_MAIN, ROM_ID_MAIN)

    except Exception as e:
        print(f"Si è verificato un errore imprevisto durante la conversione della CPU principale: {e}")
        sys.exit(1)


# --- Funzione di conversione per la CPU Secondaria (Audio) di Star Force ---

def convert_starforc_sub_cpu():
    """
    Legge il file ROM della CPU audio di Star Force e genera il file header C.
    """
    print("\n--- Inizio conversione ROM CPU Audio (starforc) ---")
    
    # Configurazione per la CPU audio
    ROM_FILE_SUB = "../roms/1.3hj"
    OUTPUT_FILE_SUB = "../../source/src/machines/starforce/starforce_sub_cpu_rom.h"
    ROM_ID_SUB = "starforce_sub_cpu_rom"
    
    # Controlla se il file ROM esiste
    print(f"Controllo del file ROM '{ROM_FILE_SUB}' per la CPU audio...")
    rom_path = "."
    if not os.path.exists(os.path.join(rom_path, ROM_FILE_SUB)):
        print(f"ERRORE: File ROM non trovato: '{ROM_FILE_SUB}'")
        sys.exit(1)
    print("File ROM della CPU audio trovato.")

    try:
        # Per la sub CPU, c'è un solo file che riempie la sua regione (0x2000)
        # ROM_LOAD( "1.3hj", 0x0000, 0x2000, ... )
        with open(os.path.join(rom_path, ROM_FILE_SUB), "rb") as f:
            rom_data = f.read()
        print(f"- {ROM_FILE_SUB} letto correttamente (dimensione: {len(rom_data)} bytes).")

        # Genera il file di output per la CPU audio
        create_output_file(rom_data, OUTPUT_FILE_SUB, ROM_ID_SUB)

    except Exception as e:
        print(f"Si è verificato un errore imprevisto durante la conversione della CPU audio: {e}")
        sys.exit(1)


if __name__ == "__main__":
    convert_starforc_main_cpu()
    convert_starforc_sub_cpu()
    print("\nConversione di tutte le ROM completata.")