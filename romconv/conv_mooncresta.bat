@echo off
echo --------- Convert Moon Cresta ---------
echo Moon Cresta Unpack roms
python ./unpack.py mooncrst.zip
if errorlevel 1 goto :error

rem echo Moon Cresta Logos
rem python ./logoconv.py ../logos/mooncresta.png ../source/src/machines/mooncresta/mooncresta_logo.h

echo Converting Moon Cresta
cd mooncresta
python ./mooncresta_rom_convert.py
cd ..
if errorlevel 1 goto :error

echo --- Success ---
goto end

:error
echo --- Error #%errorlevel%.
pause

:end