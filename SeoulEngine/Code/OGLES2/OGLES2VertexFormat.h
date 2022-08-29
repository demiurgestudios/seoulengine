/**
 * \file OGLES2VertexFormat.h
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

#pragma once
#ifndef OGLES2_VERTEX_FORMAT_H
#define OGLES2_VERTEX_FORMAT_H

#include "Vector.h"
#include "VertexFormat.h"

namespace Seoul
{

// Forward declarations
struct VertexElement;

/**
 * OGLES2VertexFormat is a specialization of VertexFormat for the OGLES2
 * platform. All draw calls must have a valid vertex format defined to
 * be issued - vertex formats describe the vertex attributes that must
 * be active for the draw call to succeed.
 */
class OGLES2VertexFormat SEOUL_SEALED : public VertexFormat
{
public:
	OGLES2VertexFormat(VertexElement const* pElements);
	virtual ~OGLES2VertexFormat();

private:
	SEOUL_REFERENCE_COUNTED_SUBCLASS(OGLES2VertexFormat);

	friend class OGLES2Device;
}; // class OGLES2VertexFormat

} // namespace Seoul

#endif // include guard
