/**
 * \file SpatialId.h
 * \brief Identifier of a spatial node in the SpatialTree.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SPATIAL_ID_H
#define SPATIAL_ID_H

#include "Prereqs.h"
#include "SeoulMath.h"

namespace Seoul
{

typedef UInt16 SpatialId;
static const SpatialId kuInvalidSpatialId = UInt16Max;

} // namespace Seoul

#endif // include guard
