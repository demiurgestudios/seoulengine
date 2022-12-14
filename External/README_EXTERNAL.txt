Third-party libraries used by SeoulTools (only, no license notice needed in shipped products):
- amd_tootle/ at commit cdde8da05a52bd80488dfd498cef2cac4bbbf332 (2016/02/28):
  - https://github.com/GPUOpen-Tools/amd-tootle
  - License: MIT
- Android/MinimumPlatformTools/ from SDK r26.0.1:
  - snapshot of adb from SDK r26.0.1 for use by Moriarty.
  - binary only.
- fastbuild/ at commit 863264ec8db56fcdfabfee7d336d5fbab453a65f (2021/02/27):
  - https://github.com/jzupko/fastbuild
  - NOTE: Built binary only - use update.bat in fastbuild/ to update the binaries.
- fbxsdk/ at v2020.3.1 VS2019 (2020/03/01):
  - https://www.autodesk.com/developer-network/platform-technologies/fbx
  - https://www.autodesk.com/products/fbx/overview
  - https://www.autodesk.com/developer-network/platform-technologies/fbx-sdk-2020-3
  - https://www.autodesk.com/content/dam/autodesk/www/adn/fbx/2020-3-1/fbx202031_fbxsdk_vs2019_win.exe
  - License: custom, see also fbxsdk/License.rtf
- glslang/ at revision 29848:
  - https://cvs.khronos.org/svn/repos/ogl/trunk/ecosystem/public/sdk/tools/glslang
  - License: BSD-3-Clause
- ispc/ at  v1.15.0 (2020/12/18):
  - https://github.com/ispc/ispc
  - https://github.com/ispc/ispc/releases/tag/v1.15.0
  - https://github.com/ispc/ispc/releases/download/v1.15.0/ispc-v1.15.0-windows.zip
  - License: BSD-3-Clause
- roslyn/ at v2.6.0, System.Collections.Immutable at v1.3.1
  - acquired via nuget.
  - License: MIT
- Unicode/ at v6.3.0 (2013/05/08):
  - http://www.unicode.org
  - http://www.unicode.org/Public/6.3.0/ucd/SpecialCasing.txt
  - ftp://ftp.unicode.org/Public/UNIDATA/UnicodeData.txt

Third-party libraries used by SeoulEngine (developer only - no license notice needed in shipped products):
- imgui/ at commit 9cd9c2eff99877a3f10a7f9c2a3a5b9c15ea36c6 (2022/06/21):
  - https://github.com/ocornut/imgui/tree/docking
  - NOTE: This is equivalent to v1.88 release of ImGui, but taken from the 'docking'
    branch, which includes docking and multi-viewport support: https://github.com/ocornut/imgui/tree/9cd9c2eff99877a3f10a7f9c2a3a5b9c15ea36c6
  - NOTE: Contains many local modifications, search for EXT_DEMIURGE:
  - License: MIT
- mongoose-mit/ at commit 04fc209644b414d915c446bb1815b55e9fe63acc (2013/08/16):
  - https://github.com/pfalcon-mirrors/mongoose-mit
  - License: MIT

Third-party libraries used by SeoulEngine (shipping - notice possibly required in shipped products):
- Android/cpufeatures/ from NDK r22:
  - License: BSD-2-Clause
- Android/native_app_glue/ from NDK r22:
  - License: Apache v2
- bounce/ at commit ff00ab381eff99d734be07463b58e18479d2db8f (2017/07/19):
  - https://github.com/demiurgestudios/bounce
  - contains (possibly many) unmarked changes and modifications from base. To
    merge updates, rebase the current repo state against the updates.
  - NOTE: origin repo https://github.com/irlanrobson/bounce has been deleted (or made private),
    so please keep https://github.com/demiurgestudios/bounce "clean" (we want to use it as a reference
    of the base state as we make Demiurge specific modifications to Bounce).
  - License: zlib
- breakpad/ at commit 9eac2058b70615519b2c4d8c6bdbfca1bd079e39 (2018/04/13):
  - https://github.com/google/breakpad
  - License: BSD-3-Clause
- breakpad/src/third_party/lss/linux_syscall_support.h at commit a89bf7903f3169e6bc7b8efc10a73a7571de21cf (2018/01/11):
  - https://chromium.googlesource.com/linux-syscall-support
  - License: BSD-3-Clause
- crunch/ at commit 8708900eca8ec609d279270e72936258f81ddfb7 (2018/10/23):
  - https://github.com/Unity-Technologies/crunch/tree/unity
  - NOTE: 'unity' branch, not the 'master' branch.
  - License: zlib
