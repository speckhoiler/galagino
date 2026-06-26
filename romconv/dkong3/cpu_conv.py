#!/usr/bin/env python
import sys
import os

# --- Configurazione per Donkey Kong 3 (Japanese) ---
# Nomi dei file ROM richiesti dal driver MAME (dkong.cpp)
ROM_FILES = {
    "7b": "../roms/dk3c.7b",
    "7c": "../roms/dk3c.7c",
    "7d": "../roms/dk3c.7d",
    "7e": "../roms/dk3c.7e"
}

# File di output
OUTPUT_FILE = "../../source/src/machines/dkong3/dkong3_rom.h"
# Nome dell'array C nel file di output
ROM_ID = "dkong3_rom_cpu"
# Dimensione totale della regione di memoria della CPU
CPU_REGION_SIZE = 0x10000  # 64KB

# --- Funzione principale di conversione ---

def create_output_file(rom_data, outfile, id_name):
    """Scrive i dati della ROM in un file header C."""
    print(f"Scrittura del file di output: {outfile}...")
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
    print("Completato.")

def convert_dkong3j_roms():
    """
    Legge i file ROM di Donkey Kong 3 (Japan), li mappa correttamente in memoria
    e genera il file header C.
    """
    # Controlla se i file ROM esistono nella directory corrente
    print("Controllo dei file ROM necessari...")
    rom_path = "." # Si aspetta i file nella stessa directory dello script
    for fname in ROM_FILES.values():
        if not os.path.exists(os.path.join(rom_path, fname)):
            print(f"ERRORE: File ROM non trovato: '{fname}'")
            print("Assicurati che i file ROM siano nella stessa directory dello script.")
            sys.exit(1)
    print("Tutti i file ROM sono stati trovati.")

    # Crea un buffer di bytearray per l'intera regione di memoria della CPU (64KB), inizializzato a 0xFF
    # 0xFF rappresenta la memoria non mappata/vuota.
    cpu_memory = bytearray([0xFF] * CPU_REGION_SIZE)
    
    print("Inizio della mappatura delle ROM in memoria...")

    # Mappatura basata sul driver MAME (dkong.cpp per dkong3j)
    try:
        # File 1: dk3c.7b
        # ROM_LOAD( "dk3c.7b", 0x0000, 0x2000, ... )
        with open(os.path.join(rom_path, ROM_FILES["7b"]), "rb") as f:
            rom_data = f.read()
        cpu_memory[0x0000:0x2000] = rom_data
        print(f"- {ROM_FILES['7b']} mappato correttamente a 0x0000.")

        # File 2: dk3c.7c
        # ROM_LOAD( "dk3c.7c", 0x2000, 0x2000, ... )
        with open(os.path.join(rom_path, ROM_FILES["7c"]), "rb") as f:
            rom_data = f.read()
        cpu_memory[0x2000:0x4000] = rom_data
        print(f"- {ROM_FILES['7c']} mappato correttamente a 0x2000.")

        # File 3: dk3c.7d
        # ROM_LOAD( "dk3c.7d", 0x4000, 0x2000, ... )
        with open(os.path.join(rom_path, ROM_FILES["7d"]), "rb") as f:
            rom_data = f.read()
        cpu_memory[0x4000:0x6000] = rom_data
        print(f"- {ROM_FILES['7d']} mappato correttamente a 0x4000.")
        
        # File 4: dk3cj.7e
        # ROM_LOAD( "dk3cj.7e", 0x8000, 0x2000, ... )
        with open(os.path.join(rom_path, ROM_FILES["7e"]), "rb") as f:
            rom_data = f.read()
        cpu_memory[0x8000:0xA000] = rom_data
        print(f"- {ROM_FILES['7e']} mappato correttamente a 0x8000.")

        # In questo caso, le ROM non sono contigue (c'è un buco tra 0x6000 e 0x7FFF).
        # Per un emulatore, è meglio esportare l'intero spazio di indirizzamento della CPU.
        # Esportiamo l'intero buffer di 64KB che contiene le ROM nelle posizioni corrette.
        final_rom_data = cpu_memory

        # Genera il file di output
        create_output_file(final_rom_data, OUTPUT_FILE, ROM_ID)

    except FileNotFoundError as e:
        print(f"ERRORE: File non trovato - {e}")
        sys.exit(1)
    except Exception as e:
        print(f"Si è verificato un errore imprevisto: {e}")
        sys.exit(1)


if __name__ == "__main__":
    convert_dkong3j_roms()