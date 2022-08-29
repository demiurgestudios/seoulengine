/**
 * \file FMODSoundUtil.h
 * \brief Macros and functions to simplify interfacing with the FMOD SDK.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FMOD_SOUND_UTIL_H
#define FMOD_SOUND_UTIL_H

#include "Prereqs.h"

#if SEOUL_WITH_FMOD
#include "fmod_errors.h"

namespace Seoul::FMODSound
{

#if !SEOUL_SHIP
#define FMOD_VERIFY(expression) \
	do \
	{ \
		FMOD_RESULT eFMODResult = (expression); \
		if (eFMODResult == FMOD_ERR_MEMORY) \
		{ \
			::Seoul::FMODSound::OutOfMemory(); \
		} \
		SEOUL_ASSERT_MESSAGE(eFMODResult == FMOD_OK, FMOD_ErrorString(eFMODResult)); \
	} while(0)
#else
#define FMOD_VERIFY(expression) ((void)(expression))
#endif

void OutOfMemory();

/**
 * Helper method, converts a Seoul engine Vector3D into an FMOD vector.
 */
inline FMOD_VECTOR Vector3DToFMOD_VECTOR(const Vector3D& vVector)
{
	FMOD_VECTOR result;
	result.x = vVector.X;
	result.y = vVector.Y;
	result.z = vVector.Z;
	return result;
}

} // namespace Seoul::FMODSound

#endif // /#if SEOUL_WITH_FMOD

#endif // include guard
