@echo off
setlocal

set SEOUL_BUILD_CONFIG=%1
set SEOUL_APP_ROOT="%~dp0..\..\App"
set SEOUL_EDITOR_ROOT="%~dp0..\..\SeoulEditor"
set SEOUL_FASTBUILD="%~dp0..\..\External\fastbuild\Windows\FBuild.exe"
set SEOUL_TOOLS_ROOT=""%~dp0..\..\SeoulTools"
set SEOUL_ANDROID_BUILD="%~dp0..\..\App\Code\AppAndroid\GradleBuild\AndroidBuild.bat"
set SEOUL_SUBPLATFORM=

if 'Debug' NEQ '%1' (
  if 'Developer' NEQ '%1' (
    if 'Profiling' NEQ '%1' (
      if 'Ship' NEQ '%1' (
        echo Valid configurations are Debug, Developer, Profiling, and Ship.
        echo FAILED. Press Enter to Continue...
        set /p TEMP_PROMPT=
        exit /B 1
      )
    )
  )
)

if '%2' EQU '-amazon' set SEOUL_SUBPLATFORM=%2
if '%2' EQU '-samsung' set SEOUL_SUBPLATFORM=%2
if '%3' EQU '-amazon' set SEOUL_SUBPLATFORM=%3
if '%3' EQU '-samsung' set SEOUL_SUBPLATFORM=%3

REM Skip tools and cooking if requested.
if '%2' EQU '-no_cook' goto build
if '%3' EQU '-no_cook' goto build

echo Building SeoulTools "PC-DeveloperTools"...
pushd %SEOUL_TOOLS_ROOT%
%SEOUL_FASTBUILD% -quiet PC-DeveloperTools
if '%errorlevel%' NEQ '0' (
  echo FAILED. Please ping an engineer.
  set /p TEMP_PROMPT=
  exit /B 1
)
popd

set SEOUL_BASE_DIR=%~dp0..\..\
set SEOUL_PLATFORM=Android
set SEOUL_PACKAGE_FILE=%SEOUL_BASE_DIR%App\Data\AppPackages%SEOUL_PLATFORM%.cfg
set SEOUL_TOOL_COOKER=%SEOUL_BASE_DIR%SeoulTools\Binaries\PC\Developer\x64\Cooker.exe

echo.
%SEOUL_TOOL_COOKER% -platform %SEOUL_PLATFORM% -package_file %SEOUL_PACKAGE_FILE% -local
if '%errorlevel%' NEQ '0' EXIT /B 1

:build
echo.
echo Building AppAndroid "%SEOUL_BUILD_CONFIG% %SEOUL_SUBPLATFORM%"...
call %SEOUL_ANDROID_BUILD% %SEOUL_BUILD_CONFIG% %SEOUL_SUBPLATFORM%
if '%errorlevel%' NEQ '0' (
  echo FAILED. Please ping an engineer.
  set /p TEMP_PROMPT=
  exit /B 1
)
