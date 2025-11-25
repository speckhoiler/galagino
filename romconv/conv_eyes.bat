@echo off
echo --------- Convert Eyes ---------
echo Eyes Unpack roms
python ./unpack.py eyes.zip
if errorlevel 1 goto :error

rem echo Eyes Logos
rem python ./logoconv.py ../logos/eyes.png ../source/src/machines/eyes/eyes_logo.h
if errorlevel 1 goto :error

echo Eyes CPU code
python ./romconv.py -d eyes_rom ./roms/d7 ./roms/e7 ./roms/f7 ./roms/h7 ../source/src/machines/eyes/eyes_rom.h
if errorlevel 1 goto :error

echo Eyes Tiles
python ./tileconv.py eyes_tilemap ./roms/d5 ../source/src/machines/eyes/eyes_tilemap.h
if errorlevel 1 goto :error

echo Eyes Sprites
python ./spriteconv.py eyes_sprites eyes ./roms/e5 ../source/src/machines/eyes/eyes_spritemap.h
if errorlevel 1 goto :error

echo Eyes Colormaps
python ./cmapconv.py eyes_colormap ./roms/82s123.7f 0 ./roms/82s129.4a ../source/src/machines/eyes/eyes_cmap.h
if errorlevel 1 goto :error

echo Eyes Audio
python ./audioconv.py eyes_wavetable ./roms/82s126.1m ../source/src/machines/eyes/eyes_wavetable.h
if errorlevel 1 goto :error

echo --- Success ---
goto end

:error
echo --- Error #%errorlevel%.
pause

:end