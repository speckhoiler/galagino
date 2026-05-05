#!/usr/bin/env python
import sys
import os

# --- Configurazione Specifica per Donkey Kong Jr. (dkongjrj) ---

# I quattro file ROM che, combinati, contengono i dati degli sprite.
# L'ordine è fondamentale per la corretta decodifica.
INPUT_ROM_FILES = [
    "../roms/v_7c.bin",
    "../roms/v_7d.bin",
    "../roms/v_7e.bin",
    "../roms/v_7f.bin"
]

# Nome del file header di output
OUTPUT_HEADER_FILE = "../../source/src/machines/dkongjr/dkongjr_spritemap.h"

# Nome dell'array C che verrà generato
OUTPUT_ARRAY_NAME = "dkongjr_sprites"

# Numero totale di sprite da processare
NUM_SPRITES = 128


def parse_sprite_dkong(data_chunks):
    """
    Decodifica un singolo sprite 16x16 da 4 chunk di dati (uno per bit-plane).
    Ogni chunk è di 16 byte.
    """
    sprite = []
    for y in range(16):
        row = []
        for x in range(16):
            # La logica è la stessa di Donkey Kong
            # Bit 0 del colore proviene dai primi due chunk
            c0 = 1 if data_chunks[y // 8][15 - x] & (0x80 >> (y & 7)) else 0
            # Bit 1 del colore proviene dagli ultimi due chunk
            c1 = 2 if data_chunks[y // 8 + 2][15 - x] & (0x80 >> (y & 7)) else 0
            row.append(c0 + c1)
        sprite.append(row)
    return sprite

def dump_sprite(data, flip_x, flip_y):
    """
    Converte una matrice di pixel 16x16 in un array di 16 long (32-bit),
    impacchettando 16 pixel a 2-bit in ogni long.
    Applica il flip orizzontale/verticale se richiesto.
    """
    hexs = []
    y_range = reversed(range(16)) if flip_y else range(16)
    
    for y_idx in y_range:
        y = y_idx
        val = 0
        x_range = reversed(range(16)) if flip_x else range(16)
        for x_idx in x_range:
            x = x_idx
            # Impacchetta i pixel a 2 bit in un intero a 32 bit
            val = (val << 2) | data[y][x]
        hexs.append(hex(val))
        
    return ",".join(hexs)

def convert_dkjr_spritemap():
    """
    Funzione principale che legge le ROM degli sprite, le decodifica
    e scrive il file header C di output.
    """
    # 1. Controlla che tutti i file ROM necessari esistano
    print("Controllo dei file ROM degli sprite...")
    for filename in INPUT_ROM_FILES:
        if not os.path.exists(filename):
            print(f"ERRORE: File ROM non trovato: '{filename}'")
            print("Assicurati che i file siano nella stessa directory dello script.")
            sys.exit(1)
    print("Tutti i file ROM sono stati trovati.")

    # 2. Legge i dati dai quattro file ROM
    try:
        spritemap_data = []
        for filename in INPUT_ROM_FILES:
            with open(filename, "rb") as f:
                data = f.read()
                if len(data) != 2048:
                    print(f"ERRORE: La dimensione del file '{filename}' non è 2048 byte.")
                    sys.exit(1)
                spritemap_data.append(data)
    except Exception as e:
        print(f"ERRORE durante la lettura dei file: {e}")
        sys.exit(1)

    # 3. Decodifica tutti gli sprite
    print(f"Decodifica di {NUM_SPRITES} sprite in corso...")
    sprites = []
    for i in range(NUM_SPRITES):
        # Ogni sprite è composto da 16 byte presi da ciascuno dei 4 file
        data_chunks = [rom[16 * i : 16 * (i + 1)] for rom in spritemap_data]
        sprites.append(parse_sprite_dkong(data_chunks))
    print("Decodifica completata.")

    # 4. Scrive il file header C di output con tutte le versioni flippate
    print(f"Scrittura del file di output: '{OUTPUT_HEADER_FILE}'...")
    with open(OUTPUT_HEADER_FILE, "w") as f:
        f.write(f"// File generato automaticamente per gli sprite di Donkey Kong Jr.\n")
        f.write(f"// Dati estratti da: {', '.join(INPUT_ROM_FILES)}\n\n")
        
        f.write(f"const unsigned long {OUTPUT_ARRAY_NAME}[4][{NUM_SPRITES}][16] = {{\n")
        
        # Array 0: Normale (no flip)
        f.write("  // Flip: No\n  {\n")
        sprite_lines = [f"    {{ {dump_sprite(s, False, False)} }}" for s in sprites]
        f.write(",\n".join(sprite_lines))
        f.write("\n  },\n")
        
        # Array 1: Flip solo Verticale
        f.write("  // Flip: Y\n  {\n")
        sprite_lines = [f"    {{ {dump_sprite(s, False, True)} }}" for s in sprites]
        f.write(",\n".join(sprite_lines))
        f.write("\n  },\n")

        # Array 2: Flip solo Orizzontale
        f.write("  // Flip: X\n  {\n")
        sprite_lines = [f"    {{ {dump_sprite(s, True, False)} }}" for s in sprites]
        f.write(",\n".join(sprite_lines))
        f.write("\n  },\n")

        # Array 3: Flip Orizzontale e Verticale
        f.write("  // Flip: XY\n  {\n")
        sprite_lines = [f"    {{ {dump_sprite(s, True, True)} }}" for s in sprites]
        f.write(",\n".join(sprite_lines))
        f.write("\n  }\n")

        f.write("};")
        
    print(f"\nProcesso completato! Il file '{OUTPUT_HEADER_FILE}' è stato creato.")


if __name__ == "__main__":
    convert_dkjr_spritemap()