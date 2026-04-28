#!/usr/bin/env python3

import os
import struct

# Configurazione percorsi
ROM_SRC = os.path.normpath(os.path.join("..", "roms"))
OUT_DIR = os.path.normpath(os.path.join("..", "..", "source", "src", "machines", "ladybug"))

# Costanti specifiche di Lady Bug
CPU_BANKS = 6
CPU_BANK_SIZE = 0x1000
CPU_TOTAL_SIZE = CPU_BANKS * CPU_BANK_SIZE

TILE_COUNT = 512
TILE_SIZE = 16

SPRITE_COUNT = 128
SPRITE_SIZE = 64

ROM_FILES = {
    "cpu": [
        ("l1.c4", 0x1000),
        ("l2.d4", 0x1000),
        ("l3.e4", 0x1000),
        ("l4.h4", 0x1000),
        ("l5.j4", 0x1000),
        ("l6.k4", 0x1000)
    ],
    "tiles": ("l0.h7", 0x1000),
    "sprites_bank0": ("l7.m7", 0x0800),
    "sprites_bank1": ("l8.l7", 0x0800),
}

def load_file(filename):
    filepath = os.path.join(ROM_SRC, filename)
    try:
        with open(filepath, 'rb') as f:
            return bytearray(f.read())
    except FileNotFoundError:
        print(f"ERRORE: File non trovato: {filepath}")
        return None
    except Exception as e:
        print(f"ERRORE caricamento {filename}: {e}")
        return None

def load_patch_values(patch_filename, expected_count):
    """Carica i valori di delta dal file (un valore per riga)"""
    if not os.path.exists(patch_filename):
        return None
    
    try:
        with open(patch_filename, 'r') as f:
            lines = [line.strip() for line in f if line.strip()]
        
        patch_values = []
        for line in lines:
            val = int(line, 16)
            patch_values.append(val)
        
        if len(patch_values) != expected_count:
            print(f"  ATTENZIONE: {patch_filename} ha {len(patch_values)} valori, attesi {expected_count}")
            if len(patch_values) < expected_count:
                patch_values.extend([0] * (expected_count - len(patch_values)))
            else:
                patch_values = patch_values[:expected_count]
        
        non_zero = sum(1 for v in patch_values if v != 0)
        if non_zero > 0:
            print(f"  Caricato {patch_filename}: {non_zero} delta non-zero")
        return patch_values
        
    except Exception as e:
        print(f"  ERRORE caricamento {patch_filename}: {e}")
        return None

def apply_patch(data_list, patch_values, bit_mask, name):
    """Applica i delta patch ai dati (somma il delta al valore esistente)"""
    if patch_values is None:
        return data_list
    
    result = data_list.copy()
    min_len = min(len(result), len(patch_values))
    
    modified_count = 0
    for i in range(min_len):
        if patch_values[i] != 0:
            new_val = (result[i] + patch_values[i]) & bit_mask
            if result[i] != new_val:
                result[i] = new_val
                modified_count += 1
    
    if modified_count > 0:
        print(f"  {name}: applicati {modified_count} delta")
    
    return result

def format_hex_32bit(value):
    return f"0x{value:08x}"

def format_hex_16bit(value):
    return f"0x{value:04x}"

def generate_base_tilemap(tile_data):
    values = []
    expected_size = TILE_COUNT * TILE_SIZE
    if len(tile_data) != expected_size:
        print(f" ATTENZIONE: tile data {len(tile_data)} bytes, attesi {expected_size}")
        if len(tile_data) < expected_size:
            tile_data.extend([0] * (expected_size - len(tile_data)))
    
    for tile in range(TILE_COUNT):
        for row in range(8):
            addr = tile * TILE_SIZE + row * 2
            if addr + 1 < len(tile_data):
                plane0 = tile_data[addr]
                plane1 = tile_data[addr + 1]
                word = 0
                for bit in range(8):
                    p0 = (plane0 >> (7 - bit)) & 1
                    p1 = (plane1 >> (7 - bit)) & 1
                    pixel = (p1 << 1) | p0
                    word |= (pixel << (bit * 2))
                values.append(word)
            else:
                values.append(0)
    return values

