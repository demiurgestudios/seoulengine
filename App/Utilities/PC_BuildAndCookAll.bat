@echo off
setlocal

call %~dp0PC_BuildAndCook.bat All
if '%errorlevel%' neq '0' exit /b 1
