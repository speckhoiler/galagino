@echo off
echo --------- Convert MsPacman ---------
echo MsPacman Unpack roms
python ./unpack.py mspacman.zip
if errorlevel 1 goto :error

rem echo MsPacman Logos
rem python ./logoconv.py ../logos/mspacman.png ../source/src/machines/mspacman/mspacman_logo.h
if errorlevel 1 goto :error

echo Converting MsPacman
cd mspacman
python mspacman_rom_convert.py
cd ..
if errorlevel 1 goto :error

echo --- Success ---
goto end

:error
echo --- Error #%errorlevel%.
pause

:end