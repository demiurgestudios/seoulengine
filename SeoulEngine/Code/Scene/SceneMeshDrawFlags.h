/**
 * \file SceneMeshDrawFlags.h
 * \brief Rendering options when drawing a mesh.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCENE_MESH_DRAW_FLAGS_H
#define SCENE_MESH_DRAW_FLAGS_H

#include "Prereqs.h"

#if SEOUL_WITH_SCENE

namespace Seoul::Scene
{

/** Must be kept in-sync with equivalent flags in MeshDrawComponent.cs in SeoulEditor. */
namespace MeshDrawFlags
{
	enum Enum
	{
		kNone = 0,

		/**
		 * Mesh will be rendered at the origin (ignore its world transform)
		 * and projected to infinity.
		 */
		kSky = (1 << 0),

		/**
		 * Mesh depth will be projected to "infinity" (to the far plane),
		 * effectively disabling depth testing and depth clipping.
		 */
		kInfiniteDepth = (1 << 1),
	};
} // namespace MeshDrawFlags

} // namespace Seoul::Scene

#endif // /#if SEOUL_WITH_SCENE

#endif // include guard
