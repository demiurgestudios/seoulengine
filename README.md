# Seoul Engine

## Overview
This repository contains the [MIT License](https://github.com/demiurgestudios/seoulengine/blob/main/LICENSE) Open Source release of the Seoul Engine source code.

Seoul Engine is the name used internally by [Demiurge Studios](https://demiurgestudios.com) for multiple incarnations of an in-house game engine, used to develop and ship several titles.

### What is it not?

Seoul Engine in its Open Source release is _not_ a shippable or even complete implementation of a game engine. Little documentation and no examples are provided and multiple systems are not functional, either due to their prerelease status or due to source code elided to account for incompatible licensing (see also [here](https://github.com/demiurgestudios/seoulengine/blob/main/External/FMOD/README_FMOD.txt), [here](https://github.com/demiurgestudios/seoulengine/blob/main/External/FMODStudio/README_FMOD_STUDIO.txt), [here](https://github.com/demiurgestudios/seoulengine/blob/main/External/FxStudio/README_FX_STUDIO.txt), and [here](https://github.com/demiurgestudios/seoulengine/blob/main/SeoulEngine/Code/Animation2D/README_ANIMATION_2D.cpp)).

This repository includes build scripts for x86-64 Windows using [Visual Studio 2019](https://visualstudio.microsoft.com/vs/older-downloads/). This repository contains essential support for ARMv7-A and ARMv8-A targeting Android and iOS as well, but not all relevant build scripts or application code have been included for those platforms and targets.

In short, if you want to build a game, see one of the many commercial or [Open Source](https://github.com/godotengine/godot) options available instead.

### What is it?

The Seoul Engine repository contains a number of components used to solve problems and ship games. It is our hope that these may be useful, directly, or as reference for other work.

## Components

Summary of various Seoul Engine components, organized (roughly) by directory structure.

For convenience, incomplete components will be tagged with :construction: . You can interpret a :construction: tag to mean that a component's source code:

* Was never shipped.
* Has no or few unit tests.
* May have crippling bugs or entirely missing features.
* Has first pass design and architecture that may not fully translate to a shippable or production feature (the source code may require significant rearchitecting or refactoring before it can be used in production).

### [App/](https://github.com/demiurgestudios/seoulengine/tree/main/App)

Game specific code and all game data goes in `App/`.

#### `App/Code/`

Game specific source code. Defines the main entry point of a game executable.

#### `App/Data/Config/`

All game configuration data, in `.json` format.

#### `App/Data/Content*/`

In Seoul Engine, content not in `App/Data/Config/` is "cooked" into platform specific runtime formats. The intermediate state of those files will be placed within `App/Data/Content*/` folders (e.g. `App/Data/ContentPC/`).

`Developer` builds of the game can read these intermediate files directly for faster iteration. `Ship` builds only read cooked data from packaged archives, which in Seoul Engine are `.sar` files (Seoul Engine Archive).

#### `App/Data/AppPackages*.cfg`

`.json` format files that define how cooked intermediate files are bundled and packaged into `.sar` files. The `.cfg` extension is a legacy quirk, these files are otherwise typical Seoul Engine `.json` files.

#### `App/Source/`

All source game content (e.g. `.png` or script `.cs` files) must be placed in this folder. Seoul Engine will cook source files into optimized runtime formats in a mirror directory structure under the `App/Data/Content*/` folders (e.g. `App/Data/ContentPC/` when cooking for x86-x64 Windows).

Human authored source is placed in `App/Source/Authored/` and various generated source files will be placed in `App/Source/Generated*/` by the Seoul Engine cooking pipeline (e.g. `.cs` script files are converted into `.lua` files which will be placed in `App/Source/Generated*/`.

#### `App/Utilities/`
Miscellaneous `.bat` scripts to e.g. build an x86-x64 Windows build (e.g. `PC_BuildAndCook.bat`) or generate `.sln` files for working in Visual Studio (e.g. `GenerateSln.bat`).

### [External/](https://github.com/demiurgestudios/seoulengine/tree/main/External)

Third party dependencies.

### :construction: [SeoulEditor/](https://github.com/demiurgestudios/seoulengine/tree/main/SeoulEditor)

Seoul Engine level editor (meant for creating assets for the `Scene` project).

### [SeoulEngine/](https://github.com/demiurgestudios/seoulengine/tree/main/SeoulEngine)

Game agnostic engine code and data, shared by runtime game executables and developer tools.

#### `SeoulEngine/Code/Animation/` and `SeoulEngine/Code/Animation2D/`

Skinned animation support. Common code is in `SeoulEngine/Code/Animation/`. 2D specific code is in `SeoulEngine/Code/Animation2D/`.

#### :construction: `SeoulEngine/Code/Animation3D/`

3D specific skinned animation support.

#### `SeoulEngine/Code/Content/`

Base classes and functionality for asynchronous, threaded runtime asset loading. In `Developer` builds, this package also provides on the fly "cooking", to prepare source assets for use by the runtime.

#### `SeoulEngine/Code/Core/`

Low-level utilities. Compared to (e.g.) `SeoulEngine/Code/Engine/`, most components in `SeoulEngine/Code/Core/` have limited or no context requirements (they can be constructed and used freely without other engine systems being initialized). Likewise, most singletons exist in projects other than `SeoulEngine/Code/Core/`.

`SeoulEngine/Code/Core/` includes (not an exhaustive list):
* `Atomic*` - atomic (thread safe) utilities, counters and simple type wrappers.
* `Compress*` - [LZ4](https://github.com/lz4/lz4), [zlib](https://zlib.net), and [zstd](https://github.com/facebook/zstd) compression.
* `Coroutine*` - implementation of context switching to support cooperative multitasking. Essentially, a platform independent implementation of Win32 [Fibers](https://docs.microsoft.com/en-us/windows/win32/procthread/fibers).
* `DataStore*` - binary container of hierarchical and flexible typed data (essentially, a binary container of JSON data).
* `Delegate*` - limited function and function-like binding without heap allocation.
* `Directory*` - disk system directory listing and other queries.
* `DiskFilesystem*` - disk system file read and write and other queries.
* `EncryptAES*` - implementation of AES encryption (CFB algorithm).
* `EncryptXXTEA*` - implementation of [XXTEA](https://en.wikipedia.org/wiki/XXTEA) encryption.
* `FileChangeNotifier*` - monitoring and response to disk system file and directory changes.
* `FileManager*` - singleton. Manages subclasses of `IFileSystem`,  which implement Seoul Engine's virtualized file system.
* `FilePath*` - file paths as cheap keys (a `FilePath` is internally a `uint32_t`). `FilePath` is case insensitive. Further, filenames that refer to the same file or directory produce the same `FilePath` (path parts are normalized). Finally, filenames to `App/Source/`  resolve to the same`FilePath` as their equivalent cooked path in `App/Data/Content*/`.
* `FixedArray*` - Seoul Engine equivalent to `std::array<>`.
* `FromString*` and `ToString*` - conversion of `Seoul::String` from and to other types (e g.`uint32_t`).
* `HashSet*` - hashed set implementation using linear probing.
* `HashTable*` - hashed key-value table implementation using linear probing.
* `Image*` - native image file (e.g. `.png`) reading and writing.
* `Logger*` - persisted (on disk) and console logging functionality, with category channels and buffering.
* `MemoryManager*` - low-level memory allocation, memory leak detection and other profiling and memory diagnostics.
* `Mutex*` - platform independent equivalent to Win32 [Critical Section Objects](https://docs.microsoft.com/en-us/windows/win32/sync/critical-section-objects).
* `NTPClient*` - client for communicating with an NTP (time) server.
* `PackageFileSystem*` - `IFileSystem` subclass that binds Seoul Engine Archive (`.sar`) files into the Seoul Engine virtualized file system.
* `Path*` - file system path manipulation, combination, and normalization.
* `PseudoRandom*` - an implementation of a 128-bit pseudo random number generator.
* `Scoped*` - collection of utilities that implement an RAII pattern for various use cases.
* `SeoulETC1*` - software decompression of [ETC1](https://registry.khronos.org/DataFormat/specs/1.1/dataformat.1.1.pdf#page=90) textures.
* `SeoulMD5*` - MD5 hash of arbitrary binary data.
* `SeoulProcess*` - API that allows executing external processes.
* `SeoulSignal*` - platform independent (rough) equivalent of [`pthread_cond`](https://man.netbsd.org/pthread_cond.3).
* `SeoulSocket*` - platform independent encapsulation of a low-level network socket.
* `SeoulString*` - Seoul Engine variation of `std::string`, with UTF8 Unicode support. 
* `SeoulUUID*` - implementation of [v4 UUID](https://www.ietf.org/rfc/rfc4122.txt) generation.
* `SharedPtr*` - Seoul Engine equivalent to [`boost::intrusive_ptr`](https://www.boost.org/doc/libs/1_46_0/libs/smart_ptr/intrusive_ptr.html).
* `Thread*` - platform independent wrapper of OS threads.
* `Vector*` - Seoul Engine equivalent to `std::vector<>`.
* `WordFilter*` - natural language word filtering using phonetic parsing.
* `ZipFile*` - read and write of `.zip` archives. 

#### `SeoulEngine/Code/DevUI/`

Developer facing debug, diagnostic, and editing UI. Used by `SeoulEditor/Code/EditorUI/` and `GameDevUI*` classes in the `SeoulEngine/Code/Game/` project.

#### `SeoulEngine/Code/Engine/`

This project binds together multiple other projects. It primarily exists to implement the `Engine` singleton, which owns the lifespan of other singleton systems (e.g. `InputManager`) and provides a convenient location for miscellaneous platform specific functionality.

Platform specific specializations of `Engine` exist in `SeoulEngine/Code/Android/` and `SeoulEngine/Code/AndroidJava/` (Android platform), `SeoulEngine/Code/IOS/` (iOS platform), `SeoulEngine/Code/PC/` (Windows platform) and `SeoulEngine/Code/Steam/` (Windows platform with Steam integration).

`SeoulEngine/Code/NullPlatform/` also implements a platform independent version of the `Engine` project. It is used (e.g.) in unit tests and automated testing.

#### `SeoulEngine/Code/Falcon/`

Optimized 2D widget rendering and scene graph meant for in-game UI. Dependency of the `UI` project.

See also [Falcon Overview and Goals](https://github.com/demiurgestudios/seoulengine/wiki/Falcon%20Overview%20and%20Goals).

#### `SeoulEngine/Code/Game/`

The `Game` project ties together various engine systems specifically for use in a game (or "game like" process) in the `Game::Main` singleton. For example, `Game::Main` instantiates `Script::Manager` to enable script support and instantiates `UI::Manager` to enable in-game UI support.

#### `SeoulEngine/Code/HTTP/`

HTTP client requests and development server. `HTTP::Manager` (the client request singleton) also supports `HTTP/2`. Factored out dependency of the `Engine` project.

#### `SeoulEngine/Code/HAL/`

Hardware abstraction layer for graphics API. Specialized implementations for backends include `SeoulEngine/Code/D3D11/`, `SeoulEngine/Code/OGLES2/`, and `SeoulEngine/Code/NullGraphics/`. The latter is a "null" device used in some tools and for headless running for (e.g.) automated and unit tests.

#### `SeoulEngine/Code/Jobs/`

[Job system](https://ubm-twvideo01.s3.amazonaws.com/o1/vault/gdc2015/presentations/Gyrling_Christian_Parallelizing_The_Naughty.pdf). Factored out dependency of the `Engine` project.

#### :construction: `SeoulEngine/Code/Navigation/`

2D grid pathfinding using [Jump Point Search](https://harablog.wordpress.com/2011/09/07/jump-point-search/).

#### `SeoulEngine/Code/Network/`

High-level messaging support for synchronous client/server communication over a TCP socket. Uses `Seoul::Socket` for low-level sockets.

#### :construction: `SeoulEngine/Code/Physics/`

3D rigid body physics.

#### `SeoulEngine/Code/Reflection/`

Runtime type and data reflection. Heavily used in Seoul Engine for binding functions and data into script (in the `Script` project), serialization, and miscellaneous tasks enabled by runtime reflection (e.g. finding and executing unit tests).

#### `SeoulEngine/Code/Rendering/`

High-level rendering support (in contrast to low-level graphics API implemented in the `HAL` project). Implements concepts such as 3D mesh, materials, and post-processes.

#### `SeoulEngine/Code/Scc/`

Source control API. Currently implements an API around the Perforce [p4](https://www.perforce.com/products/helix-core-apps/command-line-client) command-line.

#### :construction: `SeoulEngine/Code/Scene/`

3D scene representation, components, and rendering support.

#### `SeoulEngine/Code/Script/`

Implements a script virtual machine and support functionality around a [LuaJIT](https://luajit.org) virtual machine, including a debugger client integration. At runtime, scripts are Lua scripts, but script authoring is done in C#. Translation from C# to Lua is done via the `SeoulTools/Code/SlimCSLib/` project.

Code in the various `SeoulEngine/Code/Script*/` projects (e.g. `SeoulEngine/Code/ScriptEngine`) are utility classes that bind the latter name into script. e.g. `ScriptEngineProcess` is a utility class that exposes `Seoul::Process` into script.

#### `SeoulEngine/Code/Settings/`

Asynchronous loading, caching, and management of `.json` files into `Seoul::DataStore` objects. `Seoul::DataStore` is a binary but inconvenient storage of `.json` data. Typically, a `Seoul::DataStore` is either deserialized into concrete C++ objects or into script objects via the `Reflection` project for runtime use. Factored out dependency of the `Engine` project.

#### `SeoulEngine/Code/Sound/`

Implements an API for async loading, creating, and playing sound events. Implemented concretely in the `FMODSound` project. Factored out dependency of the `Engine` project.

#### `SeoulEngine/Code/UI/`

Implements high-level in-game UI support, including `UI::Movie` instances via state machines. A `UI::Movie` can be considered a toplevel root UI widget. `UI::Movie` instances are layered and transitioned via state machines configured via `.json` files in `App/Data/Config/UI/`.

The `UI` project is tightly coupled to the `Falcon` project. `Falcon` implements the details of layout and rendering within a `UI::Movie` and the `UI` project implements high-level input capture and UI state management.

#### `SeoulEngine/Code/UnitTests/`

Unit tests for all other Seoul Engine projects. Unit tests can be run by passing `-run_unit_tests` to a `Developer` build of the `App` executable.

#### `SeoulEngine/Code/Video/`
Basic support for capturing uncompressed audio and video to an `.avi` file. For developer diagnostic and debugging functionality.

### [SeoulTools/](https://github.com/demiurgestudios/seoulengine/tree/main/SeoulTools)

Implements the Seoul Engine pipeline as well as a number of miscellaneous developer utilities.

#### `SeoulTools/Code/ConsoleTool/`

Small command-line utility used to execute a Windows GUI executable as a command-line utility.

#### `SeoulTools/Code/Cooker/`

Command-line executable that handles conversion of source assets into cooked runtime formats. Includes conversion of (e.g.) `.png` to `.sif*` files and `.swf` to `.fcn` files.

The `Cooker` project depends mainly on the `Cooking` project. It also uses the `EffectCompiler` project to convert shaders into various rendering backend (e.g. `GLSL` for the `OGLES2` backend) and it uses `FalconCooker` as an external process to convert `.swf` files to `.fcn` files.

#### `SeoulTools/Code/JsonFormatter/`

Command-line utility that can be used to format `.json` files. Includes understanding and support for Seoul Engine `.json` extensions.

#### `SeoulTools/Code/JsonMerge/`

Command-line utility that can be used to diff and merge `.json` files (in a manner that understands the structure of a JSON object).

#### `SeoulTools/Code/Moriarty/`

Moriarty is a GUI utility. It allows for remote cook on the fly for `Developer` builds running on (e.g.) mobile phones.

#### `SeoulTools/Code/SarTool/`

A command-line utility that can be used to inspect and manipulate Seoul Engine Archive (`.sar`) files.

#### `SeoulTools/Code/SlimCS/`

SlimCS is a command-line compiler that converts C# `.cs` code into `.lua` files that can be used by the Seoul Engine runtime (the `SeoulEngine/Code/Script/` project).

See also [SlimCS](https://github.com/demiurgestudios/seoulengine/wiki/SlimCS).

#### `SeoulTools/Code/SlimCSVS/`

Debugging support and further Visual Studio integration for SlimCS (specifically the `Scripts.csproj` in `App/Source/Authored/Scripts/`).

## Building

### Windows

1. Install [Visual Studio 2019](https://visualstudio.microsoft.com/vs/older-downloads/).
2. Set the `SEOUL_VS2019` environment variable to your Visual Studio 2019 installation path (e.g. `C:\Program Files (x86)\Microsoft Visual Studio\2019\Community`).
3. Run `App\Utilities\PC_BuildAndCook.bat Developer` to generate:
   * `App\Binaries\PC\Developer\x64\AppPC.exe` - stub/placeholder game executable.
   * `SeoulEditor\Binaries\PC\Developer\x64\EditorPC.exe` - the Seoul Editor executable.
   * `SeoulTools\Binaries\PC\Developer\x64\*.exe` - various tool executables, including `Cooker.exe`, which is needed by other executables to cook content on the fly.
4. Run `App\Utilities\GenerateSln.bat` to generate `App\App.sln`, `SeoulEditor\SeoulEditor.sln`, and `SeoulTools\SeoulTools.sln` files:
   * Open these in Visual Studio 2019 to modify source code, build various configurations, and run executables in the Visual Studio debugger.

## License

* Seoul Engine is distributed under the [MIT License](https://github.com/demiurgestudios/seoulengine/blob/main/LICENSE).
* Files in [External](https://github.com/demiurgestudios/seoulengine/tree/main/External) are third party and are distributed under
their own license terms. See [README_EXTERNAL.txt](https://github.com/demiurgestudios/seoulengine/blob/main/External/README_EXTERNAL.txt)
and the individual project sub folders of `External` for more information.
* Files in [Scripts/DevOnly/External](https://github.com/demiurgestudios/seoulengine/tree/main/App/Source/Authored/Scripts/DevOnly/External)
are third party and are distributed under their own license terms. See [README.txt](https://github.com/demiurgestudios/seoulengine/blob/main/App/Source/Authored/Scripts/DevOnly/README.txt)
and the individual project sub folders of `External` for more information.
* Files in [pngsuite](https://github.com/demiurgestudios/seoulengine/tree/main/App/Data/Config/UnitTests/Image/pngsuite) are third party
and are distributed under the license terms described in [PngSuite.LICENSE](https://github.com/demiurgestudios/seoulengine/blob/main/App/Data/Config/UnitTests/Image/pngsuite/PngSuite.LICENSE).
* Roboto fonts, located in [Source/Authored](https://github.com/demiurgestudios/seoulengine/tree/main/App/Source/Authored) and
[UnitTests](https://github.com/demiurgestudios/seoulengine/tree/main/App/Data/Config/UnitTests), are third party and are
distributed under the [Apache License, Version 2.0](https://github.com/demiurgestudios/seoulengine/blob/main/App/Data/Config/UnitTests/Falcon/Apache%20License.txt).
* Files in [SlimCS/Mono](https://github.com/demiurgestudios/seoulengine/tree/main/SeoulTools/Data/Config/UnitTests/SlimCS/Mono) are third party (from [mcs/tests](https://github.com/mono/mono/tree/main/mcs/tests)) and are distributed under the [MIT License](https://github.com/mono/mono/blob/main/LICENSE).
