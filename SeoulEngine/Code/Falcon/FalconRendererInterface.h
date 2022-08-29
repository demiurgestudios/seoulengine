/**
 * \file FalconRendererInterface.h
 * \brief Interface to the platform dependent
 * rendering backend.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_RENDERER_INTERFACE_H
#define FALCON_RENDERER_INTERFACE_H

#include "FalconTexturePacker.h"
#include "Prereqs.h"
namespace Seoul { class FilePath; }

namespace Seoul::Falcon
{

class RendererInterface SEOUL_ABSTRACT
{
public:
	virtual ~RendererInterface()
	{
	}

	virtual void ClearPack() = 0;
	virtual void Pack(
		PackerTree2D::NodeID nodeID,
		const SharedPtr<Texture>& pSource,
		const Rectangle2DInt& source,
		const Point2DInt& destination) = 0;
	virtual UInt32 GetRenderFrameCount() const = 0;
	virtual void ResolvePackerTexture(
		TexturePacker& rPacker,
		SharedPtr<Texture>& rp) = 0;
	virtual void ResolveTexture(
		FilePath filePath,
		SharedPtr<Texture>& rp) = 0;
	virtual void ResolveTexture(
		UInt8 const* pData,
		UInt32 uDataWidth,
		UInt32 uDataHeight,
		UInt32 uStride,
		Bool bIsFullOccluder,
		SharedPtr<Texture>& rp) = 0;
	virtual void UnPack(PackerTree2D::NodeID nodeID) = 0;

protected:
	RendererInterface()
	{
	}

private:
	SEOUL_DISABLE_COPY(RendererInterface);
}; // class RendererInterface

} // namespace Seoul::Falcon

#endif // include guard
