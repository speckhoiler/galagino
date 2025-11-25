@echo off
echo --------- Convert Crush ---------
echo Crush Unpack roms
python ./unpack.py crush.zip
if errorlevel 1 goto :error

rem echo Crush Logos
rem python ./logoconv.py ../logos/crush.png ../source/src/machines/crush/crush_logo.h
if errorlevel 1 goto :error

echo Crush CPU code
python ./romconv.py crush_rom ./roms/crushkrl.6e ./roms/crushkrl.6f ./roms/crushkrl.6h ./roms/crushkrl.6j ../source/src/machines/crush/crush_rom.h
if errorlevel 1 goto :error

echo Crush Tiles
python ./tileconv.py crush_tilemap ./roms/maketrax.5e ../source/src/machines/crush/crush_tilemap.h
if errorlevel 1 goto :error

echo Crush Sprites
python ./spriteconv.py crush_sprites crush ./roms/maketrax.5f ../source/src/machines/crush/crush_spritemap.h
if errorlevel 1 goto :error

echo Crush Colormaps
python ./cmapconv.py crush_colormap ./roms/82s123.7f 0 ./roms/2s140.4a ../source/src/machines/crush/crush_cmap.h
if errorlevel 1 goto :error

echo Crush Audio
python ./audioconv.py crush_wavetable ./roms/82s126.1m ../source/src/machines/crush/crush_wavetable.h
if errorlevel 1 goto :error

echo --- Success ---
goto end

:error
echo --- Error #%errorlevel%.
pause

:end