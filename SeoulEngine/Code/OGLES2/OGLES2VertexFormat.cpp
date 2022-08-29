/**
 * \file OGLES2VertexFormat.cpp
 * \brief OGLES2VertexFormat is a specialization of VertexFormat for the
 * OGLES2 platform. VertexFormat describes the vertex attributes that will
 * be in use for the draw call(s) that are issued while the vertex format
 * is active. The actual vertex buffer and index buffer data is stored in
 * OGLES2VertexBuffer and OGLES2IndexBuffer respectively.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "OGLES2VertexFormat.h"
#include "VertexElement.h"

namespace Seoul
{

/**
 * @return A Vector<> of VertexElements defined by the
 * terminated array pElements.
 */
inline VertexFormat::VertexElements GetElements(VertexElement const* pElements)
{
	VertexFormat::VertexElements vElements;

	UInt32 nElementCount = 0u;
	while (VertexElementEnd != pElements[nElementCount])
	{
		++nElementCount;
	}

	vElements.Resize(nElementCount);
	memcpy(vElements.Get(0u), pElements, sizeof(VertexElement) * nElementCount);

	return vElements;
}

OGLES2VertexFormat::OGLES2VertexFormat(
	VertexElement const* pElements)
	: VertexFormat(GetElements(pElements))
{
}

OGLES2VertexFormat::~OGLES2VertexFormat()
{
}

} // namespace Seoul
