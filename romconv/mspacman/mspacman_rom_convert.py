#!/usr/bin/env python3
"""
Convertitore ROM Ms. Pac-Man con applicazione patch lineare
- Genera il file spritemap temporaneo (come il convertitore originale)
- Applica le patch linearmente valore per valore per posizione
- Produce il file finale corretto
- Elimina i file temporanei
"""

import os
import re

ROM_SRC = os.path.normpath(os.path.join("..", "roms"))
OUT_DIR = os.path.normpath(os.path.join("..", "..", "source", "src", "machines", "mspacman"))

def load_file(name):
    path = os.path.join(ROM_SRC, name)
    if not os.path.exists(path):
        for alt in [name.lower(), name.upper()]:
            alt_path = os.path.join(ROM_SRC, alt)
            if os.path.exists(alt_path):
                path = alt_path
                break
    if not os.path.exists(path):
        print(f"ERRORE: File '{name}' non trovato in {os.path.abspath(ROM_SRC)}")
        return None
    with open(path, "rb") as f:
        return bytearray(f.read())

def decode_byte(b):
    return (((b>>0)&1)<<7 | ((b>>4)&1)<<6 | ((b>>5)&1)<<5 |
            ((b>>7)&1)<<4 | ((b>>6)&1)<<3 | ((b>>3)&1)<<2 |
            ((b>>2)&1)<<1 | ((b>>1)&1)<<0)

def addr_swap11(i):
    return (((i>>8)&1)<<10 | ((i>>7)&1)<<9  | ((i>>5)&1)<<8  |
            ((i>>9)&1)<<7  | ((i>>10)&1)<<6 | ((i>>6)&1)<<5  |
            ((i>>3)&1)<<4  | ((i>>4)&1)<<3  | ((i>>2)&1)<<2  |
            ((i>>1)&1)<<1  | ((i>>0)&1)<<0)

def addr_swap12(i):
    return (((i>>11)&1)<<11 | ((i>>3)&1)<<10 | ((i>>7)&1)<<9  |
            ((i>>9)&1)<<8   | ((i>>10)&1)<<7 | ((i>>8)&1)<<6  |
            ((i>>6)&1)<<5   | ((i>>5)&1)<<4  | ((i>>4)&1)<<3  |
            ((i>>2)&1)<<2   | ((i>>1)&1)<<1  | ((i>>0)&1)<<0)

def load_patch_values(patch_filename):
    """Carica i delta dal file patch (un valore per riga)"""
    if not os.path.exists(patch_filename):
        print(f"  ATTENZIONE: {patch_filename} non trovato, nessuna patch applicata")
        return None
    
    try:
        with open(patch_filename, 'r') as f:
            lines = [line.strip() for line in f if line.strip()]
        
        patch_values = []
        for line in lines:
            val = int(line, 16)
            patch_values.append(val)
        
        non_zero = sum(1 for v in patch_values if v != 0)
        return patch_values
        
    except Exception as e:
        print(f"  ERRORE caricamento {patch_filename}: {e}")
        return None

