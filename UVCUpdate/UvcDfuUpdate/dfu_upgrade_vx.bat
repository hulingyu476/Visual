@echo off

set DFU_USB_VID=0x2757
set DFU_USB_PID=0x2001
rem for /f "delims=" %%i in ('dir /b JX1701U*.img') do (
for /f "delims=" %%i in ('dir /b JX1701U*.dfu') do (
set DFU_UPGRADE_FILE=%%i
)
echo %DFU_UPGRADE_FILE%
::pause 

@echo DFU Upgrade
@echo Device USB ID:%DFU_USB_VID%:%DFU_USB_PID%
@echo Upgrade File:%~dp0%DFU_UPGRADE_FILE%
@echo.

%~dp0\dfu-util.exe -l

@echo.
rem set /p Input=Press return to upgrade ...

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
@echo.
@echo ##########DFU upgrade start, please waiting......... 
@echo.
rem %~dp0\dfu-util.exe -d %DFU_USB_VID%:%DFU_USB_PID% -a 0 -E 10 -R -D %~dp0\%DFU_UPGRADE_FILE%
%~dp0\dfu-util.exe -d %DFU_USB_VID%:%DFU_USB_PID% -a 0 -E 15 -R -D %~dp0\%DFU_UPGRADE_FILE%
@echo.
@echo ##########DFU upgrade end#############################
@echo.
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
@echo.
::set /p Input=Press return to close ... 
exit
