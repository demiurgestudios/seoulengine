/**
 * \file FalconTexturePacker.cpp
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FalconRenderPoser.h"
#include "FalconRendererInterface.h"
#include "FalconTexture.h"
#include "FalconTexturePacker.h"

namespace Seoul::Falcon
{

TexturePacker::TexturePacker(
	RendererInterface* pInterface,
	Int32 iWidth,
	Int32 iHeight)
	: m_pInterface(pInterface)
	, m_iWidth(iWidth)
	, m_iHeight(iHeight)
	, m_Tree(iWidth, iHeight)
{
}

TexturePacker::~TexturePacker()
{
}

void TexturePacker::Clear()
{
	m_Tree.Clear();
	m_pInterface->ClearPack();
}

Bool TexturePacker::Pack(
	const Font& font,
	UniChar uCodePoint,
	PackerTree2D::NodeID& rNodeID,
	Glyph& rGlyph,
	SharedPtr<Texture>& rpGlyphTexture)
{
	// Get the glyph (not oversize, excluding SDF region) bounding box.
	CookedGlyphEntry const* pGlyph = font.m_pData->GetGlyph(uCodePoint);
	if (nullptr == pGlyph)
	{
		return false;
	}

	// Get the glyph data - fail if this fails.
	void* pGlyphData = nullptr;
	Int32 iFullWidth = 0;
	Int32 iFullHeight = 0;
	if (!font.m_pData->GetGlyphBitmapDataSDF(uCodePoint, pGlyphData, iFullWidth, iFullHeight))
	{
		return false;
	}

	SharedPtr<Texture> pTexture;
	m_pInterface->ResolveTexture(
		(UInt8 const*)pGlyphData,
		(UInt32)iFullWidth,
		(UInt32)iFullHeight,
		1,
		false,
		pTexture);
	MemoryManager::Deallocate(pGlyphData);
	pGlyphData = nullptr;

	PackerTree2D::NodeID nodeID = 0;
	Int32 iX = 0;
	Int32 iY = 0;
	if (!m_Tree.Pack(iFullWidth + kPadding, iFullHeight + kPadding, nodeID, iX, iY))
	{
		return false;
	}

	DoPack(nodeID, pTexture, Rectangle2DInt(0, 0, iFullWidth, iFullHeight), Point2DInt(iX + kBorder, iY + kBorder));

	rNodeID = nodeID;

	Vector2D const vOffset(Vector2D((Float)(iX + kBorder) / (Float)m_iWidth, (Float)(iY + kBorder) / (Float)m_iHeight));
	Vector2D const vScale(Vector2D((Float)iFullWidth / (Float)m_iWidth, (Float)iFullHeight / (Float)m_iHeight));

	rGlyph.m_fTX0 = vOffset.X;
	rGlyph.m_fTX1 = vScale.X + vOffset.X;
	rGlyph.m_fTY0 = vOffset.Y;
	rGlyph.m_fTY1 = vScale.Y + vOffset.Y;
	rGlyph.m_fHeight = (Float)iFullHeight;
	rGlyph.m_fWidth = (Float)iFullWidth;
	rGlyph.m_fXAdvance = font.m_pData->GetGlyphAdvance(uCodePoint);
	rGlyph.m_fXOffset = (Float)(pGlyph->m_iX0 - kiRadiusSDF);
	rGlyph.m_fYOffset = (Float)(pGlyph->m_iY0 - kiRadiusSDF) + font.m_pData->GetAscent(font.m_Overrides);
	rGlyph.m_fTextHeight = kfGlyphHeightSDF;

	rpGlyphTexture = pTexture;

	return true;
}

Bool TexturePacker::Pack(
	const SharedPtr<Texture>& pSource,
	const Rectangle2DInt& source,
	PackerTree2D::NodeID& rNodeID,
	Int32& riX,
	Int32& riY)
{
	Int32 const iWidth = source.GetWidth();
	Int32 const iHeight = source.GetHeight();

	PackerTree2D::NodeID nodeID = 0;
	Int32 iX = 0;
	Int32 iY = 0;
	if (!m_Tree.Pack(iWidth + kPadding, iHeight + kPadding, nodeID, iX, iY))
	{
		return false;
	}

	DoPack(nodeID, pSource, source, Point2DInt(iX + kBorder, iY + kBorder));
	rNodeID = nodeID;
	riX = iX + kBorder;
	riY = iY + kBorder;
	return true;
}

Bool TexturePacker::UnPack(PackerTree2D::NodeID nodeID)
{
	if (m_Tree.UnPack(nodeID))
	{
		m_pInterface->UnPack(nodeID);
		return true;
	}

	return false;
}

void TexturePacker::DoPack(
	PackerTree2D::NodeID nodeID,
	const SharedPtr<Texture>& pSource,
	const Rectangle2DInt& source,
	const Point2DInt& destination)
{
	m_pInterface->Pack(
		nodeID,
		pSource,
		source,
		destination);
}

} // namespace Seoul::Falcon
