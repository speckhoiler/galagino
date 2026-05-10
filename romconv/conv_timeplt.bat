@echo off
echo --------- Convert Time Pilot ---------
echo Time Pilot Unpack roms
python ./unpack.py timeplt.zip
if errorlevel 1 goto :error

rem echo Time Pilot Logos
rem python ./logoconv.py ../logos/timeplt.png ../source/src/machines/timeplt/timeplt_logo.h
if errorlevel 1 goto :error

echo Converting Time Pilot
cd timeplt
python timeplt_rom_convert.py
cd ..
if errorlevel 1 goto :error

echo --- Success ---
goto end

:error
echo --- Error #%errorlevel%.
pause

:end
