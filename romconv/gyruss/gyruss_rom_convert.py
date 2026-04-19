#!/usr/bin/env python3
"""
Convert Gyruss (gyrussk) raw ROM dumps to galagino .h header files.

Reads ROM files from ../roms/ and generates:
  - ../../source/src/machines/gyruss/gyruss_rom_main.h    (main Z80 CPU ROM, 24KB)
  - ../../source/src/machines/gyruss/gyruss_rom_sub.h     (M6809 sub-CPU ROM, 8KB + decrypted opcodes)
  - ../../source/src/machines/gyruss/gyruss_rom_audio.h   (audio Z80 ROM, 16KB)
  - ../../source/src/machines/gyruss/gyruss_tilemap.h     (decoded 8x8 tiles from gyrussk.4)
  - ../../source/src/machines/gyruss/gyruss_spritemap.h   (decoded 8x16 sprites from gyrussk.5/6/7/8)
  - ../../source/src/machines/gyruss/gyruss_palette.h     (RGB565 palette + color lookup tables)

Supports applying patches from spr_patch.txt and tile_patch.txt
"""

import os
import re

ROM_SRC = os.path.normpath(os.path.join("..", "roms"))
OUT_DIR = os.path.normpath(os.path.join("..", "..", "source", "src", "machines", "gyruss"))


def load_file(name):
    """Load a ROM file from the roms directory."""
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
        print(f"  Caricato {patch_filename}: {len(patch_values)} valori, {non_zero} delta non-zero")
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
    print(f"  Patch: {len(patch_values)} valori")
    
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
            # Determina la dimensione del valore (16 o 32 bit) in base alla lunghezza della stringa
            hex_len = len(old_hex) - 2  # rimuovi 0x
            if hex_len <= 4:
                mask = 0xFFFF
                new_val = (original + patch_values[i]) & mask
                new_hex = f"0x{new_val:04x}"
            else:
                mask = 0xFFFFFFFF
                new_val = (original + patch_values[i]) & mask
                new_hex = f"0x{new_val:08x}"
            
            if new_hex != old_hex:
                result = result[:match.start()] + new_hex + result[match.end():]
                modified += 1
    
    print(f"  Valori modificati: {modified}")
    
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(result)


# ============================================================
# Konami-1 opcode decryption
# ============================================================

def konami1_decrypt_opcode(val, addr):
    """Decrypt a single opcode byte using Konami-1 XOR scheme.
    XOR table based on address bits A1 and A3."""
    xor_table = {0x0: 0x22, 0x2: 0x82, 0x8: 0x28, 0xA: 0x88}
    return val ^ xor_table[addr & 0x0A]


# ============================================================
# CPU ROM assembly
# ============================================================

def assemble_main_rom():
    """Assemble main Z80 CPU ROM (24KB: 0x0000-0x5FFF)."""
    rom = bytearray(0x6000)
    rom[0x0000:0x2000] = load_file("gyrussk.1")
    rom[0x2000:0x4000] = load_file("gyrussk.2")
    rom[0x4000:0x6000] = load_file("gyrussk.3")
    return rom


def assemble_sub_rom():
    """Assemble M6809 sub-CPU ROM (8KB: 0xE000-0xFFFF).
    Returns (raw_rom, decrypted_opcodes) tuple."""
    raw = load_file("gyrussk.9")  # 8KB, mapped at 0xE000-0xFFFF
    decrypted = bytearray(len(raw))
    for i in range(len(raw)):
        addr = 0xE000 + i
        decrypted[i] = konami1_decrypt_opcode(raw[i], addr)
    return raw, decrypted


def assemble_audio_rom():
    """Assemble audio Z80 ROM (16KB: 0x0000-0x3FFF)."""
    rom = bytearray(0x4000)
    rom[0x0000:0x2000] = load_file("gyrussk.1a")
    rom[0x2000:0x4000] = load_file("gyrussk.2a")
    return rom


# ============================================================
# Tile decoding (8x8, 2bpp, 512 tiles from gyrussk.4)
# ============================================================
# MAME charlayout:
#   planes = {4, 0}
#   xoffset = {0,1,2,3, 64,65,66,67}
#   yoffset = {0,8,16,24,32,40,48,56}
#   charincrement = 128 bits = 16 bytes

