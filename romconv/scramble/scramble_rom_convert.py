#!/usr/bin/env python3
"""
Scramble ROM converter for GALAGINO

scramble dumps have been reanamed over time, so you might find:

ROM set (scramble) current MAME srcs:
  Main CPU:  8x 0x0800 s1.2d s2.2e s3.2f s4.2h s5.2j s6.2l s7.2m s8.2p
  Audio CPU: 3x 0x0800 ot1.5c ot2.5d ot3.5e
  gfx1:      2x 0x0800 c2.5f c1.5h
  prom:      1x 0x0020 c01s.6e

ROM set (scramble) older MAME:
  Main CPU:  8x 0x0800 2d.k 2e.k 2f.k 2h.k 2j.k 2l.k 2m.k 2p.k
  Audio CPU: 3x 0x0800 5c 5d 5e
  gfx1:      2x 0x0800 5f.k 5h.k
  prom:      1x 0x0020 82s123.6e

This should work with both sets and will check rom set against MAME SHA1 hashes.

"""

import os
import zipfile
import hashlib

ROM_SET = os.path.normpath(os.path.join("..", "..", "romszip", "scramble.zip"))
OUT_DIR = os.path.normpath(os.path.join("..", "..", "source", "src", "machines", "scramble"))

# -------------------------------------------------------------------
def hex8(v):
  return "0x{:02x}".format(v & 0xFF)

def hex16(v):
  return "0x{:04x}".format(v & 0xFFFF)

def hex32(v):
  return "0x{:08x}".format(v & 0xFFFFFFFF)

# -------------------------------------------------------------------
# load file from zipfile and check hash
# -------------------------------------------------------------------
# load file from zipfile and check hash
def load_file(names, sha1):
  for name in names:
    with zipfile.ZipFile(ROM_SET) as z:
      if name in z.namelist():
        with z.open(name, 'r') as file:
          rom = bytearray(file.read())
          if check_file(name, rom, sha1):
            #print(f"Loaded {name} from romset.")
            return rom
          return None
  for name in names:
    print(f"ERROR: File '{name}' not found in {os.path.abspath(ROM_SRC)}")
    return None

def check_file(name, b, h):
  digest = hashlib.sha1(b).hexdigest()
  if h != digest:
    print(f"bad hash for {name} {h}!={digest}.")
    return False
  return True

# -------------------------------------------------------------------
# ---- Write C header files ----
# -------------------------------------------------------------------
def write_rom(filename, name, data):
  with open(filename, 'w') as f:
    f.write("// Scramble program ROM ({} bytes)\n".format(len(data)))
    f.write("const unsigned char {}[] = {{\n".format(name))
    for i in range(0, len(data), 16):
      line = ", ".join(hex8(data[j]) for j in range(i, min(i+16, len(data))))
      f.write("  " + line)
      if i + 16 < len(data):
        f.write(",")
      f.write("\n")
    f.write("};\n")
  print("Wrote: {} ({} bytes)".format(os.path.abspath(filename), len(data)))

def write_tilemap(filename, tiles):
  with open(filename, 'w') as f:
    f.write("// Scramble tilemap: {} tiles, 8x8, 2bpp\n".format(len(tiles)))
    f.write("const unsigned short scramble_tilemap[][8] = {\n")
    for t, rows in enumerate(tiles):
      f.write("  { " + ", ".join(hex16(r) for r in rows) + " }")
      if t < len(tiles) - 1:
        f.write(",")
      f.write("\n")
    f.write("};\n")
  print("Wrote: {} ({} tiles)".format(os.path.abspath(filename), len(tiles)))

def write_spritemap(filename, all_orientations):
  num_sprites = len(all_orientations[0])
  with open(filename, 'w') as f:
    f.write("// Scramble spritemap: {} sprites, 16x16, 2bpp, 4 orientations\n".format(num_sprites))
    f.write("const unsigned long scramble_spritemap[][%d][16] = {\n" % num_sprites)
    for o, sprites in enumerate(all_orientations):
      f.write("  { // orientation %d\n" % o)
      for s, rows in enumerate(sprites):
        f.write("    { " + ", ".join(hex32(r) for r in rows) + " }")
        if s < len(sprites) - 1:
          f.write(",")
        f.write("\n")
      f.write("  }")
      if o < 3:
        f.write(",")
      f.write("\n")
    f.write("};\n")
  print("Wrote: {} ({} sprites x 4 orientations)".format(os.path.abspath(filename), num_sprites))

