import os
try:
    from PIL import Image, ImageDraw, ImageFont
    PIL_AVAILABLE = True
except ImportError:
    PIL_AVAILABLE = False

# --- Configurazione ---
# Nomi dei file ROM di input (dalla rom 'mrdo' di MAME)
# Assegnazione basata sulla definizione ufficiale del driver MAME
FILE_PLANE0_LSB = "../roms/u8-10.bin"
FILE_PLANE1_MSB = "../roms/s8-09.bin"

# Nome del file C di output
OUTPUT_C_FILE = "../../source/src/machines/mrdo/mrdo_fg_tiles.h"
C_ARRAY_NAME = "mrdo_fg_tiles"

# --- OPZIONI ---
ROTATE_TILES = True
FLIP_HORIZONTAL = True
GENERATE_PREVIEW = True
OUTPUT_PNG_FILE = "mrdo_fg_tiles_preview.png"

# Parametri
NUM_TILES = 512
TILE_WIDTH = 8
TILE_HEIGHT = 8

def rotate_tile_90_ccw(tile):
    """Ruota un singolo tile (lista di liste) di 90 gradi in senso antiorario."""
    new_tile = [[0] * TILE_HEIGHT for _ in range(TILE_WIDTH)]
    for y in range(TILE_HEIGHT):
        for x in range(TILE_WIDTH):
            new_tile[(TILE_WIDTH - 1) - x][y] = tile[y][x]
    return new_tile

def flip_tile_horizontal(tile):
    """Esegue il flip orizzontale di un singolo tile."""
    return [row[::-1] for row in tile]

def convert_tiles():
    """Legge i dati dei bitplane dai file ROM e li converte in un formato di tile 2D."""
    if not os.path.exists(FILE_PLANE0_LSB) or not os.path.exists(FILE_PLANE1_MSB):
        print(f"Errore: Assicurati che i file '{FILE_PLANE0_LSB}' e '{FILE_PLANE1_MSB}' siano presenti nella stessa cartella.")
        return None

    print(f"Lettura dati da '{FILE_PLANE0_LSB}' (LSB) e '{FILE_PLANE1_MSB}' (MSB)...")
    with open(FILE_PLANE0_LSB, 'rb') as f_p0, open(FILE_PLANE1_MSB, 'rb') as f_p1:
        plane0_data = f_p0.read()
        plane1_data = f_p1.read()

    if len(plane0_data) != 4096 or len(plane1_data) != 4096:
        print("Errore: Dimensione file non corretta. Dovrebbe essere 4096 bytes ciascuno.")
        return None

    print("Decodifica dei tiles in corso...")
    decoded_tiles = []
    
    for tile_idx in range(NUM_TILES):
        tile_data = []
        base_offset = tile_idx * TILE_HEIGHT
        for y in range(TILE_HEIGHT):
            row_data = []
            byte_p0 = plane0_data[base_offset + y] # Dati per il bit 0 (LSB)
            byte_p1 = plane1_data[base_offset + y] # Dati per il bit 1 (MSB)
            for x in range(TILE_WIDTH):
                bit_shift = 7 - x
                pixel_p0 = (byte_p0 >> bit_shift) & 1
                pixel_p1 = (byte_p1 >> bit_shift) & 1
                final_pixel = (pixel_p1 << 1) | pixel_p0
                row_data.append(final_pixel)
            tile_data.append(row_data)
        decoded_tiles.append(tile_data)
    
    print(f"Decodifica completata. {len(decoded_tiles)} tiles elaborati.")

    if ROTATE_TILES:
        print("Rotazione di tutti i tiles di 90 gradi in senso antiorario...")
        decoded_tiles = [rotate_tile_90_ccw(tile) for tile in decoded_tiles]

    if FLIP_HORIZONTAL:
        print("Flipping orizzontale di tutti i tiles...")
        decoded_tiles = [flip_tile_horizontal(tile) for tile in decoded_tiles]

    return decoded_tiles

