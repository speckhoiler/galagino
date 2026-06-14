@echo off
echo --------- Convert Scramble ---------
echo Scramble Unpack roms
python ./unpack.py scramble.zip
if errorlevel 1 goto :error

rem echo Scramble Logos
rem python ./logoconv.py ../logos/scramble.png ../source/src/machines/scramble/scramble_logo.h

echo Converting Scramble
cd scramble
python ./scramble_rom_convert.py
cd ..
if errorlevel 1 goto :error

echo --- Success ---
goto end

:error
echo --- Error #%errorlevel%.
pause

:end