@echo off
setlocal

set SEOUL_BUILD_CONFIG=%1
set SEOUL_APP_ROOT="%~dp0..\..\App"
set SEOUL_EDITOR_ROOT="%~dp0..\..\SeoulEditor"
set SEOUL_FASTBUILD="%~dp0..\..\External\fastbuild\Windows\FBuild.exe"
set SEOUL_TOOLS_ROOT=""%~dp0..\..\SeoulTools"

if 'Debug' NEQ '%1' (
  if 'Developer' NEQ '%1' (
    if 'Profiling' NEQ '%1' (
      if 'Ship' NEQ '%1' (
        if 'All' NEQ '%1' (
          echo Valid configurations are Debug, Developer, Profiling, Ship, and All.
          echo FAILED. Press Enter to Continue...
          set /p TEMP_PROMPT=
          exit /B 1
        )
      )
    )
  )
)

echo Building SeoulTools "PC-DeveloperTools"...
pushd %SEOUL_TOOLS_ROOT%
%SEOUL_FASTBUILD% -quiet PC-DeveloperTools
if '%errorlevel%' NEQ '0' (
  echo FAILED. Please ping an engineer.
  set /p TEMP_PROMPT=
  exit /B 1
)
popd

REM Skip cooking if requested.
if '%2' EQU '-no_cook' goto build

set SEOUL_BASE_DIR=%~dp0..\..\
set SEOUL_PLATFORM=PC
set SEOUL_PACKAGE_FILE=%SEOUL_BASE_DIR%App\Data\AppPackages%SEOUL_PLATFORM%.cfg
set SEOUL_TOOL_COOKER=%SEOUL_BASE_DIR%SeoulTools\Binaries\PC\Developer\x64\Cooker.exe

echo.
%SEOUL_TOOL_COOKER% -platform %SEOUL_PLATFORM% -package_file %SEOUL_PACKAGE_FILE% -local
if '%errorlevel%' NEQ '0' EXIT /B 1

:build
echo.
echo Building EditorPC "PC-DeveloperTools"...
pushd %SEOUL_EDITOR_ROOT%
%SEOUL_FASTBUILD% -quiet PC-DeveloperTools
if '%errorlevel%' NEQ '0' (
  echo FAILED. Please ping an engineer.
  set /p TEMP_PROMPT=
  exit /B 1
)
popd

echo.
echo Building AppPC "PC-%SEOUL_BUILD_CONFIG%"...
pushd %SEOUL_APP_ROOT%
%SEOUL_FASTBUILD% -quiet PC-%SEOUL_BUILD_CONFIG%
if '%errorlevel%' NEQ '0' (
  echo FAILED. Please ping an engineer.
  set /p TEMP_PROMPT=
  exit /B 1
)
popd
