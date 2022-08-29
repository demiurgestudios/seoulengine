===========================
Recommended upgrade process
===========================
- extract the tar into a Unix environment (Linux or Mac OS X), to maintain symbolic links. If not available, can also use MSYS: http://www.mingw.org/wiki/msys
- check the directory structure into a new folder in Perforce.
- merge our modifications into the new source:
  - extract the full source and binaries from openssl-src-with-demiurge-mods.zip.
  - search for EXT_DEMIURGE in the existing code.
  - copy Android.mk, android-config.mk, and AndroidManifest.xml
  - copy crypto/Android.mk, jni/Application.mk, and ssl/Android.mk
  - replace openssl-src-with-demiurge-mods.zip.
- also apply these changes to the tar itself and replace openssl-src-with-demiurge-mods.tar.gz with the result.

========
Building
========
Linux
- follow the standard openssl instructions in addition to the above:
  - ./config
  - make depend
  - make all

Android
- run androidbuild.sh on Mac Os X with the Android NDK installed (script supports NDK r15c)

Win32
- You need to install nasm: http://www.nasm.us/
- Run Start -> All Programs -> Visual Studio -> Visual Studio Tools -> Developer Command Prompt for VS 2017
- Configure with: "perl Configure VC-WIN32
- Run "ms\do_nasm.bat"
- Run "nmake -f ms\nt.mak"
- Copy out32\libeay32.lib into libs\Windows\x86\
- Copy out32\ssleay32.lib into libs\Windows\x86\
- Copy tmp32\app.pdb into libs\Windows\x86
- Copy tmp32\lib.pdb into libs\Windows\x86
- Now delete all other files created during the build (mostly in inc32, out32, and tmp32).

Windows x64
- You need to install nasm: http://www.nasm.us/
- Run Start -> All Programs -> Visual Studio -> Visual Studio Tools -> VC -> x64 Native Tools Command Prompt for VS2017
- Configure with "perl Configure VC-WIN64A"
- Run "ms\do_win64a.bat"
- Run "nmake -f ms\nt.mak"
- Copy out32\libeay32.lib into libs\Windows\x64\
- Copy out32\ssleay32.lib into libs\Windows\x64\
- Copy tmp32\app.pdb into libs\Windows\x64
- Copy tmp32\lib.pdb into libs\Windows\x64
- Now delete all other files created during the build (mostly in inc32, out32, and tmp32).
