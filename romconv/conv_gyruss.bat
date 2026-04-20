@echo off
echo --------- Convert Gyruss ---------
echo Gyruss Unpack roms
python ./unpack.py gyruss.zip
if errorlevel 1 goto :error

rem echo Gyruus Logos
rem python ./logoconv.py ../logos/gyruss.png ../source/src/machines/gyruss/gyruss_logo.h
if errorlevel 1 goto :error

echo Converting Gyruss
cd gyruss
python gyruss_rom_convert.py
cd ..

if errorlevel 1 goto :error

echo --- Success ---
goto end

:error
echo --- Error #%errorlevel%.
pause

:end
