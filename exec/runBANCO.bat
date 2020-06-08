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



REM BUILDING

echo "building banco.."
gcc -g "..\banco.c" -o "banco.exe" || goto SOMETHING_WRONG
IF NOT %ERRORLEVEL% == 0 GOTO SOMETHING_WRONG


REM EXECUTION

echo "starting banco.."
start "BANCO" ..\exec\run.bat "banco.exe" %PLAYERS_COUNT% || goto SOMETHING_WRONG
IF NOT %ERRORLEVEL% == 0 GOTO SOMETHING_WRONG

echo.
echo "COMPLETED!"
pause
goto REBUILD

:SOMETHING_WRONG
pause;
goto REBUILD
exit(1);