def decode_tile_pixel(data, tile_base, row, pixel):
    """Decode one pixel from a Gyruss tile.
    2bpp: plane0 at bit offset 0, plane1 at bit offset 4."""
    # xoffset for pixels 0-3: bits 0,1,2,3
    # xoffset for pixels 4-7: bits 64,65,66,67 (= byte 8, bits 0,1,2,3)
    if pixel < 4:
        byte_offset = row  # yoffset = row*8, so byte = row*8/8 = row
        bit = pixel
    else:
        byte_offset = row + 8  # 64/8 = 8
        bit = pixel - 4

    b = data[tile_base + byte_offset]
    plane0 = (b >> bit) & 1
    plane1 = (b >> (bit + 4)) & 1
    return plane0 | (plane1 << 1)


def decode_tiles(gfx_data):
    """Decode 512 tiles (8x8, 2bpp) from gyrussk.4.
    Returns list of 512 tiles, each = list of 8 unsigned short rows.
    Each row packs 8 pixels x 2 bits = 16 bits."""
    num_tiles = len(gfx_data) // 16  # 8192/16 = 512
    tiles = []
    for t in range(num_tiles):
        base = t * 16
        tile_rows = []
        for r in range(8):
            row_val = 0
            for p in range(8):
                px = decode_tile_pixel(gfx_data, base, r, p)
                row_val |= px << (p * 2)
            tile_rows.append(row_val)
        tiles.append(tile_rows)
    return tiles


# ============================================================
# Sprite decoding (8x16, 4bpp, 512 sprites from gyrussk.5/6/7/8)
# ============================================================

def decode_sprite_pixel(data, sprite_base, row, pixel, data_len):
    """Decode one pixel from a Gyruss sprite.
    4bpp: 4 planes across two ROM regions."""
    # yoffset: rows 0-7 -> offset 0,8,16..56  rows 8-15 -> offset 256,264..312
    if row < 8:
        y_bit_offset = row * 8
    else:
        y_bit_offset = 256 + (row - 8) * 8

    # xoffset: pixels 0-3 -> bits 0,1,2,3  pixels 4-7 -> bits 64,65,66,67
    if pixel < 4:
        x_bit_offset = pixel
    else:
        x_bit_offset = 64 + (pixel - 4)

    bit_offset = y_bit_offset + x_bit_offset
    byte_idx = bit_offset // 8
    bit_in_byte = bit_offset % 8

    # Plane 0: bit (bit_in_byte) in region 0
    # Plane 1: bit (bit_in_byte + 4) in region 0
    # Plane 2: bit (bit_in_byte) in region 1 (offset + 0x4000)
    # Plane 3: bit (bit_in_byte + 4) in region 1 (offset + 0x4000)
    addr0 = sprite_base + byte_idx
    addr1 = addr0 + 0x4000

    if addr0 >= data_len or addr1 >= data_len:
        return 0

    b0 = data[addr0]
    b1 = data[addr1]

    p0 = (b0 >> bit_in_byte) & 1
    p1 = (b0 >> (bit_in_byte + 4)) & 1
    p2 = (b1 >> bit_in_byte) & 1
    p3 = (b1 >> (bit_in_byte + 4)) & 1

    return p0 | (p1 << 1) | (p2 << 2) | (p3 << 3)


def decode_sprites(gfx_data):
    """Decode 512 sprites (8x16, 4bpp) from combined sprite ROMs.
    gfx_data = gyrussk.6 + gyrussk.5 (region 0) + gyrussk.8 + gyrussk.7 (region 1)
    Returns sprites[4][512][16] (4 flip variants, 512 sprites, 16 rows).
    Each row packs 8 pixels x 4 bits = 32 bits (unsigned long)."""
    data_len = len(gfx_data)
    num_sprites = 512  # 512 sprites total (128 per ROM file, 4 files)
    sprites = [[[0] * 16 for _ in range(num_sprites)] for _ in range(4)]

    for s in range(num_sprites):
        base = s * 64  # 64 bytes per sprite
        
        normal = []
        for r in range(16):
            row_val = 0
            for p in range(8):
                px = decode_sprite_pixel(gfx_data, base, r, p, data_len)
                row_val |= px << (p * 4)
            normal.append(row_val)

        # Variant 0: normal
        sprites[0][s] = normal[:]

        # Variant 1: Y flip (reverse rows)
        sprites[1][s] = normal[::-1]

        # Variant 2: X flip (reverse pixels in each row)
        for r in range(16):
            flipped = 0
            for p in range(8):
                px = (normal[r] >> (p * 4)) & 0xF
                flipped |= px << ((7 - p) * 4)
            sprites[2][s][r] = flipped

        # Variant 3: XY flip (both)
        for r in range(16):
            flipped = 0
            src_row = normal[15 - r]
            for p in range(8):
                px = (src_row >> (p * 4)) & 0xF
                flipped |= px << ((7 - p) * 4)
            sprites[3][s][r] = flipped

    return sprites


