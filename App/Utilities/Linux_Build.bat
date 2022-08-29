@echo off
setlocal

set SEOUL_BUILD_CONFIG=%1
set SEOUL_APP_ROOT="%~dp0..\..\App"
set SEOUL_FASTBUILD="%~dp0..\..\External\fastbuild\Windows\FBuild.exe"

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

set SEOUL_BASE_DIR=%~dp0..\..\
set SEOUL_PLATFORM=Linux

echo Building AppLinux "Linux-%SEOUL_BUILD_CONFIG%"...
pushd %SEOUL_APP_ROOT%
%SEOUL_FASTBUILD% -quiet Linux-%SEOUL_BUILD_CONFIG%
if '%errorlevel%' NEQ '0' (
  echo FAILED. Please ping an engineer.
  set /p TEMP_PROMPT=
  exit /B 1
)
popd