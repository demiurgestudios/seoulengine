@echo off
setlocal

REM Max run time for unit tests.
set SEOUL_TIMEOUT_SECS=600

set SEOUL_CONSOLE_TOOL="%~dp0..\..\SeoulTools\Binaries\PC\Developer\x64\ConsoleTool.exe"
set SEOUL_CPP_UNIT_TESTS="%~dp0..\..\App\Binaries\PC\Developer\x64\AppPC.exe"
set SEOUL_LUA_SCRIPT_DIR="%~dp0..\..\App\Source\Authored\Scripts\\"
set SEOUL_LUA_SCRIPT_LINT_COMMAND="%~dp0..\..\External\LuaJIT\bin\x64\ReleaseGC64\luajit.exe" "DevOnly\RunInspect.lua"
set SEOUL_LUA_SCRIPT_UNIT_TESTS=DevOnly/RunUnitTests.lua

REM Perform builds of things about to be tested if necessary.
call :build_dependencies %1
if '%errorlevel%' neq '0' goto fail

REM Change to the scripts folder
pushd %SEOUL_LUA_SCRIPT_DIR%

REM Cleanup the SEOUL_LUA_SCRIPT_DIR variable
set SEOUL_LUA_SCRIPT_DIR=%CD%

REM Lua script tests.
echo Running Lua script unit tests...
%SEOUL_CONSOLE_TOOL% -timeout_secs %SEOUL_TIMEOUT_SECS% -test_runner %SEOUL_CPP_UNIT_TESTS% -run_script=%SEOUL_LUA_SCRIPT_UNIT_TESTS%
if '%errorlevel%' neq '0' goto fail

REM Pull test name if defined.
set SEOUL_TEST_TO_RUN=
if '%1' neq '-no_build' (
  set SEOUL_TEST_TO_RUN=%1
) else (
  set SEOUL_TEST_TO_RUN=%2
)

REM Native code unit tests.
echo Running C++ unit tests...
%SEOUL_CONSOLE_TOOL% -timeout_secs %SEOUL_TIMEOUT_SECS% -test_runner %SEOUL_CPP_UNIT_TESTS% -run_unit_tests=%SEOUL_TEST_TO_RUN%
if '%errorlevel%' neq '0' goto fail

REM Lua script lint checks.
echo Running Lua script lint checks...
%SEOUL_LUA_SCRIPT_LINT_COMMAND% %SEOUL_LUA_SCRIPT_DIR%
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

REM Run PC_BuildAndCook.bat Developer and check result
call %~dp0PC_BuildAndCook.bat Developer
if '%errorlevel%' neq '0' exit /b 1

exit /b 0
goto :EOF
