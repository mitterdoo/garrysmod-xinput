@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars32.bat"
cl /I..\include /LD main.cpp /link /out:..\bin\gm%BIN_REALM%_%BIN_NAME%_win32.dll tier0_32.lib x86\Xinput.lib
exit