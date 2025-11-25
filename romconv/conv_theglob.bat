@echo off
echo --------- Convert TheGlob ---------
echo TheGlob Unpack roms
python ./unpack.py theglobp.zip
if errorlevel 1 goto :error

rem echo TheGlob Logos
rem python ./logoconv.py ../logos/theglob.png ../source/src/machines/theglob/theglob_logo.h
if errorlevel 1 goto :error

echo TheGlob CPU code
python ./romconv.py theglob_rom ./roms/glob.u2 ./roms/glob.u3 ../source/src/machines/theglob/theglob_rom.h
if errorlevel 1 goto :error

echo TheGlob Tiles
python ./tileconv.py theglob_tilemap ./roms/glob.5e ../source/src/machines/theglob/theglob_tilemap.h
if errorlevel 1 goto :error

echo TheGlob Sprites
python ./spriteconv.py theglob_sprites pacman ./roms/glob.5f ../source/src/machines/theglob/theglob_spritemap.h
if errorlevel 1 goto :error

echo TheGlob Colormaps
python ./cmapconv.py theglob_colormap ./roms/glob.7f 0 ./roms/glob.4a ../source/src/machines/theglob/theglob_cmap.h
if errorlevel 1 goto :error

echo TheGlob Audio
python ./audioconv.py theglob_wavetable ./roms/82s126.1m ../source/src/machines/theglob/theglob_wavetable.h
if errorlevel 1 goto :error

echo --- Success ---
goto end

:error
echo --- Error #%errorlevel%.
pause

:end