def write_c_array(tiles_data):
    """Scrive i dati dei tile decodificati in un file sorgente C."""
    if not tiles_data:
        print("Nessun dato da scrivere.")
        return
    print(f"Scrittura dell'array C nel file '{OUTPUT_C_FILE}'...")
    with open(OUTPUT_C_FILE, 'w') as f_c:
        f_c.write(f"// File generato da script. Contiene i {NUM_TILES} tiles di foreground (8x8, 2bpp) per Mr. Do! (Universal).\n")
        f_c.write(f"// Decodifica basata sulla definizione ufficiale del driver MAME.\n")
        if ROTATE_TILES:
             f_c.write(f"// I tiles sono stati ruotati di 90 gradi in senso antiorario.\n")
        if FLIP_HORIZONTAL:
             f_c.write(f"// I tiles sono stati flippati orizzontalmente.\n")
        f_c.write("#include <stdint.h>\n\n")
        f_c.write(f"const uint8_t {C_ARRAY_NAME}[{NUM_TILES}][{TILE_HEIGHT}][{TILE_WIDTH}] = {{\n")
        for i, tile in enumerate(tiles_data):
            f_c.write(f"  {{ // Tile {i:03d} (0x{i:03X})\n")
            for y, row in enumerate(tile):
                f_c.write("    {")
                f_c.write(", ".join(map(str, row)))
                f_c.write("}")
                if y < TILE_HEIGHT - 1:
                    f_c.write(",")
                f_c.write("\n")
            f_c.write("  }")
            if i < NUM_TILES - 1:
                f_c.write(",")
            f_c.write("\n")
        f_c.write("};\n")
    print(f"File '{OUTPUT_C_FILE}' generato con successo!")

def generate_preview_png(tiles_data):
    """Genera un'immagine PNG di anteprima con tutti i tiles in una griglia."""
    if not PIL_AVAILABLE:
        print("\nAVVISO: La libreria Pillow non è installata. Impossibile generare l'anteprima PNG.")
        print("Installa con: pip install Pillow")
        return
    if not tiles_data:
        print("Nessun dato per generare l'anteprima.")
        return
    print(f"Generazione dell'anteprima PNG in '{OUTPUT_PNG_FILE}'...")
    PREVIEW_PALETTE = [(0, 0, 0), (0, 0, 255), (0, 255, 0), (255, 255, 255)]
    GRID_COLS, UPSCALE_FACTOR = 32, 4
    GRID_ROWS = (NUM_TILES + GRID_COLS - 1) // GRID_COLS
    CELL_WIDTH = TILE_WIDTH * UPSCALE_FACTOR + 4
    CELL_HEIGHT = TILE_HEIGHT * UPSCALE_FACTOR + 16
    IMG_WIDTH, IMG_HEIGHT = GRID_COLS * CELL_WIDTH, GRID_ROWS * CELL_HEIGHT
    img = Image.new('RGB', (IMG_WIDTH, IMG_HEIGHT), (80, 80, 80))
    draw = ImageDraw.Draw(img)
    try:
        font = ImageFont.truetype("arial.ttf", 10)
    except IOError:
        font = ImageFont.load_default()
        
    for i, tile in enumerate(tiles_data):
        # --- INIZIO DELLA CORREZIONE ---
        # Applica una rotazione di 180 gradi solo per la visualizzazione,
        # in modo che l'anteprima corrisponda all'orientamento finale in-game.
        # Questo non altera i dati scritti nel file C.
        tile_for_preview = [row[::-1] for row in tile[::-1]]
        # --- FINE DELLA CORREZIONE ---
        
        grid_x, grid_y = i % GRID_COLS, i // GRID_COLS
        cell_ox, cell_oy = grid_x * CELL_WIDTH, grid_y * CELL_HEIGHT
        
        # Usa il tile corretto per il disegno (tile_for_preview)
        for y in range(TILE_HEIGHT):
            for x in range(TILE_WIDTH):
                color_index = tile_for_preview[y][x]
                draw.rectangle(
                    (cell_ox + x * UPSCALE_FACTOR, cell_oy + y * UPSCALE_FACTOR,
                     cell_ox + (x + 1) * UPSCALE_FACTOR - 1, cell_oy + (y + 1) * UPSCALE_FACTOR - 1),
                    fill=PREVIEW_PALETTE[color_index]
                )
                
        text_pos_x = cell_ox + (CELL_WIDTH - 4) // 2
        text_pos_y = cell_oy + TILE_HEIGHT * UPSCALE_FACTOR + 2
        draw.text((text_pos_x, text_pos_y), str(i), font=font, fill=(220, 220, 220), anchor="mt")
        
    img.save(OUTPUT_PNG_FILE)
    print("Anteprima PNG generata con successo!")

if __name__ == "__main__":
    tile_database = convert_tiles()
    if tile_database:
        write_c_array(tile_database)
        if GENERATE_PREVIEW:
            generate_preview_png(tile_database)