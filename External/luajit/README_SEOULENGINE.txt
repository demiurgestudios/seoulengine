IMPORTANT: We've modified LuaJIT to add necessary bits to
efficiently support int32 operations (+, /, %, *, -) in
the interpreter and JIT. Note that assembly for these
changes has not been implemented for PPC, MIPS, or MIPS64.

IMPORTANT: We've modified LuaJIT to add a "memfree" hook.
We want our C++ destructors to be called when an object's
memory is about to be freed (not as part of a finalizer),
since calling the destructor in the finalizer can result
in access of a destroyed object (since finalizers can
potentially result in resurrection of an object).

Search the code for EXT_DEMIURGE for these changes. Changes must
be maintained across upgrades to the LuaJIT codebase.

We've made small modifications to msvcbuild.bat to cleanup
and setup output directories. We also added additional compilation
flags (see last note).

androidbuild, iosbuild, and linuxbuild are also Demiurge creations.

Finally, we build with the following additional compilation flags:
- LUAJIT_ENABLE_LUA52COMPAT - enable a subset of Lua 5.2 functionality.
- LUAJIT_DISABLE_JIT - a biggie, due to:
  - https://github.com/LuaJIT/LuaJIT/issues/197
  - https://www.freelists.org/post/luajit/Android-performance-drop-moving-from-LuaJIT201-LuaJIT202
  - we are hitting this bug, which makes JIT *much* slower on Android
    (and unpredictably - we've seen benchmarks that normally take parts of a second
    occasionally spike to whole minutes). Since we aren't allowed to JIT
    on iOS anyway, the prudent decision was to just disable JIT across
    the board and also simplify the matrix of tests we need to run for
    our custom integer operations.
- LUAJIT_DISABLE_FFI - disabling FFI because it is always prohibitively expensive
  when JIT is disabled.

========
Building
========
Android
-------
Run androidbuild on Mac OS with a NDK setup.

iOS
---
Run iosbuild on Mac OS.

Linux
-----
In a Linux VM, run linuxbuild.

Windows
-------
In a Windows command shell, run msvcbuild.bat (in the root folder,
this is a Demiurge addition/convenience script).
