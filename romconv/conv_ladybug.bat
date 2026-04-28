@echo off
echo --------- Convert Ladybug ---------
echo Ladybug Unpack roms
python ./unpack.py ladybug.zip
if errorlevel 1 goto :error

rem echo Ladybug Logos
rem python ./logoconv.py ../logos/ladybug.png ../source/src/machines/ladybug/ladybug_logo.h
if errorlevel 1 goto :error

echo Converting Ladybug
cd ladybug
python ladybug_rom_convert.py
cd..
if errorlevel 1 goto :error

echo --- Success ---
goto end

:error
echo --- Error #%errorlevel%.
pause

:end