def generate_base_spritemap(sprite_bank0, sprite_bank1):
    sprite_data = bytearray()
    sprite_data.extend(sprite_bank0)
    sprite_data.extend(sprite_bank1)
    
    expected_size = SPRITE_COUNT * SPRITE_SIZE
    if len(sprite_data) != expected_size:
        print(f" ATTENZIONE: sprite data {len(sprite_data)} bytes, attesi {expected_size}")
        if len(sprite_data) < expected_size:
            sprite_data.extend([0] * (expected_size - len(sprite_data)))
    
    original_sprites = []
    for sprite in range(SPRITE_COUNT):
        for row in range(16):
            addr = sprite * SPRITE_SIZE + row * 4
            if addr + 3 < len(sprite_data):
                plane0_low = sprite_data[addr]
                plane0_high = sprite_data[addr + 1]
                plane1_low = sprite_data[addr + 2]
                plane1_high = sprite_data[addr + 3]
                
                low_word = 0
                high_word = 0
                
                for bit in range(8):
                    p0 = (plane0_low >> (7 - bit)) & 1
                    p1 = (plane1_low >> (7 - bit)) & 1
                    pixel = (p1 << 1) | p0
                    low_word |= (pixel << (bit * 2))
                    
                    p0 = (plane0_high >> (7 - bit)) & 1
                    p1 = (plane1_high >> (7 - bit)) & 1
                    pixel = (p1 << 1) | p0
                    high_word |= (pixel << (bit * 2))
                
                long_val = (high_word << 16) | low_word
                original_sprites.append(long_val)
            else:
                original_sprites.append(0)
    
    return original_sprites

def generate_all_flips(sprite_base):
    """Genera tutti e 4 i flip partendo dagli sprite base"""
    all_sprites = []
    
    # Ricostruisci gli sprite base (128 sprite * 16 righe)
    base_sprites = []
    for sprite in range(SPRITE_COUNT):
        start = sprite * 16
        base_sprites.append(sprite_base[start:start+16])
    
    for flip in range(4):
        for sprite in range(SPRITE_COUNT):
            sprite_rows = base_sprites[sprite].copy()
            
            # Applica flip X
            if flip & 1:
                new_rows = []
                for row_val in sprite_rows:
                    new_val = 0
                    for c in range(16):
                        pixel = (row_val >> (c * 2)) & 3
                        new_val |= (pixel << ((15 - c) * 2))
                    new_rows.append(new_val)
                sprite_rows = new_rows
            
            # Applica flip Y
            if flip & 2:
                sprite_rows = sprite_rows[::-1]
            
            all_sprites.extend(sprite_rows)
    
    return all_sprites

def write_cpu_rom_header(cpu_data, output_file):
    print(f"Generazione {output_file}...")
    
    TARGET_SIZE = 24 * 1024
    if len(cpu_data) > TARGET_SIZE:
        cpu_data = cpu_data[:TARGET_SIZE]
    elif len(cpu_data) < TARGET_SIZE:
        cpu_data.extend([0] * (TARGET_SIZE - len(cpu_data)))
    
    with open(output_file, 'w') as f:
        f.write("// Lady Bug CPU ROM - 24KB (6 x 4KB)\n")
        f.write("// Generated from l1.c4-l6.k4 concatenated\n")
        f.write(f"// Total size: {len(cpu_data)} bytes\n\n")
        f.write(f'const unsigned char ladybug_rom_cpu1[{len(cpu_data)}] = {{\n')
        
        for i in range(0, len(cpu_data), 16):
            chunk = cpu_data[i:i+16]
            line = "  " + ",".join(f"0x{b:02X}" for b in chunk)
            if i + 16 < len(cpu_data):
                line += ","
            f.write(line + "\n")
        
        f.write("};\n")
    
    print(f" Scritti {len(cpu_data)} bytes")

def write_tilemap_header(tile_values, output_file):
    with open(output_file, 'w') as f:
        f.write("// Lady Bug character tiles - 512 chars, 8x8, 2bpp packed\n")
        f.write("// Generated by ladybug_rom_convert.py\n\n")
        f.write("const unsigned short ladybug_tilemap[512][8] = {\n")
        
        idx = 0
        for tile in range(512):
            f.write(" { ")
            row_values = []
            for row in range(8):
                row_values.append(format_hex_16bit(tile_values[idx]))
                idx += 1
            f.write(",".join(row_values))
            f.write(" },\n")
        
        f.write("};\n")

