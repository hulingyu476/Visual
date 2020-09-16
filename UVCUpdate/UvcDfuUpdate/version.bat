@echo off
::echo [version] > uvcversion.txt

for /F "tokens=3,4 delims= " %%i in ('%~dp0\dfu-util.exe -l') do (
set pidvid=%%i
set swver=%%j
)

if "%pidvid:~1,9%"=="2757:2001" (goto find) else (goto notfind)

:find
::echo find

::echo vid:pid=%pidvid:~1,9%

set vid=%pidvid:~1,4%
set pid=%pidvid:~6,4%

::echo vid=%vid% >> uvcversion.txt
::echo pid=%pid% >> uvcversion.txt

for /F "tokens=1 delims=ver^=" %%i in ("%swver%") do (set swver=%%i)
for /F "tokens=1 delims=," %%i in ("%swver%") do (set swver=%%i)


set MainVersion=V8
set SubVersion=%swver:~0,1%
set CusVersion=%swver:~1,3%

::echo JX1701U:V8.%swver%> uvcversion.txt
echo JX1701U:%MainVersion%.%SubVersion%.%CusVersion%> uvcversion.txt

exit

:notfind
::echo notfind

echo off> uvcversion.txt

exit

