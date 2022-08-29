/**
 * \file FalconConstants.h
 * \brief Common constant values and a few
 * simple shared functions for the Falcon project.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_CONSTANTS_H
#define FALCON_CONSTANTS_H

#include "SeoulMath.h"

namespace Seoul::Falcon
{

/**
 * Position values are stored in Twips in the Falcon data. Converts to pixels prior to assignment.
 * Function is not intended for use outside of Falcon loaders.
 */
template <typename T>
static inline Float TwipsToPixels(T v)
{
	return ((Float)v / 20.0f);
}

/** Factor used to convert twips into pixels. */
static const Float kfTwipsToPixelsFactor = (Float)(1.0 / 20.0);

//
// Tolerance used to determine "about equal" when
// comparing translation values (based on flash
// twips, which is 20x a pixel, which is the finest
// resolution of translation from flash data).
//
static const Float kfAboutEqualPosition = (1.0f / 19.0f);

/**
 * The resolution (in pixels) that SDF glyphs are generated at.
 *
 * WARNING: Changing this constant means recooking all font assets,
 * as this value is used to bake out glyph data in the pipeline.
 */
static const Float kfGlyphHeightSDF = 24.0f;

/**
 * The distance from the base font glyphs to which SDF is computed, in pixels at the SDF resolution.
 *
 *
 * WARNING: Changing this constant means recooking all font assets,
 * as this value is used to bake out glyph data in the pipeline.
 */
static const Int kiRadiusSDF = 8;
static const Int kiDiameterSDF = 2 * kiRadiusSDF;

/**
 * The quantizing size of the negative portion of an SDF distance.
 *
 * Negative values are inside a shape. This value is the number of UInt8
 * values dedicated to distances inside the shape.
 */
static const Float kfNegativeQuantizeSDF = 63.0f;

/**
 * Color multiply alpha values below this threshold will
 * disable occlusion casting on a shape.
 */
static const Float kfOcclusionAlphaThreshold = ((255.0f - 7.0f) / 255.0f);

/**
 * TODO: Not an easy constant to set - because we're calculationg "overfill"
 * based on the max size of a single draw, we can potentially miss high amounts of
 * overfill due to many small shapes.
 *
 * The correct solution would be an optimizer that considers the entire frame
 * and tries to minimize overfill while maximizing batch sizes. This is challenging
 * because it both needs to be applied during batch optimization and because it requires
 * an accurate cost metric which can reasonably weight the cost of too many draw calls
 * against the cost of more expensive pixel shaders (which is ultimately a device
 * and screen dependent quantity). So we've settled for "don't overfill the background
 * of a UI popup that takes occupies 10% of the screen".
 *
 * Used to break up batches to avoid using an expensive
 * shader on an shape that occupies a lot of screen space.
 * Ratio multiplied against (width * height).
 */
static const Double kfMaxCostInBatchFromOverfillFactor = 0.1;

/**
* Bit value used for click mouse input hit tests.
*/
static const UInt8 kuClickMouseInputHitTest = (1 << 0);

/**
* Bit value used for horizontal drag input hit tests.
*/
static const UInt8 kuHorizontalDragMouseInputHitTest = (1 << 1);

/**
* Bit value used for vertical drag input hit tests.
*/
static const UInt8 kuVerticalDragMouseInputHitTest = (1 << 2);

/**
* Bit value used for drag input hit tests. Includes
* both horizontal and vertical drag.
*/
static const UInt8 kuDragMouseInputHitTest = (1 << 1) | (1 << 2);

} // namespace Seoul::Falcon

#endif // include guard
