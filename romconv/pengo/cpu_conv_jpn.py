# -*- coding: utf-8 -*-
"""
Questo script converte i file ROM binari di Pengo in un header file C (.h).
"""

# --- CONFIGURAZIONE DEI LAVORI DI CONVERSIONE ---
CONVERSION_JOBS = [
    {
        "description": "Pengo Main CPU ROM (set 'pengo')",
        "input_files": [
            "../roms/epr-5120.ic8",   # 0000-0fff
            "../roms/epr-5121.ic7",   # 1000-1fff
            "../roms/epr-5122.ic15",  # 2000-2fff
            "../roms/epr-5123.ic14", # 3000-3fff
            "../roms/ep5124.21",  # 4000-4fff
            "../roms/epr-5125.ic20",  # 5000-5fff
            "../roms/ep5126.32",  # 6000-6fff
            "../roms/epr-5127.ic31"  # 7000-7fff
        ],
        "output_file": "../../source/src/machines/pengo/pengo_rom.h",
        "array_name": "pengo_rom"
    }
]

def create_c_array(job_description, infiles, outfile, array_name):
    """
    Legge una lista di file binari, li concatena e li converte
    in un array di byte in formato C.
    """
    print(f"--- Inizio processo per: {job_description} ---")
    all_rom_data = bytearray()
    
    # Legge tutti i file ROM per questo lavoro e li concatena
    print("Lettura dei file ROM in corso...")
    for filename in infiles:
        try:
            with open(filename, "rb") as f:
                print(f"  - Leggendo {filename}...")
                all_rom_data.extend(f.read())
        except FileNotFoundError:
            print(f"\nERRORE: File non trovato '{filename}'.")
            print("Assicurati che tutti i file .bin siano nella stessa cartella dello script.")
            print(f"--- Processo per {job_description} interrotto. ---\n")
            return

    # Scrive i dati concatenati nel file di output
    print(f"\nScrittura dell'array C nel file '{outfile}'...")
    with open(outfile, "w") as f:
        f.write(f"// File generato da cpu_conv.py\n")
        f.write(f"// ROM per: {job_description}\n")
        f.write(f"// Dimensione totale: {len(all_rom_data)} bytes\n\n")
        f.write(f"const unsigned char {array_name}[] = {{\n  ")

        for i, byte in enumerate(all_rom_data):
            f.write(f"0x{byte:02X}")
            if i < len(all_rom_data) - 1:
                f.write(",")
                if (i + 1) % 16 == 0:
                    f.write("\n  ")
                else:
                    f.write(" ")

        f.write("\n};")
    
    print(f"\nConversione per '{job_description}' completata con successo!")
    print(f"Il file '{outfile}' è stato creato.")
    print(f"--- Fine processo per: {job_description} ---\n")

if __name__ == "__main__":
    print("Avvio dello script di conversione ROM di Pengo...")
    print("="*50 + "\n")

    for job in CONVERSION_JOBS:
        create_c_array(
            job_description=job["description"],
            infiles=job["input_files"],
            outfile=job["output_file"],
            array_name=job["array_name"]
        )
    
    print("Tutti i lavori di conversione sono terminati.")