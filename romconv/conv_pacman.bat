@echo off
echo --------- Convert Pacman ---------
echo Pacman Unpack roms
python ./unpack.py pacman.zip
if errorlevel 1 goto :error

rem echo Pacman Logos
rem python ./logoconv.py ../logos/pacman.png ../source/src/machines/pacman/pacman_logo.h
if errorlevel 1 goto :error

echo Pacman CPU code
python ./romconv.py pacman_rom ./roms/pacman.6e ./roms/pacman.6f ./roms/pacman.6h ./roms/pacman.6j ../source/src/machines/pacman/pacman_rom.h
if errorlevel 1 goto :error

echo Pacman Tiles
python ./tileconv.py pacman_tilemap ./roms/pacman.5e ../source/src/machines/pacman/pacman_tilemap.h
if errorlevel 1 goto :error

echo Pacman Sprites
python ./spriteconv.py pacman_sprites pacman ./roms/pacman.5f ../source/src/machines/pacman/pacman_spritemap.h
if errorlevel 1 goto :error

echo Pacman Colormaps
python ./cmapconv.py pacman_colormap ./roms/82s123.7f 0 ./roms/82s126.4a ../source/src/machines/pacman/pacman_cmap.h
if errorlevel 1 goto :error

echo Pacman Audio
python ./audioconv.py pacman_wavetable ./roms/82s126.1m ../source/src/machines/pacman/pacman_wavetable.h
if errorlevel 1 goto :error

echo --- Success ---
goto end

:error
echo --- Error #%errorlevel%.
pause

:end