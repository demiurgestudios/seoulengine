/**
 * \file D3D11VertexFormat.h
 *
 * \brief D3D11VertexFormat is a specialization of VertexFormat for the
 * D3D11 API. VertexFormat describes the vertex attributes that will
 * be in use for the draw call(s) that are issued while the vertex format
 * is active. The actual vertex buffer and index buffer data is stored in
 * D3D11VertexBuffer and D3D11IndexBuffer respectively.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef D3D11_VERTEX_FORMAT_H
#define D3D11_VERTEX_FORMAT_H

#include "Vector.h"
#include "VertexFormat.h"
struct ID3D11InputLayout;

namespace Seoul
{

/**
 * D3D11 specialization of VertexFormat.
 */
class D3D11VertexFormat SEOUL_SEALED : public VertexFormat
{
public:
	virtual Bool OnCreate() SEOUL_OVERRIDE;

private:
	D3D11VertexFormat(VertexElement const* pElements);
	~D3D11VertexFormat();

	SEOUL_REFERENCE_COUNTED_SUBCLASS(D3D11VertexFormat);

	friend class D3D11Device;
	friend class D3D11RenderCommandStreamBuilder;

	CheckedPtr<ID3D11InputLayout> m_pInputLayout;

	SEOUL_DISABLE_COPY(D3D11VertexFormat);
}; // class D3D11VertexFormat

} // namespace Seoul

#endif // include guard
