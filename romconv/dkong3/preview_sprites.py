# File: 3_generate_preview.py (versione a prova di errore)
import os
import re
from PIL import Image, ImageDraw, ImageFont

# --- CONFIGURAZIONE ---
SPRITE_DATA_FILE = "../../source/src/machines/dkong3/dkong3_spritemap.h"
COLOR_MAP_FILE = "../../source/src/machines/dkong3/dkong3_cmap.h"
OUTPUT_IMAGE_FILE = "spritesheet_preview.png"
SPRITES_PER_ROW = 16
SPRITE_WIDTH = 16
SPRITE_HEIGHT = 16

# --- PARSING ROBUSTO (SENZA REGEX COMPLICATE) ---

def parse_c_array_from_file(filename, start_line_pattern):
    """Legge un array C da un file, riga per riga."""
    print(f"Parsing di '{filename}'...")
    if not os.path.exists(filename):
        raise FileNotFoundError(f"File non trovato: {filename}")
        
    all_values = []
    in_array = False
    with open(filename, 'r') as f:
        for line in f:
            line = line.strip()
            if start_line_pattern in line:
                in_array = True
                # Inizia a raccogliere dati dalla fine della parentesi graffa
                line = line.split('{', 1)[-1]
            
            if in_array:
                # Pulisci la riga da commenti e parentesi non necessarie
                line = re.sub(r'//.*', '', line)
                line = re.sub(r'/\*.*?\*/', '', line)
                line = line.replace('{', '').replace('}', '').replace(';', '')
                
                # Estrai tutti i numeri (hex o decimali)
                values_in_line = re.findall(r'0x[0-9a-fA-F]+|\d+', line)
                all_values.extend([int(v, 0) for v in values_in_line])

                if ';' in line:
                    break # Fine dell'array
    
    if not all_values:
        raise ValueError(f"Nessun dato numerico trovato per l'array in '{filename}'")
        
    return all_values

# --- FUNZIONE PRINCIPALE ---

def create_preview():
    try:
        # Estrai i dati grezzi usando il nuovo parser robusto
        sprite_data_flat = parse_c_array_from_file(SPRITE_DATA_FILE, "dkong3_sprites[4][256][16]")
        palette_data_flat = parse_c_array_from_file(COLOR_MAP_FILE, "dkong3_colormap_sprite[64][4]")

        # Prendi solo il primo blocco di sprite (no-flip)
        # Ci sono 4 blocchi (flip) * 256 sprite * 16 righe
        num_sprites = 256
        sprite_data_noflip = sprite_data_flat[:num_sprites * SPRITE_HEIGHT]

        # Converti la palette in tuple (R,G,B)
        sprite_palette = []
        for val16 in palette_data_flat:
            rgb565 = ((val16 & 0xFF00) >> 8) | ((val16 & 0x00FF) << 8)
            r5 = (rgb565 >> 11) & 0x1F
            g6 = (rgb565 >> 5) & 0x3F
            b5 = rgb565 & 0x1F
            r8 = (r5 * 255 + 15) // 31
            g8 = (g6 * 255 + 31) // 63
            b8 = (b5 * 255 + 15) // 31
            sprite_palette.append((r8, g8, b8))
            
        print(f"Palette sprite con {len(sprite_palette)} colori caricata.")

        # Crea l'immagine
        print("Creazione immagine di anteprima...")
        num_rows = num_sprites // SPRITES_PER_ROW
        cell_w, cell_h = SPRITE_WIDTH + 1, SPRITE_HEIGHT + 1 + 15
        img = Image.new('RGB', (cell_w * SPRITES_PER_ROW, cell_h * num_rows), (20, 20, 20))
        draw = ImageDraw.Draw(img)
        font = ImageFont.load_default()

        for idx in range(num_sprites):
            row_idx, col_idx = idx // SPRITES_PER_ROW, idx % SPRITES_PER_ROW
            x_offset, y_offset = col_idx * cell_w, row_idx * cell_h
            
            # Per l'anteprima, usiamo i primi 4 colori della palette degli sprite
            preview_palette = {
                0: sprite_palette[0],
                1: sprite_palette[1],
                2: sprite_palette[2],
                3: (20, 20, 20) # Trasparente
            }

            for y in range(SPRITE_HEIGHT):
                row_data = sprite_data_noflip[idx * SPRITE_HEIGHT + y]
                for x in range(SPRITE_WIDTH):
                    px_val = (row_data >> ((SPRITE_WIDTH - 1 - x) * 2)) & 0x03
                    if px_val != 3:
                        img.putpixel((x_offset + x, y_offset + y), preview_palette[px_val])
            
            draw.text((x_offset + 2, y_offset + SPRITE_HEIGHT + 2), str(idx), font=font, fill=(255, 255, 255))
            draw.rectangle([x_offset, y_offset, x_offset + SPRITE_WIDTH, y_offset + SPRITE_HEIGHT], outline=(100, 100, 100))

        img.save(OUTPUT_IMAGE_FILE)
        print(f"Immagine di anteprima salvata come '{OUTPUT_IMAGE_FILE}'")
        
    except Exception as e:
        print(f"ERRORE: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    create_preview()