/**
 * \file D3D11VertexBuffer.h
 * \brief A collection of GPU vertices used for drawing geometry.
 * Vertices include position, normal, and other data that can be stored
 * per vertex to represent renderable geometry.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef D3D11_VERTEX_BUFFER_H
#define D3D11_VERTEX_BUFFER_H

#include "CheckedPtr.h"
#include "VertexBuffer.h"
struct ID3D11Buffer;

namespace Seoul
{

/**
 * D3D11 specific implementation of the VertexBuffer class.
 */
class D3D11VertexBuffer SEOUL_SEALED : public VertexBuffer
{
public:
	virtual Bool OnCreate() SEOUL_OVERRIDE;

private:
	D3D11VertexBuffer(
		void const* pInitialData,
		UInt32 zInitialDataSizeInBytes,
		UInt32 zTotalSizeInBytes,
		UInt32 zVertexStrideInBytes,
		Bool bDynamic);
	virtual ~D3D11VertexBuffer();

	friend class D3D11Device;
	friend class D3D11RenderCommandStreamBuilder;

	SEOUL_REFERENCE_COUNTED_SUBCLASS(D3D11VertexBuffer);

	void InternalFreeInitialData();

	void const* m_pInitialData;
	UInt32 m_zInitialDataSizeInBytes;
	CheckedPtr<ID3D11Buffer> m_pVertexBuffer;
	Bool m_bDynamic;

	SEOUL_DISABLE_COPY(D3D11VertexBuffer);
}; // class D3D11VertexBuffer

} // namespace Seoul

#endif // include guard
