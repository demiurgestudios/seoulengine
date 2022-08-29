/**
 * \file FalconClipperTest.h
 * \brief Unit test for clipping functionality in the Falcon project.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_CLIPPER_TEST_H
#define FALCON_CLIPPER_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class FalconClipperTest SEOUL_SEALED
{
public:
	void TestClipStackNone();
	void TestClipStackConvexOneLevel();
	void TestClipStackConvexOneLevelMatrix();
	void TestClipStackConvexTwoLevels();
	void TestClipStackConvexTwoLevels2();
	void TestClipStackRectangleOneLevel();
	void TestClipStackRectangleOneLevelMatrix();
	void TestClipStackRectangleOneLevelMulti();
	void TestClipStackRectangleOneLevelMultiNoneClipAllClip();
	void TestClipStackRectangleTwoLevels();
	void TestClipStackRectangleTwoLevelsAllClipped();
	void TestClipStackRectangleTwoLevelsMulti();
	void TestClipStackRectangleTwoLevelsNoneClipAllClip();

	void TestConvexRectangleBasic();
	void TestConvexRectanglePartial();
	void TestConvexVerticesBasic();
	void TestConvexVerticesPartial();

	void TestEmpty();

	void TestMeshRectangleConvex();
	void TestMeshRectangleConvexClipAll();
	void TestMeshRectangleConvexClipNone();
	void TestMeshRectangleNotSpecific();
	void TestMeshRectangleQuadList();
	void TestMeshRectangleQuadListMulti();
	void TestMeshVerticesConvex();
	void TestMeshVerticesNotSpecific();
	void TestMeshVerticesQuadList();

	void TestMeshRectangleNotSpecificNotClippingInitially();
	void TestMeshTextChunkNoClip();

	void TestMeshTextChunkClipRegression();
	void TestMeshTextChunkClipRegression2();

	void TestMirrorTransform();

	void TestClipperClipRegression();
	void TestZeroSizeClipRegression();
}; // class FalconClipperTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
