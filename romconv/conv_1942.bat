echo "Unpack roms"
python ./unpack.py 1942.zip

echo "Logos"
python ./logoconv.py ../logos/1942.png ../source/src/machines/1942/1942_logo.h

echo "CPU code"
python ./romconv.py _1942_rom_cpu1 ./roms/srb-03.m3 ./roms/srb-04.m4 ../source/src/machines/1942/1942_rom1.h
python ./romconv.py _1942_rom_cpu1_b0 ./roms/srb-05.m5 ../source/src/machines/1942/1942_rom1_b0.h
python ./romconv.py _1942_rom_cpu1_b1 ./roms/srb-06.m6 ../source/src/machines/1942/1942_rom1_b1.h
python ./romconv.py _1942_rom_cpu1_b2 ./roms/srb-07.m7 ../source/src/machines/1942/1942_rom1_b2.h
python ./romconv.py _1942_rom_cpu2 ./roms/sr-01.c11 ../source/src/machines/1942/1942_rom2.h

echo "Tiles"
python ./tileconv.py _1942_charmap ./roms/sr-02.f2 ../source/src/machines/1942/1942_charmap.h
python ./tileconv.py _1942_tilemap ./roms/sr-08.a1 ./roms/sr-09.a2 ./roms/sr-10.a3 ./roms/sr-11.a4 ./roms/sr-12.a5 ./roms/sr-13.a6 ../source/src/machines/1942/1942_tilemap.h

echo "Sprites"
python ./spriteconv.py _1942_sprites 1942 ./roms/sr-14.l1 ./roms/sr-15.l2 ./roms/sr-16.n1 ./roms/sr-17.n2 ../source/src/machines/1942/1942_spritemap.h

echo "Colormaps"
python ./cmapconv.py _1942_colormap_chars ./roms/sb-5.e8,./roms/sb-6.e9,./roms/sb-7.e10 128 ./roms/sb-0.f1 ../source/src/machines/1942/1942_character_cmap.h
python ./cmapconv.py _1942_colormap_tiles ./roms/sb-5.e8,./roms/sb-6.e9,./roms/sb-7.e10 -1 ./roms/sb-4.d6,./roms/sb-3.d2,./roms/sb-2.d1 ../source/src/machines/1942/1942_tile_cmap.h
python ./cmapconv.py _1942_colormap_sprites ./roms/sb-5.e8,./roms/sb-6.e9,./roms/sb-7.e10 64 ./roms/sb-8.k3 ../source/src/machines/1942/1942_sprite_cmap.h

Pause