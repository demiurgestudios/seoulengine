@echo off
setlocal

git clone git@github.com:jzupko/fastbuild.git
if '%errorlevel%' neq '0' goto fail
echo #define CI_BUILD > %~dp0fastbuild\Code\CI.bff
echo #include 'fbuild.bff' >> %~dp0fastbuild\Code\CI.bff

pushd %~dp0fastbuild\Code
if '%errorlevel%' neq '0' goto fail
%~dp0Windows\FBuild.exe -config %~dp0fastbuild\Code\CI.bff FBuild-x64-Release FBuildWorker-x64-Release
if '%errorlevel%' neq '0' goto fail
copy /y %~dp0fastbuild\tmp\x64-Release\Tools\FBuild\FBuild\FBuild.exe %~dp0Windows\FBuild.exe
if '%errorlevel%' neq '0' goto fail
copy /y %~dp0fastbuild\tmp\x64-Release\Tools\FBuild\FBuildWorker\FBuildWorker.exe %~dp0Windows\FBuildWorker.exe
if '%errorlevel%' neq '0' goto fail

:success
popd
rmdir /q /s %~dp0fastbuild > nul
echo [SUCCESS]
exit /b 0

:fail
popd
rmdir /q /s %~dp0fastbuild > nul
echo [FAILED]
exit /b 1
