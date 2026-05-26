@echo off
echo --------- Convert Pengo ---------
echo Pengo Unpack roms
python ./unpack.py pengo2u.zip
if errorlevel 1 goto :error

rem echo Pengo Logos
rem python ./logoconv.py ../logos/pengo.png ../source/src/machines/pengo/pengo_logo.h
if errorlevel 1 goto :error

cd pengo

echo Pengo CPU code
python ./cpu_conv.py
python ./audio_conv.py
if errorlevel 1 goto :error

echo Peno Tiles
python ./tiles_fg_conv.py
if errorlevel 1 goto :error

echo Peno Sprites
python ./sprites_conv.py
if errorlevel 1 goto :error

echo Pengo Colormaps
python ./colormap.py
if errorlevel 1 goto :error

echo --- Success ---
goto end

:error
echo --- Error #%errorlevel%.
pause

:end
cd..
