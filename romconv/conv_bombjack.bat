@echo off
echo --------- Convert Bombjack ---------
echo Bombjack Unpack roms
python ./unpack.py bombjack.zip
if errorlevel 1 goto :error

rem echo Bombjack Logos
rem python ./logoconv.py ../logos/bombjack.png ../source/src/machines/bombjack/bombjack_logo.h
if errorlevel 1 goto :error

cd bombjack

echo Bombjack CPU code
cpu_conv.py
audio_cpu_conv.py
if errorlevel 1 goto :error

echo Bombjack Tiles
tiles_bg_conv.py
tiles_fg_conv.py
bgmaps_conv.py
if errorlevel 1 goto :error

echo Bombjack Sprites
sprites_conv.py
if errorlevel 1 goto :error

echo --- Success ---
goto end

:error
echo --- Error #%errorlevel%.
pause

:end
cd..
