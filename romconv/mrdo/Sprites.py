import os
try:
    from PIL import Image, ImageDraw, ImageFont
    PIL_AVAILABLE = True
except ImportError:
    PIL_AVAILABLE = False

# --- Configurazione per gli SPRITE ---
SPRITE_ROM_FILES = ["../roms/h5-05.bin", "../roms/k5-06.bin"]
OUTPUT_C_FILE = "../../source/src/machines/mrdo/mrdo_sprites.h"
C_ARRAY_NAME = "mrdo_sprites"

# --- OPZIONI ---
ROTATE_SPRITES = True
FLIP_HORIZONTAL = True
GENERATE_PREVIEW = True
OUTPUT_PNG_FILE = "mrdo_sprites_preview.png"

# Parametri
NUM_SPRITES = 128
SPRITE_WIDTH = 16
SPRITE_HEIGHT = 16
BYTES_PER_SPRITE = 64

def rotate_sprite_90_ccw(sprite):
    """Ruota un singolo sprite (16x16) di 90 gradi in senso antiorario."""
    new_sprite = [[0] * SPRITE_HEIGHT for _ in range(SPRITE_WIDTH)]
    for y in range(SPRITE_HEIGHT):
        for x in range(SPRITE_WIDTH):
            new_sprite[(SPRITE_WIDTH - 1) - x][y] = sprite[y][x]
    return new_sprite

def flip_sprite_horizontal(sprite):
    """Esegue il flip orizzontale di un singolo sprite."""
    return [row[::-1] for row in sprite]

def convert_sprites():
    """Legge i dati dei ROM degli sprite e li decodifica in modo corretto."""
    rom_data = bytearray()
    print("Lettura e concatenazione dei file ROM degli sprite...")
    for filename in SPRITE_ROM_FILES:
        if not os.path.exists(filename):
            print(f"Errore: Il file '{filename}' non è stato trovato.")
            return None
        with open(filename, 'rb') as f:
            rom_data.extend(f.read())

    if len(rom_data) != NUM_SPRITES * BYTES_PER_SPRITE:
        print(f"Errore: La dimensione totale dei dati ROM ({len(rom_data)} bytes) non corrisponde a quella attesa.")
        return None

    print("Decodifica degli sprite in corso (con la logica corretta)...")
    
    # Pre-calcola gli offset X come da gfx_layout
    x_offsets = []
    x_offsets.extend(range(3, -1, -1))    # STEP4(0*8+3, -1)
    x_offsets.extend(range(11, 7, -1))   # STEP4(1*8+3, -1)
    x_offsets.extend(range(19, 15, -1))  # STEP4(2*8+3, -1)
    x_offsets.extend(range(27, 23, -1))  # STEP4(3*8+3, -1)

    decoded_sprites = []
    
    for sprite_idx in range(NUM_SPRITES):
        sprite_data = []
        base_sprite_bit_offset = sprite_idx * BYTES_PER_SPRITE * 8
        
        for y in range(SPRITE_HEIGHT):
            row_data = []
            # L'offset in bit per l'inizio di questa riga è y * 32
            row_bit_offset = base_sprite_bit_offset + (y * 32)
            
            for x in range(SPRITE_WIDTH):
                # L'offset del pixel all'interno della riga è dato da x_offsets
                pixel_offset_in_row = x_offsets[x]
                
                # Calcola l'offset totale del bit per questo pixel
                total_pixel_bit_offset = row_bit_offset + pixel_offset_in_row
                
                # --- Estrazione dei bit LSB e MSB ---
                # Il bitplane 0 (LSB) ha un offset di +0
                bit_offset_lsb = total_pixel_bit_offset + 0
                byte_index_lsb = bit_offset_lsb // 8
                bit_in_byte_lsb = 7 - (bit_offset_lsb % 8)
                lsb = (rom_data[byte_index_lsb] >> bit_in_byte_lsb) & 1
                
                # Il bitplane 1 (MSB) ha un offset di +4
                bit_offset_msb = total_pixel_bit_offset + 4
                byte_index_msb = bit_offset_msb // 8
                bit_in_byte_msb = 7 - (bit_offset_msb % 8)
                msb = (rom_data[byte_index_msb] >> bit_in_byte_msb) & 1
                
                final_pixel = (msb << 1) | lsb
                row_data.append(final_pixel)
                
            sprite_data.append(row_data)
            
        decoded_sprites.append(sprite_data)
        
    print(f"Decodifica completata. {len(decoded_sprites)} sprite elaborati.")

    if ROTATE_SPRITES:
        print("Rotazione di tutti gli sprite di 90 gradi in senso antiorario...")
        decoded_sprites = [rotate_sprite_90_ccw(sprite) for sprite in decoded_sprites]

    if FLIP_HORIZONTAL:
        print("Flipping orizzontale di tutti gli sprite...")
        decoded_sprites = [flip_sprite_horizontal(sprite) for sprite in decoded_sprites]

    return decoded_sprites