def write_colormap(filename, rgb565):
  with open(filename, 'w') as f:
    f.write("// scramble colormap: 8 palettes x 4 colors, RGB565\n")
    f.write("const unsigned short scramble_colormap[][4] = {\n")
    for pal in range(8):
      colors = rgb565[pal*4 : pal*4+4]
      f.write("  { " + ", ".join(hex16(c) for c in colors) + " }")
      if pal < 7:
        f.write(",")
      f.write("  // palette {}\n".format(pal))
    f.write("};\n")
  print("Wrote: {} (8 palettes)".format(os.path.abspath(filename)))

# -------------------------------------------------------------------
# ---- Tile conversion using original galagino parse_chr_2 + dump_chr ----
def parse_chr_2(data0, data1):
  """Parse 8x8 tile from two separate plane ROMs.
  Same as original galagino tileconv.py parse_chr_2."""
  char = []
  for y in range(8):
    row = []
    for x in range(8):
      c0 = 1 if data0[7 - x] & (0x80 >> y) else 0
      c1 = 2 if data1[7 - x] & (0x80 >> y) else 0
      row.append(c0 + c1)
    char.append(row)
  return char

def dump_chr(data):
  """Pack 8x8 tile into unsigned short values.
  Same as original galagino tileconv.py dump_chr."""
  vals = []
  for y in range(8):
    val = 0
    for x in range(8):
      val = (val >> 2) + (data[y][x] << (16 - 2))
    vals.append(val)
  return vals

def convert_tiles(plane0, plane1):
  num_tiles = len(plane0) // 8  # 256
  tiles = []
  for t in range(num_tiles):
    d0 = plane0[t * 8 : t * 8 + 8]
    d1 = plane1[t * 8 : t * 8 + 8]
    char_data = parse_chr_2(d0, d1)
    tiles.append(dump_chr(char_data))
  return tiles

# -------------------------------------------------------------------
# ---- Sprite conversion using original galagino parse_sprite_frogger + dump_sprite ----
# -------------------------------------------------------------------

def parse_sprite_galaxian(data0, data1):
  """
  Parse 16x16 sprite from Galaxian hardware (same as Frogger without D0/D1 swap).
  Based on original galagino spriteconv.py parse_sprite_frogger.
  """
  sprite = []
  for y in range(16):
    row = []
    for x in range(16):
      ym = (y & 7) | ((x & 8) ^ 8)
      xm = (x & 7) | (y & 8)
      byte_idx = (xm ^ 7) + ((ym & 8) << 1)
      bit_mask = 0x80 >> (ym & 7)
      c0 = 1 if data0[byte_idx] & bit_mask else 0
      c1 = 2 if data1[byte_idx] & bit_mask else 0
      row.append(c0 + c1)
    sprite.append(row)
  return sprite

def dump_sprite(data, flip_x, flip_y):
  """
  Pack 16x16 sprite into unsigned long values.
  Same as original galagino spriteconv.py dump_sprite.
  """
  vals = []
  y_range = range(16) if not flip_y else reversed(range(16))
  for y in y_range:
    val = 0
    for x in range(16):
      if not flip_x:
        val = (val >> 2) + (data[y][x] << (32 - 2))
      else:
        val = (val << 2) + data[y][x]
    vals.append(val)
  return vals

def convert_sprites(plane0, plane1):
  num_sprites = len(plane0) // 32  # 64
  # Parse all sprites first
  sprites = []
  for s in range(num_sprites):
    d0 = plane0[32 * s : 32 * (s + 1)]
    d1 = plane1[32 * s : 32 * (s + 1)]
    sprites.append(parse_sprite_galaxian(d0, d1))

  # Generate 4 orientations: [no flip, Y flip, X flip, XY flip]
  all_orientations = []
  for flip_x, flip_y in [(False, False), (False, True), (True, False), (True, True)]:
    orientation = []
    for s in sprites:
      orientation.append(dump_sprite(s, flip_x, flip_y))
    all_orientations.append(orientation)

  return all_orientations

# -------------------------------------------------------------------
# ---- Color PROM -> RGB565 palette ----
# -------------------------------------------------------------------
def convert_colors(prom):
  rgb565 = [] 
  for i in range(32):
    bits = prom[i]
    r = 0x21 * ((bits >> 0) & 1) + 0x47 * ((bits >> 1) & 1) + 0x97 * ((bits >> 2) & 1)
    g = 0x21 * ((bits >> 3) & 1) + 0x47 * ((bits >> 4) & 1) + 0x97 * ((bits >> 5) & 1)
    b = 0x4F * ((bits >> 6) & 1) + 0xA8 * ((bits >> 7) & 1)
    r5 = (r >> 3) & 0x1F
    g6 = (g >> 2) & 0x3F
    b5 = (b >> 3) & 0x1F
    val = (r5 << 11) | (g6 << 5) | b5

    # PROM: 7 6 5 4 3 2 1 0
    #   : b b g g g r r r
    red   = ((bits >> 0) & 0x07)
    green = ((bits >> 3) & 0x07)
    blue  = ((bits >> 6) & 0x03)
    rgb565.append(((val & 0xFF) << 8) | ((val >> 8) & 0xFF))
  return rgb565

