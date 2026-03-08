#!/usr/bin/env python
# -*- coding: utf-8 -*-

import re

# --- CONFIGURAZIONE ---
SOURCE_H_FILE = "../../source/src/machines/mrdo/mrdo_palette.h" 
SPRITE_LOOKUP_PROM_FILE = "../roms/f10--1.bin"
OUTPUT_H_FILE = "../../source/src/machines/mrdo/mrdo_sprite_colormap.h"

def parse_header_palette(file_content):
    """
    Estrae i dati dall'array C 'mrdo_master_palette' da un file .h.
    Questa versione ignora correttamente i commenti.
    """
    print(f"Parsing del file header sorgente '{SOURCE_H_FILE}'...")
    
    match = re.search(r"const uint16_t mrdo_master_palette\[256\] = \{(.*?)\};", file_content, re.DOTALL)
    if not match:
        print(f"ERRORE: Impossibile trovare l'array 'mrdo_master_palette' nel file.")
        return None
        
    palette_data_str = match.group(1)
    
    # --- ESPRESSIONE REGOLARE CORRETTA ---
    # Cerca le righe che iniziano (dopo spazi bianchi) con un valore esadecimale.
    # Il gruppo di cattura (parentesi) prende solo il valore esadecimale, ignorando il resto.
    hex_values = re.findall(r"^\s*(0x[0-9a-fA-F]+)", palette_data_str, re.MULTILINE)
    
    if len(hex_values) != 256:
        print(f"ERRORE: Trovati {len(hex_values)} colori, ma ne erano attesi 256. Controlla il formato del file di input.")
        return None
        
    palette_le = [int(val, 16) for val in hex_values]
    
    print(f"Estratti con successo {len(palette_le)} colori Little Endian dalla Master Palette.")
    return palette_le

def generate_sprite_colormap_file(master_palette_le):
    """
    Usa la Master Palette e il PROM di lookup per costruire la colormap finale degli sprite.
    """
    if not master_palette_le: return

    print(f"Fase 2: Lettura del PROM di lookup degli sprite '{SPRITE_LOOKUP_PROM_FILE}'...")
    try:
        with open(SPRITE_LOOKUP_PROM_FILE, 'rb') as f:
            lookup_prom = f.read(32)
    except FileNotFoundError:
        print(f"ERRORE: File PROM '{SPRITE_LOOKUP_PROM_FILE}' non trovato.")
        return

    print(f"Fase 3: Costruzione della colormap 16x4 degli sprite e scrittura del file '{OUTPUT_H_FILE}'...")
    
    with open(OUTPUT_H_FILE, 'w') as f:
        f.write(f"// Colormap per gli SPRITE di Mr. Do! (formato Little Endian).\n")
        f.write(f"// Generato usando '{SOURCE_H_FILE}' e '{SPRITE_LOOKUP_PROM_FILE}'.\n\n")
        f.write("#pragma once\n#include <stdint.h>\n\n")
        f.write(f"const uint16_t mrdo_sprite_colormap[16][4] = {{\n")
        
        transparent_color = 0x0000

        for palette_num in range(16):
            group_colors = [transparent_color, 0, 0, 0]

            for pen_num in range(1, 4):
                mame_loop_i = (palette_num * 4) + pen_num
                ctabentry_base = lookup_prom[mame_loop_i & 0x1f]
                ctabentry = (ctabentry_base >> 4) if (mame_loop_i & 0x20) else (ctabentry_base & 0x0f)
                final_source_index = ctabentry + ((ctabentry & 0x0c) << 3)
                group_colors[pen_num] = master_palette_le[final_source_index]
            
            f.write(f"  {{ {', '.join(f'0x{c:04X}' for c in group_colors)} }}")
            if palette_num < 15: f.write(",")
            f.write(f" // Palette {palette_num}\n")
            
        f.write("};\n")
    print("File colormap per gli sprite generato con successo!")


def main():
    """Funzione principale."""
    try:
        with open(SOURCE_H_FILE, 'r') as f:
            c_content = f.read()
    except FileNotFoundError:
        print(f"ERRORE: File sorgente '{SOURCE_H_FILE}' non trovato.")
        return
        
    master_palette = parse_header_palette(c_content)
    if master_palette:
        generate_sprite_colormap_file(master_palette)
        print("\nOperazione completata.")

if __name__ == "__main__":
    main()