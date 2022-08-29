@echo off
setlocal

REM Prime first - let FASTBuild optimize building all these configurations,
REM and they will be done when the gradle build script invokes them one-by-one.
pushd %~dp0..\
%~dp0..\..\External\fastbuild\Windows\FBuild.exe -quiet Android-armeabi-v7a-All PC-All
if '%errorlevel%' neq '0' exit /b 1
popd

REM Now complete the builds in full.
call %~dp0PC_BuildAndCookAll.bat
if '%errorlevel%' neq '0' exit /b 1
call %~dp0Android_BuildAndCookAll.bat
if '%errorlevel%' neq '0' exit /b 1