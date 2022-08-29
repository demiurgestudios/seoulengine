@echo off
setlocal

set SEOUL_BASE_DIR=%~dp0..\..\
set SEOUL_TOOL_ASSET_TOOL=%SEOUL_BASE_DIR%SeoulTools\Binaries\PC\Developer\x64\AssetTool.exe

echo Running AssetTool
%SEOUL_TOOL_ASSET_TOOL% Android
