@echo off
setlocal

REM Note that this script deliberately does not build dependencies
REM because we want it to be runnable on a machine without
REM Visual Studio.

set SEOUL_APP_PC="%~dp0..\..\App\Binaries\PC\Developer\x64\AppPC.exe"
set SEOUL_BASE_DIR=%~dp0..\..\
set SEOUL_CONSOLE_TOOL="%~dp0..\..\SeoulTools\Binaries\PC\Developer\x64\ConsoleTool.exe"
set SEOUL_PLATFORM=PC
set SEOUL_PACKAGE_FILE=%SEOUL_BASE_DIR%App\Data\AppPackages%SEOUL_PLATFORM%.cfg
set SEOUL_TOOL_COOKER=%SEOUL_BASE_DIR%SeoulTools\Binaries\PC\Developer\x64\Cooker.exe

if '%1' equ '-no_build' goto run

REM Cook first.
%SEOUL_TOOL_COOKER% -platform %SEOUL_PLATFORM% -local
if '%errorlevel%' neq '0' goto fail

REM Content checks.
:run
echo Running content checks...
%SEOUL_CONSOLE_TOOL% -test_runner %SEOUL_APP_PC% -run_script=DevOnly/RunContentChecks.lua -ui_manager=config://UnitTests/ContentChecks/gui.json
if '%errorlevel%' neq '0' goto fail

:success
popd
echo SUCCESS. Press Enter to Continue...
set /p TEMP_PROMPT=
exit /B 0

:fail
popd
echo FAILED. Press Enter to Continue...
set /p TEMP_PROMPT=
exit /B 1
