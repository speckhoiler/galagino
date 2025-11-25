@echo off
echo --------- Convert Frogger ---------
echo Frogger Unpack roms
python ./unpack.py frogger.zip
if errorlevel 1 goto :error

rem echo Frogger Logos
rem python ./logoconv.py ../logos/frogger.png ../source/src/machines/frogger/frogger_logo.h
if errorlevel 1 goto :error

echo Frogger CPU code
python ./romconv.py frogger_rom_cpu1 ./roms/frogger.26 ./roms/frogger.27 ./roms/frsm3.7 ../source/src/machines/frogger/frogger_rom1.h
python ./romconv.py frogger_rom_cpu2 ./roms/frogger.608 ./roms/frogger.609 ./roms/frogger.610 ../source/src/machines/frogger/frogger_rom2.h
if errorlevel 1 goto :error

echo Frogger Tiles
python ./tileconv.py frogger_tilemap ./roms/frogger.606 ./roms/frogger.607 ../source/src/machines/frogger/frogger_tilemap.h
if errorlevel 1 goto :error

echo Frogger Sprites
python ./spriteconv.py frogger_sprites frogger ./roms/frogger.606 ./roms/frogger.607 ../source/src/machines/frogger/frogger_spritemap.h
if errorlevel 1 goto :error

echo Frogger Colormaps
python ./cmapconv.py frogger_colormap ./roms/pr-91.6l ../source/src/machines/frogger/frogger_cmap.h
if errorlevel 1 goto :error

echo --- Success ---
goto end

:error
echo --- Error #%errorlevel%.
pause

:end