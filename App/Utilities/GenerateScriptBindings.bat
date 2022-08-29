@echo off
setlocal

set SEOUL_APP_PC="%~dp0..\Binaries\PC\Developer\x64\AppPC.exe"
set SEOUL_APP_ROOT="%~dp0.."
set SEOUL_FASTBUILD="%~dp0..\..\External\fastbuild\Windows\FBuild.exe"
set SEOUL_CONSOLE_TOOL="%~dp0..\..\SeoulTools\Binaries\PC\Developer\x64\ConsoleTool.exe"
set SEOUL_OUT_DIR="%~dp0..\..\App\Source\Authored\Scripts\Engine\Native

REM Perform builds.
call :build_dependencies %1
if '%errorlevel%' NEQ '0' goto fail

REM Generate bindings
%SEOUL_CONSOLE_TOOL% -test_runner %SEOUL_APP_PC% -generate_script_bindings=%SEOUL_OUT_DIR%
if '%errorlevel%' NEQ '0' goto fail

:success
popd
echo [SUCCESS]
exit /b 0

:fail
popd
echo [FAIL]
exit /B 1

REM
REM Function to build dependencies prior to running tests.
REM
:build_dependencies

REM Nothing to do
if '%1' EQU '-no_build' (
  exit /b 0
)

call %~dp0PC_BuildAndCook.bat Developer -no_cook
if '%errorlevel%' NEQ '0' (
  exit /B 1
)
exit /b 0
goto :EOF