def write_c_array(sprites_data):
    """Scrive i dati degli sprite in un file sorgente C."""
    if not sprites_data: return
    print(f"Scrittura dell'array C nel file '{OUTPUT_C_FILE}'...")
    with open(OUTPUT_C_FILE, 'w') as f_c:
        f_c.write(f"// File generato da script. Contiene i {NUM_SPRITES} sprite (16x16, 2bpp) per Mr. Do! (Universal).\n")
        f_c.write(f"// Decodifica basata sulla definizione ufficiale del driver MAME.\n")
        if ROTATE_SPRITES: f_c.write(f"// Gli sprite sono stati ruotati di 90 gradi in senso antiorario.\n")
        if FLIP_HORIZONTAL: f_c.write(f"// Gli sprite sono stati flippati orizzontalmente.\n")
        f_c.write("#include <stdint.h>\n\n")
        f_c.write(f"const uint8_t {C_ARRAY_NAME}[{NUM_SPRITES}][{SPRITE_HEIGHT}][{SPRITE_WIDTH}] = {{\n")
        for i, sprite in enumerate(sprites_data):
            f_c.write(f"  {{ // Sprite {i:03d} (0x{i:02X})\n")
            for y, row in enumerate(sprite):
                f_c.write("    {")
                f_c.write(", ".join(f"{p:2}" for p in row))
                f_c.write("}")
                if y < SPRITE_HEIGHT - 1: f_c.write(",")
                f_c.write("\n")
            f_c.write("  }")
            if i < NUM_SPRITES - 1: f_c.write(",")
            f_c.write("\n")
        f_c.write("};\n")
    print(f"File '{OUTPUT_C_FILE}' generato con successo!")

def generate_preview_png(sprites_data):
    """Genera un'immagine PNG di anteprima con tutti gli sprite in una griglia."""
    if not PIL_AVAILABLE:
        print("\nAVVISO: La libreria Pillow non è installata. Impossibile generare l'anteprima PNG.")
        print("Installa con: pip install Pillow")
        return
    if not sprites_data: return
    print(f"Generazione dell'anteprima PNG in '{OUTPUT_PNG_FILE}'...")
    PREVIEW_PALETTE = [(0, 0, 0, 0), (0, 0, 255), (0, 255, 0), (255, 255, 255)]
    GRID_COLS, UPSCALE_FACTOR = 16, 2
    GRID_ROWS = (NUM_SPRITES + GRID_COLS - 1) // GRID_COLS
    CELL_WIDTH = SPRITE_WIDTH * UPSCALE_FACTOR + 4
    CELL_HEIGHT = SPRITE_HEIGHT * UPSCALE_FACTOR + 16
    IMG_WIDTH, IMG_HEIGHT = GRID_COLS * CELL_WIDTH, GRID_ROWS * CELL_HEIGHT
    img = Image.new('RGBA', (IMG_WIDTH, IMG_HEIGHT), (80, 80, 80, 255))
    draw = ImageDraw.Draw(img)
    try: font = ImageFont.truetype("arial.ttf", 10)
    except IOError: font = ImageFont.load_default()
        
    for i, sprite in enumerate(sprites_data):
        sprite_for_preview = [row[::-1] for row in sprite[::-1]]
        grid_x, grid_y = i % GRID_COLS, i // GRID_COLS
        cell_ox, cell_oy = grid_x * CELL_WIDTH, grid_y * CELL_HEIGHT
        for y in range(SPRITE_HEIGHT):
            for x in range(SPRITE_WIDTH):
                color_index = sprite_for_preview[y][x]
                if color_index == 0: continue
                draw.rectangle(
                    (cell_ox + x * UPSCALE_FACTOR, cell_oy + y * UPSCALE_FACTOR,
                     cell_ox + (x + 1) * UPSCALE_FACTOR - 1, cell_oy + (y + 1) * UPSCALE_FACTOR - 1),
                    fill=PREVIEW_PALETTE[color_index]
                )
        text_pos_x = cell_ox + (CELL_WIDTH - 4) // 2
        text_pos_y = cell_oy + SPRITE_HEIGHT * UPSCALE_FACTOR + 2
        draw.text((text_pos_x, text_pos_y), str(i), font=font, fill=(220, 220, 220), anchor="mt")
        
    img.save(OUTPUT_PNG_FILE)
    print("Anteprima PNG generata con successo!")

if __name__ == "__main__":
    sprite_database = convert_sprites()
    if sprite_database:
        write_c_array(sprite_database)
        if GENERATE_PREVIEW:
            generate_preview_png(sprite_database)