# ============================================================
# Palette and color lookup tables from PROMs
# ============================================================

def generate_palette_and_colormaps(pr3, pr1, pr2):
    """Generate palette and color lookup tables."""
    # Generate 32 base colors from palette PROM
    palette_rgb = []
    for i in range(32):
        val = pr3[i]
        r = 0x21 * ((val >> 0) & 1) + 0x47 * ((val >> 1) & 1) + 0x97 * ((val >> 2) & 1)
        g = 0x21 * ((val >> 3) & 1) + 0x47 * ((val >> 4) & 1) + 0x97 * ((val >> 5) & 1)
        b = 0x47 * ((val >> 6) & 1) + 0x97 * ((val >> 7) & 1)
        palette_rgb.append((r, g, b))

    def rgb_to_565(r, g, b):
        """Convert RGB888 to RGB565 with byte swap for galagino TFT."""
        r5 = (r >> 3) & 0x1F
        g6 = (g >> 2) & 0x3F
        b5 = (b >> 3) & 0x1F
        val = (r5 << 11) | (g6 << 5) | b5
        return ((val & 0xFF) << 8) | ((val >> 8) & 0xFF)

    # Palette as RGB565
    palette_565 = [rgb_to_565(r, g, b) for r, g, b in palette_rgb]

    # Sprite color lookup: 16 groups x 16 colors
    sprite_colormap = []
    for group in range(16):
        colors = []
        for px in range(16):
            idx = group * 16 + px
            pal_idx = pr1[idx] & 0x0F
            r, g, b = palette_rgb[pal_idx]
            if px == 0:
                colors.append(0)  # transparent
            else:
                colors.append(rgb_to_565(r, g, b))
        sprite_colormap.append(colors)

    # Character color lookup: pr2 maps to palette entries with +0x10 offset
    char_colormap = []
    for group in range(16):
        colors = []
        for px in range(4):
            c = px
            if px == 1:
              c = 2
            elif px == 2:
              c = 1
            idx = group * 4 + c
            pal_idx = (pr2[idx] & 0x0F) + 0x10
            if pal_idx >= 32:
                pal_idx = 0
            r, g, b = palette_rgb[pal_idx]
            if px == 0:
                colors.append(0)  # transparent
            else:
                colors.append(rgb_to_565(r, g, b))
        char_colormap.append(colors)

    return palette_565, sprite_colormap, char_colormap


# ============================================================
# Output .h file writers
# ============================================================

def write_array_h(filepath, array_name, data, bytes_per_line=16, type_str="unsigned char"):
    with open(filepath, "w") as f:
        f.write(f"const {type_str} {array_name}[] = {{\n")
        for i in range(len(data)):
            if i % bytes_per_line == 0:
                f.write("  ")
            f.write("0x{:02X}".format(data[i]))
            if i < len(data) - 1:
                f.write(",")
            if i % bytes_per_line == bytes_per_line - 1 or i == len(data) - 1:
                f.write("\n")
        f.write("};\n")


def write_main_rom_h(rom, filepath):
    """Write main Z80 ROM header."""
    with open(filepath, "w") as f:
        f.write("// Gyruss main Z80 CPU ROM (24KB: 0x0000-0x5FFF)\n")
        f.write("// Generated from gyrussk.1 + gyrussk.2 + gyrussk.3\n\n")
        f.write("const unsigned char gyruss_rom_main[] = {\n")
        for i in range(len(rom)):
            if i % 16 == 0:
                f.write("  ")
            f.write("0x{:02X}".format(rom[i]))
            if i < len(rom) - 1:
                f.write(",")
            if i % 16 == 15 or i == len(rom) - 1:
                f.write("\n")
        f.write("};\n")


