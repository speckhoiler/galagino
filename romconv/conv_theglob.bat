echo "Unpack roms"
python ./unpack.py theglobp.zip

echo "Logos"
python ./logoconv.py ../logos/theglob.png ../source/src/machines/theglob/theglob_logo.h

echo "CPU code"
python ./romconv.py theglob_rom ./roms/glob.u2 ./roms/glob.u3 ../source/src/machines/theglob/theglob_rom.h

echo "Tiles"
python ./tileconv.py theglob_tilemap ./roms/glob.5e ../source/src/machines/theglob/theglob_tilemap.h

echo "Sprites"
python ./spriteconv.py theglob_sprites pacman ./roms/glob.5f ../source/src/machines/theglob/theglob_spritemap.h

echo "Colormaps"
python ./cmapconv.py theglob_colormap ./roms/glob.7f 0 ./roms/glob.4a ../source/src/machines/theglob/theglob_cmap.h

echo "Audio"
python ./audioconv.py theglob_wavetable ./roms/82s126.1m ../source/src/machines/theglob/theglob_wavetable.h

Pause