def write_spritemap_header(all_sprites, output_file):
    """Scrive il file header degli sprite da una lista piatta di 8192 valori (4 flip x 128 sprite x 16 righe)"""
    with open(output_file, 'w') as f:
        f.write("// Lady Bug sprites - 128 sprites, 16x16, 2bpp packed\n")
        f.write("// 4 flip variants: [0]=normal, [1]=flipX, [2]=flipY, [3]=flipXY\n")
        f.write("// Generated by ladybug_rom_convert.py\n\n")
        f.write("const unsigned long ladybug_sprites[4][128][16] = {\n")
        
        idx = 0
        for flip in range(4):
            f.write(" {\n")
            for sprite in range(SPRITE_COUNT):
                f.write(" { ")
                row_values = []
                for row in range(16):
                    row_values.append(format_hex_32bit(all_sprites[idx]))
                    idx += 1
                f.write(",".join(row_values))
                f.write(" },\n")
            f.write(" },\n")
        
        f.write("};\n")

def write_colormap_header(output_file):
    char_palette = [
        [0x0000, 0x7fad, 0x7ffd, 0xea57],
        [0x0000, 0x00f8, 0xe007, 0xe0ff],
        [0x0000, 0xffaf, 0xffff, 0x75ad],
        [0x0000, 0xffff, 0x75ad, 0x6afd],
        [0x0000, 0xe007, 0xf5af, 0x6005],
        [0x0000, 0xea57, 0x7f05, 0x7ffd],
        [0x0000, 0xffff, 0xe0ff, 0x7ffd],
        [0x0000, 0x0000, 0x80fa, 0x7ffd]
    ]
    
    sprite_palette = [
        [0x0000, 0x00f8, 0xe007, 0xe0ff],
        [0x0000, 0x9faa, 0xe007, 0xffff],
        [0x0000, 0x15f8, 0xe007, 0xffff],
        [0x0000, 0x60fd, 0xe0ff, 0xffff],
        [0x0000, 0xe007, 0xf5af, 0xffff],
        [0x0000, 0x60fd, 0xe0ff, 0xffff],
        [0x0000, 0x60fd, 0xf5af, 0xe0ff],
        [0x0000, 0x60fd, 0xe0ff, 0xffff]
    ]
    
    with open(output_file, 'w') as f:
        f.write("// Lady Bug color maps - RGB565\n")
        f.write("// Generated by ladybug_rom_convert.py\n\n")
        
        f.write("// Character color palette: 8 groups x 4 colors\n")
        f.write("const unsigned short ladybug_colormap[8][4] = {\n")
        for pal in char_palette:
            f.write("{" + ",".join(f"0x{col:04x}" for col in pal) + "},\n")
        f.write("};\n\n")
        
        f.write("// Sprite color palette: 8 groups x 4 colors\n")
        f.write("const unsigned short ladybug_sprite_colormap[8][4] = {\n")
        for pal in sprite_palette:
            f.write("{" + ",".join(f"0x{col:04x}" for col in pal) + "},\n")
        f.write("};\n")

