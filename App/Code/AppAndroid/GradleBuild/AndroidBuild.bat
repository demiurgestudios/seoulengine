@echo off
setlocal

REM Copyright (c) 2012-2022 Demiurge Studios Inc. All rights reserved.

REM Setup basic environment.
set OLD_DIR=%CD%
set CUR_DIR=%~dp0
call :set_full_path APP_DIR,%~dp0..\..\..\
call :set_full_path EXT_DIR,%~dp0..\..\..\..\External\
call :set_full_path DATA_DIR,%CUR_DIR%..\..\..\Data\
call :set_full_path ASSETS_DIR,%CUR_DIR%assets\
call :set_full_path BIN_DIR,%CUR_DIR%..\..\..\Binaries\Android\
call :set_full_path INT_DIR,%CUR_DIR%..\..\..\Intermediate\Gradle\
call :set_full_path SEOUL_JAVA_LIB,%CUR_DIR%..\..\..\..\SeoulEngine\Code\AndroidJava\
set SEOUL_INCLUDE_ARM64=
set SEOUL_REMOVE_CONTENTSAR=
set SEOUL_CONFIG=%1
set SEOUL_GENERATE_BUNDLE=0
set SEOUL_WITH_AMAZON=0
set SEOUL_WITH_SAMSUNG=0

echo APP_DIR=%APP_DIR%
echo EXT_DIR=%EXT_DIR%
echo DATA_DIR=%DATA_DIR%
echo ASSETS_DIR=%ASSETS_DIR%
echo BIN_DIR=%BIN_DIR%
echo INT_DIR=%INT_DIR%
echo SEOUL_JAVA_LIB=%SEOUL_JAVA_LIB%

if '%2' EQU '-include_arm64' set SEOUL_INCLUDE_ARM64="SEOUL_INCLUDE_ARM64=1"
if '%2' EQU '-remove_contentsar' set SEOUL_REMOVE_CONTENTSAR=-remove_contentsar
if '%2' EQU '-generate_bundle' set SEOUL_GENERATE_BUNDLE=1
if '%2' EQU '-amazon' set SEOUL_WITH_AMAZON=1
if '%2' EQU '-samsung' set SEOUL_WITH_SAMSUNG=1
if '%3' EQU '-include_arm64' set SEOUL_INCLUDE_ARM64="SEOUL_INCLUDE_ARM64=1"
if '%3' EQU '-remove_contentsar' set SEOUL_REMOVE_CONTENTSAR=-remove_contentsar
if '%3' EQU '-generate_bundle' set SEOUL_GENERATE_BUNDLE=1
if '%3' EQU '-amazon' set SEOUL_WITH_AMAZON=1
if '%3' EQU '-samsung' set SEOUL_WITH_SAMSUNG=1
if '%4' EQU '-include_arm64' set SEOUL_INCLUDE_ARM64="SEOUL_INCLUDE_ARM64=1"
if '%4' EQU '-remove_contentsar' set SEOUL_REMOVE_CONTENTSAR=-remove_contentsar
if '%4' EQU '-generate_bundle' set SEOUL_GENERATE_BUNDLE=1
if '%4' EQU '-amazon' set SEOUL_WITH_AMAZON=1
if '%4' EQU '-samsung' set SEOUL_WITH_SAMSUNG=1

if '%SEOUL_GENERATE_BUNDLE%' EQU '1' (
    set SEOUL_BUILD_TYPE=bundle
) else (
    set SEOUL_BUILD_TYPE=assemble
)

chdir /D %CUR_DIR%

call %CUR_DIR%AndroidEnvironment.bat
if '%errorlevel%' NEQ '0' goto fail

REM Ran into this when an engineer moved from a project
REM using Android Studio - it decided to generate
REM a local.properties file that apparently overrides
REM the command-line sdk.dir value.
if exist %SEOUL_JAVA_LIB%local.properties del %SEOUL_JAVA_LIB%local.properties /f /q
if exist %CUR_DIR%local.properties del %CUR_DIR%local.properties /f /q

if not exist %CUR_DIR%ext_libs (
    mkdir %CUR_DIR%ext_libs
)

if not exist %CUR_DIR%assets (
    mkdir %CUR_DIR%assets
)

if not exist %CUR_DIR%assets\Data (
    mkdir %CUR_DIR%assets\Data
)

if not exist %DATA_DIR%Android_ClientSettings.sar (
    REM chastise.py
    echo Missing Android_ClientSettings.sar, you should be running Utilities\Android_BuildAndCook.bat instead.
    goto fail
)

xcopy %DATA_DIR%Android_ClientSettings.sar %ASSETS_DIR%Data\ /y /d /i
if '%errorlevel%' NEQ '0' goto fail

xcopy %DATA_DIR%Android_Config.sar %ASSETS_DIR%Data\ /y /d /i
if '%errorlevel%' NEQ '0' goto fail

