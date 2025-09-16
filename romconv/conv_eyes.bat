echo "Unpack roms"
python ./unpack.py eyes.zip

echo "Logos"
python ./logoconv.py ../logos/eyes.png ../source/src/machines/eyes/eyes_logo.h

echo "CPU code"
python ./romconv.py -d eyes_rom ./roms/d7 ./roms/e7 ./roms/f7 ./roms/h7 ../source/src/machines/eyes/eyes_rom.h

echo "Tiles"
python ./tileconv.py eyes_tilemap ./roms/d5 ../source/src/machines/eyes/eyes_tilemap.h

echo "Sprites"
python ./spriteconv.py eyes_sprites eyes ./roms/e5 ../source/src/machines/eyes/eyes_spritemap.h

echo "Colormaps"
python ./cmapconv.py eyes_colormap ./roms/82s123.7f 0 ./roms/82s129.4a ../source/src/machines/eyes/eyes_cmap.h

echo "Audio"
python ./audioconv.py eyes_wavetable ./roms/82s126.1m ../source/src/machines/eyes/eyes_wavetable.h

Pause