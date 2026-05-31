@echo off
echo --------- Convert Star Force ---------
echo Star Force Unpack roms
python ./unpack.py starforc.zip
if errorlevel 1 goto :error

rem echo Star Force Logos
rem python ./logoconv.py ../logos/starforce.png ../source/src/machines/starforce/starforce_logo.h
if errorlevel 1 goto :error

cd starforce

echo Star Force CPU code
python ./cpu_conv.py
if errorlevel 1 goto :error

echo Star Force Tiles
python ./bg1_tiles.py
python ./bg2_tiles.py
python ./bg3_tiles.py
python ./fg_tiles.py
if errorlevel 1 goto :error

echo Star Force Sprites
python ./sprites.py
if errorlevel 1 goto :error

cd..

echo --- Success ---
goto end

:error
echo --- Error #%errorlevel%.
pause

:end
