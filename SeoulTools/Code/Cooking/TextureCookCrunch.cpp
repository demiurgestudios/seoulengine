/**
 * \file TextureCookCrunch.cpp
 * \brief Implementation of texture compression using Crunch.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DDS.h"
#include "PixelFormat.h"
#include "ScopedAction.h"
#include "TextureCookCrunch.h"
#include "Thread.h"
#include <crnlib.h>

namespace Seoul::Cooking
{

namespace CompressorCrunch
{

Bool CompressBlocksETC1(
	UInt8 const* pInput,
	UInt32 uWidth,
	UInt32 uHeight,
	Int32 iQuality,
	UInt8*& rpOutput,
	UInt32& ruOutSize)
{
	// Setup crunch compression parameters.
	crn_comp_params params;
	params.m_pImages[0][0] = (UInt32 const*)pInput;
	params.m_file_type = cCRNFileTypeCRN;
	params.m_faces = 1;
	params.m_width = uWidth;
	params.m_height = uHeight;
	params.m_levels = 1u;
	params.m_format = cCRNFmtETC1;
	params.m_num_helper_threads = Min(Thread::GetProcessorCount(), (UInt32)cCRNMaxHelperThreads);
	params.m_quality_level = iQuality;

	UInt32 uNew = 0u;
	void* pNew = crn_compress(params, uNew);
	if (nullptr == pNew)
	{
		return false;
	}

	rpOutput = (UInt8*)pNew;
	ruOutSize = uNew;
	return true;
}

} // namespace CompressorCrunch

} // namespace Seoul::Cooking
