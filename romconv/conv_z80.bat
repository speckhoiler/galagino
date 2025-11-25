@echo off
echo --------- Convert Z80 ---------
echo Z80
python ./z80patch.py
if errorlevel 1 goto :error

echo --- Success ---
goto end

:error
echo --- Error #%errorlevel%.
pause

:end