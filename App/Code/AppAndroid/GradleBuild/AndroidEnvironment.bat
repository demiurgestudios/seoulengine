@echo off

REM Copyright (c) 2012-2022 Demiurge Studios Inc. All rights reserved.

set ANDROID_ROOT=%SEOUL_ANDROID_SDK%
set ANDROID_HOME=%ANDROID_ROOT%\sdk-r26.1.1
set JAVA_HOME=%ANDROID_ROOT%\jdk-8u131

REM Make sure that repeated calls of AndroidEnvironment.bat in the same
REM shell do not eventually overflow the maximum %PATH% variable
REM length.
set ANDROID_PATHS=%JAVA_HOME%\bin\;%ANDROID_HOME%\platform-tools\;%ANDROID_HOME%\tools\
call set PATH=%%PATH:%ANDROID_PATHS%;=%%

REM Append the paths to %PATH%
set PATH=%ANDROID_PATHS%;%PATH%

exit /B 0
