@echo off

REM Into the src folder.
pushd %~dp0src

REM 32-bit
call "%VS150COMNTOOLS%..\..\VC\Auxiliary\Build\vcvars32.bat"
if '%errorlevel%' NEQ '0' goto fail
call %~dp0src\msvcbuild.bat static
if '%errorlevel%' NEQ '0' goto fail

REM 64-bit
call "%VS150COMNTOOLS%..\..\VC\Auxiliary\Build\vcvars64.bat"
if '%errorlevel%' NEQ '0' goto fail
call %~dp0src\msvcbuild.bat gc64 static
if '%errorlevel%' NEQ '0' goto fail

:success
popd
echo [SUCCESS]
exit /b 0

:fail
popd
echo [FAILURE]
exit /b 1
