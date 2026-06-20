@echo off
echo --------- Convert Super Cobra ---------
rem echo Super Cobra Logos
rem python ./logoconv.py ../logos/supercobra.png ../source/src/machines/supercobra/supercobra_logo.h

echo Converting Super Cobra
cd supercobra
python ./supercobra_rom_convert.py
cd ..
if errorlevel 1 goto :error

echo --- Success ---
goto end

:error
echo --- Error #%errorlevel%.
pause

:end
