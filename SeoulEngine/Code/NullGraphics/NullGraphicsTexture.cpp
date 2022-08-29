/**
 * \file NullGraphicsTexture.cpp
 * \brief Nop implementation of a Texture for
 * contexts without graphics hardware.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DDS.h"
#include "NullGraphicsTexture.h"
#include "ThreadId.h"

namespace Seoul
{

NullGraphicsTexture::NullGraphicsTexture(
	UInt uWidth,
	UInt uHeight,
	PixelFormat eFormat,
	Bool bSecondary)
	: BaseTexture()
	, m_bSecondary(bSecondary)
{
	m_iWidth = uWidth;
	m_iHeight = uHeight;
	m_eFormat = eFormat;
}

NullGraphicsTexture::~NullGraphicsTexture()
{
	SEOUL_ASSERT(IsRenderThread());
}

Bool NullGraphicsTexture::OnCreate()
{
	SEOUL_ASSERT(IsRenderThread());

	SEOUL_VERIFY(BaseTexture::OnCreate());
	return true;
}

void NullGraphicsTexture::OnReset()
{
	SEOUL_ASSERT(IsRenderThread());

	BaseTexture::OnReset();
}

void NullGraphicsTexture::OnLost()
{
	SEOUL_ASSERT(IsRenderThread());

	BaseTexture::OnLost();
}

} // namespace Seoul
