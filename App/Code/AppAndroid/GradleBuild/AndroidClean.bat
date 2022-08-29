@echo off

REM Copyright (c) 2012-2022 Demiurge Studios Inc. All rights reserved.

REM Clean dependency
call :do_del_dir %~dp0..\..\..\..\External\Samsung\Libs\IAP5Helper\build

REM Clean dependency
call :do_clean %~dp0..\..\..\..\SeoulEngine\Code\AndroidJava\

REM Clean root
call :do_clean %~dp0

REM Cleanup intermediate dir
call :do_del_dir %~dp0..\..\..\Intermediate\Android
call :do_del_dir %~dp0..\..\..\Intermediate\Gradle

REM Cleanup some miscellaneous intermediate files
call :do_del_file %~dp0gradle\wrapper\gradle-5.5.1-all.zip

REM Done
echo.
echo Done cleaning Android build
exit /B 0

:do_clean
set cleaned_path=%~f1

call :del_file %cleaned_path%src\main\AndroidManifest.xml
call :del_dir %cleaned_path%.gradle
call :del_dir %cleaned_path%assets\Data
call :del_dir %cleaned_path%bin
call :del_dir %cleaned_path%build
call :del_dir %cleaned_path%ext_libs
call :del_dir %cleaned_path%gen
call :del_dir %cleaned_path%libs

for /d %%d in ("%cleaned_path%obj*") do (
  call :del_dir %%~d
)
for /d %%d in ("%cleaned_path%libs*") do (
  call :del_dir %%~d
)
exit /B 0

:del_file
if exist %~f1 (
  echo Removing %~f1
  del \\?\%~f1 /F /Q
)
exit /B 0

:del_dir
if exist %~f1 (
  echo Removing %~f1
  REM To handle long paths here, we robocopy an empty directory
  REM to the target in purge mode, which effectively copies an
  REM empty folder over the target, deleting all files from the
  REM target.
  mkdir tmp
  robocopy tmp %~f1 /purge > nul
  rmdir /S /Q tmp
  rmdir /S /Q \\?\%~f1
)
exit /B 0

:do_del_dir
set cleaned_path=%~f1
call :del_dir %cleaned_path%
exit /B 0

:do_del_file
set cleaned_path=%~f1
call :del_file %cleaned_path%
exit /B 0
