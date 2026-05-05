#!/usr/bin/env python
import sys
import os

# --- Configurazione per Donkey Kong Jr. (dkongjrj) ---

# File PROM per la palette (2 file da 4 bit ciascuno)
PALETTE_LOW_FILE  = "../roms/c-2e.bpr"
PALETTE_HIGH_FILE = "../roms/c-2f.bpr"

# File PROM per la tabella di lookup dei colori (colormap)
COLORMAP_LOOKUP_FILE = "../roms/v-2n.bpr"

# Nome del file header di output (come richiesto)
OUTPUT_HEADER_FILE = "../../source/src/machines/dkongjr/dkongjr_cmap.h"

# Nome base per gli array C che verranno generati
OUTPUT_ARRAY_NAME_BASE = "dkongjr_colormap"


def get_bit(value, bit):
    """Estrae un singolo bit da un valore."""
    return (value >> bit) & 1

def generate_palette():
    """Usa la TUA logica di conversione colore, ma con la mappatura dei bit di MAME."""
    print("Fase 1: Generazione della palette (Metodo Ibrido)...")
    
    with open(PALETTE_LOW_FILE, "rb") as f:
        prom_low_data = f.read()
    with open(PALETTE_HIGH_FILE, "rb") as f:
        prom_high_data = f.read()

    final_palette = []
    for i in range(256):
        # NOTA: L'hardware inverte i segnali, quindi è corretto usare i dati NON invertiti 
        # qui, perché la tua formula `MAX - val` già gestisce l'inversione.
        low_val = prom_low_data[i]
        high_val = prom_high_data[i]

        # --- Mappatura dei bit ESATTA come da MAME (dkong_decode_info) ---
        # Componente Rossa (3 bit)
        r3 = ((get_bit(high_val, 3) << 2) | 
              (get_bit(high_val, 2) << 1) | 
              (get_bit(high_val, 1) << 0))

        # Componente Verde (3 bit)
        g3 = ((get_bit(high_val, 0) << 2) | 
              (get_bit(low_val, 3) << 1) | 
              (get_bit(low_val, 2) << 0))

        # Componente Blu (2 bit)
        b2 = ((get_bit(low_val, 1) << 1) | 
              (get_bit(low_val, 0) << 0))
        
        # --- La TUA FORMULA di conversione lineare invertita ---
        # Applichiamola ai valori di bit appena calcolati
        r5 = 31 - (31 * r3 // 7)
        g6 = 63 - (63 * g3 // 7)
        b5 = 31 - (31 * b2 // 3)
        
        rgb565 = (r5 << 11) | (g6 << 5) | b5
        
        # Byte swap per il display
        rgb_swapped = ((rgb565 & 0xFF00) >> 8) | ((rgb565 & 0x00FF) << 8)
        final_palette.append(rgb_swapped)
        
    print(f"Palette di {len(final_palette)} colori generata con successo.")
    return final_palette


def generate_colormap(palette):
    """Genera gli array della colormap per tile e sprite."""
    print(f"Fase 2: Generazione della colormap dal file '{COLORMAP_LOOKUP_FILE}'...")
    
    with open(COLORMAP_LOOKUP_FILE, "rb") as f:
        colormap_lookup_data = f.read()

    if len(colormap_lookup_data) != 256:
        raise ValueError("Dimensione del file di colormap non valida.")

    with open(OUTPUT_HEADER_FILE, "w") as f:
        f.write(f"// File generato automaticamente per la colormap di Donkey Kong Jr.\n")
        f.write(f"// Palette da: {PALETTE_LOW_FILE}, {PALETTE_HIGH_FILE}\n")
        f.write(f"// Lookup da:  {COLORMAP_LOOKUP_FILE}\n\n")

        # --- Scrittura della colormap per i TILE ---
        tile_map_name = f"{OUTPUT_ARRAY_NAME_BASE}"
        f.write(f"const unsigned short {tile_map_name}[][256][4] = {{\n")
        
        screen_banks = []
        for s in range(4):  # Loop per i 4 banchi di palette
            color_entries = []
            for idx in colormap_lookup_data:
                offset_idx = idx + (16 * s)
                # Ogni indice punta a un gruppo di 4 colori nella palette finale
                color_indices = [
                    4 * offset_idx + 0,
                    4 * offset_idx + 1,
                    4 * offset_idx + 2,
                    4 * offset_idx + 3
                ]
                # Prende i valori dalla palette pre-calcolata e li formatta
                color_group = "{" + ",".join([hex(palette[ci]) for ci in color_indices]) + "}"
                color_entries.append(color_group)
            
            screen_banks.append("  {\n    " + ",\n    ".join(color_entries) + "\n  }")

        f.write(",\n".join(screen_banks))
        f.write("\n};\n\n")

        # --- Scrittura della colormap per gli SPRITE ---
        sprite_map_name = f"{OUTPUT_ARRAY_NAME_BASE}_sprite"
        f.write(f"const unsigned short {sprite_map_name}[][16][4] = {{\n")
        
        sprite_banks = []
        for s in range(4):  # Loop per i 4 banchi di palette
            sprite_entries = []
            for i in range(16): # Gli sprite usano solo 16 indici di colore
                offset_idx = i + (16 * s)
                color_indices = [
                    4 * offset_idx + 0,
                    4 * offset_idx + 1,
                    4 * offset_idx + 2,
                    4 * offset_idx + 3
                ]
                color_group = "{" + ",".join([hex(palette[ci]) for ci in color_indices]) + "}"
                sprite_entries.append(color_group)

            sprite_banks.append("  {\n    " + ",\n    ".join(sprite_entries) + "\n  }")
        
        f.write(",\n".join(sprite_banks))
        f.write("\n};")
        
    print(f"File '{OUTPUT_HEADER_FILE}' scritto con successo.")


def main():
    """Funzione principale per eseguire il processo di conversione."""
    # Controlla che tutti i file necessari esistano
    required_files = [PALETTE_LOW_FILE, PALETTE_HIGH_FILE, COLORMAP_LOOKUP_FILE]
    for filename in required_files:
        if not os.path.exists(filename):
            print(f"ERRORE: File ROM necessario non trovato: '{filename}'")
            print("Assicurati che i file siano nella stessa directory dello script.")
            sys.exit(1)
            
    # Esegui i passaggi di conversione
    final_palette = generate_palette()
    generate_colormap(final_palette)
    print("\nProcesso completato!")

if __name__ == "__main__":
    main()