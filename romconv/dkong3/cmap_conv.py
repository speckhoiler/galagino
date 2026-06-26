#!/usr/bin/env python
import sys
import os

# --- Configurazione per Donkey Kong 3 (dkong3j) ---
PALETTE_RG_FILE = "../roms/dkc1-c.1d"
PALETTE_B_FILE  = "../roms/dkc1-c.1c"
COLORMAP_LOOKUP_FILE = "../roms/dkc1-v.2n"
OUTPUT_HEADER_FILE = "../../source/src/machines/dkong3/dkong3_cmap.h"
OUTPUT_ARRAY_NAME_BASE = "dkong3_colormap"

def generate_master_palette():
    """
    Genera la palette COMPLETA di 512 colori, rispettando la logica di MAME.
    - Colori TILE (0-255): dati INVERTITI.
    - Colori SPRITE (256-511): dati NON INVERTITI.
    """
    print("Fase 1: Generazione della palette master (512 colori)...")
    
    with open(PALETTE_RG_FILE, "rb") as f:
        prom_rg_data = f.read()
    with open(PALETTE_B_FILE, "rb") as f:
        prom_b_data = f.read()

    if len(prom_rg_data) < 512 or len(prom_b_data) < 512:
        print(f"ERRORE: I file palette devono essere di 512 bytes.")
        sys.exit(1)

    master_palette = []
    for i in range(512):
        if i < 256:
            # Sezione TILE: usa i dati INVERTITI (come nel tuo script originale)
            byte_rg = ~prom_rg_data[i] & 0xFF
            byte_b  = ~prom_b_data[i] & 0xFF
        else:
            # Sezione SPRITE: usa i dati NON INVERTITI, come da commento MAME
            byte_rg = prom_rg_data[i]
            byte_b  = prom_b_data[i]

        # LA TUA FORMULA DI CONVERSIONE (INTATTA, PERCHÉ È CORRETTA)
        r4 = (byte_rg >> 4) & 0x0F
        g4 = byte_rg & 0x0F
        b4 = byte_b & 0x0F
        
        r8 = int((r4 / 15.0) * 255)
        g8 = int((g4 / 15.0) * 255)
        b8 = int((b4 / 15.0) * 255)
        
        r5 = r8 >> 3
        g6 = g8 >> 2
        b5 = b8 >> 3
        rgb565 = (r5 << 11) | (g6 << 5) | b5
        
        rgb_swapped = ((rgb565 & 0xFF00) >> 8) | ((rgb565 & 0x00FF) << 8)
        master_palette.append(rgb_swapped)
        
    print(f"Palette master di {len(master_palette)} colori generata.")
    return master_palette


def generate_c_header(tile_palette, sprite_palette):
    """
    Genera il file header C usando le palette separate per tile e sprite.
    """
    print(f"Fase 2: Scrittura del file header C...")
    
    with open(COLORMAP_LOOKUP_FILE, "rb") as f:
        colormap_lookup_data = f.read()

    with open(OUTPUT_HEADER_FILE, "w") as f:
        f.write(f"// File generato automaticamente per la colormap di Donkey Kong 3.\n\n")

        # --- Palette per i TILE ---
        # Questa è la palette di 256 colori che il tuo codice usa per i tile.
        tile_palette_name = f"{OUTPUT_ARRAY_NAME_BASE}_tile_palette"
        f.write(f"const unsigned short {tile_palette_name}[256] = {{\n  ")
        color_hex = [hex(c) for c in tile_palette]
        for i in range(len(color_hex)):
            f.write(color_hex[i])
            if i < len(color_hex) - 1:
                f.write(", ")
            if (i + 1) % 16 == 0:
                f.write("\n  ")
        f.write("\n};\n\n")

        # --- Lookup Table per i TILE ---
        tile_map_name = f"{OUTPUT_ARRAY_NAME_BASE}_tile_lookup"
        f.write(f"const unsigned char {tile_map_name}[256] = {{\n  ")
        f.write(", ".join([hex(val & 0x0F) for val in colormap_lookup_data]))
        f.write("\n};\n\n")

        # --- Colormap pre-calcolata per gli SPRITE ---
        sprite_map_name = f"{OUTPUT_ARRAY_NAME_BASE}_sprite"
        f.write(f"const unsigned short {sprite_map_name}[64][4] = {{\n")
        
        sprite_entries = []
        for i in range(64):
            # Usiamo la palette dedicata agli sprite che abbiamo già estratto
            base_idx = i * 4
            color_group = [
                sprite_palette[base_idx + 0],
                sprite_palette[base_idx + 1],
                sprite_palette[base_idx + 2],
                sprite_palette[base_idx + 3]
            ]
            sprite_entries.append("  /* Palette " + str(i) + " */ {" + ",".join([hex(c) for c in color_group]) + "}")

        f.write(",\n".join(sprite_entries))
        f.write("\n};")
        
    print(f"File '{OUTPUT_HEADER_FILE}' scritto con successo.")


def main():
    required_files = [PALETTE_RG_FILE, PALETTE_B_FILE, COLORMAP_LOOKUP_FILE]
    for filename in required_files:
        if not os.path.exists(filename):
            print(f"ERRORE: File ROM necessario non trovato: '{filename}'")
            sys.exit(1)
    
    # 1. Genera la palette master completa
    master_palette = generate_master_palette()
    
    # 2. Dividi la palette nelle due sezioni corrette
    tile_palette = master_palette[:256]
    sprite_palette = master_palette[256:]
    
    # 3. Scrivi il file C header
    generate_c_header(tile_palette, sprite_palette)
    
    print("\nProcesso completato!")

if __name__ == "__main__":
    main()