def write_sub_rom_h(raw, decrypted, filepath):
    """Write M6809 sub-CPU ROM header with both raw and decrypted opcodes."""
    with open(filepath, "w") as f:
        f.write("// Gyruss M6809 sub-CPU ROM (8KB: mapped at 0xE000-0xFFFF)\n")
        f.write("// Generated from gyrussk.9\n")
        f.write("// Raw data for operand reads, decrypted for opcode fetches (Konami-1)\n\n")

        f.write("const unsigned char gyruss_rom_sub_raw[] = {\n")
        for i in range(len(raw)):
            if i % 16 == 0:
                f.write("  ")
            f.write("0x{:02X}".format(raw[i]))
            if i < len(raw) - 1:
                f.write(",")
            if i % 16 == 15 or i == len(raw) - 1:
                f.write("\n")
        f.write("};\n\n")

        f.write("// Decrypted opcodes (Konami-1 XOR scheme)\n")
        f.write("const unsigned char gyruss_rom_sub_decrypt[] = {\n")
        for i in range(len(decrypted)):
            if i % 16 == 0:
                f.write("  ")
            f.write("0x{:02X}".format(decrypted[i]))
            if i < len(decrypted) - 1:
                f.write(",")
            if i % 16 == 15 or i == len(decrypted) - 1:
                f.write("\n")
        f.write("};\n")


def write_audio_rom_h(rom, filepath):
    """Write audio Z80 ROM header."""
    with open(filepath, "w") as f:
        f.write("// Gyruss audio Z80 CPU ROM (16KB: 0x0000-0x3FFF)\n")
        f.write("// Generated from gyrussk.1a + gyrussk.2a\n\n")
        f.write("const unsigned char gyruss_rom_audio[] = {\n")
        for i in range(len(rom)):
            if i % 16 == 0:
                f.write("  ")
            f.write("0x{:02X}".format(rom[i]))
            if i < len(rom) - 1:
                f.write(",")
            if i % 16 == 15 or i == len(rom) - 1:
                f.write("\n")
        f.write("};\n")


def write_tilemap_h(tiles, filepath):
    """Write tilemap as C header. Each tile = 8 rows of uint16 (8 pixels x 2bpp)."""
    with open(filepath, "w") as f:
        f.write("// Gyruss character tiles (512 tiles, 8x8, 2bpp)\n")
        f.write("// Generated from gyrussk.4\n\n")
        f.write("const unsigned short gyruss_tilemap[][8] = {\n")
        for t_idx, tile in enumerate(tiles):
            f.write("  { ")
            f.write(",".join("0x{:04x}".format(v) for v in tile))
            f.write(" }")
            if t_idx < len(tiles) - 1:
                f.write(",")
            f.write("\n")
        f.write("};\n")


def generate_temp_tilemap(tiles, temp_file):
    """Genera il file tilemap temporaneo."""
    with open(temp_file, 'w', encoding='utf-8') as f:
        f.write("const unsigned short gyruss_tilemap[][8] = {\n")
        for t_idx, tile in enumerate(tiles):
            f.write("  { ")
            f.write(",".join("0x{:04x}".format(v) for v in tile))
            f.write(" }")
            if t_idx < len(tiles) - 1:
                f.write(",")
            f.write("\n")
        f.write("};\n")
    return len(tiles)


def write_spritemap_h(sprites, filepath):
    """Write spritemap as C header."""
    with open(filepath, "w") as f:
        f.write("// Gyruss sprites (512 sprites, 8x16, 4bpp, 4 flip variants)\n")
        f.write("// Generated from gyrussk.6 + gyrussk.5 + gyrussk.8 + gyrussk.7\n")
        f.write("// Variant 0=normal, 1=Y-flip, 2=X-flip, 3=XY-flip\n\n")
        f.write("const unsigned long gyruss_sprites[][512][16] = {\n")
        for v in range(4):
            f.write("  {\n")
            for s in range(512):
                f.write("    { ")
                f.write(",".join("0x{:08x}".format(row) for row in sprites[v][s]))
                f.write(" }")
                if s < 511:
                    f.write(",")
                f.write("\n")
            f.write("  }")
            if v < 3:
                f.write(",")
            f.write("\n")
        f.write("};\n")


def generate_temp_spritemap(sprites, temp_file):
    """Genera il file spritemap temporaneo."""
    with open(temp_file, 'w', encoding='utf-8') as f:
        f.write("const unsigned long gyruss_sprites[][512][16] = {\n")
        for v in range(4):
            f.write("  {\n")
            for s in range(512):
                f.write("    { ")
                f.write(",".join("0x{:08x}".format(row) for row in sprites[v][s]))
                f.write(" }")
                if s < 511:
                    f.write(",")
                f.write("\n")
            f.write("  }")
            if v < 3:
                f.write(",")
            f.write("\n")
        f.write("};\n")
    return 4 * 512 * 16  # numero di valori


