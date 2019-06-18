@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars32.bat"
cl /I..\include /LD main.cpp /link /out:gm%BIN_REALM%_%BIN_NAME%_win32.dll x86\Xinput.lib
copy gm%BIN_REALM%_%BIN_NAME%_win32.dll ..\bin
exit