#!/usr/bin/env python
import sys
import os

# --- Configurazione per Donkey Kong Jr. (Japanese) ---
# Nomi dei file ROM richiesti dal driver MAME
ROM_FILES = {
    "5b": "../roms/c_5ba.bin",
    "5c": "../roms/c_5ca.bin",
    "5e": "../roms/c_5ea.bin"
}

# File di output
OUTPUT_FILE = "../../source/src/machines/dkongjr/dkongjr_rom1.h"
# Nome dell'array C nel file di output
ROM_ID = "dkongjr_rom1"
# Dimensione totale della regione di memoria della CPU
CPU_REGION_SIZE = 0x10000  # 64KB

# --- Funzione principale di conversione ---

def create_output_file(rom_data, outfile, id_name):
    """Scrive i dati della ROM in un file header C."""
    print(f"Scrittura del file di output: {outfile}...")
    with open(outfile, "w") as of:
        of.write(f"// File generato automaticamente per {id_name}\n")
        of.write(f"const unsigned char {id_name}[] = {{\n  ")
        
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

def convert_dkjrj_roms():
    """
    Legge i file ROM di Donkey Kong Jr. (Japan), li mappa correttamente in memoria
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
    cpu_memory = bytearray([0xFF] * CPU_REGION_SIZE)
    
    print("Inizio della mappatura delle ROM in memoria...")

    # Mappatura basata sul driver MAME (dkongjr.cpp)
    try:
        # File 1: c_5ba.bin
        with open(os.path.join(rom_path, ROM_FILES["5b"]), "rb") as f:
            rom_5b = f.read()
        # ROM_LOAD( "c_5ba.bin", 0x0000, 0x1000, ... )
        cpu_memory[0x0000:0x1000] = rom_5b[0x0000:0x1000]
        # ROM_CONTINUE(             0x3000, 0x1000 )
        cpu_memory[0x3000:0x4000] = rom_5b[0x1000:0x2000]
        print(f"- {ROM_FILES['5b']} mappato correttamente.")

        # File 2: c_5ca.bin
        with open(os.path.join(rom_path, ROM_FILES["5c"]), "rb") as f:
            rom_5c = f.read()
        # ROM_LOAD( "c_5ca.bin", 0x2000, 0x0800, ... )
        cpu_memory[0x2000:0x2800] = rom_5c[0x0000:0x0800]
        # ROM_CONTINUE(             0x4800, 0x0800 )
        cpu_memory[0x4800:0x5000] = rom_5c[0x0800:0x1000]
        # ROM_CONTINUE(             0x1000, 0x0800 )
        cpu_memory[0x1000:0x1800] = rom_5c[0x1000:0x1800]
        # ROM_CONTINUE(             0x5800, 0x0800 )
        cpu_memory[0x5800:0x6000] = rom_5c[0x1800:0x2000]
        print(f"- {ROM_FILES['5c']} mappato correttamente.")

        # File 3: c_5ea.bin
        with open(os.path.join(rom_path, ROM_FILES["5e"]), "rb") as f:
            rom_5e = f.read()
        # ROM_LOAD( "c_5ea.bin", 0x4000, 0x0800, ... )
        cpu_memory[0x4000:0x4800] = rom_5e[0x0000:0x0800]
        # ROM_CONTINUE(             0x2800, 0x0800 )
        cpu_memory[0x2800:0x3000] = rom_5e[0x0800:0x1000]
        # ROM_CONTINUE(             0x5000, 0x0800 )
        cpu_memory[0x5000:0x5800] = rom_5e[0x1000:0x1800]
        # ROM_CONTINUE(             0x1800, 0x0800 )
        cpu_memory[0x1800:0x2000] = rom_5e[0x1800:0x2000]
        print(f"- {ROM_FILES['5e']} mappato correttamente.")

        # Taglia il buffer ai dati effettivamente utilizzati (fino a 0x5FFF)
        final_rom_data = cpu_memory[:0x6000]

        # Genera il file di output
        create_output_file(final_rom_data, OUTPUT_FILE, ROM_ID)

    except FileNotFoundError as e:
        print(f"ERRORE: File non trovato - {e}")
        sys.exit(1)
    except Exception as e:
        print(f"Si è verificato un errore imprevisto: {e}")
        sys.exit(1)


if __name__ == "__main__":
    convert_dkjrj_roms()