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
python ./cpu_conv.py
python ./audio_cpu_conv.py
if errorlevel 1 goto :error

echo Bombjack Tiles
python ./tiles_bg_conv.py
python ./tiles_fg_conv.py
python ./bgmaps_conv.py
if errorlevel 1 goto :error

echo Bombjack Sprites
python ./sprites_conv.py
if errorlevel 1 goto :error

echo --- Success ---
goto end

:error
echo --- Error #%errorlevel%.
pause

:end
cd..
