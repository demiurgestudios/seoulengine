@echo off

REM Copyright (c) 2012-2022 Demiurge Studios Inc. All rights reserved.

set INT_DIR=%CUR_DIR%..\..\..\Intermediate\Gradle\

REM Cleanup existing
if exist %~dp0local-maven-repo call :del_dir %~dp0local-maven-repo

REM Setup environment
call %~dp0AndroidEnvironment.bat
if '%errorlevel%' NEQ '0' goto fail

REM TODO: Figure out how to use a different build output folder per config
REM       so we don't need to nuke it each time.
set SEOUL_JAVA_LIB=%~dp0..\..\..\..\SeoulEngine\Code\AndroidJava\
if exist %SEOUL_JAVA_LIB%.gradle call :del_dir %SEOUL_JAVA_LIB%.gradle
if exist %SEOUL_JAVA_LIB%build call :del_dir %SEOUL_JAVA_LIB%build
if exist %~dp0.gradle call :del_dir %~dp0.gradle
if exist %~dp0build call :del_dir %~dp0build

REM Copy the gradle distro file locally so
REM the wrapper can find it.
if exist %~dp0gradle\wrapper\gradle-5.5.1-all.zip del %~dp0gradle\wrapper\gradle-5.5.1-all.zip /f /q
copy %SEOUL_ANDROID_SDK%\gradle-5.5.1\gradle-5.5.1-all.zip %~dp0gradle\wrapper\gradle-5.5.1-all.zip /y
if '%errorlevel%' NEQ '0' goto fail

REM Gradle build
call :set_gradle_user_home %INT_DIR%Debug
"%JAVA_HOME%\bin\java.exe" -Dorg.gradle.daemon=false "-Dorg.gradle.appname=AppAndroid" -Dsdk.dir=%ANDROID_HOME% -classpath "%~dp0gradle\wrapper\gradle-wrapper.jar" org.gradle.wrapper.GradleWrapperMain assembleDebug -PappJniLibsDir=libs-debug
if '%errorlevel%' NEQ '0' goto fail

REM Package.
python %~dp0create_local_maven_repo.py
if '%errorlevel%' NEQ '0' goto fail

:success
echo [SUCCESS]
exit /B 0

:fail
echo [FAIL]
exit /B 1

:del_dir
if exist %~f1 (
  echo Removing %~f1
  rmdir /S /Q \\?\%~f1
)
exit /B 0

:set_gradle_user_home
set GRADLE_USER_HOME=%~f1
exit /B 0
