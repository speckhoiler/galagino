@echo off
echo --------- Convert Tutankhm ---------
echo Tutankm Unpack roms
python ./unpack.py tutankhm.zip
if errorlevel 1 goto :error

rem echo Tutankhm Logos
rem python ./logoconv.py ../logos/tutankhm.png ../source/src/machines/tutankhm/tutankhm_logo.h
if errorlevel 1 goto :error

echo Converting Tutancham
cd tutankhm
python tutankhm_rom_convert.py
cd ..
if errorlevel 1 goto :error

echo --- Success ---
goto end

:error
echo --- Error #%errorlevel%.
pause

:end