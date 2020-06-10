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

echo "building giocatore.."
gcc -g "..\giocatore.c" -o "giocatore.exe" || goto SOMETHING_WRONG
IF NOT %ERRORLEVEL% == 0 GOTO SOMETHING_WRONG
echo "build completed!"



REM EXECUTION

set PLAYERS_COUNT=3

echo "starting banco.."
start "BANCO" ..\exec\run.bat "banco.exe" %PLAYERS_COUNT% || goto SOMETHING_WRONG
IF NOT %ERRORLEVEL% == 0 GOTO SOMETHING_WRONG

FOR /L %%i IN (1, 1, %PLAYERS_COUNT% ) DO (
    echo "exe started!"
    echo "starting giocatore %%i.."
    start "GIOCATORE (%%i)" ..\exec\run.bat "giocatore.exe" "%%i" || goto SOMETHING_WRONG
    IF NOT %ERRORLEVEL% == 0 GOTO SOMETHING_WRONG
    echo "exe started!"
)

echo.
echo "COMPLETED!"
pause
goto REBUILD

:SOMETHING_WRONG
pause;
goto REBUILD
exit(1);