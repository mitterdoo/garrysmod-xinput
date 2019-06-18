@echo off
set BIN_NAME=xinput
set BIN_REALM=cl

echo Creating 64-bit
call %comspec% /k make64.cmd
echo Done.

echo Creating 32-bit
call %comspec% /k make32.cmd
echo Done.
pause