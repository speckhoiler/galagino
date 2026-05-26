@echo off
echo --------- Convert Donkey Kong Junior ---------
echo Donkey Kong Junior Unpack roms
python ./unpack.py dkongjrj.zip
if errorlevel 1 goto :error

rem echo Donkey Kong Junior Logos
rem python ./logoconv.py ../logos/dkongjr.png ../source/src/machines/dkongjr/dkongjr_logo.h

if errorlevel 1 goto :error
cd dkongjr

echo Donkey Kong Junior CPU code
python ./cpu_conv.py
python ./sound_conv.py
if errorlevel 1 goto :error

echo Donkey Kong Junior Tiles
python ./tilemap_conv.py
python ./view_tiles_graphic.py
if errorlevel 1 goto :error

echo Donkey Kong Junior Colormaps
python ./cmap_conv.py
if errorlevel 1 goto :error

echo Donkey Kong Junior Sprites
python ./sprites_conv.py
if errorlevel 1 goto :error

echo --- Success ---
goto end

:error
echo --- Error #%errorlevel%.
pause

:end
cd..
