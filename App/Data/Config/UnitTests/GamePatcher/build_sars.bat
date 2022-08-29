@echo off

echo Constructing intermediate filenames...

set SEOUL_TMP_DIR=%TEMP%\patcher_test_temp
set SEOUL_BASE=%SEOUL_TMP_DIR%\Base\
set SEOUL_PATCH_A=%SEOUL_TMP_DIR%\PatchA\
set SEOUL_PATCH_B=%SEOUL_TMP_DIR%\PatchB\
set SEOUL_TOOL_COOKER=%~dp0..\..\..\..\..\SeoulTools\Binaries\PC\Developer\x64\Cooker.exe

REM Copy files to tmp - need to do this, as the Cooker will get confused
REM if we have a SeoulEngine directory structure inside another SeoulEngine
REM directory structure
call :copy_dir %~dp0. %SEOUL_TMP_DIR%

REM cook.
call :cook Android
if '%errorlevel%' NEQ '0' goto fail

call :cook IOS
if '%errorlevel%' NEQ '0' goto fail

call :cook PC
if '%errorlevel%' NEQ '0' goto fail

REM finalize.
echo Moving results...
move /Y %SEOUL_TMP_DIR%\*.sar %~dp0 > nul
if '%errorlevel%' NEQ '0' goto fail

:success
call :cleanup_dir %SEOUL_TMP_DIR%
exit /B 0

:fail
call :cleanup_dir %SEOUL_TMP_DIR%
echo FAILED. Press Enter to Continue...
set /p TEMP_PROMPT=
exit /B 1

:cook
echo Cooking base...
%SEOUL_TOOL_COOKER% -base_dir %SEOUL_BASE% -platform %1 -package_file %SEOUL_TMP_DIR%\%1_Base.cfg -srccl 1 -force_gen_cdict
if '%errorlevel%' NEQ '0' goto fail

echo Cooking patch A...
%SEOUL_TOOL_COOKER% -base_dir %SEOUL_PATCH_A% -platform %1 -package_file %SEOUL_TMP_DIR%\%1_PatchA.cfg -srccl 2 -force_gen_cdict
if '%errorlevel%' NEQ '0' goto fail

echo Cooking patch B...
%SEOUL_TOOL_COOKER% -base_dir %SEOUL_PATCH_B% -platform %1 -package_file %SEOUL_TMP_DIR%\%1_PatchB.cfg -srccl 3 -force_gen_cdict
if '%errorlevel%' NEQ '0' goto fail
exit /B 0

:copy_dir
mkdir %2 > nul
xcopy /E /R /Y %1 %2 > nul
if '%errorlevel%' NEQ '0' exit /B 1
exit /B 0

:cleanup_dir
rmdir /S /Q %1 > nul
exit /B 0
