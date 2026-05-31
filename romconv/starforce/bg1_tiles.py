import os
try:
    from PIL import Image, ImageDraw, ImageFont
    PIL_AVAILABLE = True
except ImportError:
    PIL_AVAILABLE = False

# --- CONFIGURAZIONE ---
# Decommenta il blocco relativo al layer che vuoi generare.

# --- Background Layer 1 (256 tiles) ---
FILE_PLANE0 = "../roms/15.10jk"  # Questo sarà trattato come MSB
FILE_PLANE1 = "../roms/14.9jk"  # Questo rimane il bit centrale
FILE_PLANE2 = "../roms/13.8jk"  # Questo sarà trattato come LSB
NUM_TILES = 256
INVERT_BITPLANES = True  # <-- OPZIONE CHIAVE ATTIVATA PER BG1
OUTPUT_C_FILE = "../../source/src/machines/starforce/starforce_bg1_tiles.h"
C_ARRAY_NAME = "starforce_bg1_tilemap"
PREVIEW_PNG_FILE = "starforce_bg1_tiles_preview.png"

# --- Background Layer 2 (256 tiles) ---
# FILE_PLANE0 = "12.10de"
# FILE_PLANE1 = "11.9de"
# FILE_PLANE2 = "10.8de"
# NUM_TILES = 256
# INVERT_BITPLANES = False # Per BG2 proviamo a lasciare l'ordine standard
# OUTPUT_C_FILE = "starforc_bg2_tiles.h"
# C_ARRAY_NAME = "starforc_bg2_tilemap"
# PREVIEW_PNG_FILE = "starforc_bg2_tiles_preview.png"

# --- Background Layer 3 (128 tiles) ---
# FILE_PLANE0 = "18.10pq"
# FILE_PLANE1 = "17.9pq"
# FILE_PLANE2 = "16.8pq"
# NUM_TILES = 128
# INVERT_BITPLANES = True # Anche per BG3 l'inversione è probabilmente corretta
# OUTPUT_C_FILE = "starforc_bg3_tiles.h"
# C_ARRAY_NAME = "starforc_bg3_tilemap"
# PREVIEW_PNG_FILE = "starforc_bg3_tiles_preview.png"


# --- OPZIONI ---
ROTATION_ANGLE = 0
FLIP_HORIZONTAL = False
GENERATE_PREVIEW = True

# --- PARAMETRI HARDWARE ---
TILE_WIDTH = 16
TILE_HEIGHT = 16
BPP = 3

# ... (funzioni di trasformazione invariate) ...
def rotate_tile_90_cw(tile, w, h):
    new_tile = [[0] * h for _ in range(w)]
    for y in range(h):
        for x in range(w):
            new_tile[x][(h - 1) - y] = tile[y][x]
    return new_tile
def rotate_tile_180(tile, w, h):
    new_tile = [[0] * w for _ in range(h)]
    for y in range(h):
        for x in range(w):
            new_tile[(h - 1) - y][(w - 1) - x] = tile[y][x]
    return new_tile
def rotate_tile_270_cw(tile, w, h):
    new_tile = [[0] * h for _ in range(w)]
    for y in range(h):
        for x in range(w):
            new_tile[(w - 1) - x][y] = tile[y][x]
    return new_tile
def flip_tile_horizontal(tile):
    return [row[::-1] for row in tile]


