/**
 * \file VideoCodec.cpp
 * \brief Video encoding codecs supported by VideoEncoder
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionDefine.h"
#include "VideoCodec.h"

namespace Seoul
{

SEOUL_BEGIN_ENUM(Video::Codec)
	SEOUL_ENUM_N("Lossless", Video::Codec::kLossless)
SEOUL_END_ENUM()

} // namespace Seoul
