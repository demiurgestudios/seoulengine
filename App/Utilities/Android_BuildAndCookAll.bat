@echo off
setlocal

REM Prime first - let FASTBuild optimize building all these configurations,
REM and they will be done when the gradle build script invokes them one-by-one.
pushd %~dp0..\
%~dp0..\..\External\fastbuild\Windows\FBuild.exe -quiet Android-armeabi-v7a-All
if '%errorlevel%' neq '0' exit /b 1
popd

REM Now gradle build each config.
call %~dp0Android_BuildAndCook Debug %1
if '%errorlevel%' neq '0' exit /b 1

call %~dp0Android_BuildAndCook Developer -no_cook %1
if '%errorlevel%' neq '0' exit /b 1

call %~dp0Android_BuildAndCook Profiling -no_cook %1
if '%errorlevel%' neq '0' exit /b 1

call %~dp0Android_BuildAndCook Ship -no_cook %1
if '%errorlevel%' neq '0' exit /b 1
