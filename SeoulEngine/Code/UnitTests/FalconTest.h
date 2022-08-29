/**
 * \file FalconTest.h
 * \brief Unit test for functionality in the Falcon project.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_TEST_H
#define FALCON_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class FalconTest SEOUL_SEALED
{
public:
	void TestInstanceTransform();
	void TestRenderBatchOptimizerNoIntersection();
	void TestRenderBatchOptimizerNoIntersectionWide();
	void TestRenderBatchOptimizerPartialIntersectionWide();
	void TestRenderBatchOptimizerInterrupt();
	void TestRenderBatchOptimizerIntersection();
	void TestRenderBatchOptimizerPartialIntersection();
	void TestRenderBatchOptimizerPartialIntersectionBlocked();
	void TestRenderOcclusionOptimizerNoOcclusion();
	void TestRenderOcclusionOptimizerOccluded();
	void TestRenderOcclusionOptimizerMixed();
	void TestRenderOcclusionOptimizerMultiple();
	void TestWriteGlyphBitmap();
	void TestRectangleIntersect();
	void TestSetTransform();
	void TestSetTransformTerms();
	void TestScaleRegressionX();
	void TestScaleRegressionY();
	void TestSkewRegression();
	void TestRotationUpdate();
	void TestScaleUpdate();
	void TestHtmlFormattingCharRefs();
	void TestHtmlFormattingRegression();
	void TestHtmlFormattingRobustness();
	void TestHtmlFormattingStrings();
	void TestHtmlFormattingValues();
	void TestProperties();
	void TestGetFcnDependencies();
}; // class FalconTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
