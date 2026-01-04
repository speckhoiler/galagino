@echo off
echo --------- Convert Anteater ---------
echo Anteater Unpack roms
python ./unpack.py anteater.zip
if errorlevel 1 goto :error

rem echo Anteater Logos
rem python ./logoconv.py ../logos/anteater.png ../source/src/machines/anteater/anteater_logo.h
if errorlevel 1 goto :error

echo Anteater CPU code
python ./romconv.py anteater_rom_cpu1 ./roms/ra1-2c ./roms/ra1-2e ./roms/ra1-2f ./roms/ra1-2h ../source/src/machines/anteater/anteater_rom1.h
python ./romconv.py anteater_rom_cpu2 ./roms/ra4-5c ./roms/ra4-5d ../source/src/machines/anteater/anteater_rom2.h
if errorlevel 1 goto :error

echo Anteater Tiles
python ./tileconv.py anteater anteater_tilemap ./roms/ra6-5f ./roms/ra6-5h ../source/src/machines/anteater/anteater_tilemap.h
if errorlevel 1 goto :error

echo Anteater Sprites
python ./spriteconv.py anteater_sprites anteater ./roms/ra6-5f ./roms/ra6-5h ../source/src/machines/anteater/anteater_spritemap.h
if errorlevel 1 goto :error

echo Anteater Colormaps
python ./cmapconv.py anteater_colormap ./roms/colr6f.cpu ../source/src/machines/anteater/anteater_cmap.h
if errorlevel 1 goto :error

echo --- Success ---
goto end

:error
echo --- Error #%errorlevel%.
pause

:end