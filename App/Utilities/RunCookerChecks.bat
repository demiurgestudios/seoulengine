@echo off
setlocal

set SEOUL_FASTBUILD="%~dp0..\..\External\fastbuild\Windows\FBuild.exe"
set SEOUL_TOOLS_ROOT=""%~dp0..\..\SeoulTools"
set SEOUL_CONSOLE_TOOL="%~dp0..\..\SeoulTools\Binaries\PC\Developer\x64\ConsoleTool.exe"
set SEOUL_UNIT_TESTS="%~dp0..\..\SeoulTools\Binaries\PC\Developer\x64\Cooker.exe"

REM Perform builds of things about to be tested if necessary.
call :build_dependencies %1
if '%errorlevel%' neq '0' goto fail

REM Pull test name if defined.
set SEOUL_TEST_TO_RUN=
if '%1' NEQ '-no_build' (
  set SEOUL_TEST_TO_RUN=%1
) else (
  set SEOUL_TEST_TO_RUN=%2
)

REM Cooker unit tests.
echo Running C++ unit tests...
%SEOUL_CONSOLE_TOOL% -test_runner %SEOUL_UNIT_TESTS% -run_unit_tests=%SEOUL_TEST_TO_RUN%
if '%errorlevel%' neq '0' goto fail

:success
popd
echo [SUCCESS]
exit /b 0

:fail
popd
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

echo.
echo Building SeoulTools "PC-DeveloperTools"...
pushd %SEOUL_TOOLS_ROOT%
%SEOUL_FASTBUILD% -quiet PC-DeveloperTools
if '%errorlevel%' NEQ '0' (
  echo FAILED. Please ping an engineer.
  set /p TEMP_PROMPT=
  exit /B 1
)
popd


exit /B 0
goto :EOF
