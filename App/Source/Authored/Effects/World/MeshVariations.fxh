/**
 * \file MeshVariations.fxh
 * \brief Utility header, define after defining SEOUL_MESH_STATES,
 * SEOUL_MESH_FRAGMENT, SEOUL_MESH_VERTEX, and SEOUL_MESH_VERTEX_SKINNED
 * to generate variations of that technique.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#ifndef SEOUL_MESH_STATES
#	error "Must define SEOUL_MESH_STATES prior to including this header."
#endif
#ifndef SEOUL_MESH_FRAGMENT
#	error "Must define SEOUL_MESH_FRAGMENT prior to including this header."
#endif
#ifndef SEOUL_MESH_VERTEX
#	error "Must define SEOUL_MESH_VERTEX prior to including this header."
#endif
#ifndef SEOUL_MESH_VERTEX_SKINNED
#	error "Must define SEOUL_MESH_VERTEX_SKINNED prior to including this header."
#endif

#define SEOUL_MESH_VAR_TECHNIQUE(d, e, am) \
	SEOUL_TECHNIQUE(seoul_Render_FM_##d##_FM_##e##_FM_##am)
#define SEOUL_MESH_VAR_TECHNIQUE_SKINNED(d, e, am) \
	SEOUL_TECHNIQUE(seoul_Render_FM_##d##_FM_##e##_FM_##am##_Skinned)

#define SEOUL_MESH_VAR_UTIL(d, e, am) \
	SEOUL_MESH_VAR_TECHNIQUE(d, e, am) \
	{ \
		pass \
		{ \
			SEOUL_MESH_STATES \
			SEOUL_PS(SEOUL_MESH_FRAGMENT(FM_##d, FM_##e, FM_##am)); \
			SEOUL_VS(SEOUL_MESH_VERTEX()); \
		} \
	} \
	SEOUL_MESH_VAR_TECHNIQUE_SKINNED(d, e, am) \
	{ \
		pass \
		{ \
			SEOUL_MESH_STATES \
			SEOUL_PS(SEOUL_MESH_FRAGMENT(FM_##d, FM_##e, FM_##am)); \
			SEOUL_VS(SEOUL_MESH_VERTEX()); \
		} \
	}

SEOUL_MESH_VAR_UTIL(0, 0, 0)
SEOUL_MESH_VAR_UTIL(C, 0, 0)
SEOUL_MESH_VAR_UTIL(T, 0, 0)
SEOUL_MESH_VAR_UTIL(0, C, 0)
SEOUL_MESH_VAR_UTIL(C, C, 0)
SEOUL_MESH_VAR_UTIL(T, C, 0)
SEOUL_MESH_VAR_UTIL(0, T, 0)
SEOUL_MESH_VAR_UTIL(C, T, 0)
SEOUL_MESH_VAR_UTIL(T, T, 0)

SEOUL_MESH_VAR_UTIL(0, 0, C)
SEOUL_MESH_VAR_UTIL(C, 0, C)
SEOUL_MESH_VAR_UTIL(T, 0, C)
SEOUL_MESH_VAR_UTIL(0, C, C)
SEOUL_MESH_VAR_UTIL(C, C, C)
SEOUL_MESH_VAR_UTIL(T, C, C)
SEOUL_MESH_VAR_UTIL(0, T, C)
SEOUL_MESH_VAR_UTIL(C, T, C)
SEOUL_MESH_VAR_UTIL(T, T, C)

SEOUL_MESH_VAR_UTIL(0, 0, T)
SEOUL_MESH_VAR_UTIL(C, 0, T)
SEOUL_MESH_VAR_UTIL(T, 0, T)
SEOUL_MESH_VAR_UTIL(0, C, T)
SEOUL_MESH_VAR_UTIL(C, C, T)
SEOUL_MESH_VAR_UTIL(T, C, T)
SEOUL_MESH_VAR_UTIL(0, T, T)
SEOUL_MESH_VAR_UTIL(C, T, T)
SEOUL_MESH_VAR_UTIL(T, T, T)

#undef SEOUL_MESH_VAR_UTIL
#undef SEOUL_MESH_VAR_TECHNIQUE_SKINNED
#undef SEOUL_MESH_VAR_TECHNIQUE
#undef SEOUL_MESH_VERTEX_SKINNED
#undef SEOUL_MESH_VERTEX
#undef SEOUL_MESH_FRAGMENT
#undef SEOUL_MESH_TECHNIQUE