def apply_patches_to_file(input_file, output_file, patch_values):
    """Applica le patch linearmente per posizione, non per valore"""
    if patch_values is None:
        with open(input_file, 'r') as f:
            content = f.read()
        with open(output_file, 'w') as f:
            f.write(content)
        return
    
    with open(input_file, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Trova tutti i valori esadecimali con le loro posizioni
    hex_pattern = r'0[xX][0-9a-fA-F]+'
    matches = list(re.finditer(hex_pattern, content))
    
    print(f"  File input: {len(matches)} valori esadecimali trovati")
    
    if len(matches) != len(patch_values):
        print(f"  ATTENZIONE: lunghezze diverse, uso il minimo comune")
        min_len = min(len(matches), len(patch_values))
        matches = matches[:min_len]
        patch_values = patch_values[:min_len]
    
    # Applica le patch per posizione (dalla fine all'inizio per non alterare le posizioni)
    result = content
    modified = 0
    for i in range(len(matches) - 1, -1, -1):
        match = matches[i]
        old_hex = match.group()
        original = int(old_hex, 16)
        if patch_values[i] != 0:
            new_val = (original + patch_values[i]) & 0xFFFFFFFF
            new_hex = f"0x{new_val:08x}"
            if new_hex != old_hex:
                result = result[:match.start()] + new_hex + result[match.end():]
                modified += 1
    
    
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(result)

def generate_temp_spritemap(gfx5e, gfx5f, temp_file):
    sprite_offset = 2048
    max_gfx_size = len(gfx5f)
    SPRITE_COUNT = 64
    
    all_values = []
    
    for flip in range(4):
        for s in range(SPRITE_COUNT):
            for r in range(16):
                base = sprite_offset + (s * 64) + r
                
                b0l = gfx5f[base] if base < max_gfx_size else 0
                b0h = gfx5f[base + 16] if (base + 16) < max_gfx_size else 0
                b1l = gfx5f[base + 32] if (base + 32) < max_gfx_size else 0
                b1h = gfx5f[base + 48] if (base + 48) < max_gfx_size else 0
                
                wl, wh = 0, 0
                for bit in range(8):
                    p0l, p1l = (b0l >> (7-bit)) & 1, (b1l >> (7-bit)) & 1
                    p0h, p1h = (b0h >> (7-bit)) & 1, (b1h >> (7-bit)) & 1
                    wl |= (((p1l << 1) | p0l) << (bit * 2))
                    wh |= (((p1h << 1) | p0h) << (bit * 2))
                val = (wh << 16) | wl
                
                if flip & 1:
                    new_val = 0
                    for c in range(16):
                        pixel = (val >> (c * 2)) & 3
                        new_val |= (pixel << ((15 - c) * 2))
                    val = new_val
                
                all_values.append(val)
    
    # Scrivi il file temporaneo
    with open(temp_file, 'w', encoding='utf-8') as f:
        f.write("const unsigned long mspacman_sprites[][64][16] = {\n")
        
        idx = 0
        for flip in range(4):
            f.write(" {\n")
            for s in range(SPRITE_COUNT):
                rows = []
                for r in range(16):
                    rows.append(f"0x{all_values[idx]:08x}")
                    idx += 1
                f.write(f"  {{ {', '.join(rows)} }},\n")
            f.write(" },\n")
        f.write("};\n")
    
    return len(all_values)

def main():
    if not os.path.exists(OUT_DIR):
        os.makedirs(OUT_DIR, exist_ok=True)

    # Carica i ROM
    pac6e = load_file("pacman.6e")
    pac6f = load_file("pacman.6f")
    pac6h = load_file("pacman.6h")
    u5 = load_file("u5")
    u6 = load_file("u6")
    u7 = load_file("u7")
    gfx5e = load_file("5e")
    gfx5f = load_file("5f")

    if not all(v is not None for v in [pac6e, pac6f, pac6h, u5, u6, u7, gfx5e, gfx5f]):
        return

    # 1. CPU ROM
    print("Generazione CPU ROM...")
    drom = bytearray([0xFF] * 0xC000)
    for i in range(0x1000):
        drom[0x0000 + i] = pac6e[i]
        drom[0x1000 + i] = pac6f[i]
        drom[0x2000 + i] = pac6h[i]
    for i in range(0x1000):
        drom[0x3000 + i] = decode_byte(u7[addr_swap12(i) & 0xFFF])
    for i in range(0x800):
        drom[0x8000 + i] = decode_byte(u5[addr_swap11(i) & 0x7FF])
        drom[0x8800 + i] = decode_byte(u6[(addr_swap12(i) & 0x7FF) + 0x800])
        drom[0x9000 + i] = decode_byte(u6[addr_swap12(i) & 0x7FF])
        drom[0x9800 + i] = pac6f[0x800 + i]

    # Patch CPU (originali)
    patches = [(0x0410, 0x8008), (0x08E0, 0x81D8), (0x0A30, 0x8118), (0x0BD0, 0x80D8),
               (0x0C20, 0x8120), (0x0E58, 0x8168), (0x0EA8, 0x8198), (0x1000, 0x8020),
               (0x1008, 0x8010), (0x1288, 0x8098), (0x1348, 0x8048), (0x1688, 0x8088),
               (0x16B0, 0x8188), (0x16D8, 0x80C8), (0x16F8, 0x81C8), (0x19A8, 0x80A8),
               (0x19B8, 0x81A8), (0x2060, 0x8148), (0x2108, 0x8018), (0x21A0, 0x81A0),
               (0x2298, 0x80A0), (0x23E0, 0x80E8), (0x2418, 0x8000), (0x2448, 0x8058),
               (0x2470, 0x8140), (0x2488, 0x8080), (0x24B0, 0x8180), (0x24D8, 0x80C0),
               (0x24F8, 0x81C0), (0x2748, 0x8050), (0x2780, 0x8090), (0x27B8, 0x8190),
               (0x2800, 0x8028), (0x2B20, 0x8100), (0x2B30, 0x8110), (0x2BF0, 0x81D0),
               (0x2CC0, 0x80D0), (0x2CD8, 0x80E0), (0x2CF0, 0x81E0), (0x2D60, 0x8160)]
    for dst, src in patches:
        for i in range(8):
            drom[dst + i] = drom[src + i]

    # 2. TILEMAP
    print("Generazione mspacman_tilemap.h...")
    tile_count = 256
    tile_base = []
    
    for t in range(tile_count):
        for row in range(8):
            idx = t * 8 + row
            b0, b1 = gfx5e[idx], gfx5f[idx]
            val = 0
            for bit in range(8):
                p0, p1 = (b0 >> (7-bit)) & 1, (b1 >> (7-bit)) & 1
                val |= (((p1 << 1) | p0) << (bit * 2))
            tile_base.append(val)
    
    tile_patch = load_patch_values("til_patch.table")
    if tile_patch:
        for i in range(len(tile_base)):
            if i < len(tile_patch) and tile_patch[i] != 0:
                tile_base[i] = (tile_base[i] + tile_patch[i]) & 0xFFFF
    
    with open(os.path.join(OUT_DIR, "mspacman_tilemap.h"), "w") as f:
        f.write("const unsigned short mspacman_tilemap[][8] = {\n")
        idx = 0
        for t in range(tile_count):
            row_words = []
            for row in range(8):
                row_words.append(f"0x{tile_base[idx]:04x}")
                idx += 1
            f.write(f"  {{ {', '.join(row_words)} }},\n")
        f.write("};\n")
    print(f"  Scritto mspacman_tilemap.h")

    # 3. SPRITEMAP - genera temporaneo, applica patch per posizione, poi finale
    print("\n=== GENERAZIONE SPRITEMAP ===")
    
    # File temporaneo nella stessa cartella
    temp_file = "temp_mspacman_spritemap.h"
    final_file = os.path.join(OUT_DIR, "mspacman_spritemap.h")
    
    # Genera il file spritemap temporaneo (come il convertitore originale)
    value_count = generate_temp_spritemap(gfx5e, gfx5f, temp_file)
    print(f"  Generati {value_count} valori")
    
    # Carica le patch
    sprite_patch = load_patch_values("spr_patch.table")
    
    # Applica le patch al file temporaneo per posizione
    if sprite_patch:
        apply_patches_to_file(temp_file, final_file, sprite_patch)
    else:
        with open(temp_file, 'r') as f:
            content = f.read()
        with open(final_file, 'w') as f:
            f.write(content)
    
    # Elimina il file temporaneo
    if os.path.exists(temp_file):
        os.remove(temp_file)
    
    print(f"  File finale: {final_file}")

    # 4. ROM FILES
    print("\nGenerazione ROM files...")
    with open(os.path.join(OUT_DIR, "mspacman_pacrom.h"), "w") as f:
        f.write("const unsigned char mspacman_pacrom[] = {\n")
        for i in range(0, 0x4000, 16):
            f.write("  " + ", ".join(f"0x{b:02X}" for b in drom[i:i+16]) + ",\n")
        f.write("};\n")

    with open(os.path.join(OUT_DIR, "mspacman_auxrom.h"), "w") as f:
        f.write("const unsigned char mspacman_auxrom[] = {\n")
        for i in range(0x8000, 0xA000, 16):
            f.write("  " + ", ".join(f"0x{b:02X}" for b in drom[i:i+16]) + ",\n")
        f.write("};\n")

    print(f"\nCompletato con successo.")

if __name__ == "__main__":
    main()