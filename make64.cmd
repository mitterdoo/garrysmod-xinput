@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat"
cl /I..\include /LD main.cpp /link /out:..\bin\gm%BIN_REALM%_%BIN_NAME%_win64.dll tier0_64.lib x64\Xinput.lib
exit