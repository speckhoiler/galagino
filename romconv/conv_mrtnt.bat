@echo off
echo --------- Convert MrTNT ---------
echo MrTNT Unpack roms
python ./unpack.py mrtnt.zip
if errorlevel 1 goto :error

rem echo MrTNT Logos
rem python ./logoconv.py ../logos/mrtnt.png ../source/src/machines/mrtnt/mrtnt_logo.h
if errorlevel 1 goto :error

echo MrTNT CPU code
python ./romconv.py -d mrtnt_rom ./roms/tnt.1 ./roms/tnt.2 ./roms/tnt.3 ./roms/tnt.4 ../source/src/machines/mrtnt/mrtnt_rom.h
if errorlevel 1 goto :error

echo MrTNT Tiles
python ./tileconv.py mrtnt_tilemap ./roms/tnt.5 ../source/src/machines/mrtnt/mrtnt_tilemap.h
if errorlevel 1 goto :error

echo MrTNT Sprites
python ./spriteconv.py mrtnt_sprites mrtnt ./roms/tnt.6 ../source/src/machines/mrtnt/mrtnt_spritemap.h
if errorlevel 1 goto :error

echo MrTNT Colormaps
python ./cmapconv.py mrtnt_colormap ./roms/82s123.7f 0 ./roms/82s126.4a ../source/src/machines/mrtnt/mrtnt_cmap.h
if errorlevel 1 goto :error

echo MrTNT Audio
python ./audioconv.py mrtnt_wavetable ./roms/82s126.1m ../source/src/machines/mrtnt/mrtnt_wavetable.h
if errorlevel 1 goto :error

echo --- Success ---
goto end

:error
echo --- Error #%errorlevel%.
pause

:end