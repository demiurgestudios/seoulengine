/**
 * \file NullGraphicsVertexFormat.cpp
 * \brief Nop implementation of a VertexFormat for
 * contexts without graphics hardware.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "NullGraphicsVertexFormat.h"
#include "ThreadId.h"

namespace Seoul
{

NullGraphicsVertexFormat::NullGraphicsVertexFormat(const VertexElements& vVertexElements)
	: VertexFormat(vVertexElements)
{
}

NullGraphicsVertexFormat::~NullGraphicsVertexFormat()
{
	SEOUL_ASSERT(IsRenderThread());
}

Bool NullGraphicsVertexFormat::OnCreate()
{
	SEOUL_ASSERT(IsRenderThread());

	SEOUL_VERIFY(VertexFormat::OnCreate());

	return true;
}

} // namespace Seoul
