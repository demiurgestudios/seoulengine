/**
 * \file FalconTexturePacker.h
 * \brief Uses a PackerTree2D to manage a dynamic texture atlas.
 *
 * Texture atlasses are used aggressively by Falcon to increase
 * batch sizes and reduce draw calls.
 *
 * To faciliate this, textures are managed dynamically. Textures
 * are "paged" in and out of atlasses on-the-fly based on LRU lists.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_TEXTURE_PACKER_H
#define FALCON_TEXTURE_PACKER_H

#include "FalconFont.h"
#include "FalconPackerTree2D.h"
namespace Seoul { namespace Falcon { class RendererInterface; } }
namespace Seoul { namespace Falcon { class Texture; } }

namespace Seoul::Falcon
{

/**
 * \sa https://developer.nvidia.com/sites/default/files/akamai/tools/files/Texture_Atlas_Whitepaper.pdf
 * \sa http://gamedev.stackexchange.com/a/49585
 * \sa http://pages.jh.edu/~dighamm/research/2004_01_sta.pdf
 */
class TexturePacker SEOUL_SEALED
{
public:
	static const Int32 kBorder = 1;
	static const Int32 kPadding = (2 * kBorder);

	TexturePacker(
		RendererInterface* pInterface,
		Int32 iWidth,
		Int32 iHeight);
	~TexturePacker();

	void Clear();

	void CollectGarbage()
	{
		m_Tree.CollectGarbage();
	}

	Int32 GetHeight() const
	{
		return m_iHeight;
	}

	Int32 GetWidth() const
	{
		return m_iWidth;
	}

	Bool Pack(
		const Font& font,
		UniChar uCodePoint,
		PackerTree2D::NodeID& rNodeID,
		Glyph& rGlyph,
		SharedPtr<Texture>& rpGlyphTexture);

	Bool Pack(
		const SharedPtr<Texture>& pSource,
		const Rectangle2DInt& source,
		PackerTree2D::NodeID& rNodeID,
		Int32& riX,
		Int32& riY);

	Bool UnPack(PackerTree2D::NodeID nodeID);

private:
	void DoPack(
		PackerTree2D::NodeID nodeID,
		const SharedPtr<Texture>& pSource,
		const Rectangle2DInt& source,
		const Point2DInt& destination);

	RendererInterface* m_pInterface;
	Int32 m_iWidth;
	Int32 m_iHeight;
	PackerTree2D m_Tree;

	SEOUL_DISABLE_COPY(TexturePacker);
}; // class TexturePacker

} // namespace Seoul::Falcon

#endif // include guard
