@echo off
echo --------- Convert Lizwiz ---------
echo Lizwiz Unpack roms
python ./unpack.py lizwiz.zip
if errorlevel 1 goto :error

rem echo Lizwiz Logos
rem python ./logoconv.py ../logos/lizwiz.png ../source/src/machines/lizwiz/lizwiz_logo.h
if errorlevel 1 goto :error

echo Lizwiz CPU code
python ./romconv.py lizwiz_rom ./roms/6e.cpu ./roms/6f.cpu ./roms/6h.cpu ./roms/6j.cpu ./roms/wiza ./roms/wizb ../source/src/machines/lizwiz/lizwiz_rom.h
if errorlevel 1 goto :error

echo Lizwiz Tiles
python ./tileconv.py lizwiz_tilemap ./roms/5e.cpu ../source/src/machines/lizwiz/lizwiz_tilemap.h
if errorlevel 1 goto :error

echo Lizwiz Sprites
python ./spriteconv.py lizwiz_sprites lizwiz ./roms/5f.cpu ../source/src/machines/lizwiz/lizwiz_spritemap.h
if errorlevel 1 goto :error

echo Lizwiz Colormaps
python ./cmapconv.py lizwiz_colormap ./roms/7f.cpu 0 ./roms/4a.cpu ../source/src/machines/lizwiz/lizwiz_cmap.h
if errorlevel 1 goto :error

echo Lizwiz Audio
python ./audioconv.py lizwiz_wavetable ./roms/82s126.1m ../source/src/machines/lizwiz/lizwiz_wavetable.h
if errorlevel 1 goto :error

echo --- Success ---
goto end

:error
echo --- Error #%errorlevel%.
pause

:end