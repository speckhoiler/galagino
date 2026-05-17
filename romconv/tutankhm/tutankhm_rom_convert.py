#!/usr/bin/env python3
"""
ROM converter for Tutankham (Konami/Stern 1982)
Generates header files for GALAGONE ESP32-S3 emulator.

Hardware:
  Main CPU: MC6809E @ 1.5 MHz
  Sound: timeplt_audio (Z80 @ 1.789 MHz + 2x AY-3-8910)
  Video: 256x256 bitmap, 4bpp (2 pixels/byte), ROT90
  Palette: 16 colors, CPU-written, RRRGGGBB format

MAME memory map (Main CPU):
  0x0000-0x7FFF: Video RAM (32KB bitmap, 2 pixels/byte)
  0x8000-0x800F: Palette RAM (mirror 0xF0)
  0x8100:        Scroll register (mirror 0xF)
  0x8160:        DSW2 read (mirror 0xF)
  0x8180:        IN0 read (coin/start, mirror 0xF)
  0x81A0:        IN1 read (P1 controls, mirror 0xF)
  0x81C0:        IN2 read (P2 controls, mirror 0xF)
  0x81E0:        DSW1 read (mirror 0xF)
  0x8200-0x8207: LS259 latch (mirror 0xF8)
  0x8300:        Bank select (mirror 0xFF)
  0x8600:        Sound trigger (mirror 0xFF)
  0x8700:        Sound data latch (mirror 0xFF)
  0x8800-0x8FFF: Work RAM (2KB)
  0x9000-0x9FFF: Banked ROM (4KB, 16 banks selectable)
  0xA000-0xFFFF: Fixed ROM (24KB: m1+m2+3j+m4+m5+j6)

ROM files from tutankhm.zip:
  Fixed ROM: m1.1h (0xA000), m2.2h (0xB000), 3j.3h (0xC000),
             m4.4h (0xD000), m5.5h (0xE000), j6.6h (0xF000)
  Banked ROM: c1.1i - c9.9i (4KB each, banks 0-8)
  Sound ROM: s1.7a (0x0000), s2.8a (0x1000)

Usage: python tutankhm.py
"""

import os
import sys

# ── ROM source directory (like mspacman_rom_convert.py) ──
ROM_SRC = os.path.normpath(os.path.join("..", "roms"))
OUT_DIR = os.path.normpath(os.path.join("..", "..", "source", "src", "machines", "tutankhm"))

def load_file(filename):
    """Load a ROM file from the ROM source directory."""
    path = os.path.join(ROM_SRC, filename)
    
    if not os.path.exists(path):
        # Try lowercase/uppercase variations like mspacman_rom_convert.py
        for alt in [filename.lower(), filename.upper()]:
            alt_path = os.path.join(ROM_SRC, alt)
            if os.path.exists(alt_path):
                path = alt_path
                break
    
    if not os.path.exists(path):
        print(f"ERROR: File '{filename}' not found in {os.path.abspath(ROM_SRC)}")
        return None
    
    with open(path, "rb") as f:
        data = bytearray(f.read())
        print(f"  Loaded {os.path.basename(path)}: {len(data)} bytes")
        return data

def write_header(filename, varname, data, comment=""):
    """Write data to a C header file."""
    path = os.path.join(OUT_DIR, filename)
    with open(path, "w") as f:
        f.write(f"// {comment}\n")
        f.write(f"const unsigned char {varname}[] = {{\n")
        for i in range(0, len(data), 16):
            chunk = data[i:i+16]
            f.write("  " + ", ".join(f"0x{b:02X}" for b in chunk) + ",\n")
        f.write("};\n")
    print(f"  Written {os.path.basename(path)} ({len(data)} bytes)")

