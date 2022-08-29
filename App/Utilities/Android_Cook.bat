@echo off
setlocal

set SEOUL_BASE_DIR=%~dp0..\..\
set SEOUL_PLATFORM=Android
set SEOUL_PACKAGE_FILE=%SEOUL_BASE_DIR%App\Data\AppPackages%SEOUL_PLATFORM%.cfg
set SEOUL_TOOL_COOKER=%SEOUL_BASE_DIR%SeoulTools\Binaries\PC\Developer\x64\Cooker.exe

%SEOUL_TOOL_COOKER% -platform %SEOUL_PLATFORM% -package_file %SEOUL_PACKAGE_FILE% -local
if '%errorlevel%' NEQ '0' (
  echo FAILED. Press Enter to Continue...
  set /p TEMP_PROMPT=
  exit /B 1
)
