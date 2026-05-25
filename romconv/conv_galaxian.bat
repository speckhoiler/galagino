@echo off
echo --------- Convert Galaxian ---------
echo Galaxian Unpack roms
python ./unpack.py galaxian.zip
if errorlevel 1 goto :error

rem echo Galaxian Logos
rem python ./logoconv.py ../logos/galaxian.png ../source/src/machines/galaxian/galaxian_logo.h
if errorlevel 1 goto :error

echo Converting Galaxian
cd galaxian
python galaxian_rom_convert.py
cd ..
if errorlevel 1 goto :error

echo --- Success ---
goto end

:error
echo --- Error #%errorlevel%.
pause

:end