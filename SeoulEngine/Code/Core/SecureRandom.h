/**
 * \file SecureRandom.h
 * \brief Utility function that generates a sequence of cryptographically
 * secure random bytes. Typically, this function is more expensive
 * than using PseudoRandom, so it should only be used in situations where
 * extremely high quality random generation is needed.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SECURE_RANDOM_H
#define SECURE_RANDOM_H

#include "Prereqs.h"

namespace Seoul::SecureRandom
{

void GetBytes(UInt8* p, UInt32 uSizeInBytes);

} // namespace Seoul::SecureRandom

#endif // include guard
