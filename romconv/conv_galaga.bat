echo "Unpack roms"
python ./unpack.py galaga.zip

echo "Logos"
python ./logoconv.py ../logos/galaga.png ../source/src/machines/galaga/galaga_logo.h

echo "CPU code"
python ./romconv.py -p galaga_rom_cpu1 ./roms/gg1_1b.3p ./roms/gg1_2b.3m ./roms/gg1_3.2m ./roms/gg1_4b.2l ../source/src/machines/galaga/galaga_rom1.h
python ./romconv.py galaga_rom_cpu2 ./roms/gg1_5b.3f ../source/src/machines/galaga/galaga_rom2.h
python ./romconv.py galaga_rom_cpu3 ./roms/gg1_7b.2c ../source/src/machines/galaga/galaga_rom3.h

echo "Tiles"
python ./tileconv.py galaga_tilemap ./roms/gg1_9.4l ../source/src/machines/galaga/galaga_tilemap.h

echo "Sprites"
python ./spriteconv.py galaga_sprites galaga ./roms/gg1_11.4d ./roms/gg1_10.4f ../source/src/machines/galaga/galaga_spritemap.h

echo "Colormaps"
python ./cmapconv.py galaga_colormap_sprites ./roms/prom-5.5n 0 ./roms/prom-3.1c ../source/src/machines/galaga/galaga_cmap_sprites.h
python ./cmapconv.py galaga_colormap_tiles ./roms/prom-5.5n 16 ./roms/prom-4.2n ../source/src/machines/galaga/galaga_cmap_tiles.h

echo "Audio"
python ./audioconv.py galaga_wavetable ./roms/prom-1.1d ../source/src/machines/galaga/galaga_wavetable.h

Pause