- curl/ at v7.61.1 (2018/09/05):
  - https://github.com/curl/curl
  - https://github.com/curl/curl/releases
  - https://github.com/curl/curl/archive/curl-7_61_1.zip
  - NOTE: We have modifications to libcurl. Search for EXT_DEMIURGE in the codebase.
  - License: custom, modified MIT, see also https://curl.se/docs/copyright.html
- fx11/ at commit 5a89c2bfb35eccf90ec765cc99c707f22fd0d58d (2022/05/23):
  - https://github.com/microsoft/FX11
  - NOTE: Contains local modifications, search for EXT_DEMIURGE:
    being maintained or developed.
  - License: MIT
- gles2/ at revision 16803 (2012/02/02):
  - https://registry.khronos.org/OpenGL/api/
  - License: Apache v2
- jemalloc/ at v4.5.0 (2017/02/28):
  - https://github.com/jemalloc/jemalloc
  - https://github.com/jemalloc/jemalloc/releases
  - https://github.com/jemalloc/jemalloc/releases/download/4.5.0/jemalloc-4.5.0.tar.bz2
  - License: BSD-2-Clause
- lua_protobuf/ at commit 185644f4a709c195f154e0bcac651d71fada9bed (2015/10/2015):
  - https://github.com/starwing/lua-protobuf
  - License: MIT
- luajit/ at commit 9b41062156779160b88fe5e1eb1ece1ee1fe6a74/v2.1 (2018/06/24):
  - https://github.com/LuaJIT/LuaJIT/tree/v2.1
  - License: MIT
- lz4/ at v1.9.3 (2020/11/16):
  - https://github.com/lz4/lz4
  - https://github.com/lz4/lz4/releases
  - https://github.com/lz4/lz4/archive/v1.9.3.zip
  - License: BSD-2-Clause
- miniz/ at commit 0f6b199e5b95fe5eb4057201e3fee9b6662011b4 (2017/10/17):
  - https://github.com/richgel999/miniz
  - NOTE: We have modifications to miniz. Search for EXT_DEMIURGE in the codebase.
  - License: MIT
- nghttp2/ at v1.33.0 (2018/09/02):
  - https://nghttp2.org/
  - https://github.com/nghttp2/nghttp2/releases
  - https://github.com/nghttp2/nghttp2/releases/download/v1.33.0/nghttp2-1.33.0.tar.gz
  - License: MIT
- openssl/ at v1.0.1t (2016/05/03):
  - https://www.openssl.org/source/
  - https://www.openssl.org/source/openssl-1.0.1t.tar.gz
  - License: custom, "dual OpenSSL and SSLeay", see also https://www.openssl.org/source/license-openssl-ssleay.txt
- plcrashreporter/ at v1.2 (2014/06/06):
  - https://www.plcrashreporter.org/
  - https://www.plcrashreporter.org/download/
  - https://www.plcrashreporter.org/static/downloads/PLCrashReporter-1.2.zip
  - License: MIT and Apache v2
- protobuf/ at v3.0.0-beta-1:
  - https://github.com/google/protobuf/releases/tag/v3.0.0-beta-1
  - License: BSD-3-Clause
- pugi/ at v1.11.4 (2020/12/22):
  - https://github.com/zeux/pugixml
  - https://github.com/zeux/pugixml/releases
  - https://github.com/zeux/pugixml/archive/v1.11.4.zip
  - License: MIT
- rapidjson/ at commit 67a17cfdbc25ff1fc8d01714be87e242b03a4cc9 (2018/03/20):
  - https://github.com/miloyip/rapidjson
  - License: MIT
- spng/ at commit 1cf2b38665dd44896bc482d8d17e58bf0e867625 (2021/02/08):
  - https://github.com/randy408/libspng
  - License: BSD-2-Clause
- stb/ at commit b42009b3b9d4ca35bc703f5310eedc74f584be58 (2020/07/13):
  - https://github.com/nothings/stb
  - License: MIT -or- Unlicense (public domain)
- tess2/ at v1.0.1 (2013/09/11):
  - https://github.com/memononen/libtess2
  - https://github.com/memononen/libtess2/releases
  - https://github.com/memononen/libtess2/archive/v1.0.1.zip
  - License: SGI-B
- wjcryptlib/ at commit ba2153ebc09a3ba7d600109e366bcd0bab8f113c (2020/01/12):
  - https://github.com/jzupko/WjCryptLib
  - License: Unlicense (public domain)
- zlib/ at v1.2.8 (2013/04/28):
  - http://www.zlib.net/
  - http://zlib.net/zlib-1.2.8.tar.gz
  - License: zlib
- zstd/ at v1.4.8 (2020/12/18):
  - https://github.com/facebook/zstd
  - https://github.com/facebook/zstd/releases
  - https://github.com/facebook/zstd/archive/v1.4.8.zip
  - License: BSD-3-Clause