xcopy %DATA_DIR%Android_BaseContent.sar %ASSETS_DIR%Data\ /y /d /i
if '%errorlevel%' NEQ '0' goto fail

if '%SEOUL_REMOVE_CONTENTSAR%' EQU '-remove_contentsar' (
    if exist %ASSETS_DIR%Data\Android_Content.sar del %ASSETS_DIR%Data\Android_Content.sar /f /q
)
if '%SEOUL_REMOVE_CONTENTSAR%' NEQ '-remove_contentsar' (
    xcopy %DATA_DIR%Android_Content.sar %ASSETS_DIR%Data\ /y /d /i
    if '%errorlevel%' NEQ '0' goto fail
)

if '%SEOUL_WITH_AMAZON%' EQU '1' (
    REM Add marker file to tag this as an Amazon (vs. e.g. GooglePlay) build.
    copy NUL %ASSETS_DIR%Data\a03e8f09bd0bf96e2255f82d794afd90b526c1781f74b41b03dcc7ebab199db1
    REM Delete the marker file to tag this as a GooglePlay build.
    del %ASSETS_DIR%Data\a219f49e3c2cd2d29c21da4a93c4b587724ea7d209a51d946019de699841d141
) else (
    REM Delete the marker file to tag this as an Amazon (vs. e.g. GooglePlay) build.
    del %ASSETS_DIR%Data\a03e8f09bd0bf96e2255f82d794afd90b526c1781f74b41b03dcc7ebab199db1
)

if '%SEOUL_WITH_SAMSUNG%' EQU '1' (
    REM Add marker file to tag this as an Samsung (vs. e.g. GooglePlay) build.
    copy NUL %ASSETS_DIR%Data\9e4955f29d704dcf891e9464180557569ab4acb067524e9387531ef534681a96
    REM Delete the marker file to tag this as a GooglePlay build.
    del %ASSETS_DIR%Data\a219f49e3c2cd2d29c21da4a93c4b587724ea7d209a51d946019de699841d141
) else (
    REM Delete the marker file to tag this as an Samsung (vs. e.g. GooglePlay) build.
    del %ASSETS_DIR%Data\9e4955f29d704dcf891e9464180557569ab4acb067524e9387531ef534681a96
)

REM Debug and Developer include ScriptsDebug.
if '%SEOUL_CONFIG%' EQU 'Debug' (
    xcopy %DATA_DIR%Android_ScriptsDebug.sar %ASSETS_DIR%Data\ /y /d /i
    if '%errorlevel%' NEQ '0' goto fail
)
if '%SEOUL_CONFIG%' EQU 'Developer' (
    xcopy %DATA_DIR%Android_ScriptsDebug.sar %ASSETS_DIR%Data\ /y /d /i
    if '%errorlevel%' NEQ '0' goto fail
)
REM Profiling and Ship exclude it for space - must make sure it's removed.
if '%SEOUL_CONFIG%' EQU 'Profiling' (
    if exist %ASSETS_DIR%Data\Android_ScriptsDebug.sar del %ASSETS_DIR%Data\Android_ScriptsDebug.sar /f /q
)
if '%SEOUL_CONFIG%' EQU 'Ship' (
    if exist %ASSETS_DIR%Data\Android_ScriptsDebug.sar del %ASSETS_DIR%Data\Android_ScriptsDebug.sar /f /q
)

REM Defaults for the build
set BUILD_CONFIG_TYPE=Ship
set GRADLE_BUILD_CONFIG_TYPE=%SEOUL_BUILD_TYPE%Release
set APK_INPUT_PATH=%CUR_DIR%build\outputs\apk\release\AppAndroid-release.apk
set BUNDLE_INPUT_PATH=%CUR_DIR%build\outputs\bundle\release\AppAndroid.aab

REM Debug build
if '%SEOUL_CONFIG%' EQU 'Debug' (
    set BUILD_CONFIG_TYPE=Debug
    set GRADLE_BUILD_CONFIG_TYPE=%SEOUL_BUILD_TYPE%Debug
    set APK_INPUT_PATH=%CUR_DIR%build\outputs\apk\debug\AppAndroid-debug.apk
    set BUNDLE_INPUT_PATH=%CUR_DIR%build\outputs\bundle\debug\AppAndroid.aab
    set GRADLE_USER_HOME=%INT_DIR%Debug
)

REM Developer build
if '%SEOUL_CONFIG%' EQU 'Developer' (
    set BUILD_CONFIG_TYPE=Developer
    set GRADLE_BUILD_CONFIG_TYPE=%SEOUL_BUILD_TYPE%Debug
    set APK_INPUT_PATH=%CUR_DIR%build\outputs\apk\debug\AppAndroid-debug.apk
    set BUNDLE_INPUT_PATH=%CUR_DIR%build\outputs\bundle\debug\AppAndroid.aab
    set GRADLE_USER_HOME=%INT_DIR%Developer
)

