import os
try:
    from PIL import Image, ImageDraw, ImageFont
    PIL_AVAILABLE = True
except ImportError:
    PIL_AVAILABLE = False

# --- Configurazione ---
FILE_PLANE0_LSB = "../roms/7.2fh"
FILE_PLANE1     = "../roms/8.3fh"
FILE_PLANE2_MSB = "../roms/9.3fh"
OUTPUT_C_FILE = "../../source/src/machines/starforce/starforce_fg_tiles.h"
C_ARRAY_NAME = "starforce_fg_tilemap"

# --- OPZIONI ---
ROTATE_TILES = True 
FLIP_HORIZONTAL = False
GENERATE_PREVIEW = True
OUTPUT_PNG_FILE = "starforce_fg_tiles_preview.png"

# Parametri basati sull'hardware
NUM_TILES = 512
TILE_WIDTH = 8
TILE_HEIGHT = 8
BPP = 3 # Bits Per Pixel

def rotate_tile_90_cw(tile):
    """Ruota un singolo tile (lista di liste) di 90 gradi in senso orario."""
    new_tile = [[0] * TILE_HEIGHT for _ in range(TILE_WIDTH)]
    for y in range(TILE_HEIGHT):
        for x in range(TILE_WIDTH):
            new_tile[x][(TILE_HEIGHT - 1) - y] = tile[y][x]
    return new_tile

def flip_tile_horizontal(tile):
    """Esegue il flip orizzontale di un singolo tile."""
    return [row[::-1] for row in tile]

def convert_tiles():
    """Legge i dati dei 3 bitplane e li converte in un formato di tile 2D."""
    files_to_check = [FILE_PLANE0_LSB, FILE_PLANE1, FILE_PLANE2_MSB]
    for filename in files_to_check:
        if not os.path.exists(filename):
            print(f"Errore: File ROM '{filename}' non trovato. Assicurati che sia nella stessa cartella.")
            return None

    print(f"Lettura dati da '{FILE_PLANE0_LSB}', '{FILE_PLANE1}', '{FILE_PLANE2_MSB}'...")
    with open(FILE_PLANE0_LSB, 'rb') as f_p0, open(FILE_PLANE1, 'rb') as f_p1, open(FILE_PLANE2_MSB, 'rb') as f_p2:
        plane0_data = f_p0.read() # Dati da 7.2fh
        plane1_data = f_p1.read() # Dati da 8.3fh
        plane2_data = f_p2.read() # Dati da 9.3fh

    expected_size = 4096
    if len(plane0_data) != expected_size or len(plane1_data) != expected_size or len(plane2_data) != expected_size:
        print(f"Errore: Dimensione file non corretta. Dovrebbe essere {expected_size} bytes per ciascuno.")
        return None

    print("Decodifica dei tiles in corso (3bpp)...")
    decoded_tiles = []
    
    for tile_idx in range(NUM_TILES):
        tile_data = []
        base_offset = tile_idx * TILE_HEIGHT
        for y in range(TILE_HEIGHT):
            row_data = []
            byte_p0 = plane0_data[base_offset + y] # Byte da 7.2fh
            byte_p1 = plane1_data[base_offset + y] # Byte da 8.3fh
            byte_p2 = plane2_data[base_offset + y] # Byte da 9.3fh
            
            for x in range(TILE_WIDTH):
                bit_shift = 7 - x
                
                # ================================================================
                # --- INIZIO DELLA CORREZIONE APPLICATA ---
                #
                # L'analisi ha dimostrato che i bitplane sono scambiati.
                # Assegniamo i dati dei file ai bit corretti.
                
                pixel_lsb = (byte_p2 >> bit_shift) & 1 # 9.3fh fornisce il bit 0 (LSB)
                pixel_mid = (byte_p1 >> bit_shift) & 1 # 8.3fh fornisce il bit 1
                pixel_msb = (byte_p0 >> bit_shift) & 1 # 7.2fh fornisce il bit 2 (MSB)
                
                # Assembla il valore finale del pixel (0-7)
                final_pixel = (pixel_msb << 2) | (pixel_mid << 1) | pixel_lsb
                # --- FINE DELLA CORREZIONE APPLICATA ---
                # ================================================================

                row_data.append(final_pixel)
            tile_data.append(row_data)
        decoded_tiles.append(tile_data)
    
    print(f"Decodifica completata. {len(decoded_tiles)} tiles 8x8 elaborati.")

    if ROTATE_TILES:
        print("Rotazione di tutti i tiles di 90 gradi in senso orario...")
        decoded_tiles = [rotate_tile_90_cw(tile) for tile in decoded_tiles]

    if FLIP_HORIZONTAL:
        print("Flipping orizzontale di tutti i tiles...")
        decoded_tiles = [flip_tile_horizontal(tile) for tile in decoded_tiles]

    return decoded_tiles

