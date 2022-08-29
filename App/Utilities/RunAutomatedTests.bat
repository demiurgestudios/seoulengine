@echo off
setlocal

REM Keep this in sync with the equivalent value kfRunTimeSeconds in the automated test script.
set SEOUL_TIMEOUT_SECS=100

set SEOUL_CONSOLE_TOOL="%~dp0..\..\SeoulTools\Binaries\PC\Developer\x64\ConsoleTool.exe"
set SEOUL_CPP_UNIT_TESTS="%~dp0..\Binaries\PC\Developer\x64\AppPC.exe"

REM Perform builds of things about to be tested if necessary.
call :build_dependencies %1
if '%errorlevel%' neq '0' goto fail

REM Native code automated tests
echo Running C++ automated tests (FTUE Enabled)...
%SEOUL_CONSOLE_TOOL% -timeout_secs %SEOUL_TIMEOUT_SECS% -test_runner %SEOUL_CPP_UNIT_TESTS% -run_automated_test=DevOnly/AutomatedTests/BasicAutomation.lua
if '%errorlevel%' neq '0' goto fail

echo Running C++ automated tests (FTUE Disable)...
%SEOUL_CONSOLE_TOOL% -timeout_secs %SEOUL_TIMEOUT_SECS% -test_runner %SEOUL_CPP_UNIT_TESTS% -run_automated_test=DevOnly/AutomatedTests/BasicAutomation.lua -disable_ftue
if '%errorlevel%' neq '0' goto fail

:success
echo [SUCCESS]
exit /b 0

:fail
echo [FAIL]
exit /b 1

REM
REM Function to build dependencies prior to running tests.
REM
:build_dependencies

REM Nothing to do
if '%1' EQU '-no_build' (
  exit /b 0
)

REM Run PC_BuildAndCook.bat Developer and check result
call %~dp0PC_BuildAndCook.bat Developer
if '%errorlevel%' neq '0' exit /b 1

exit /b 0
goto :EOF
