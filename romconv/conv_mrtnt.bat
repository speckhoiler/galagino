echo "Unpack roms"
python ./unpack.py mrtnt.zip

echo "Logos"
python ./logoconv.py ../logos/mrtnt.png ../source/src/machines/mrtnt/mrtnt_logo.h

echo "CPU code"
python ./romconv.py -d mrtnt_rom ./roms/tnt.1 ./roms/tnt.2 ./roms/tnt.3 ./roms/tnt.4 ../source/src/machines/mrtnt/mrtnt_rom.h

echo "Tiles"
python ./tileconv.py mrtnt_tilemap ./roms/tnt.5 ../source/src/machines/mrtnt/mrtnt_tilemap.h

echo "Sprites"
python ./spriteconv.py mrtnt_sprites mrtnt ./roms/tnt.6 ../source/src/machines/mrtnt/mrtnt_spritemap.h

echo "Colormaps"
python ./cmapconv.py mrtnt_colormap ./roms/82s123.7f 0 ./roms/82s126.4a ../source/src/machines/mrtnt/mrtnt_cmap.h

echo "Audio"
python ./audioconv.py mrtnt_wavetable ./roms/82s126.1m ../source/src/machines/mrtnt/mrtnt_wavetable.h

Pause