# -------------------------------------------------------------------
# main()
# -------------------------------------------------------------------
def main():
    os.makedirs(OUT_DIR, exist_ok=True)

    print(f"Load ROM from: {os.path.abspath(ROM_SET)}")
    print(f"Target files:  {os.path.abspath(OUT_DIR)}")

    # maincpu
    rom_2d = load_file(["2d.k", "s1.2d"], "1dcb375987fe21e0483c27d485c405de53848d61")
    rom_2e = load_file(["2e.k", "s2.2e"], "240877576045fddcc9ff01d97dc78139454ac4f1")
    rom_2f = load_file(["2f.k", "s3.2f"], "a84d191c7be8700f630a83ddad798be9e83b5d55")
    rom_2h = load_file(["2h.k", "s4.2h"], "5d155808c19dcf2e14aa8e29c0ee41a6d3d3c43a")
    rom_2j = load_file(["2j.k", "s5.2j"], "9cb5861c61e4783e5fbaa3869d51195f127b1129")
    rom_2l = load_file(["2l.k", "s6.2l"], "67c0fa81729370631647b5d78bb5a61433facd7f")
    rom_2m = load_file(["2m.k", "s7.2m"], "05a6fe3010c2136284ca76352dac147797c79778")
    rom_2p = load_file(["2p.k", "s8.2p"], "e3b09141cee26857d626412e9d1a0e759469b97a")

    # audiocpu
    rom_5c = load_file(["5c", "ot1.5c"], "8ed78487d76fd0a917ab7b258937a46e2cd9800c")
    rom_5d = load_file(["5d", "ot2.5d"], "8558b4eff5d7e63029b325edef9914feda5834c3")
    rom_5e = load_file(["5e", "ot3.5e"], "1f976d8595706730e29f93027e7ab4620075c078")

    # gfx1
    rom_5f = load_file(["5f.k", "c2.5f"], "a8b1ad19a95a9d35050a2ab7194cc96fc5afcdc9")
    rom_5h = load_file(["5h.k", "c1.5h"], "69844e48bb4d372cac7ae83c953df573c7ecbb7f")

    # prom
    prom_6e = load_file(["82s123.6e", "c01s.6e"], "a25083c3e36d28afdefe4af6e6d4f3155e303625")
    
    files_ok = all(v is not None for v in [rom_2d, rom_2e, rom_2f, rom_2h, rom_2j, rom_2l, rom_2m, rom_2p,
                                           rom_5c, rom_5d, rom_5e,
                                           rom_5f, rom_5h,
                                           prom_6e])
    if not files_ok:
      print("ERROR: Not all files have been loaded")
      return

    # convert main cpu rom - 0x4000
    main_cpu = rom_2d + rom_2e + rom_2f + rom_2h + rom_2j + rom_2l + rom_2m + rom_2p
    if len(main_cpu) < 0x4000:
      main_cpu += bytearray([0xFF] * (0x4000 - len(main_cpu)))
    write_rom(os.path.join(OUT_DIR, "scramble_main_rom.h"), "scramble_main_rom", main_cpu)
       
    # convert audio cpu rom - 0x2000
    audio_cpu = rom_5c + rom_5d + rom_5e
    if len(audio_cpu) < 0x2000:
      audio_cpu += bytearray([0xFF] * (0x2000 - len(audio_cpu)))
    write_rom(os.path.join(OUT_DIR, "scramble_audio_rom.h"), "scramble_audio_rom", audio_cpu)

    # Convert tiles and sprites using original galagino algorithms
    tiles = convert_tiles(rom_5f, rom_5h)
    write_tilemap(os.path.join(OUT_DIR, "scramble_tilemap.h"), tiles)

    sprites = convert_sprites(rom_5f, rom_5h)
    write_spritemap(os.path.join(OUT_DIR, "scramble_spritemap.h"), sprites)

    rgb565 = convert_colors(prom_6e)
    write_colormap(os.path.join(OUT_DIR, "scramble_cmap.h"), rgb565)

    print("\n--- Complete ---")
    print(f"All files generated in: {os.path.abspath(OUT_DIR)}")
     
if __name__ == "__main__":
    main()
