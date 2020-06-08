@ECHO OFF
cd ..\bin

:REBUILD
clear 

taskkill /F /T /IM banco.exe
taskkill /F /T /IM giocatore.exe
echo.
echo.

ipcrm -M 5634284608833979236
ipcrm -S 5634284608833979236
ipcrm -Q 5634284608833979236
echo.
echo.

echo "building banco.."
gcc -g "..\banco.c" -o "banco.exe" || goto SOMETHING_WRONG
IF NOT %ERRORLEVEL% == 0 GOTO SOMETHING_WRONG

echo "building giocatore.."
gcc -g "..\giocatore.c" -o "giocatore.exe" || goto SOMETHING_WRONG
IF NOT %ERRORLEVEL% == 0 GOTO SOMETHING_WRONG
echo "build completed!"

echo "starting banco.."
start "BANCO" ..\exec\run.bat "banco.exe" || goto SOMETHING_WRONG
IF NOT %ERRORLEVEL% == 0 GOTO SOMETHING_WRONG
echo "starting giocatore 1.."
start "GIOCATORE (1)" ..\exec\run.bat "giocatore.exe" "1" || goto SOMETHING_WRONG

REM echo "exe started!"
REM echo "starting giocatore 2.."
REM start "GIOCATORE (2)" ..\exec\run.bat "giocatore.exe" "2" || goto SOMETHING_WRONG
REM IF NOT %ERRORLEVEL% == 0 GOTO SOMETHING_WRONG
REM echo "exe started!"

echo.
echo "COMPLETED!"
pause
goto REBUILD

:SOMETHING_WRONG
pause;
goto REBUILD
exit(1);