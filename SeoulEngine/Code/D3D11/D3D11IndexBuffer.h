/**
 * \file D3D11IndexBuffer.h
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
#ifndef D3D11_INDEX_BUFFER_H
#define D3D11_INDEX_BUFFER_H

#include "CheckedPtr.h"
#include "IndexBuffer.h"
struct ID3D11Buffer;

namespace Seoul
{

/**
 * D3D11 specific implementation of the IndexBuffer class.
 */
class D3D11IndexBuffer SEOUL_SEALED : public IndexBuffer
{
public:
	virtual Bool OnCreate() SEOUL_OVERRIDE;

private:
	D3D11IndexBuffer(
		void const* pInitialData,
		UInt32 zInitialDataSizeInBytes,
		UInt32 zTotalSizeInBytes,
		IndexBufferDataFormat eFormat,
		Bool bDynamic);
	virtual ~D3D11IndexBuffer();

	friend class D3D11Device;
	friend class D3D11RenderCommandStreamBuilder;

	SEOUL_REFERENCE_COUNTED_SUBCLASS(D3D11IndexBuffer);

	void InternalFreeInitialData();

	void const* m_pInitialData;
	UInt32 m_zInitialDataSizeInBytes;
	IndexBufferDataFormat m_eFormat;
	CheckedPtr<ID3D11Buffer> m_pIndexBuffer;
	Bool m_bDynamic;

	SEOUL_DISABLE_COPY(D3D11IndexBuffer);
}; // class D3D11IndexBuffer

} // namespace Seoul

#endif // include guard
