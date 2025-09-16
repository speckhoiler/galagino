echo "Unpack roms"
python ./unpack.py frogger.zip

echo "Logos"
python ./logoconv.py ../logos/frogger.png ../source/src/machines/frogger/frogger_logo.h

echo "CPU code"
python ./romconv.py frogger_rom_cpu1 ./roms/frogger.26 ./roms/frogger.27 ./roms/frsm3.7 ../source/src/machines/frogger/frogger_rom1.h
python ./romconv.py frogger_rom_cpu2 ./roms/frogger.608 ./roms/frogger.609 ./roms/frogger.610 ../source/src/machines/frogger/frogger_rom2.h

echo "Tiles"
python ./tileconv.py frogger_tilemap ./roms/frogger.606 ./roms/frogger.607 ../source/src/machines/frogger/frogger_tilemap.h

echo "Sprites"
python ./spriteconv.py frogger_sprites frogger ./roms/frogger.606 ./roms/frogger.607 ../source/src/machines/frogger/frogger_spritemap.h

echo "Colormaps"
python ./cmapconv.py frogger_colormap ./roms/pr-91.6l ../source/src/machines/frogger/frogger_cmap.h

Pause