def write_palette_h(palette_565, sprite_cmap, char_cmap, filepath):
    """Write palette and color lookup tables."""
    with open(filepath, "w") as f:
        f.write("// Gyruss color data\n")
        f.write("// Generated from gyrussk.pr1 + gyrussk.pr2 + gyrussk.pr3\n\n")

        # Master palette (32 colors)
        f.write("// Master palette (32 entries, RGB565 byte-swapped)\n")
        f.write("const unsigned short gyruss_palette[] = {\n  ")
        f.write(",".join("0x{:04x}".format(c) for c in palette_565))
        f.write("\n};\n\n")

        # Sprite colormap (16 groups x 16 colors)
        f.write("// Sprite color lookup (16 groups x 16 colors, RGB565)\n")
        f.write("const unsigned short gyruss_sprite_colormap[][16] = {\n")
        for i, colors in enumerate(sprite_cmap):
            f.write("  { ")
            f.write(",".join("0x{:04x}".format(c) for c in colors))
            f.write(" }")
            if i < len(sprite_cmap) - 1:
                f.write(",")
            f.write("\n")
        f.write("};\n\n")

        # Character colormap (16 groups x 4 colors)
        f.write("// Character color lookup (16 groups x 4 colors, RGB565)\n")
        f.write("const unsigned short gyruss_char_colormap[][4] = {\n")
        for i, colors in enumerate(char_cmap):
            f.write("  { ")
            f.write(",".join("0x{:04x}".format(c) for c in colors))
            f.write(" }")
            if i < len(char_cmap) - 1:
                f.write(",")
            f.write("\n")
        f.write("};\n")


# ============================================================
# Main
# ============================================================

