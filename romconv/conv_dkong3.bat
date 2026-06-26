@echo off
echo --------- Convert Donkey Kong 3 ---------
echo Donkey Kong Junior Unpack roms
python ./unpack.py dkong3.zip
if errorlevel 1 goto :error

rem echo Donkey Kong 3 Logos
rem python ./logoconv.py ../logos/dkong3.png ../source/src/machines/dkong3/dkong3_logo.h
if errorlevel 1 goto :error
cd dkong3

echo Donkey Kong 3 CPU code
python ./cpu_conv.py
python ./sound_conv.py
if errorlevel 1 goto :error

echo Donkey Kong 3 Tiles
python ./tilemap_conv.py
rem python ./view_tiles_graphic.py
if errorlevel 1 goto :error

echo Donkey Kong 3 Colormaps
python ./cmap_conv.py
python ./color_codes_conv.py
if errorlevel 1 goto :error

echo Donkey Kong 3 Sprites
python ./sprites_conv.py
if errorlevel 1 goto :error

echo --- Success ---
goto end

:error
echo --- Error #%errorlevel%.
pause

:end
cd..