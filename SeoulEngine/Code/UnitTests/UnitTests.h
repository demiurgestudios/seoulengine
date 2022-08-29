/**
 * \file UnitTests.h
 * \brief Root header to include SeoulEngine unit tests in a project.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UNIT_TESTS_H
#define UNIT_TESTS_H

#include "ReflectionType.h"

namespace Seoul
{

#if SEOUL_BENCHMARK_TESTS
SEOUL_LINK_ME(class, ScriptBenchmarks)
#endif // /#if SEOUL_BENCHMARK_TESTS

#if SEOUL_UNIT_TESTS

//
// SeoulEngine unit tests.
//
SEOUL_LINK_ME(class, AABBTest);
SEOUL_LINK_ME(class, AlgorithmsTest);
#if SEOUL_WITH_ANIMATION_2D
SEOUL_LINK_ME(class, Animation2DTest);
SEOUL_LINK_ME(class, Animation2DBenchmarks);
SEOUL_LINK_ME(class, Animation2DBinaryTest);
#endif // /#if SEOUL_WITH_ANIMATION_2D
SEOUL_LINK_ME(class, AtomicTest);
SEOUL_LINK_ME(class, AtomicHandleTest);
SEOUL_LINK_ME(class, CachingDiskFileSystemTest);
SEOUL_LINK_ME(class, ColorTest);
SEOUL_LINK_ME(class, CompressionTest);
SEOUL_LINK_ME(class, CookDatabaseTest);
SEOUL_LINK_ME(class, CoreTest);
SEOUL_LINK_ME(class, CoroutineTest);
SEOUL_LINK_ME(class, DataStoreTest);
SEOUL_LINK_ME(class, DataStoreCommandsTest);
SEOUL_LINK_ME(class, DelegateTest);
SEOUL_LINK_ME(class, DirectoryTest);
SEOUL_LINK_ME(class, DownloadablePackageFileSystemTest);
SEOUL_LINK_ME(class, EncryptAESTest);
SEOUL_LINK_ME(class, EncryptXXTEATest);
SEOUL_LINK_ME(class, EventsTest);
SEOUL_LINK_ME(class, ETC1Test);
SEOUL_LINK_ME(class, FalconClipperTest);
SEOUL_LINK_ME(class, FalconTest);
SEOUL_LINK_ME(class, FilePathTest);
SEOUL_LINK_ME(class, FixedArrayTest);
#if SEOUL_WITH_FMOD
SEOUL_LINK_ME(class, FMODTest);
#endif // /#if SEOUL_WITH_FMOD
SEOUL_LINK_ME(class, FrustumTest);
SEOUL_LINK_ME(class, GameMigrationTest);
SEOUL_LINK_ME(class, GamePatcherTest);
SEOUL_LINK_ME(class, GamePathsTest);
SEOUL_LINK_ME(class, HashSetTest);
SEOUL_LINK_ME(class, HashTableTest);
SEOUL_LINK_ME(class, HStringTest);
SEOUL_LINK_ME(class, HTTPTest);
SEOUL_LINK_ME(class, HTTPHeaderTableTest);
SEOUL_LINK_ME(class, ImageTest);
#if SEOUL_PLATFORM_IOS
SEOUL_LINK_ME(class, IOSUtilTest);
#endif // /#if SEOUL_PLATFORM_IOS
SEOUL_LINK_ME(class, JobsTest);
SEOUL_LINK_ME(class, LatchTest);
SEOUL_LINK_ME(class, LexerParserTest);
SEOUL_LINK_ME(class, ListTest);
SEOUL_LINK_ME(class, LocManagerTest);
SEOUL_LINK_ME(class, Matrix2DTest);
SEOUL_LINK_ME(class, Matrix4DTest);
SEOUL_LINK_ME(class, MemoryManagerTest);
SEOUL_LINK_ME(class, MixpanelTest);
SEOUL_LINK_ME(class, NamedTypeTest);
#if SEOUL_WITH_NAVIGATION
SEOUL_LINK_ME(class, NavigationTest);
#endif
SEOUL_LINK_ME(class, NTPClientTest);
SEOUL_LINK_ME(class, PackageFileSystemTest);
SEOUL_LINK_ME(class, PatchablePackageFileSystemTest);
SEOUL_LINK_ME(class, PathTest);
SEOUL_LINK_ME(class, PerThreadStorageTest);
#if SEOUL_WITH_PHYSICS
SEOUL_LINK_ME(class, PhysicsTest);
#endif
SEOUL_LINK_ME(class, PlaneTest);
SEOUL_LINK_ME(class, PseudoRandomTest);
SEOUL_LINK_ME(class, QuaternionTest);
SEOUL_LINK_ME(class, QueueTest);
SEOUL_LINK_ME(class, RemapDiskFileSystemTest);
SEOUL_LINK_ME(class, ReflectionTest);
SEOUL_LINK_ME(class, SaveApiTest);
SEOUL_LINK_ME(class, ScriptTest);
SEOUL_LINK_ME(class, SeoulFileTest);
SEOUL_LINK_ME(class, SeoulMathTest);
SEOUL_LINK_ME(class, SeoulRegexTest);
SEOUL_LINK_ME(class, SeoulTimeTest);
SEOUL_LINK_ME(class, SeoulUtilTest);
SEOUL_LINK_ME(class, SeoulUtilDllTest);
SEOUL_LINK_ME(class, SeoulWildcardTest);
SEOUL_LINK_ME(class, SharedPtrTest);
SEOUL_LINK_ME(class, SpatialTreeTest);
SEOUL_LINK_ME(class, StackOrHeapArrayTest);
SEOUL_LINK_ME(class, StreamBufferTest);
SEOUL_LINK_ME(class, StringTest);
SEOUL_LINK_ME(class, ThreadTest);
SEOUL_LINK_ME(class, UITest);
SEOUL_LINK_ME(class, UnsafeBufferTest);
SEOUL_LINK_ME(class, Vector2DTest);
SEOUL_LINK_ME(class, Vector3DTest);
SEOUL_LINK_ME(class, Vector4DTest);
SEOUL_LINK_ME(class, VectorTest);
SEOUL_LINK_ME(class, VectorUtilTest);
SEOUL_LINK_ME(class, ViewportTest);
SEOUL_LINK_ME(class, WordFilterTest);
SEOUL_LINK_ME(class, ZipFileTest);

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
