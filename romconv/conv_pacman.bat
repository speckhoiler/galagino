echo "Unpack roms"
python ./unpack.py pacman.zip

echo "Logos"
python ./logoconv.py ../logos/pacman.png ../source/src/machines/pacman/pacman_logo.h

echo "CPU code"
python ./romconv.py pacman_rom ./roms/pacman.6e ./roms/pacman.6f ./roms/pacman.6h ./roms/pacman.6j ../source/src/machines/pacman/pacman_rom.h

echo "Tiles"
python ./tileconv.py pacman_tilemap ./roms/pacman.5e ../source/src/machines/pacman/pacman_tilemap.h

echo "Sprites"
python ./spriteconv.py pacman_sprites pacman ./roms/pacman.5f ../source/src/machines/pacman/pacman_spritemap.h

echo "Colormaps"
python ./cmapconv.py pacman_colormap ./roms/82s123.7f 0 ./roms/82s126.4a ../source/src/machines/pacman/pacman_cmap.h

echo "Audio"
python ./audioconv.py pacman_wavetable ./roms/82s126.1m ../source/src/machines/pacman/pacman_wavetable.h

Pause