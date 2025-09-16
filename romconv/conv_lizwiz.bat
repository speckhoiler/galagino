echo "Unpack roms"
python ./unpack.py lizwiz.zip

echo "Logos"
python ./logoconv.py ../logos/lizwiz.png ../source/src/machines/lizwiz/lizwiz_logo.h

echo "maincpu"
python ./romconv.py lizwiz_rom ./roms/6e.cpu ./roms/6f.cpu ./roms/6h.cpu ./roms/6j.cpu ./roms/wiza ./roms/wizb ../source/src/machines/lizwiz/lizwiz_rom.h

echo "gfx1 Tiles (pacman.5e)"
python ./tileconv.py lizwiz_tilemap ./roms/5e.cpu ../source/src/machines/lizwiz/lizwiz_tilemap.h

echo "gfx1 Sptites (pacman.5f)"
python ./spriteconv.py lizwiz_sprites lizwiz ./roms/5f.cpu ../source/src/machines/lizwiz/lizwiz_spritemap.h

echo "proms"
python ./cmapconv.py lizwiz_colormap ./roms/7f.cpu 0 ./roms/4a.cpu ../source/src/machines/lizwiz/lizwiz_cmap.h

echo "mamco"
python ./audioconv.py lizwiz_wavetable ./roms/82s126.1m ../source/src/machines/lizwiz/lizwiz_wavetable.h

Pause