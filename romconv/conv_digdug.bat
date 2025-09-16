echo "Unpack roms"
python ./unpack.py digdug.zip

echo "Logos"
python ./logoconv.py ../logos/digdug.png ../source/src/machines/digdug/digdug_logo.h

echo "CPU code"
python ./romconv.py digdug_rom_cpu1 ./roms/dd1a.1 ./roms/dd1a.2 ./roms/dd1a.3 ./roms/dd1a.4 ../source/src/machines/digdug/digdug_rom1.h
python ./romconv.py digdug_rom_cpu2 ./roms/dd1a.5 ./roms/dd1a.6 ../source/src/machines/digdug/digdug_rom2.h
python ./romconv.py digdug_rom_cpu3 ./roms/dd1.7 ../source/src/machines/digdug/digdug_rom3.h
python ./romconv.py digdug_playfield ./roms/dd1.10b ../source/src/machines/digdug/digdug_playfield.h

echo "Tiles"
python ./tileconv.py digdug_tilemap ./roms/dd1.9 ../source/src/machines/digdug/digdug_tilemap.h
python ./tileconv.py digdug_pftiles ./roms/dd1.11 ../source/src/machines/digdug/digdug_pftiles.h

echo "Sprites"
python ./spriteconv.py digdug_sprites digdug ./roms/dd1.15 ./roms/dd1.14 ./roms/dd1.13 ./roms/dd1.12 ../source/src/machines/digdug/digdug_spritemap.h

echo "Colormaps"
python ./cmapconv.py digdug_colormap_tiles ./roms/136007.113 0 ./roms/136007.112 ../source/src/machines/digdug/digdug_cmap_tiles.h
python ./cmapconv.py digdug_colormap_sprites ./roms/136007.113 16 ./roms/136007.111 ../source/src/machines/digdug/digdug_cmap_sprites.h
python ./cmapconv.py digdug_colormaps ./roms/136007.113 ../source/src/machines/digdug/digdug_cmap.h

echo "Audio"
python ./audioconv.py digdug_wavetable ./roms/136007.110 ../source/src/machines/digdug/digdug_wavetable.h

Pause