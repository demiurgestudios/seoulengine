@echo off
setlocal

set SEOUL_APP_ROOT="%~dp0..\..\App"
set SEOUL_EDITOR_ROOT="%~dp0..\..\SeoulEditor"
set SEOUL_FASTBUILD="%~dp0..\..\External\fastbuild\Windows\FBuild.exe"
set SEOUL_TOOLS_ROOT=""%~dp0..\..\SeoulTools"

echo Generating App.sln...
pushd %SEOUL_APP_ROOT%
%SEOUL_FASTBUILD% -quiet GenSln
if '%errorlevel%' NEQ '0' (
  echo FAILED. Please ping an engineer.
  set /p TEMP_PROMPT=
  exit /B 1
)
popd

echo Generating SeoulEditor.sln...
pushd %SEOUL_EDITOR_ROOT%
%SEOUL_FASTBUILD% -quiet GenSln
if '%errorlevel%' NEQ '0' (
  echo FAILED. Please ping an engineer.
  set /p TEMP_PROMPT=
  exit /B 1
)
popd

echo Generating SeoulTools.sln...
pushd %SEOUL_TOOLS_ROOT%
%SEOUL_FASTBUILD% -quiet GenSln
if '%errorlevel%' NEQ '0' (
  echo FAILED. Please ping an engineer.
  set /p TEMP_PROMPT=
  exit /B 1
)
popd
