    @ECHO OFF
    clear
    REM echo "parametri: 1:%1 e 2:%2"
    call "%1" "%2"
    echo. 
    echo.
    echo "*******************************"
    echo "PROGRAMMA TERMINATO"
    echo "*******************************"
    echo. 
    echo.
    pause
    exit(0)