def decode_bg_tiles():
    files_to_check = [FILE_PLANE0, FILE_PLANE1, FILE_PLANE2]
    for filename in files_to_check:
        if not os.path.exists(filename):
            print(f"Errore: File ROM '{filename}' non trovato.")
            return None

    print(f"Lettura dati da '{FILE_PLANE0}', '{FILE_PLANE1}', '{FILE_PLANE2}'...")
    with open(FILE_PLANE0, 'rb') as f_p0, open(FILE_PLANE1, 'rb') as f_p1, open(FILE_PLANE2, 'rb') as f_p2:
        plane0_data = f_p0.read()
        plane1_data = f_p1.read()
        plane2_data = f_p2.read()

    bytes_per_tile_plane = (TILE_WIDTH * TILE_HEIGHT) // 8
    expected_size = NUM_TILES * bytes_per_tile_plane
    if len(plane0_data) != expected_size or len(plane1_data) != expected_size or len(plane2_data) != expected_size:
        print(f"Errore: Dimensione file non corretta. Dovrebbe essere {expected_size} bytes per {NUM_TILES} tiles.")
        return None

    layout_x = [ 0, 1, 2, 3, 4, 5, 6, 7, 8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 ]
    layout_y = [ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 ]
    
    print(f"Decodifica dei {NUM_TILES} background tiles ({TILE_WIDTH}x{TILE_HEIGHT}, 3bpp)...")
    if INVERT_BITPLANES:
        print("AVVISO: Ordine dei bitplane INVERTITO (MSB<->LSB).")

    decoded_tiles = []
    
    for tile_idx in range(NUM_TILES):
        tile_data = [[0] * TILE_WIDTH for _ in range(TILE_HEIGHT)]
        char_base_offset = tile_idx * bytes_per_tile_plane

        for y in range(TILE_HEIGHT):
            for x in range(TILE_WIDTH):
                bit_offset = layout_y[y] + layout_x[x]
                byte_addr = char_base_offset + (bit_offset // 8)
                bit_pos = 7 - (bit_offset % 8)
                
                # Leggi i bit dai dati dei piani
                val_p0 = (plane0_data[byte_addr] >> bit_pos) & 1
                val_p1 = (plane1_data[byte_addr] >> bit_pos) & 1
                val_p2 = (plane2_data[byte_addr] >> bit_pos) & 1
                
                # Assegna ai bitplane corretti in base all'opzione
                if INVERT_BITPLANES:
                    # Stessa logica che ha funzionato per il foreground
                    bit_msb = val_p0
                    bit_mid = val_p1
                    bit_lsb = val_p2
                else:
                    # Ordine standard
                    bit_lsb = val_p0
                    bit_mid = val_p1
                    bit_msb = val_p2
                
                pixel_value = (bit_msb << 2) | (bit_mid << 1) | bit_lsb
                tile_data[y][x] = pixel_value
                
        decoded_tiles.append(tile_data)
    
    print(f"Decodifica completata.")
    # ... resto dello script invariato ...
    if ROTATION_ANGLE == 90:
        print("Rotazione di tutti i tiles di 90 gradi in senso orario...")
        decoded_tiles = [rotate_tile_90_cw(t, TILE_WIDTH, TILE_HEIGHT) for t in decoded_tiles]
    elif ROTATION_ANGLE == 180:
        print("Rotazione di tutti i tiles di 180 gradi...")
        decoded_tiles = [rotate_tile_180(t, TILE_WIDTH, TILE_HEIGHT) for t in decoded_tiles]
    elif ROTATION_ANGLE == 270:
        print("Rotazione di tutti i tiles di 270 gradi in senso orario...")
        decoded_tiles = [rotate_tile_270_cw(t, TILE_WIDTH, TILE_HEIGHT) for t in decoded_tiles]
    elif ROTATION_ANGLE != 0:
        print(f"AVVISO: Angolo di rotazione '{ROTATION_ANGLE}' non valido. Nessuna rotazione applicata.")
    if FLIP_HORIZONTAL:
        print("Flipping orizzontale di tutti i tiles (post-rotazione)...")
        decoded_tiles = [flip_tile_horizontal(t) for t in decoded_tiles]
    return decoded_tiles
    
# ... (le funzioni write_c_array_packed e generate_preview_png sono identiche a prima) ...
def write_c_array_packed(tiles_data):
    if not tiles_data:
        print("Nessun dato da scrivere.")
        return
    print(f"Scrittura dell'array C compatto nel file '{OUTPUT_C_FILE}'...")
    with open(OUTPUT_C_FILE, 'w') as f_c:
        f_c.write(f"// File generato da bg_tiles.py\n")
        f_c.write(f"// Contiene i {NUM_TILES} tiles di background ({TILE_WIDTH}x{TILE_HEIGHT}, 3bpp).\n")
        if ROTATION_ANGLE != 0:
             f_c.write(f"// I tiles sono stati ruotati di {ROTATION_ANGLE} gradi in senso orario.\n")
        else:
             f_c.write(f"// I tiles non sono stati ruotati.\n")
        if FLIP_HORIZONTAL:
            f_c.write(f"// I tiles sono stati flippati orizzontalmente (dopo la rotazione).\n")
        if INVERT_BITPLANES:
            f_c.write(f"// L'ordine dei bitplane è stato INVERTITO.\n")
        f_c.write("#include <stdint.h>\n\n")
        if not tiles_data: return
        height = len(tiles_data[0])
        width = len(tiles_data[0][0])
        chunks_per_row = width // 8
        f_c.write(f"const uint32_t {C_ARRAY_NAME}[{NUM_TILES}][{height}][{chunks_per_row}] = {{\n")
        for i, tile in enumerate(tiles_data):
            f_c.write(f"  {{ // Tile {i:03d} (0x{i:03X})\n")
            for y, row in enumerate(tile):
                f_c.write("    { ")
                packed_chunks = []
                for chunk_idx in range(chunks_per_row):
                    packed_int = 0
                    for x_in_chunk in range(8):
                        x = chunk_idx * 8 + x_in_chunk
                        pixel_value = row[x]
                        packed_int |= pixel_value << (BPP * (7 - x_in_chunk))
                    packed_chunks.append(f"0x{packed_int:06X}")
                f_c.write(", ".join(packed_chunks))
                f_c.write(" },\n")
            f_c.write("  },\n")
        f_c.write("};\n")
    print(f"File '{OUTPUT_C_FILE}' generato con successo!")
def generate_preview_png(tiles_data):
    if not PIL_AVAILABLE:
        print("\nAVVISO: La libreria Pillow (PIL) non è installata.")
        return
    if not tiles_data: return
    print(f"Generazione dell'anteprima PNG in '{PREVIEW_PNG_FILE}'...")
    PREVIEW_PALETTE = [ (0,0,0), (0,0,255), (0,255,0), (0,255,255), (255,0,0), (255,0,255), (255,255,0), (255,255,255) ]
    if not tiles_data: return
    height = len(tiles_data[0])
    width = len(tiles_data[0][0])
    GRID_COLS, UPSCALE_FACTOR = 16, 2
    GRID_ROWS = (NUM_TILES + GRID_COLS - 1) // GRID_COLS
    CELL_WIDTH = width * UPSCALE_FACTOR + 4
    CELL_HEIGHT = height * UPSCALE_FACTOR + 16
    IMG_WIDTH, IMG_HEIGHT = GRID_COLS * CELL_WIDTH, GRID_ROWS * CELL_HEIGHT
    img = Image.new('RGB', (IMG_WIDTH, IMG_HEIGHT), (80, 80, 80))
    draw = ImageDraw.Draw(img)
    try: font = ImageFont.truetype("arial.ttf", 10)
    except IOError: font = ImageFont.load_default()
    for i, tile in enumerate(tiles_data):
        grid_x, grid_y = i % GRID_COLS, i // GRID_COLS
        cell_ox, cell_oy = grid_x * CELL_WIDTH, grid_y * CELL_HEIGHT
        for y in range(height):
            for x in range(width):
                color_index = tile[y][x]
                draw.rectangle((cell_ox + x * UPSCALE_FACTOR, cell_oy + y * UPSCALE_FACTOR, cell_ox + (x + 1) * UPSCALE_FACTOR - 1, cell_oy + (y + 1) * UPSCALE_FACTOR - 1), fill=PREVIEW_PALETTE[color_index])
        text_pos_x = cell_ox + (CELL_WIDTH - 4) // 2
        text_pos_y = cell_oy + height * UPSCALE_FACTOR + 2
        draw.text((text_pos_x, text_pos_y), str(i), font=font, fill=(220, 220, 220), anchor="mt")
    img.save(PREVIEW_PNG_FILE)
    print("Anteprima PNG generata con successo!")

if __name__ == "__main__":
    tile_database = decode_bg_tiles()
    if tile_database:
        if GENERATE_PREVIEW:
            generate_preview_png(tile_database)
        write_c_array_packed(tile_database)