def main():
    # Create output directory if it doesn't exist
    os.makedirs(OUT_DIR, exist_ok=True)
    
    print(f"ROM source directory: {os.path.abspath(ROM_SRC)}")
    print(f"Output directory: {os.path.abspath(OUT_DIR)}")
    
    # ══════════════════════════════════════════════════════════════════
    # 1. FIXED ROM — 24KB covering 0xA000-0xFFFF
    # ══════════════════════════════════════════════════════════════════
    print("\n=== Fixed CPU ROM (0xA000-0xFFFF) ===")
    
    # Load all fixed ROM files
    rom_m1 = load_file("m1.1h")   # 4KB at 0xA000
    rom_m2 = load_file("m2.2h")   # 4KB at 0xB000
    rom_3j = load_file("3j.3h")   # 4KB at 0xC000
    rom_m4 = load_file("m4.4h")   # 4KB at 0xD000
    rom_m5 = load_file("m5.5h")   # 4KB at 0xE000
    rom_j6 = load_file("j6.6h")   # 4KB at 0xF000 (interrupt vectors!)
    
    # Check if any file failed to load
    if None in [rom_m1, rom_m2, rom_3j, rom_m4, rom_m5, rom_j6]:
        print("ERROR: Failed to load one or more fixed ROM files")
        sys.exit(1)
    
    # Build 24KB ROM: 0xA000-0xFFFF
    cpu_rom = rom_m1 + rom_m2 + rom_3j + rom_m4 + rom_m5 + rom_j6
    expected_size = 6 * 4096  # 6 files * 4KB = 24KB
    assert len(cpu_rom) == expected_size, f"Expected {expected_size} bytes, got {len(cpu_rom)}"
    print(f"  CPU ROM total: {len(cpu_rom)} bytes (24KB: m1+m2+3j+m4+m5+j6)")
    
    # Verify interrupt vectors (at end of j6.6h = offset 0x5FFx)
    reset_vec = (cpu_rom[0x5FFE] << 8) | cpu_rom[0x5FFF]
    nmi_vec   = (cpu_rom[0x5FFC] << 8) | cpu_rom[0x5FFD]
    irq_vec   = (cpu_rom[0x5FF8] << 8) | cpu_rom[0x5FF9]
    print(f"  Reset vector: 0x{reset_vec:04X}")
    print(f"  NMI vector:   0x{nmi_vec:04X}")
    print(f"  IRQ vector:   0x{irq_vec:04X}")
    
    write_header("tutankhm_rom.h", "tutankhm_rom", cpu_rom,
                 "Tutankham fixed CPU ROM (24KB: 0xA000-0xFFFF, indexed as rom[addr - 0xA000])")
    
    # ══════════════════════════════════════════════════════════════════
    # 2. BANKED ROM — c1-c9 (9 x 4KB = 36KB, banks at 0x9000-0x9FFF)
    # ══════════════════════════════════════════════════════════════════
    print("\n=== Banked ROM (0x9000-0x9FFF, 4KB per bank) ===")
    
    bank_roms = b""
    for i in range(1, 10):
        filename = f"c{i}.{i}i"
        data = load_file(filename)
        if data is None:
            print(f"ERROR: Failed to load bank ROM {filename}")
            sys.exit(1)
        bank_roms += data
    
    # Pad to 16 banks (16 x 4KB = 64KB) with zeros for unused banks 9-15
    bank_roms += b'\x00' * (16 * 0x1000 - len(bank_roms))
    print(f"  Bank ROM total: {len(bank_roms)} bytes (16 x 4KB, banks 9-15 empty)")
    
    write_header("tutankhm_bank_rom.h", "tutankhm_bank_rom", bank_roms,
                 "Tutankham banked ROM (64KB: 16 x 4KB banks at 0x9000-0x9FFF, indexed as bank_rom[bank*0x1000 + (addr-0x9000)])")
    
    # ══════════════════════════════════════════════════════════════════
    # 3. SOUND CPU ROM — 8KB (timeplt_audio board)
    # ══════════════════════════════════════════════════════════════════
    print("\n=== Sound CPU ROM ===")
    
    snd_s1 = load_file("s1.7a")   # 4KB at 0x0000
    snd_s2 = load_file("s2.8a")   # 4KB at 0x1000
    
    if None in [snd_s1, snd_s2]:
        print("ERROR: Failed to load sound ROM files")
        sys.exit(1)
    
    snd_rom = snd_s1 + snd_s2
    print(f"  Sound ROM total: {len(snd_rom)} bytes (8KB)")
    
    write_header("tutankhm_snd_rom.h", "tutankhm_snd_rom", snd_rom,
                 "Tutankham sound CPU ROM (8KB: s1+s2, timeplt_audio Z80)")
    
    print("\n--- OPERAZIONE COMPLETATA ---")
    print(f"Tutti i file sono stati generati in: {os.path.abspath(OUT_DIR)}")
    print("\nFile generati:")
    print("  tutankhm_rom.h       - Fixed CPU ROM (24KB: m1+m2+3j+m4+m5+j6)")
    print("  tutankhm_bank_rom.h  - Banked ROM (64KB: 16 x 4KB)")
    print("  tutankhm_snd_rom.h   - Sound CPU ROM (8KB)")

if __name__ == "__main__":
    main()