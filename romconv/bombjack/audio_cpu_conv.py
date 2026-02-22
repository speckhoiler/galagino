#!/usr/bin/env python
import sys
import os

# --- Configurazione per Bomb Jack (Audio CPU) ---
# Nomi dei file ROM richiesti dal driver MAME (bombjack.cpp)
ROM_FILES = {
    "audio1": "../roms/01_h03t.bin"
}

# File di output
OUTPUT_FILE = "../../source/src/machines/bombjack/bombjack_rom2.h"
# Nome dell'array C nel file di output
ROM_ID = "bombjack_rom_cpu2"
# Dimensione totale della regione di memoria per la ROM della CPU Audio
# (come da definizione ROM_REGION( 0x4000, "audiocpu", 0 ))
AUDIO_REGION_SIZE = 0x4000  # 16KB

# --- Funzione principale di conversione (invariata) ---

def create_output_file(rom_data, outfile, id_name):
    """Scrive i dati della ROM in un file header C."""
    print(f"Scrittura del file di output: {outfile}...")
    with open(outfile, "w") as of:
        of.write(f"// File generato automaticamente per {id_name}\n")
        of.write(f"// Contiene la ROM della CPU Audio di Bomb Jack\n")
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

def convert_bombjack_audio_rom():
    """
    Legge il file ROM della CPU audio di Bomb Jack, lo mappa correttamente
    e genera il file header C.
    """
    # Controlla se il file ROM esiste nella directory corrente
    print("Controllo del file ROM necessario per l'Audio CPU di Bomb Jack...")
    rom_path = "." # Si aspetta il file nella stessa directory dello script
    for fname in ROM_FILES.values():
        if not os.path.exists(os.path.join(rom_path, fname)):
            print(f"ERRORE: File ROM non trovato: '{fname}'")
            print("Assicurati che il file ROM sia nella stessa directory dello script.")
            sys.exit(1)
    print("Il file ROM dell'Audio CPU è stato trovato.")

    # Crea un buffer di bytearray per l'intera regione di memoria audio (16KB), inizializzato a 0xFF
    audio_memory = bytearray([0xFF] * AUDIO_REGION_SIZE)
    
    print("Inizio della mappatura della ROM in memoria...")

    # Mappatura basata sul driver MAME (bombjack.cpp)
    try:
        # ROM_LOAD( "01_h03t.3h", 0x0000, 0x2000, ... )
        with open(os.path.join(rom_path, ROM_FILES["audio1"]), "rb") as f:
            rom_data = f.read()
        
        # Inserisce la ROM all'inizio del buffer di memoria
        audio_memory[0x0000:0x2000] = rom_data
        print(f"- {ROM_FILES['audio1']} mappato correttamente a 0x0000.")
        print("- La restante area di memoria (0x2000-0x3FFF) è vuota, come previsto.")

        # L'intero buffer da 16KB è il nostro dato finale.
        final_rom_data = audio_memory

        # Genera il file di output
        create_output_file(final_rom_data, OUTPUT_FILE, ROM_ID)

    except FileNotFoundError as e:
        print(f"ERRORE: File non trovato - {e}")
        sys.exit(1)
    except Exception as e:
        print(f"Si è verificato un errore imprevisto: {e}")
        sys.exit(1)


if __name__ == "__main__":
    convert_bombjack_audio_rom()