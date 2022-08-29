# Seoul Engine
Seoul Engine source code

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
