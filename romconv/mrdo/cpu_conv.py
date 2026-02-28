# -*- coding: utf-8 -*-

# Nomi dei file ROM da leggere, in ordine di concatenazione.
# Questi file devono trovarsi nella stessa cartella dello script.
ROM_FILES = [
    "../roms/a4-01.bin",
    "../roms/c4-02.bin",
    "../roms/e4-03.bin",
    "../roms/f4-04.bin"
]

# Nome del file di output che verrà generato.
OUTPUT_FILE = "../../source/src/machines/mrdo/mrdo_rom1.h"

# Nome dell'array C che verrà creato nel file di output.
ARRAY_NAME = "mrdo_rom1"

def create_c_array(infiles, outfile, array_name):
    """
    Legge una lista di file binari, li concatena e li converte
    in un array di byte in formato C.
    """
    all_rom_data = bytearray()

    print("Lettura dei file ROM in corso...")
    # Legge tutti i file ROM e li concatena in un unico bytearray
    for filename in infiles:
        try:
            with open(filename, "rb") as f:
                print(f"  - Leggendo {filename}...")
                all_rom_data.extend(f.read())
        except FileNotFoundError:
            print(f"ERRORE: File non trovato '{filename}'.")
            print("Assicurati che tutti i file .bin siano nella stessa cartella dello script.")
            return # Interrompe l'esecuzione se un file non viene trovato

    print(f"\nScrittura dell'array C nel file '{outfile}'...")
    # Scrive i dati concatenati nel file di output
    with open(outfile, "w") as f:
        # Scrive l'intestazione dell'array
        f.write(f"// File generato da convert_rom.py\n")
        f.write(f"// Dimensione totale: {len(all_rom_data)} bytes\n\n")
        f.write(f"const unsigned char {array_name}[] = {{\n  ")

        # Scrive ogni byte formattato in esadecimale
        for i, byte in enumerate(all_rom_data):
            f.write(f"0x{byte:02X}")

            # Aggiunge una virgola se non è l'ultimo byte
            if i < len(all_rom_data) - 1:
                f.write(",")
                # Va a capo ogni 16 byte per una migliore leggibilità
                if (i + 1) % 16 == 0:
                    f.write("\n  ")
                else:
                    f.write(" ") # Aggiunge uno spazio dopo la virgola

        # Chiude l'array
        f.write("\n};")
    
    print("\nConversione completata con successo!")
    print(f"Il file '{outfile}' è stato creato.")


# Esecuzione della funzione principale
if __name__ == "__main__":
    create_c_array(ROM_FILES, OUTPUT_FILE, ARRAY_NAME)