def write_c_array_packed(tiles_data):
    """Scrive i dati dei tile in un file C, impacchettando ogni riga in un uint32_t."""
    if not tiles_data:
        print("Nessun dato da scrivere.")
        return
        
    print(f"Scrittura dell'array C compatto ('packed') nel file '{OUTPUT_C_FILE}'...")
    with open(OUTPUT_C_FILE, 'w') as f_c:
        f_c.write(f"// File generato da starforce_fg_conv.py\n")
        f_c.write(f"// Contiene i {NUM_TILES} tiles di foreground (8x8, 3bpp) per Star Force.\n")
        f_c.write(f"// Ogni riga di 8 pixel (ciascuno di 3 bit) è impacchettata in un intero a 32 bit.\n")
        if ROTATE_TILES:
             f_c.write(f"// I tiles sono stati ruotati di 90 gradi in senso orario.\n")
        if FLIP_HORIZONTAL:
             f_c.write(f"// I tiles sono stati flippati orizzontalmente.\n")
        f_c.write("#include <stdint.h>\n\n")
        f_c.write(f"const uint32_t {C_ARRAY_NAME}[{NUM_TILES}][{TILE_HEIGHT}] = {{\n")

        for i, tile in enumerate(tiles_data):
            f_c.write(f"  {{ // Tile {i:03d} (0x{i:03X})\n")
            packed_rows = []
            for y, row in enumerate(tile):
                packed_row_int = 0
                for x, pixel_value in enumerate(row):
                    packed_row_int |= pixel_value << (BPP * (TILE_WIDTH - 1 - x))
                packed_rows.append(f"0x{packed_row_int:06X}")
            
            f_c.write("    " + ", ".join(packed_rows))
            f_c.write("\n  }")
            if i < NUM_TILES - 1:
                f_c.write(",")
            f_c.write("\n")

        f_c.write("};\n")
    print(f"File '{OUTPUT_C_FILE}' generato con successo!")

def generate_preview_png(tiles_data):
    """Genera un'immagine PNG di anteprima con tutti i tiles in una griglia."""
    if not PIL_AVAILABLE:
        print("\nAVVISO: La libreria Pillow (PIL) non è installata. Impossibile generare l'anteprima PNG.")
        print("Installa con: pip install Pillow")
        return
    if not tiles_data: return
    
    print(f"Generazione dell'anteprima PNG in '{OUTPUT_PNG_FILE}'...")
    PREVIEW_PALETTE = [
        (0, 0, 0), (255, 0, 0), (0, 255, 0), (255, 255, 0),
        (0, 0, 255), (255, 0, 255), (0, 255, 255), (255, 255, 255)
    ]
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
        grid_x, grid_y = i % GRID_COLS, i // GRID_COLS
        cell_ox, cell_oy = grid_x * CELL_WIDTH, grid_y * CELL_HEIGHT
        
        for y in range(TILE_HEIGHT):
            for x in range(TILE_WIDTH):
                color_index = tile[y][x]
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
        if GENERATE_PREVIEW:
            generate_preview_png(tile_database)
        write_c_array_packed(tile_database)