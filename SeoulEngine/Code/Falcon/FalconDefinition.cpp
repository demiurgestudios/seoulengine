/**
 * \file FalconDefinition.cpp
 * \brief A Definition is the base class of all shared
 * data in a Falcon scene graph.
 *
 * Definitions are instantiated/exist in a Falcon scene
 * graph via instances. All definitions have a corresponding
 * instance (e.g. BitmapDefinition and BitmapInstance).
 *
 * The Definition represents the shared, immutable data while
 * the instance holds any per-node, mutable data.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FalconDefinition.h"
#include "FalconInstance.h"
#include "ReflectionDefine.h"

namespace Seoul
{

SEOUL_BEGIN_ENUM(Falcon::DefinitionType)
	SEOUL_ENUM_N("Unknown", Falcon::DefinitionType::kUnknown)
	SEOUL_ENUM_N("BinaryData", Falcon::DefinitionType::kBinaryData)
	SEOUL_ENUM_N("Bitmap", Falcon::DefinitionType::kBitmap)
	SEOUL_ENUM_N("EditText", Falcon::DefinitionType::kEditText)
	SEOUL_ENUM_N("Font", Falcon::DefinitionType::kFont)
	SEOUL_ENUM_N("MovieClip", Falcon::DefinitionType::kMovieClip)
	SEOUL_ENUM_N("Shape", Falcon::DefinitionType::kShape)
SEOUL_END_ENUM()

namespace Falcon
{

void Definition::DoCreateInstance(SharedPtr<Instance>& rp) const
{
	// Default, set to the invalid pointer.
	rp.Reset();
}

} // namespace Falcon

} // namespace Seoul
