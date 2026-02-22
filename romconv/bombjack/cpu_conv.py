#!/usr/bin/env python
import sys
import os

# --- Configurazione per Bomb Jack (set 'bombjack') ---
# Nomi dei file ROM richiesti dal driver MAME (bombjack.cpp)
ROM_FILES = {
    "cpu1": "../roms/09_j01b.bin",
    "cpu2": "../roms/10_l01b.bin",
    "cpu3": "../roms/11_m01b.bin",
    "cpu4": "../roms/12_n01b.bin",
    "cpu5": "../roms/13.1r"
}

# File di output
OUTPUT_FILE = "../../source/src/machines/bombjack/bombjack_rom1.h"
# Nome dell'array C nel file di output
ROM_ID = "bombjack_rom_cpu1"
# Dimensione totale della regione di memoria della CPU (Z80)
CPU_REGION_SIZE = 0x10000  # 64KB

# --- Funzione principale di conversione (invariata) ---

def create_output_file(rom_data, outfile, id_name):
    """Scrive i dati della ROM in un file header C."""
    print(f"Scrittura del file di output: {outfile}...")
    with open(outfile, "w") as of:
        of.write(f"// File generato automaticamente per {id_name}\n")
        of.write(f"// Contiene le ROM della CPU principale di Bomb Jack\n")
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

def convert_bombjack_roms():
    """
    Legge i file ROM di Bomb Jack, li mappa correttamente nello spazio di 
    indirizzamento della CPU e genera il file header C.
    """
    # Controlla se i file ROM esistono nella directory corrente
    print("Controllo dei file ROM necessari per Bomb Jack...")
    rom_path = "." # Si aspetta i file nella stessa directory dello script
    for fname in ROM_FILES.values():
        if not os.path.exists(os.path.join(rom_path, fname)):
            print(f"ERRORE: File ROM non trovato: '{fname}'")
            print("Assicurati che i file ROM siano nella stessa directory dello script.")
            sys.exit(1)
    print("Tutti i file ROM della CPU sono stati trovati.")

    # Crea un buffer di bytearray per l'intera regione di memoria della CPU (64KB), inizializzato a 0xFF
    # 0xFF rappresenta la memoria non mappata/vuota.
    cpu_memory = bytearray([0xFF] * CPU_REGION_SIZE)
    
    print("Inizio della mappatura delle ROM in memoria...")

    # Mappatura basata sul driver MAME (bombjack.cpp per il set 'bombjack')
    try:
        # ROM_LOAD( "09_j01b.bin", 0x0000, 0x2000, ... )
        with open(os.path.join(rom_path, ROM_FILES["cpu1"]), "rb") as f:
            rom_data = f.read()
        cpu_memory[0x0000:0x2000] = rom_data
        print(f"- {ROM_FILES['cpu1']} mappato correttamente a 0x0000.")

        # ROM_LOAD( "10_l01b.bin", 0x2000, 0x2000, ... )
        with open(os.path.join(rom_path, ROM_FILES["cpu2"]), "rb") as f:
            rom_data = f.read()
        cpu_memory[0x2000:0x4000] = rom_data
        print(f"- {ROM_FILES['cpu2']} mappato correttamente a 0x2000.")

        # ROM_LOAD( "11_m01b.bin", 0x4000, 0x2000, ... )
        with open(os.path.join(rom_path, ROM_FILES["cpu3"]), "rb") as f:
            rom_data = f.read()
        cpu_memory[0x4000:0x6000] = rom_data
        print(f"- {ROM_FILES['cpu3']} mappato correttamente a 0x4000.")
        
        # ROM_LOAD( "12_n01b.bin", 0x6000, 0x2000, ... )
        with open(os.path.join(rom_path, ROM_FILES["cpu4"]), "rb") as f:
            rom_data = f.read()
        cpu_memory[0x6000:0x8000] = rom_data
        print(f"- {ROM_FILES['cpu4']} mappato correttamente a 0x6000.")

        # NOTA: C'è un "buco" nella mappa ROM tra 0x8000 e 0xBFFF.
        # Questa area è usata per RAM e I/O, quindi lo script la lascerà
        # correttamente riempita con 0xFF.

        # ROM_LOAD( "13.1r", 0xc000, 0x2000, ... )
        with open(os.path.join(rom_path, ROM_FILES["cpu5"]), "rb") as f:
            rom_data = f.read()
        cpu_memory[0xC000:0xE000] = rom_data
        print(f"- {ROM_FILES['cpu5']} mappato correttamente a 0xC000.")
        
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
    convert_bombjack_roms()