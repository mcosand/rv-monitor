@echo off
del electron*.bin
call particle compile electron || echo %ERRORLEVEL%
move electron*.bin firmware.bin
call particle flash --serial firmware.bin