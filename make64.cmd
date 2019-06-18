@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat"
cl /I..\include /LD main.cpp /link /out:gm%BIN_REALM%_%BIN_NAME%_win64.dll x64\Xinput.lib
copy gm%BIN_REALM%_%BIN_NAME%_win64.dll ..\bin\
exit