def main():
    if not os.path.exists(OUT_DIR):
        os.makedirs(OUT_DIR, exist_ok=True)
    
    print("Lady Bug ROM Converter - CON PATCH (delta)")
    print("================================================")
    print(f"ROM source: {os.path.abspath(ROM_SRC)}")
    print(f"Output dir: {os.path.abspath(OUT_DIR)}")
    print()
    
    print("Caricamento file ROM...")
    cpu_data = bytearray()
    for filename, expected_size in ROM_FILES["cpu"]:
        data = load_file(filename)
        if data is None:
            print("ERRORE: Impossibile procedere senza tutti i file CPU")
            return
        if len(data) != expected_size:
            print(f" ATTENZIONE: {filename} dimensione {len(data)} bytes, attesi {expected_size}")
            if len(data) > expected_size:
                data = data[:expected_size]
        cpu_data.extend(data)
    
    TARGET_SIZE = 24 * 1024
    if len(cpu_data) != TARGET_SIZE:
        print(f" ATTENZIONE: cpu_data totale {len(cpu_data)} bytes, forzatura a {TARGET_SIZE}")
        if len(cpu_data) > TARGET_SIZE:
            cpu_data = cpu_data[:TARGET_SIZE]
        else:
            cpu_data.extend([0] * (TARGET_SIZE - len(cpu_data)))
    
    tile_file, tile_size = ROM_FILES["tiles"]
    tile_data = load_file(tile_file)
    if tile_data is None:
        print("ERRORE: Impossibile procedere senza file tile")
        return
    
    sprite_bank0_file, sprite_bank0_size = ROM_FILES["sprites_bank0"]
    sprite_bank0 = load_file(sprite_bank0_file)
    if sprite_bank0 is None:
        print("ERRORE: Impossibile procedere senza sprite bank 0")
        return
    
    sprite_bank1_file, sprite_bank1_size = ROM_FILES["sprites_bank1"]
    sprite_bank1 = load_file(sprite_bank1_file)
    if sprite_bank1 is None:
        print("ERRORE: Impossibile procedere senza sprite bank 1")
        return
    
    print()
    
    # === PASSO 1: Genera valori base ===
    print("FASE 1: Generazione dati base...")
    
    tile_base = generate_base_tilemap(tile_data)
    print(f" Generati {len(tile_base)} valori tile base")
    
    sprite_base = generate_base_spritemap(sprite_bank0, sprite_bank1)
    print(f" Generati {len(sprite_base)} valori sprite base")
    
    cpu_list = list(cpu_data)
    print(f" CPU data: {len(cpu_list)} bytes")
    print()
    
    # === PASSO 2: Applica le patch ===
    print("FASE 2: Applicazione patch...")
    
    # Tile patch (16 bit)
    tile_patch = load_patch_values("til_patch.table", len(tile_base))
    if tile_patch:
        tile_final = apply_patch(tile_base, tile_patch, 0xFFFF, "tile")
    else:
        tile_final = tile_base
        print("  Nessuna patch tile")
    
    # CPU patch (8 bit)
    cpu_patch = load_patch_values("rom_patch.table", len(cpu_list))
    if cpu_patch:
        cpu_final_list = apply_patch(cpu_list, cpu_patch, 0xFF, "cpu")
        cpu_final = bytearray(cpu_final_list)
    else:
        cpu_final = cpu_data
        print("  Nessuna patch CPU")
    
    # Sprite patch - genera TUTTI i flip e applica le patch a TUTTI
    sprite_patch = load_patch_values("spr_patch.table", SPRITE_COUNT * 16 * 4)  # 128 * 16 * 4 = 8192
    if sprite_patch:
        print("  Generazione di tutti i flip...")
        all_sprites = generate_all_flips(sprite_base)
        print(f"  Generati {len(all_sprites)} valori totali (4 flip x 128 sprite x 16 righe)")
        
        all_sprites = apply_patch(all_sprites, sprite_patch, 0xFFFFFFFF, "sprite")
        sprite_final_for_write = all_sprites
    else:
        print("  Nessuna patch sprite, generazione normale...")
        all_sprites = generate_all_flips(sprite_base)
        sprite_final_for_write = all_sprites
    
    print()
    
    # === PASSO 3: Scrivi i file finali ===
    print("FASE 3: Scrittura file finali...")
    
    write_cpu_rom_header(cpu_final, os.path.join(OUT_DIR, "ladybug_rom.h"))
    write_tilemap_header(tile_final, os.path.join(OUT_DIR, "ladybug_tilemap.h"))
    print(f" Scritto {os.path.join(OUT_DIR, 'ladybug_tilemap.h')}")
    write_spritemap_header(sprite_final_for_write, os.path.join(OUT_DIR, "ladybug_spritemap.h"))
    print(f" Scritto {os.path.join(OUT_DIR, 'ladybug_spritemap.h')}")
    write_colormap_header(os.path.join(OUT_DIR, "ladybug_cmap.h"))
    print(f" Scritto {os.path.join(OUT_DIR, 'ladybug_cmap.h')}")
    
    print("\n--- OPERAZIONE COMPLETATA ---")
    print(f"File finali generati in: {os.path.abspath(OUT_DIR)}")

if __name__ == "__main__":
    main()