def main():
    os.makedirs(OUT_DIR, exist_ok=True)

    print("Gyruss ROM Conversion Tool (with patch support)")
    print(f"  ROM dir: {os.path.abspath(ROM_SRC)}")
    print(f"  Output dir: {os.path.abspath(OUT_DIR)}")
    print()

    # Verifica che tutti i file esistano prima di procedere
    required_files = [
        "gyrussk.1", "gyrussk.2", "gyrussk.3", "gyrussk.4", "gyrussk.5",
        "gyrussk.6", "gyrussk.7", "gyrussk.8", "gyrussk.9",
        "gyrussk.1a", "gyrussk.2a",
        "gyrussk.pr1", "gyrussk.pr2", "gyrussk.pr3"
    ]
    
    files_ok = True
    for fname in required_files:
        path = os.path.join(ROM_SRC, fname)
        if not os.path.exists(path):
            # Prova con nomi alternativi (minuscolo/maiuscolo)
            alt_path = os.path.join(ROM_SRC, fname.lower())
            if not os.path.exists(alt_path):
                alt_path = os.path.join(ROM_SRC, fname.upper())
            if not os.path.exists(alt_path):
                print(f"ERRORE: File '{fname}' non trovato in {os.path.abspath(ROM_SRC)}")
                files_ok = False
    
    if not files_ok:
        print("\nERRORE: Non tutti i file ROM sono presenti. Verifica la directory dei ROM.")
        return

    # 1. Main Z80 CPU ROM (no patch)
    print("Assembling main Z80 ROM (24KB)...")
    main_rom = assemble_main_rom()
    if main_rom is None:
        print("ERRORE: Impossibile caricare i file per la main ROM")
        return
    path = os.path.join(OUT_DIR, "gyruss_rom_main.h")
    write_main_rom_h(main_rom, path)
    print(f"  Written: {path} ({len(main_rom)} bytes)")

    # 2. M6809 sub-CPU ROM with Konami-1 decryption (no patch)
    print("Assembling M6809 sub-CPU ROM (8KB + decrypted)...")
    sub_raw, sub_decrypt = assemble_sub_rom()
    if sub_raw is None:
        print("ERRORE: Impossibile caricare i file per la sub ROM")
        return
    path = os.path.join(OUT_DIR, "gyruss_rom_sub.h")
    write_sub_rom_h(sub_raw, sub_decrypt, path)
    print(f"  Written: {path} ({len(sub_raw)} bytes raw + {len(sub_decrypt)} bytes decrypted)")

    # 3. Audio Z80 ROM (no patch)
    print("Assembling audio Z80 ROM (16KB)...")
    audio_rom = assemble_audio_rom()
    if audio_rom is None:
        print("ERRORE: Impossibile caricare i file per la audio ROM")
        return
    path = os.path.join(OUT_DIR, "gyruss_rom_audio.h")
    write_audio_rom_h(audio_rom, path)
    print(f"  Written: {path} ({len(audio_rom)} bytes)")

    # 4. Tiles from gyrussk.4
    print("\n=== GENERAZIONE TILEMAP ===")
    tile_data = load_file("gyrussk.4")
    if tile_data is None:
        print("ERRORE: Impossibile caricare il file tile gyrussk.4")
        return
    tiles = decode_tiles(tile_data)
    
    temp_file = "temp_gyruss_tilemap.h"
    final_file = os.path.join(OUT_DIR, "gyruss_tilemap.h")
    
    print("Generazione file temporaneo...")
    generate_temp_tilemap(tiles, temp_file)
    print(f"  File temporaneo: {temp_file}")
    
    tile_patch = load_patch_values("til_patch.table")
    if tile_patch:
        print("Applicazione patch per posizione...")
        apply_patches_to_file(temp_file, final_file, tile_patch)
    else:
        print("Nessuna patch, copio il file temporaneo...")
        with open(temp_file, 'r') as f:
            content = f.read()
        with open(final_file, 'w') as f:
            f.write(content)
    
    if os.path.exists(temp_file):
        os.remove(temp_file)
        print(f"  File temporaneo eliminato: {temp_file}")
    
    print(f"  File finale: {final_file} ({len(tiles)} tiles)")

    # 5. Sprites from gyrussk.6+5 (region 0) + gyrussk.8+7 (region 1)
    print("\n=== GENERAZIONE SPRITEMAP ===")
    print("Decoding sprites (512 x 8x16 x 4bpp, 4 variants)...")
    sprite_data = bytearray(0x8000)
    sprite_data[0x0000:0x2000] = load_file("gyrussk.6") or bytearray(0x2000)
    sprite_data[0x2000:0x4000] = load_file("gyrussk.5") or bytearray(0x2000)
    sprite_data[0x4000:0x6000] = load_file("gyrussk.8") or bytearray(0x2000)
    sprite_data[0x6000:0x8000] = load_file("gyrussk.7") or bytearray(0x2000)
    sprites = decode_sprites(sprite_data)
    
    temp_file = "temp_gyruss_spritemap.h"
    final_file = os.path.join(OUT_DIR, "gyruss_spritemap.h")
    
    print("Generazione file temporaneo...")
    generate_temp_spritemap(sprites, temp_file)
    print(f"  File temporaneo: {temp_file}")
    
    sprite_patch = load_patch_values("spr_patch.table")
    if sprite_patch:
        print("Applicazione patch per posizione...")
        apply_patches_to_file(temp_file, final_file, sprite_patch)
    else:
        print("Nessuna patch, copio il file temporaneo...")
        with open(temp_file, 'r') as f:
            content = f.read()
        with open(final_file, 'w') as f:
            f.write(content)
    
    if os.path.exists(temp_file):
        os.remove(temp_file)
        print(f"  File temporaneo eliminato: {temp_file}")
    
    print(f"  File finale: {final_file} (512 sprites x 4 variants)")

    # 6. Palette and color maps from PROMs (no patch)
    print("\nGenerazione palette e color maps...")
    pr3 = load_file("gyrussk.pr3")  # 32 bytes - palette
    pr1 = load_file("gyrussk.pr1")  # 256 bytes - sprite lookup
    pr2 = load_file("gyrussk.pr2")  # 256 bytes - char lookup
    
    if pr3 is None or pr1 is None or pr2 is None:
        print("ERRORE: Impossibile caricare i file PROM")
        return
        
    palette_565, sprite_cmap, char_cmap = generate_palette_and_colormaps(pr3, pr1, pr2)
    path = os.path.join(OUT_DIR, "gyruss_palette.h")
    write_palette_h(palette_565, sprite_cmap, char_cmap, path)
    print(f"  Written: {path} (32 palette + 16x16 sprite + 16x4 char colors)")

    print("\nConversion complete!")
    print(f"\nSummary:")
    print(f"  Main ROM:    {len(main_rom):6d} bytes")
    print(f"  Sub ROM:     {len(sub_raw):6d} bytes (+ {len(sub_decrypt)} decrypted)")
    print(f"  Audio ROM:   {len(audio_rom):6d} bytes")
    print(f"  Tiles:       {len(tiles):6d} (8x8, 2bpp)")
    print(f"  Sprites:     512 (8x16, 4bpp, 4 variants)")
    print(f"  Palette:     32 base colors + lookup tables")


if __name__ == "__main__":
    main()