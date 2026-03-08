@echo off
echo --------- Convert Bagman ---------
echo Bagman Unpack roms
python ./unpack.py bagmanm2.zip
if errorlevel 1 goto :error

rem echo Bagman Logos
rem python ./logoconv.py ../logos/bagman.png ../source/src/machines/bagman/bagman_logo.h
if errorlevel 1 goto :error

echo Bagman CPU code
python ./romconv.py bagman_rom_cpu ./roms/bagmanm2.1 ./roms/bagmanm2.2 ./roms/bagmanm2.3  ../source/src/machines/bagman/bagman_rom.h
if errorlevel 1 goto :error

echo Bagman Tiles
python ./tileconv.py bagman bagman_tilemap ./roms/bagmanm2.9 ./roms/bagmanm2.7 ../source/src/machines/bagman/bagman_tilemap.h
if errorlevel 1 goto :error

echo Bagman Sprites
python ./spriteconv.py bagman_sprites bagman ./roms/bagmanm2.9 ./roms/bagmanm2.7 ../source/src/machines/bagman/bagman_spritemap.h
if errorlevel 1 goto :error

echo Bagman Colormaps
python ./cmapconv.py bagman_colormap ./roms/bagmanmc.clr ../source/src/machines/bagman/bagman_cmap.h
if errorlevel 1 goto :error

echo --- Success ---
goto end

:error
echo --- Error #%errorlevel%.
pause

:end
