#!/usr/bin/env python
import sys
import os

# --- Configurazione per Donkey Kong 3 (dkong3j) ---
INPUT_ROM_FILES = [
    "../roms/dk3v.7c",
    "../roms/dk3v.7d",
    "../roms/dk3v.7e",
    "../roms/dk3v.7f"
]
OUTPUT_HEADER_FILE = "../../source/src/machines/dkong3/dkong3_spritemap.h"
OUTPUT_ARRAY_NAME = "dkong3_sprites"
NUM_SPRITES = 256
SPRITE_WIDTH = 16
SPRITE_HEIGHT = 16

# ---------------------------------------------------------------------
# Ruotare automaticamente tutti gli sprite di N gradi? (0, 90, 180, 270)
DEFAULT_ROTATE_ALL = 0   # <--- Cambia qui se vuoi ruotare diversamente, 270° = come MAME
# ---------------------------------------------------------------------
# TABELLONE: rotazione e flip X/Y per singolo sprite (modifica qui)
# Chiave: indice sprite; Valore: (rotazione in gradi, flip_x, flip_y)
# Esempio: 25: (90, True, False)   => Sprite 25 ruotato 90° e flippato su X
#          30: (180, False, True)  => Sprite 30 ruotato 180° e flippato su Y
SPRITE_TRANSFORM = {
    #183: (180, False, False),
    #184: (180, False, False),
    #160: (180, False, False),
    #161: (180, False, False),
    #162: (180, False, False),
    #163: (180, False, False),
    # 30: (180, False, True),
    # Puoi aggiungere qui altri sprite e trasformazioni!
}
# ---------------------------------------------------------------------

def flip_matrix_x(matrix):
    # Flip orizzontale
    return [row[::-1] for row in matrix]

def flip_matrix_y(matrix):
    # Flip verticale
    return matrix[::-1]

def rotate_matrix(matrix, angle):
    # Rotazione in senso orario (clockwise)
    if angle == 0:
        return matrix
    elif angle == 90:
        return [[matrix[15 - x][y] for x in range(16)] for y in range(16)]
    elif angle == 180:
        return [row[::-1] for row in matrix[::-1]]
    elif angle == 270:
        return [[matrix[x][15 - y] for x in range(16)] for y in range(16)]
    else:
        raise ValueError(f"Angolo di rotazione non valido: {angle}")

def parse_sprite_dkong(data_chunks, sprite_index=None):
    """
    Decodifica un singolo sprite 16x16.
    Applica anche eventuali trasformazioni custom da SPRITE_TRANSFORM e la rotazione globale.
    """
    sprite = []
    for y in range(SPRITE_HEIGHT):
        row = []
        for x in range(SPRITE_WIDTH):
            # Invertiamo l'assegnazione di c0 e c1 (patch storica)
            c1 = 2 if data_chunks[y // 8 + 2][15 - x] & (0x80 >> (y & 7)) else 0
            c0 = 1 if data_chunks[y // 8][15 - x] & (0x80 >> (y & 7)) else 0
            row.append(c0 + c1)
        sprite.append(row)
    # --- Applica rotazione globale per matchare MAME ---
    if DEFAULT_ROTATE_ALL:
        sprite = rotate_matrix(sprite, DEFAULT_ROTATE_ALL)
    # --- Applica rotazione/flip custom (override per singolo sprite) ---
    rot, fx, fy = SPRITE_TRANSFORM.get(sprite_index, (0, False, False))
    if rot:
        sprite = rotate_matrix(sprite, rot)
    if fx:
        sprite = flip_matrix_x(sprite)
    if fy:
        sprite = flip_matrix_y(sprite)
    return sprite

def dump_sprite(data, flip_x, flip_y):
    """Converte la matrice di pixel in un array C, gestendo il flip per header."""
    hexs = []
    y_range = reversed(range(16)) if flip_y else range(16)

    for y_idx in y_range:
        val = 0
        pixel_row = data[y_idx]
        if flip_x:
            pixel_row = pixel_row[::-1]
        for pixel_value in pixel_row:
            val = (val << 2) | pixel_value
        hexs.append(hex(val))

    return ",".join(hexs)

def main():
    print("Avvio conversione sprite per DK3j...")
    try:
        roms_data = []
        for filename in INPUT_ROM_FILES:
            if not os.path.exists(filename):
                raise FileNotFoundError(f"File ROM non trovato: {filename}")
            with open(filename, "rb") as f:
                roms_data.append(f.read())

        sprites = []
        for i in range(NUM_SPRITES):
            sprite_offset = i * 16
            data_chunks = [rom[sprite_offset : sprite_offset + 16] for rom in roms_data]
            sprites.append(parse_sprite_dkong(data_chunks, i))

        with open(OUTPUT_HEADER_FILE, "w") as f:
            f.write(f"// File generato da sprites_conv.py (modifica: flip/rotazione per sprite)\n")
            f.write(f"// Dati da: {', '.join(INPUT_ROM_FILES)}\n\n")
            f.write("#include <pgmspace.h>\n\n")

            f.write(f"const unsigned long PROGMEM {OUTPUT_ARRAY_NAME}[4][{NUM_SPRITES}][{SPRITE_HEIGHT}] = {{\n")

            for flip_flag in range(4):
                flip_x = (flip_flag & 1) != 0
                flip_y = (flip_flag & 2) != 0
                f.write(f"  // Flip: X={flip_x}, Y={flip_y}\n  {{\n")

                sprite_lines = [f"    {{ {dump_sprite(s, flip_x, flip_y)} }}" for s in sprites]
                f.write(",\n".join(sprite_lines))

                f.write("\n  }" + ("," if flip_flag < 3 else ""))
            f.write("\n};\n")

        print(f"\nProcesso completato! Creato '{OUTPUT_HEADER_FILE}'.")
    except Exception as e:
        print(f"ERRORE: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
