@echo off
setlocal

set SEOUL_LUA_SCRIPT_DIR="%~dp0..\Source\Authored\Scripts\\"
set SEOUL_LUA_SCRIPT_LINT_COMMAND="%~dp0..\..\External\LuaJIT\bin\x64\ReleaseGC64\luajit.exe" "DevOnly\RunInspect.lua"

REM Change to the scripts folder
pushd %SEOUL_LUA_SCRIPT_DIR%

REM Cleanup the SEOUL_LUA_SCRIPT_DIR variable
set SEOUL_LUA_SCRIPT_DIR=%CD%

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
