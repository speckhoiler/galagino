@echo off
echo --------- Convert MrDo ---------
echo MrDo Unpack roms
python ./unpack.py mrdo.zip
if errorlevel 1 goto :error

rem echo MrDo Logos
rem python ./logoconv.py ../logos/mrdo.png ../source/src/machines/mrdo/mrdo_logo.h
if errorlevel 1 goto :error

cd mrdo

echo MrDo CPU code
python ./cpu_conv.py
if errorlevel 1 goto :error

echo MrDo Tiles
python ./bg_tiles.py
python ./fg_tiles.py
if errorlevel 1 goto :error

echo MrDo Sprites
python ./sprites.py
if errorlevel 1 goto :error

echo MrDo Colormaps
python ./Palette_mrdo.py
python ./sprite_colormap.py
if errorlevel 1 goto :error

echo --- Success ---
goto end

:error
echo --- Error #%errorlevel%.
pause

:end
cd..
