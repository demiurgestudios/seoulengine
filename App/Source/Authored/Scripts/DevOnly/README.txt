This folder contains Lua script files to be used for in-house development
purposes only, including debugging, linting, and testing.

DO NOT place files in this folder that will be used as shipping dependencies
of script files outside this folder. This entire folder is excluded from
shipping .sar archives and none of these scripts will be included in
shipping builds.

NOTES:

AutomatedTests\
- automated test scripts - passed to the game via -run_automated_test=content://Authored/Scripts/DevOnly/<test_name>.lua
- automated tests are complex tests that run the game in a headless mode, to stress or validate complex functionality.

External\
- lunatest\ - lunatest lua unit test framework, from https://github.com/silentbicycle/lunatest/releases at v0.9.5
- luainspect\ - lua lint and AST inspection, copied from External/ZeroBraneStudio/luainspect at v0.9.0
- mobdebug\ - debugging interface, copied from External/ZeroBraneStudio/lualibs at v0.9.0
- metalua\ - lua AST functionality, copied from External/ZeroBraneStudio/metalua at v0.9.0

UnitTests\
- bucket of unit test files that will be processed by the RunUnitTests.lua script.
