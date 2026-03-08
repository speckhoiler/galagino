#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os

# --- CONFIGURAZIONE PER PENGO ---
PALETTE_PROM_FILE = "../roms/pr1633.78"
LOOKUP_PROM_FILE = "../roms/pr1634.88"
OUTPUT_H_FILE = "../../source/src/machines/pengo/pengo_colormap.h"
C_COLORMAP_ARRAY_NAME = "pengo_colormap"

def get_bit(value, bit):
    return (value >> bit) & 1

def rgb888_to_rgb565_le(r, g, b):
    """Converte un colore RGB 888 in formato RGB565 Little Endian."""
    r, g, b = [max(0, min(255, c)) for c in (r, g, b)]
    val_be = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
    return ((val_be & 0x00FF) << 8) | ((val_be & 0xFF00) >> 8)

def process_pengo_colormaps():
    """
    Costruisce la colormap finale per Pengo seguendo la logica hardware corretta.
    """
    if not os.path.exists(PALETTE_PROM_FILE):
        print(f"ERRORE: File PROM '{PALETTE_PROM_FILE}' non trovato.")
        return None
    if not os.path.exists(LOOKUP_PROM_FILE):
        print(f"ERRORE: File PROM '{LOOKUP_PROM_FILE}' non trovato.")
        return None

    # --- Step 1: Decodifica la palette base di 32 colori (logica di MAME) ---
    print(f"Lettura e decodifica dei 32 colori base da '{PALETTE_PROM_FILE}'...")
    with open(PALETTE_PROM_FILE, "rb") as f:
        palette_prom_data = f.read()

    base_palette_rgb565 = []
    for i in range(32):
        prom_byte = palette_prom_data[i]
        r = 0x21 * get_bit(prom_byte, 0) + 0x47 * get_bit(prom_byte, 1) + 0x97 * get_bit(prom_byte, 2)
        g = 0x21 * get_bit(prom_byte, 3) + 0x47 * get_bit(prom_byte, 4) + 0x97 * get_bit(prom_byte, 5)
        b = 0x55 * get_bit(prom_byte, 6) + 0xae * get_bit(prom_byte, 7)
        base_palette_rgb565.append(rgb888_to_rgb565_le(r, g, b))
    print(f"Decodificati {len(base_palette_rgb565)} colori base.")

    # --- Step 2: Costruisce la colormap finale pre-calcolata ---
    print(f"Lettura lookup table da '{LOOKUP_PROM_FILE}' e costruzione colormap [2][256][4]...")
    with open(LOOKUP_PROM_FILE, "rb") as f:
        lookup_prom_data = f.read()

    final_colormap = []
    # Itera sui 2 'palette_bank' (selezionati dal gioco via I/O 0x9042)
    for bank in range(2):
        bank_data = []
        
        # L'hardware applica un offset di 0x10 agli indici colore quando il bank è 1
        color_offset = 0x10 if bank == 1 else 0x00
            
        # Itera sui 256 possibili "gruppi" di palette
        # Un gruppo è selezionato da 2 bit di 'colortable_bank' + 6 bit di 'color_attr'
        for group_index in range(256):
            group_data = []
            
            # L'indirizzo base nella PROM di lookup per questo gruppo
            lookup_base_addr = group_index * 4
            
            # Itera sui 4 colori del gruppo (pen 0, 1, 2, 3)
            for pen in range(4):
                # Il colore 0 (pen 0) è sempre trasparente
                if pen == 0:
                    group_data.append(0x0000)
                    continue

                # Legge l'indice a 5 bit dalla lookup table PROM
                raw_color_index = lookup_prom_data[lookup_base_addr + pen]
                
                # Applica l'offset del banco e una maschera per assicurarsi che resti a 5 bit (0-31)
                final_color_index = (raw_color_index + color_offset) & 0x1F
                
                # Ottiene il colore RGB565 finale dalla palette base e lo aggiunge
                final_color = base_palette_rgb565[final_color_index]
                group_data.append(final_color)

            bank_data.append(group_data)
        final_colormap.append(bank_data)

    return final_colormap

def write_c_header(colormap):
    if not colormap:
        print("Dati non validi, scrittura del file C annullata.")
        return

    print(f"\nScrittura del file header '{OUTPUT_H_FILE}'...")
    with open(OUTPUT_H_FILE, 'w') as f:
        f.write(f"// Colormap pre-calcolata di Pengo\n")
        f.write("#pragma once\n#include <stdint.h>\n\n")

        f.write(f"// Colormap finale [palette_bank][group_index][pen] -> [2][256][4]\n")
        f.write(f"// Questa tabella contiene già i colori finali in RGB565 Little Endian.\n")
        f.write(f"const uint16_t {C_COLORMAP_ARRAY_NAME}[2][256][4] = {{\n")
        for bank_idx, bank_data in enumerate(colormap):
            f.write(f"  {{ // --- PALETTE BANK {bank_idx} (I/O 0x9042 = {bank_idx}) ---\n")
            for group_idx, group_data in enumerate(bank_data):
                f.write(f"    {{ ") # Un gruppo di 4 colori
                f.write(", ".join([f"0x{color:04X}" for color in group_data]))
                f.write(f" }}, // Gruppo {group_idx}\n")
            f.write("  },\n")
        f.write("};\n")

    print(f"File '{OUTPUT_H_FILE}' generato con successo!")

if __name__ == "__main__":
    final_colormap = process_pengo_colormaps()
    write_c_header(final_colormap)