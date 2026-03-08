#!/usr/bin/env python
# -*- coding: utf-8 -*-

import re

# --- CONFIGURAZIONE ---
SOURCE_C_FILE = "palette_sprite.c"
OUTPUT_H_FILE = "../../source/src/machines/mrdo/mrdo_palette.h" # Nome file cambiato per chiarezza

def rgb888_to_rgb565_be(r, g, b):
    """
    Converte RGB888 in RGB565 standard (Big Endian), applicando il clamping.
    """
    r = max(0, min(255, r))
    g = max(0, min(255, g))
    b = max(0, min(255, b))
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)

# --- NUOVA FUNZIONE DI CONVERSIONE ---
def rgb565_be_to_le(val_be):
    """Converte un valore RGB565 da Big Endian a Little Endian (scambia i byte)."""
    return ((val_be & 0x00FF) << 8) | ((val_be & 0xFF00) >> 8)

def parse_c_palette_array(file_content):
    """Estrae i dati dall'array C 'mrdo_palette'."""
    print(f"Parsing del file sorgente C '{SOURCE_C_FILE}'...")
    
    match = re.search(r"static const uint8_t mrdo_palette\[256\]\[3\] = \{(.*?)\};", file_content, re.DOTALL)
    if not match:
        print("ERRORE: Impossibile trovare l'array 'mrdo_palette' nel file.")
        return None
        
    palette_data_str = match.group(1)
    numbers = [int(n) for n in re.findall(r"-?\d+", palette_data_str)]
    
    if len(numbers) != 256 * 3:
        print(f"ERRORE: Trovati {len(numbers)} valori, ma ne erano attesi 768 (256x3).")
        return None
        
    palette_rgb888 = [tuple(numbers[i:i+3]) for i in range(0, len(numbers), 3)]
    
    print(f"Estratti con successo {len(palette_rgb888)} colori RGB888.")
    return palette_rgb888

def generate_palette_header(palette_rgb888):
    """
    Converte la palette in RGB565 LITTLE ENDIAN e scrive il file header C.
    """
    if not palette_rgb888:
        return
        
    print(f"Conversione in RGB565 Little Endian e scrittura del file header '{OUTPUT_H_FILE}'...")
    
    try:
        with open(OUTPUT_H_FILE, 'w') as f:
            f.write(f"// Master Palette di Mr. Do! convertita in formato RGB565 LITTLE ENDIAN.\n")
            f.write("#pragma once\n#include <stdint.h>\n\n")
            f.write(f"// Palette completa da 256 colori (formato Little Endian)\n")
            f.write(f"const uint16_t mrdo_master_palette[256] = {{\n")
            
            for i, (r, g, b) in enumerate(palette_rgb888):
                # Fase 1: Converti in RGB565 standard (Big Endian)
                rgb565_be_color = rgb888_to_rgb565_be(r, g, b)
                
                # Fase 2: Converti il risultato in Little Endian
                rgb565_le_color = rgb565_be_to_le(rgb565_be_color)
                
                # Scrivi il valore Little Endian nel file, ma commenta con entrambi per chiarezza
                f.write(f"  0x{rgb565_le_color:04X},")
                f.write(f" // Indice {i:3d} | Original: ({r:4d}, {g:4d}, {b:4d}) | BE: 0x{rgb565_be_color:04X}\n")
            
            f.write("};\n")
        print("File header della palette Little Endian generato con successo!")
    except Exception as e:
        print(f"ERRORE durante la scrittura del file di output: {e}")


def main():
    """Funzione principale che orchestra il processo."""
    try:
        with open(SOURCE_C_FILE, 'r') as f:
            c_content = f.read()
    except FileNotFoundError:
        print(f"ERRORE: File sorgente '{SOURCE_C_FILE}' non trovato.")
        return
        
    palette_data = parse_c_palette_array(c_content)
    if palette_data:
        generate_palette_header(palette_data)
        print("\nOperazione completata.")

if __name__ == "__main__":
    main()