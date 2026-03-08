#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import datetime

# --- CONFIGURAZIONE PER PENGO (fissa all'interno dello script) ---
# Nome del file ROM di input
INPUT_ROM_FILE = "../roms/pr1635.51"
# Nome del file header C di output
OUTPUT_H_FILE = "../../source/src/machines/pengo/pengo_wavetable.h"
# Nome dell'array che verrà generato nel file C
C_ARRAY_NAME = "pengo_wavetable"

def parse_pengo_wavetable():
    """
    Funzione specifica per convertire la ROM audio di Pengo.
    Legge un file di input e genera un file header C.
    """
    print(f"--- Conversione Wavetable Audio di Pengo ---")
    
    # Controlla se il file ROM di input esiste
    try:
        print(f"Lettura del file ROM: '{INPUT_ROM_FILE}'...")
        with open(INPUT_ROM_FILE, "rb") as f:
            wave_data = f.read()
    except FileNotFoundError:
        print(f"\nERRORE: File ROM '{INPUT_ROM_FILE}' non trovato.")
        print("Assicurati che il file sia presente nella cartella corretta prima di eseguire lo script.")
        exit(-1)

    # Controlla che il file abbia la dimensione corretta (256 byte)
    if len(wave_data) != 256:
        print(f"\nERRORE: Il file '{INPUT_ROM_FILE}' ha una dimensione di {len(wave_data)} byte, ma dovrebbe essere di 256 byte.")
        exit(-1)
        
    print("ROM letta con successo. Inizio la generazione del file header...")

    # Apre il file di output in scrittura
    with open(OUTPUT_H_FILE, "w") as of:
        # Scrive l'intestazione del file C
        of.write(f"// Wavetable audio per Pengo\n")
        of.write(f"// Generato da script il {datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        of.write(f"// ROM di origine: {os.path.basename(INPUT_ROM_FILE)}\n\n")
        of.write("#pragma once\n\n")
        
        # Inizia la definizione dell'array
        of.write(f"const signed char {C_ARRAY_NAME}[][32] = {{\n")
    
        # La ROM contiene 8 forme d'onda
        for w in range(8):
            of.write(f"  // Waveform #{w}\n")
            
            # (Opzionale) Disegna una rappresentazione ASCII della forma d'onda
            for y in range(8):
                of.write("  //")
                for s in range(32):                
                    val = wave_data[32*w+s]
                    if val == 15 - 2*y:
                        of.write("---")
                    elif val == 15 - (2*y+1):
                        of.write("___")
                    else:
                        of.write("   ")                    
                of.write("\n")            
            
            # Scrive i 32 valori della forma d'onda
            of.write("  {")
            for s in range(32):
                # Converte il valore da 0-15 (unsigned) a -7 a +8 (signed)
                # per centrarlo attorno allo zero, come richiesto dal player audio.
                sample_value = wave_data[32*w+s] - 7
                of.write(f"{sample_value:3d}") # {:3d} formatta il numero per allinearlo bene
                
                # Aggiunge la virgola se non è l'ultimo elemento
                if s != 31:
                    of.write(",")
            
            of.write(" }")
            # Aggiunge la virgola se non è l'ultima forma d'onda
            if w != 7:
                of.write(",")
            
            of.write("\n\n")            
            
        # Chiude la definizione dell'array
        of.write("};\n")

    print(f"\nFile header '{OUTPUT_H_FILE}' generato con successo!")


if __name__ == "__main__":
    # Esegue la funzione di conversione
    parse_pengo_wavetable()