REM Profiling build
if '%SEOUL_CONFIG%' EQU 'Profiling' (
    set BUILD_CONFIG_TYPE=Profiling
    set GRADLE_BUILD_CONFIG_TYPE=%SEOUL_BUILD_TYPE%Debug
    set APK_INPUT_PATH=%CUR_DIR%build\outputs\apk\debug\AppAndroid-debug.apk
    set BUNDLE_INPUT_PATH=%CUR_DIR%build\outputs\bundle\debug\AppAndroid.aab
    set GRADLE_USER_HOME=%INT_DIR%Profiling
)

REM Ship build
if '%SEOUL_CONFIG%' EQU 'Ship' (
    set BUILD_CONFIG_TYPE=Ship
    set GRADLE_BUILD_CONFIG_TYPE=%SEOUL_BUILD_TYPE%Release
    set APK_INPUT_PATH=%CUR_DIR%build\outputs\apk\release\AppAndroid-release.apk
    set BUNDLE_INPUT_PATH=%CUR_DIR%build\outputs\bundle\release\AppAndroid.aab
    set GRADLE_USER_HOME=%INT_DIR%Ship
)

REM If not including arm64, cleanup any possible leftovers so
REM we don't accidentally package in a stale arm64 .so.
if '%SEOUL_INCLUDE_ARM64%' EQU '' call :del_dir %CUR_DIR%\libs-%BUILD_CONFIG_TYPE%\arm64-v8a

REM Run the build
echo BUILD_CONFIG_TYPE=%BUILD_CONFIG_TYPE%
echo GRADLE_BUILD_CONFIG_TYPE=%GRADLE_BUILD_CONFIG_TYPE%
echo APK_INPUT_PATH=%APK_INPUT_PATH%
echo BUNDLE_INPUT_PATH=%BUNDLE_INPUT_PATH%

REM Prepare for output copy
if not exist %BIN_DIR%%BUILD_CONFIG_TYPE% (
    mkdir %BIN_DIR%%BUILD_CONFIG_TYPE%
)

REM Perform the necessary build.
set FBUILD_OPTIONS=-quiet Android-%BUILD_CONFIG_TYPE%-armeabi-v7a
if '%SEOUL_INCLUDE_ARM64%' NEQ '' (
    set FBUILD_OPTIONS=%FBUILD_OPTIONS% Android-%BUILD_CONFIG_TYPE%-arm64-v8a
)
pushd %APP_DIR%
%EXT_DIR%fastbuild\Windows\FBuild.exe %FBUILD_OPTIONS%
if '%errorlevel%' NEQ '0' goto fail
popd

REM Copy the gradle distro file locally so
REM the wrapper can find it.
if exist %~dp0gradle\wrapper\gradle-5.5.1-all.zip del %~dp0gradle\wrapper\gradle-5.5.1-all.zip /f /q
copy %SEOUL_ANDROID_SDK%\gradle-5.5.1\gradle-5.5.1-all.zip %~dp0gradle\wrapper\gradle-5.5.1-all.zip /y
if '%errorlevel%' NEQ '0' goto fail

REM Gradle - Lint checking, Java code, and packaging.
"%JAVA_HOME%\bin\java.exe" -Dorg.gradle.parallel=true -Dorg.gradle.daemon=false "-Dorg.gradle.appname=AppAndroid" -Dsdk.dir=%ANDROID_HOME% -classpath "%~dp0gradle\wrapper\gradle-wrapper.jar" org.gradle.wrapper.GradleWrapperMain check %GRADLE_BUILD_CONFIG_TYPE% --warning-mode all --offline -PappJniLibsDir=libs-%BUILD_CONFIG_TYPE%
if '%errorlevel%' NEQ '0' goto fail

if '%SEOUL_GENERATE_BUNDLE%' EQU '1' (
    REM Copy AppBundle output
    set SEOUL_COPY_FROM=%BUNDLE_INPUT_PATH%
    set SEOUL_COPY_TO=%BIN_DIR%%BUILD_CONFIG_TYPE%\SETestApp.aab
) else (
    REM Copy APK output
    set SEOUL_COPY_FROM=%APK_INPUT_PATH%
    set SEOUL_COPY_TO=%BIN_DIR%%BUILD_CONFIG_TYPE%\SETestApp.apk
)

REM Perform the copy
copy "%SEOUL_COPY_FROM%" "%SEOUL_COPY_TO%" /y
if '%errorlevel%' NEQ '0' goto fail

REM Done
chdir /D %OLD_DIR%
exit /B 0

:fail
chdir /D %OLD_DIR%
exit /B 1

:del_file
if exist %~f1 (
  echo Removing %~f1
  del \\?\%~f1 /F /Q
)
exit /B 0

:del_dir
if exist %~f1 (
  echo Removing %~f1
  rmdir /S /Q \\?\%~f1
)
exit /B 0

:set_full_path
set %~1=%~f2
exit /B 0
