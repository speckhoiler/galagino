import os
import re
from PIL import Image, ImageDraw, ImageFont

# --- CONFIGURAZIONE ---
INPUT_HEADER_FILE = "../../source/src/machines/tileaddr.h"
OUTPUT_IMAGE_FILE = "tileaddr_map_visualization.png"

# Dimensioni della griglia basate sulla tua tabella
GRID_ROWS = 36
GRID_COLS = 28

# Dimensioni in pixel per ogni cella nell'immagine di output
CELL_WIDTH = 40
CELL_HEIGHT = 20


def parse_tileaddr_from_c_header(filename):
    """
    Legge la matrice 2D 'tileaddr' da un file header C.
    È un parser specifico e robusto per questo formato.
    """
    if not os.path.exists(filename):
        raise FileNotFoundError(f"File non trovato: {filename}")

    print(f"Parsing del file '{filename}'...")
    with open(filename, 'r') as f:
        content = f.read()

    # Rimuovi commenti e newlines per pulire il testo
    content = re.sub(r'//.*', '', content)
    content = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL)
    content = content.replace('\n', '').replace('\r', '')

    # Isola il contenuto dell'array tra le prime { e le ultime }
    try:
        array_content = re.search(r'\{\s*(\{.*?\})\s*\}', content).group(1)
    except AttributeError:
        raise ValueError("Impossibile trovare il contenuto dell'array nel formato atteso.")

    # Dividi in righe (ogni riga è tra {})
    rows_str = re.findall(r'\{([0-9,\s]+)\}', array_content)
    
    if len(rows_str) != GRID_ROWS:
        print(f"Attenzione: Trovate {len(rows_str)} righe, ma attese {GRID_ROWS}.")

    # Converte ogni riga di testo in una lista di interi
    grid = []
    for row_str in rows_str:
        values = [int(v.strip()) for v in row_str.split(',') if v.strip()]
        if len(values) != GRID_COLS:
             print(f"Attenzione: Trovati {len(values)} valori in una riga, ma attesi {GRID_COLS}.")
        grid.append(values)
        
    print(f"Parsing completato. Griglia {len(grid)}x{len(grid[0])} caricata.")
    return grid

def create_visualization(grid):
    """
    Genera un'immagine PNG che visualizza la mappa degli indirizzi.
    """
    print("Creazione dell'immagine di visualizzazione...")

    # Calcola le dimensioni totali dell'immagine
    img_width = GRID_COLS * CELL_WIDTH
    img_height = GRID_ROWS * CELL_HEIGHT
    
    # Crea un'immagine bianca
    img = Image.new('RGB', (img_width, img_height), (255, 255, 255))
    draw = ImageDraw.Draw(img)

    # Prova a usare un font piccolo, altrimenti usa quello di default
    try:
        font = ImageFont.truetype("arial.ttf", 10)
    except IOError:
        font = ImageFont.load_default()

    # Disegna la griglia e i valori
    for r in range(GRID_ROWS):
        for c in range(GRID_COLS):
            # Calcola le coordinate dell'angolo in alto a sx della cella
            x0 = c * CELL_WIDTH
            y0 = r * CELL_HEIGHT
            x1 = x0 + CELL_WIDTH
            y1 = y0 + CELL_HEIGHT

            # Disegna il rettangolo della cella
            draw.rectangle([x0, y0, x1, y1], outline=(180, 180, 180))

            # Scrivi il valore dell'indirizzo al centro della cella
            addr_value = str(grid[r][c])
            text_bbox = draw.textbbox((0,0), addr_value, font=font)
            text_w = text_bbox[2] - text_bbox[0]
            text_h = text_bbox[3] - text_bbox[1]
            
            text_x = x0 + (CELL_WIDTH - text_w) / 2
            text_y = y0 + (CELL_HEIGHT - text_h) / 2
            
            draw.text((text_x, text_y), addr_value, fill=(0, 0, 0), font=font)

    # Salva l'immagine
    img.save(OUTPUT_IMAGE_FILE)
    print(f"Visualizzazione salvata con successo in '{OUTPUT_IMAGE_FILE}'")


if __name__ == "__main__":
    try:
        tile_address_map = parse_tileaddr_from_c_header(INPUT_HEADER_FILE)
        create_visualization(tile_address_map)
    except Exception as e:
        print(f"\nERRORE: {e}")