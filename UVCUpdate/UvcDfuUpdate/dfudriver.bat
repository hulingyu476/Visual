@echo off
if "%PROCESSOR_ARCHTECTURE%"=="x86" (goto x86) else (goto x64)
exit

:x86
@echo.
@echo.
@echo _________________________________________
@echo win32 system
@echo _________________________________________
@echo.
@echo.
"%~dp0camera_driver\dpinst32.exe" /PATH "%~dp0camera_driver\x86" /SA /C /F

set /p Input=Press return to close ... 
exit

:x64
@echo.
@echo.
@echo _________________________________________
@echo x64 system
@echo _________________________________________
@echo.
@echo.
"%~dp0camera_driver\dpinst64.exe" /PATH "%~dp0camera_driver\x64" /SA /C /F


set /p Input=Press return to close ... 
exit


