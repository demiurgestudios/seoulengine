/**
 * \file FalconRenderDrawer.cpp
 * \brief The Drawer is responsible for building vertex
 * and index buffers and then submitting them to the
 * graphics API.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FalconClipper.h"
#include "FalconRenderDrawer.h"
#include "FalconScalingGrid.h"
#include "FalconTextureCache.h"

namespace Seoul::Falcon
{

namespace Render
{

Drawer::Drawer()
	: m_pState()
	, m_pScalingGrid()
	, m_pActiveColorTexture()
	, m_pActiveDetailTexture()
	, m_vVertices()
	, m_vIndices()
	, m_vPlanarShadowPosition(0, 0)
	, m_PlanarShadowBounds(Rectangle::InverseMax())
	, m_Features()
	, m_fHighestCostInBatch(0.0)
#if SEOUL_ENABLE_CHEATS
	, m_eMode(Mode::kDefault)
	, m_eLastTextureType(FileType::kTexture0)
	, m_fDebugScanningOffset(0.0f)
	, m_bDebugScanning(false)
#endif // /#if SEOUL_ENABLE_CHEATS
{
	m_pScalingGrid.Reset(SEOUL_NEW(MemoryBudgets::Falcon) ScalingGrid(*this));
}

Drawer::~Drawer()
{
}

void Drawer::Begin(State& rState)
{
	m_pState = &rState;
}

void Drawer::End()
{
	Flush();
	m_pState->EndPhase();
	m_pState.Reset();
}

/**
 * Generate quads for rendering the provided text chunk.
 */
void Drawer::DrawTextChunk(
	const TextChunk& textChunk,
	const Matrix2x3& mWorld,
	RGBA color,
	RGBA colorSecondary,
	const Rectangle& objectClipRectangle,
	Bool bShouldClip,
	const SettingsSDF& settingsSDF /*= SettingsSDF()*/,
	TextEffectSettings const* pSettings /*= nullptr*/,
	TextureReference const* pDetailTex /*= nullptr*/)
{
	// Cache the texture cache pointer.
	auto pCache = m_pState->m_pCache.Get();

	// Assume we're using the packer texture and check for
	// state change against the entire vertex count.
	//
	// Using glyph textures is a rare fallback case that
	// we don't expect to happen much.
	const SharedPtr<Texture>& pPackerTexture = pCache->GetPackerTexture();
	UInt32 const uVertexCount = (4u * textChunk.m_uNumberOfCharacters);

	// TODO: The Rectangle never matters since drawing text
	// always requires alpha shaping, however, we should still be
	// computing it for completeness and future development.
	auto const bDetail = (
		nullptr != pSettings && 
		pSettings->m_bDetail &&
		nullptr != pDetailTex &&
		pDetailTex->m_pTexture.IsValid());
	if (bDetail)
	{
		CheckForStateChange(Rectangle(), pPackerTexture, pDetailTex->m_pTexture, DeriveIndexCount(uVertexCount, TriangleListDescription::kTextChunk), uVertexCount, Render::Feature::kDetail);

		// Text rendering needs full detail (multi texture + alpha shaping).
		m_Features.SetDetail();
	}
	else
	{
		CheckForStateChange(Rectangle(), pPackerTexture, SharedPtr<Texture>(), DeriveIndexCount(uVertexCount, TriangleListDescription::kTextChunk), uVertexCount, Render::Feature::kAlphaShape);

		// Text rendering needs alpha shaping.
		m_Features.SetAlphaShape();
	}

	// Cache formatting constants.
	Float const fTextHeightInWorld = Matrix2x3::TransformDirectionY(mWorld, textChunk.m_Format.GetTextHeight());
	Float const fTextHeightOnScreen = fTextHeightInWorld * m_pState->m_fWorldHeightToScreenHeight;

	// Cache color terms per vertex.
	RGBA const colorMultiply(RGBA::Create(color.m_R, color.m_G, color.m_B, color.m_A));
	RGBA const colorMultiply2(RGBA::Create(colorSecondary.m_R, colorSecondary.m_G, colorSecondary.m_B, colorSecondary.m_A));
	ColorAdd const colorAdd(settingsSDF.ToColorAdd(fTextHeightOnScreen));

	// Cache the glyph table we'll use for lookup.
	TextureCache::GlyphsTable& rtGlyphs = pCache->ResolveGlyphTable(textChunk);

	// Initial base vertex calculation.
	UInt16 uBaseVertex = (UInt16)m_vVertices.GetSize();
	m_vVertices.Reserve(m_vVertices.GetSize() + uVertexCount);

	// Progression variables.
	UInt32 uGlyphs = 0;
	Glyph glyph;
	Float fX = textChunk.m_fXOffset;
	Float fY = textChunk.m_fYOffset;

	// Text box culling.
	auto tightGlyphBounds = textChunk.ComputeGlyphBounds();

	// Developer only functionality, conditionally enabled - scan text boxes
	// that are clipped horizontally if enabled.
#if SEOUL_ENABLE_CHEATS
	Bool bDebugScanning = false;
	if (m_bDebugScanning)
	{
		if (textChunk.m_fRightGlyphBorder > objectClipRectangle.m_fRight)
		{
			Float const fDebugScanningOffset = Matrix2x3::TransformDirectionX(mWorld.Inverse(), m_fDebugScanningOffset);
			Float const fWidth = (textChunk.m_fRightGlyphBorder - objectClipRectangle.m_fRight);
			Float const fOffset = ((((Int)(fDebugScanningOffset / fWidth) % 2) == 0)
				? (fWidth - Fmod(fDebugScanningOffset, fWidth))
				: Fmod(fDebugScanningOffset, fWidth));

			fX -= fOffset;
			tightGlyphBounds.m_fLeft -= fOffset;
			tightGlyphBounds.m_fRight -= fOffset;
			bDebugScanning = true;
			bShouldClip = true;
		}
	}
#endif // /#if SEOUL_ENABLE_CHEATS

	// Final text bounds.
	tightGlyphBounds = TransformRectangle(mWorld, tightGlyphBounds);

	// Iterate over each glyph and process.
	for (auto i = textChunk.m_iBegin; textChunk.m_iEnd != i; ++i)
	{
		// Get the glyph data.
		auto pGlyphEntry = pCache->ResolveGlyph(textChunk, rtGlyphs, *i);
		if (nullptr == pGlyphEntry ||
			!pGlyphEntry->m_pTexture.IsValid())
		{
			continue;
		}

		// Handle the unlikely case that the glyph is not packed.
		// This is only expected to happen the first time the glyph is
		// used, until extremely high pressure memory situations,
		// or after background/foreground events, when the graphics
		// hardware performs a reset and invalidates the texture atlas.
		Bool const bPacked = pGlyphEntry->IsPackReady();

		// Unlikely case - need to flush and change textures.
		if (SEOUL_UNLIKELY(
				// Glyph is packed but we're not currently using the packer texture.
				(bPacked && (m_pActiveColorTexture != pPackerTexture)) ||
				// Glyph is not packed and we're not using the glyph's texture.
				(!bPacked && (m_pActiveColorTexture != pGlyphEntry->m_pTexture))))
		{
			// Finalize texture quads we've drawn so far - this will
			// add indices and prepare for flush.
			InternalFinalizeDrawText(textChunk, uBaseVertex, uGlyphs, mWorld, tightGlyphBounds, pSettings, pDetailTex);

			// Flush and update the active texture based on whether
			// we're switching to a packed glyph or not.
			Flush();
			m_pActiveColorTexture = (bPacked ? pPackerTexture : pGlyphEntry->m_pTexture);

			// Restore state for next run of glyphs.
			if (bDetail)
			{
				m_Features.SetDetail();
			}
			else
			{
				m_Features.SetAlphaShape();
			}

			uBaseVertex = m_vVertices.GetSize();
			uGlyphs = 0u;
		}

		// Make a local copy of the glyph data to modify.
		glyph = pGlyphEntry->m_Glyph;

		// Reset texture coordinates in the (unlikely) case that
		// we're rendering the glyph texture directly and
		// not using the atlas.
		if (SEOUL_UNLIKELY(!bPacked))
		{
			glyph.m_fTX0 = 0.0f;
			glyph.m_fTX1 = 1.0f;
			glyph.m_fTY0 = 0.0f;
			glyph.m_fTY1 = 1.0f;
		}

		// Adjust for text height.
		glyph.m_fXAdvance += textChunk.m_Format.GetLetterSpacing();
		if (glyph.m_fTextHeight != textChunk.m_Format.GetTextHeight())
		{
			Float const fRescale = (textChunk.m_Format.GetTextHeight() / glyph.m_fTextHeight);
			glyph.m_fHeight *= fRescale;
			glyph.m_fWidth *= fRescale;
			glyph.m_fXAdvance *= fRescale;
			glyph.m_fXOffset *= fRescale;
			glyph.m_fYOffset *= fRescale;
		}

		// Compute formatting constants.
		Float fX0 = fX + glyph.m_fXOffset;
		Float const fY0 = fY + glyph.m_fYOffset;
		Float fX1 = fX0 + glyph.m_fWidth;
		Float const fY1 = fY0 + glyph.m_fHeight;

		// TODO: We don't normally need left clipping, only when
		// debug scanning is active. Decide if this should be an always
		// on feature.
#if SEOUL_ENABLE_CHEATS
		if (bDebugScanning)
		{
			if (fX1 < objectClipRectangle.m_fLeft)
			{
				fX += glyph.m_fXAdvance;
				continue;
			}

			if (fX0 < objectClipRectangle.m_fLeft)
			{
				Float const fNewX0 = objectClipRectangle.m_fLeft;
				Float const fRescale = (fNewX0 - fX0) / (fX1 - fX0);
				fX0 = fNewX0;
				glyph.m_fTX0 = fRescale * (glyph.m_fTX1 - glyph.m_fTX0) + glyph.m_fTX0;
			}
		}
#endif // /#if SEOUL_ENABLE_CHEATS

		// TODO: Do this during chunk generation, instead of with every draw operation.
		if (bShouldClip)
		{
			// Culled, break out of processing.
			if (fX0 >= objectClipRectangle.m_fRight)
			{
				break;
			}

			// Clipped, apply clipping to the glyph quad.
			if (fX1 > objectClipRectangle.m_fRight)
			{
				Float const fNewX1 = objectClipRectangle.m_fRight;
				Float const fRescale = (fNewX1 - fX0) / (fX1 - fX0);
				fX1 = fNewX1;
				glyph.m_fTX1 = fRescale * (glyph.m_fTX1 - glyph.m_fTX0) + glyph.m_fTX0;
			}
		}

		// Generate quads.
		m_vVertices.PushBack(ShapeVertex::Create(fX0, fY0, colorMultiply, colorAdd, glyph.m_fTX0, glyph.m_fTY0));
		m_vVertices.PushBack(ShapeVertex::Create(fX0, fY1, colorMultiply2, colorAdd, glyph.m_fTX0, glyph.m_fTY1));
		m_vVertices.PushBack(ShapeVertex::Create(fX1, fY1, colorMultiply2, colorAdd,glyph.m_fTX1, glyph.m_fTY1));
		m_vVertices.PushBack(ShapeVertex::Create(fX1, fY0, colorMultiply, colorAdd, glyph.m_fTX1, glyph.m_fTY0));

		// Advance.
		++uGlyphs;

		fX += glyph.m_fXAdvance;
	}

	InternalFinalizeDrawText(textChunk, uBaseVertex, uGlyphs, mWorld, tightGlyphBounds, pSettings, pDetailTex);
}

void Drawer::CheckForStateChange(
	const Rectangle& worldBoundsPreClip,
	const SharedPtr<Texture>& pColorTexture,
	const SharedPtr<Texture>& pDetailTexture,
	UInt32 uIndexCount,
	UInt32 uVertexCount,
	Feature::Enum eFeature)
{
	using namespace Render;
	using namespace Render::Feature;

	// Adjust counts for clipping.
	m_pState->m_pClipStack->AddWorstCaseClippingCounts(uIndexCount, uVertexCount);

	// Compute screen area. Clamp to world bounds so that
	// a largeness factor of 1.0 turns off batch breaking.
	Double const fBaseCost = Min((Double)worldBoundsPreClip.GetWidth() * (Double)worldBoundsPreClip.GetHeight(), (Double)m_pState->m_fWorldCullScreenArea);

	// TODO: Note that the overfill computation here depends on an internal detail
	// of Features::Cost() - TransformNVertices() can potentially add both kColorMultiply
	// and/or kColorAdd to the features of a draw call *after* CheckForStateChange has
	// been called. As a result, we assume and rely upon the fact that neither multiply
	// nor add is factored into the cost of a shader (we consider the multiply+additive
	// shader to be our 0 cost baseline shader).
	//
	// We break a batch if:
	// - feature requirements are incompatible
	// - the overfill of this single draw would exceed the overfill factor (due to the batch at higher cost).
	// - the overfill of the largest draw in the batch would exceed the overfill factor (due to
	//   the draw at higher cost).
	Bool bBatchBreak = false;
	auto const uCurrent = m_Features.GetBits();
	auto const uNew = (UInt32)eFeature;

	// Break if existing features do not encompass the new feature.
	if (!Features::Compatible(uCurrent, uNew))
	{
		bBatchBreak = true;
	}
	else
#if SEOUL_ENABLE_CHEATS
	if (m_bDebugEnableOverfillOptimizer)
#endif // /#if SEOUL_ENABLE_CHEATS
	{
		// Estimate of cost per.
		auto const iCurrentCostUnit = Features::Cost(uCurrent);
		auto const iNewCostUnit = Features::Cost(uNew);

		// If we're going to increase the unit cost of the batch, check increase in
		// overfill against threshold.
		if (iNewCostUnit > iCurrentCostUnit)
		{
			// Enhance the cost by the overfill delta - this is why the value is called
			// "cost" and not "area", since once overfill is introduced, it becomes
			// a multiple of the base cost.
			m_fHighestCostInBatch = Max(m_fHighestCostInBatch * (Double)(iNewCostUnit - iCurrentCostUnit), fBaseCost);
			if (m_fHighestCostInBatch > m_pState->m_fMaxCostInBatchFromOverfill)
			{
				// Break the batch if we've exceeded the overfill threshold.
				bBatchBreak = true;
			}
		}
		// Otherwise, if the batch will increase the unit cost of this draw,
		// check overfill against threshold.
		else if (iCurrentCostUnit > iNewCostUnit)
		{
			// We add the overfilled cost of the next draw to the overfill total.
			m_fHighestCostInBatch = Max(m_fHighestCostInBatch, (fBaseCost * (Double)(iCurrentCostUnit - iNewCostUnit)));
			if (m_fHighestCostInBatch > m_pState->m_fMaxCostInBatchFromOverfill)
			{
				// Break the batch if we've exceeded the overfill threshold.
				bBatchBreak = true;
			}
		}
		// Just update cost tracking.
		else
		{
			m_fHighestCostInBatch = Max(m_fHighestCostInBatch, fBaseCost);
		}
	}

	// Flush is needed on a texture change, or if we will exceed
	// our vertex buffer or index buffer limits.
	if (bBatchBreak ||
		m_pActiveColorTexture != pColorTexture ||
		m_pActiveDetailTexture != pDetailTexture ||
		(m_vVertices.GetSize() + uVertexCount) > m_pState->m_Settings.m_uMaxVertexCountBatch ||
		(m_vIndices.GetSize() + uIndexCount) > m_pState->m_Settings.m_uMaxIndexCountBatch)
	{
		Flush();
		m_pActiveColorTexture = pColorTexture;
		m_pActiveDetailTexture = pDetailTexture;
	}

#if SEOUL_ENABLE_CHEATS
	// By default, set texture0 as the type. May be override by a CheckForStateChange(reference,...) call.
	m_eLastTextureType = FileType::kTexture0;
#endif // /#if SEOUL_ENABLE_CHEATS
}

/**
 * Called to tie off a run of text drawing - typically called
 * once per DrawTextChunk() call, unless we need to fall back
 * to individual glyph textures, due to glyphs not being
 * packed yet.
 */
void Drawer::InternalFinalizeDrawText(
	const TextChunk& textChunk,
	UInt32 uBaseVertex,
	UInt32 uGlyphs,
	const Matrix2x3& mWorld,
	const Rectangle& tightGlyphBounds,
	TextEffectSettings const* pSettings,
	TextureReference const* pDetailTex)
{
	// Before generating indices, transform, and clip, generate
	// the secondary texture coordinates for a face texture,
	// if enabled.
	InternalFinalizeDrawTextSecondaryTexCoords(mWorld, textChunk, uBaseVertex, uGlyphs, pSettings, pDetailTex);

	UInt32 const uBaseIndex = m_vIndices.GetSize();
	m_vIndices.ResizeNoInitialize(m_vIndices.GetSize() + 6 * uGlyphs);
	for (UInt32 i = 0; i < uGlyphs; ++i)
	{
		UInt32 uIndexOffset = (uBaseIndex + (i * 6));
		UInt16 uVertexOffset = (UInt16)(uBaseVertex + (i * 4));
		m_vIndices[uIndexOffset + 0] = uVertexOffset + 0;
		m_vIndices[uIndexOffset + 1] = uVertexOffset + 1;
		m_vIndices[uIndexOffset + 2] = uVertexOffset + 2;
		m_vIndices[uIndexOffset + 3] = uVertexOffset + 0;
		m_vIndices[uIndexOffset + 4] = uVertexOffset + 2;
		m_vIndices[uIndexOffset + 5] = uVertexOffset + 3;
	}

	TransformLastNVertices(mWorld, uGlyphs * 4);
	ClipLastN(TriangleListDescription::kTextChunk, tightGlyphBounds, uGlyphs * 6, uGlyphs * 4);
}

void Drawer::InternalFinalizeDrawTextSecondaryTexCoords(
	const Matrix2x3& mWorld,
	const TextChunk& textChunk,
	UInt32 uBaseVertex,
	UInt32 uGlyphs,
	TextEffectSettings const* pSettings,
	TextureReference const* pDetailTex)
{
	// Generation of secondary texture coordinates for text chunks that use a detail
	// (face) texture.
	if (nullptr == pSettings || 
		!pSettings->m_bDetail || 
		nullptr == pDetailTex ||
		!pDetailTex->m_pTexture.IsValid() ||
		0u == uGlyphs)
	{
		// No detail texture, nothing to do.
		return;
	}

	// Transform into local space, values are treated as world space.
	auto const vAnimOffsetLocal = Matrix2x3::TransformDirection(
		mWorld.Inverse(),
		-pSettings->m_vDetailAnimOffsetInWorld);

	// Apply to the inner glyph, exclude the SDF region. Diameter
	// here, since we're effectively using the range [-1, 1] of the
	// texture space with wrapping (instead of the expected [0, 1]).
	auto const fBorder = (Float)kiDiameterSDF * (textChunk.m_Format.GetTextHeight() / kfGlyphHeightSDF);

	// Handling for the case where a face texture is mapped to each individual glyph/character.
	if (TextEffectDetailMode::Character == pSettings->m_eDetailMode)
	{
		// Base UV is the accumulated fixed offset and animated offset.
		auto const fBaseTU0 = -pSettings->m_vDetailOffset.X;
		auto const fBaseTU1 = fBaseTU0 + 1.0f;
		auto const fBaseTV0 = -pSettings->m_vDetailOffset.Y;
		auto const fBaseTV1 = fBaseTV0 + 1.0f;

		// Cheapest case, stretch maps [0, 1] on both axes.
		if (TextEffectDetailStretchMode::Stretch == pSettings->m_eDetailStretchMode)
		{
			for (UInt32 i = 0u; i < uGlyphs; ++i)
			{
				auto const u = (uBaseVertex + i * 4u);

				// Width/height of the glyph.
				auto const fWidth = (m_vVertices[u + 2].m_vP.X - m_vVertices[u + 0].m_vP.X);
				auto const fHeight = (m_vVertices[u + 2].m_vP.Y - m_vVertices[u + 0].m_vP.Y);

				// Skip further processing of the glyph if it is zero sized.
				if (IsZero(fWidth) || IsZero(fHeight))
				{
					continue;
				}

				// Compute adjustment to factor out the SDF
				// overdraw radius. The quads are larger than the visible
				// glyph to allow outline/anti-alias effects, but we
				// want to exclude that region when aligning the face texture.
				auto const fAdjustU = (fBorder / fWidth);
				auto const fAdjustV = (fBorder / fHeight);

				// Finaly UV is base adjusted by SDF region.
				auto fTU0 = fBaseTU0 - fAdjustU;
				auto fTU1 = fBaseTU1 + fAdjustU;
				auto fTV0 = fBaseTV0 - fAdjustV;
				auto fTV1 = fBaseTV1 + fAdjustV;

				// Anim offset is specified in world space, so compute
				// and adjust now. Must be done after adjustments for
				// stretch mode.
				if (pSettings->m_vDetailAnimOffsetInWorld.X != 0.0f)
				{
					auto fAdjust = vAnimOffsetLocal.X * ((fTU1 - fTU0) / fWidth);

					// Convenience and numerical robustness, "circle" clamp to [-1, 1] (values
					// are in wrapped texture sampling on [0, 1], so [-1, 1] produces equivalent
					// results to values outside this range).
					while (fAdjust > 1.0f) { fAdjust -= 2.0f; }
					while (fAdjust < -1.0f) { fAdjust += 2.0f; }

					fTU0 += fAdjust;
					fTU1 += fAdjust;
				}
				if (pSettings->m_vDetailAnimOffsetInWorld.Y != 0.0f)
				{
					auto fAdjust = vAnimOffsetLocal.Y * ((fTV1 - fTV0) / fHeight);

					// Convenience and numerical robustness, "circle" clamp to [-1, 1] (values
					// are in wrapped texture sampling on [0, 1], so [-1, 1] produces equivalent
					// results to values outside this range).
					while (fAdjust > 1.0f) { fAdjust -= 2.0f; }
					while (fAdjust < -1.0f) { fAdjust += 2.0f; }

					fTV0 += fAdjust;
					fTV1 += fAdjust;
				}

				// Apply.
				m_vVertices[u + 0].m_vT.Z = fTU0;
				m_vVertices[u + 0].m_vT.W = fTV0;
				m_vVertices[u + 1].m_vT.Z = fTU0;
				m_vVertices[u + 1].m_vT.W = fTV1;
				m_vVertices[u + 2].m_vT.Z = fTU1;
				m_vVertices[u + 2].m_vT.W = fTV1;
				m_vVertices[u + 3].m_vT.Z = fTU1;
				m_vVertices[u + 3].m_vT.W = fTV0;
			}
		}
		// In either fit mode, we need to rescale an axis based
		// on the other axis, per glyph, based on the texture and
		// the glyph.
		else
		{
			TextureMetrics metrics;
			if (pDetailTex->m_pTexture->ResolveTextureMetrics(metrics) &&
				metrics.m_iWidth > 0 &&
				metrics.m_iHeight > 0)
			{
				auto const fTarget = (Float)metrics.m_iWidth / (Float)metrics.m_iHeight;
				for (UInt32 i = 0u; i < uGlyphs; ++i)
				{
					auto const u = (uBaseVertex + i * 4u);

					// Width/height of the glyph.
					auto const fWidth = (m_vVertices[u + 2].m_vP.X - m_vVertices[u + 0].m_vP.X);
					auto const fHeight = (m_vVertices[u + 2].m_vP.Y - m_vVertices[u + 0].m_vP.Y);

					// Skip further processing of the glyph if it is zero sized.
					if (IsZero(fWidth) || IsZero(fHeight))
					{
						continue;
					}

					// Compute adjustment to factor out the SDF
					// overdraw radius.
					auto const fAdjustU = fBorder / fWidth;
					auto const fAdjustV = fBorder / fHeight;

					auto fTU0 = fBaseTU0 - fAdjustU;
					auto fTU1 = fBaseTU1 + fAdjustU;
					auto fTV0 = fBaseTV0 - fAdjustV;
					auto fTV1 = fBaseTV1 + fAdjustV;

					// Now compute compensation for aspect ratio. Rescale
					// either U or V based on mode and the other axis.
					auto const fCurrent = (fWidth / fHeight);
					if (TextEffectDetailStretchMode::FitWidth == pSettings->m_eDetailStretchMode)
					{
						if (!IsZero(fCurrent))
						{
							fTV1 = (((fTV1 - fTV0) * fTarget) / fCurrent) + fTV0;
						}
					}
					else
					{
						if (!IsZero(fTarget))
						{
							fTU1 = (((fTU1 - fTU0) * fCurrent) / fTarget) + fTU0;
						}
					}

					// Anim offset is specified in world space, so compute
					// and adjust now. Must be done after adjustments for
					// stretch mode.
					if (pSettings->m_vDetailAnimOffsetInWorld.X != 0.0f)
					{
						auto fAdjust = vAnimOffsetLocal.X * ((fTU1 - fTU0) / fWidth);

						// Convenience and numerical robustness, "circle" clamp to [-1, 1] (values
						// are in wrapped texture sampling on [0, 1], so [-1, 1] produces equivalent
						// results to values outside this range).
						while (fAdjust > 1.0f) { fAdjust -= 2.0f; }
						while (fAdjust < -1.0f) { fAdjust += 2.0f; }

						fTU0 += fAdjust;
						fTU1 += fAdjust;
					}
					if (pSettings->m_vDetailAnimOffsetInWorld.Y != 0.0f)
					{
						auto fAdjust = vAnimOffsetLocal.Y * ((fTV1 - fTV0) / fHeight);

						// Convenience and numerical robustness, "circle" clamp to [-1, 1] (values
						// are in wrapped texture sampling on [0, 1], so [-1, 1] produces equivalent
						// results to values outside this range).
						while (fAdjust > 1.0f) { fAdjust -= 2.0f; }
						while (fAdjust < -1.0f) { fAdjust += 2.0f; }

						fTV0 += fAdjust;
						fTV1 += fAdjust;
					}

					// Apply.
					m_vVertices[u + 0].m_vT.Z = fTU0;
					m_vVertices[u + 0].m_vT.W = fTV0;
					m_vVertices[u + 1].m_vT.Z = fTU0;
					m_vVertices[u + 1].m_vT.W = fTV1;
					m_vVertices[u + 2].m_vT.Z = fTU1;
					m_vVertices[u + 2].m_vT.W = fTV1;
					m_vVertices[u + 3].m_vT.Z = fTU1;
					m_vVertices[u + 3].m_vT.W = fTV0;
				}
			}
			else
			{
				// TODO: Should never happen, probably SEOUL_FAIL() here,
				// since if we have a TextureReference to the detail texture,
				// metrics acquisition is always expected to succeed.
				return;
			}
		}
	}
	// Handling for the case where a face texture is mapped across the entire text chunk.
	else if (TextEffectDetailMode::Word == pSettings->m_eDetailMode)
	{
		// Base UV is the accumulated fixed offset and animated offset.
		auto fWordTU0 = -pSettings->m_vDetailOffset.X;
		auto fWordTU1 = fWordTU0 + 1.0f;
		auto fWordTV0 = -pSettings->m_vDetailOffset.Y;
		auto fWordTV1 = fWordTV0 + 1.0f;

		// Compute the min/max of all the generated vertices of the text chunk.
		Vector2D vMin( FloatMax,  FloatMax);
		Vector2D vMax(-FloatMax, -FloatMax);
		for (UInt32 i = 0u; i < uGlyphs; ++i)
		{
			auto const u = (uBaseVertex + i * 4u);

			vMin = Vector2D::Min(vMin, m_vVertices[u + 0].m_vP);
			vMax = Vector2D::Max(vMax, m_vVertices[u + 2].m_vP);
		}

		// Width and height of the text chunk vertices.
		auto const fWidth = (vMax.X - vMin.X);
		auto const fHeight = (vMax.Y - vMin.Y);

		// Done with processing of the entire chunk is 0 sized.
		if (IsZero(fWidth) || IsZero(fHeight))
		{
			return;
		}

		// Compute adjustment to factor out the SDF
		// overdraw radius.
		auto const fAdjustU = fBorder / fWidth;
		auto const fAdjustV = fBorder / fHeight;

		// Adjust base vertices to remove the SDF region.
		fWordTU0 -= fAdjustU;
		fWordTU1 += fAdjustU;
		fWordTV0 -= fAdjustV;
		fWordTV1 += fAdjustV;

		// If mode other than stretch, compensate sampling to maintain
		// desired aspect ratio.
		if (TextEffectDetailStretchMode::Stretch != pSettings->m_eDetailStretchMode)
		{
			TextureMetrics metrics;
			if (pDetailTex->m_pTexture->ResolveTextureMetrics(metrics) &&
				metrics.m_iWidth > 0 &&
				metrics.m_iHeight > 0)
			{
				auto const fTarget = (Float)metrics.m_iWidth / (Float)metrics.m_iHeight;
				
				// Now compute compensation for aspect ratio. Rescale
				// either U or V based on mode and the other axis.
				auto const fCurrent = (fWidth / fHeight);
				if (TextEffectDetailStretchMode::FitWidth == pSettings->m_eDetailStretchMode)
				{
					if (!IsZero(fCurrent))
					{
						fWordTV1 = (((fWordTV1 - fWordTV0) * fTarget) / fCurrent) + fWordTV0;
					}
				}
				else
				{
					if (!IsZero(fTarget))
					{
						fWordTU1 = (((fWordTU1 - fWordTU0) * fCurrent) / fTarget) + fWordTU0;
					}
				}
			}
			else
			{
				// TODO: Should never happen, probably SEOUL_FAIL() here,
				// since if we have a TextureReference to the detail texture,
				// metrics acquisition is always expected to succeed.
				return;
			}
		}

		// Anim offset is specified in world space, so compute
		// and adjust now. Must be done after adjustments for
		// stretch mode.
		if (pSettings->m_vDetailAnimOffsetInWorld.X != 0.0f)
		{
			auto fAdjust = vAnimOffsetLocal.X * ((fWordTU1 - fWordTU0) / fWidth);

			// Convenience and numerical robustness, "circle" clamp to [-1, 1] (values
			// are in wrapped texture sampling on [0, 1], so [-1, 1] produces equivalent
			// results to values outside this range).
			while (fAdjust > 1.0f) { fAdjust -= 2.0f; }
			while (fAdjust < -1.0f) { fAdjust += 2.0f; }

			fWordTU0 += fAdjust;
			fWordTU1 += fAdjust;
		}
		if (pSettings->m_vDetailAnimOffsetInWorld.Y != 0.0f)
		{
			auto fAdjust = vAnimOffsetLocal.Y * ((fWordTV1 - fWordTV0) / fHeight);

			// Convenience and numerical robustness, "circle" clamp to [-1, 1] (values
			// are in wrapped texture sampling on [0, 1], so [-1, 1] produces equivalent
			// results to values outside this range).
			while (fAdjust > 1.0f) { fAdjust -= 2.0f; }
			while (fAdjust < -1.0f) { fAdjust += 2.0f; }

			fWordTV0 += fAdjust;
			fWordTV1 += fAdjust;
		}

		// Now that we've computed the UV for the entire word, we apply to each
		// glyph by lerp, based on glyph corner positions vs. word corner positions.
		for (UInt32 i = 0u; i < uGlyphs; ++i)
		{
			auto const u = (uBaseVertex + i * 4u);

			// Convenience.
			auto const fPX0 = m_vVertices[u + 0].m_vP.X;
			auto const fPX1 = m_vVertices[u + 2].m_vP.X;
			auto const fPY0 = m_vVertices[u + 0].m_vP.Y;
			auto const fPY1 = m_vVertices[u + 2].m_vP.Y;
			
			// Compute UV.
			auto const fTU0 = Lerp(fWordTU0, fWordTU1, Clamp((fPX0 - vMin.X) / fWidth, 0.0f, 1.0f));
			auto const fTU1 = Lerp(fWordTU0, fWordTU1, Clamp((fPX1 - vMin.X) / fWidth, 0.0f, 1.0f));
			auto const fTV0 = Lerp(fWordTV0, fWordTV1, Clamp((fPY0 - vMin.Y) / fHeight, 0.0f, 1.0f));
			auto const fTV1 = Lerp(fWordTV0, fWordTV1, Clamp((fPY1 - vMin.Y) / fHeight, 0.0f, 1.0f));

			// Apply.
			m_vVertices[u + 0].m_vT.Z = fTU0;
			m_vVertices[u + 0].m_vT.W = fTV0;
			m_vVertices[u + 1].m_vT.Z = fTU0;
			m_vVertices[u + 1].m_vT.W = fTV1;
			m_vVertices[u + 2].m_vT.Z = fTU1;
			m_vVertices[u + 2].m_vT.W = fTV1;
			m_vVertices[u + 3].m_vT.Z = fTU1;
			m_vVertices[u + 3].m_vT.W = fTV0;
		}
	}
	else
	{
		SEOUL_FAIL("Out-of-sync enum, unexpected TextEffectDetailMode.");
		return;
	}

	// If the detail texture is in an atlas, we need to adjust the secondary texture coordinates
	// that we just generated.
	if (nullptr != pDetailTex)
	{
		AdjustSecondaryTexCoordsForLastNVertices(*pDetailTex, uGlyphs * 4);
	}
}

void Drawer::PreClip(
	TriangleListDescription eDescription,
	UInt32 uIndexCount,
	UInt32 uVertexCount)
{
	// Prior to clipping, apple shadow transformation, if enabled.
	if (SEOUL_UNLIKELY(0 != m_pState->m_iInPlanarShadowRender))
	{
		ShadowProjectLastNVertices(uVertexCount);
	}
}

void Drawer::ClipLastN(
	TriangleListDescription eDescription,
	UInt32 uIndexCount,
	UInt32 uVertexCount)
{
	PreClip(eDescription, uIndexCount, uVertexCount);

	// Capture starting point prior to clip.
	UInt32 const uBeginI = (m_vIndices.GetSize() - uIndexCount);
	UInt32 const uBeginV = (m_vVertices.GetSize() - uVertexCount);

	// Perform the actual clip.
	m_pState->m_pClipStack->MeshClip(
		eDescription,
		m_vIndices,
		m_vVertices,
		(Int32)uIndexCount,
		(Int32)uVertexCount);

	// Clipping may modify m_vIndices and m_vVertices, we means we need to recompute
	// uIndexCount and uVertexCount.
	uIndexCount = (m_vIndices.GetSize() - uBeginI);
	uVertexCount = (m_vVertices.GetSize() - uBeginV);

	PostClip(eDescription, uIndexCount, uVertexCount);
}

void Drawer::ClipLastN(
	TriangleListDescription eDescription,
	const Rectangle& vertexBounds,
	UInt32 uIndexCount,
	UInt32 uVertexCount)
{
	PreClip(eDescription, uIndexCount, uVertexCount);

	// Capture starting point prior to clip.
	UInt32 const uBeginI = (m_vIndices.GetSize() - uIndexCount);
	UInt32 const uBeginV = (m_vVertices.GetSize() - uVertexCount);

	// Perform the actual clip.
	m_pState->m_pClipStack->MeshClip(
		eDescription,
		vertexBounds,
		m_vIndices,
		m_vVertices,
		(Int32)uIndexCount,
		(Int32)uVertexCount);

	// Clipping may modify m_vIndices and m_vVertices, we means we need to recompute
	// uIndexCount and uVertexCount.
	uIndexCount = (m_vIndices.GetSize() - uBeginI);
	uVertexCount = (m_vVertices.GetSize() - uBeginV);

	PostClip(eDescription, uIndexCount, uVertexCount);
}

#if SEOUL_ENABLE_CHEATS
static inline RGBA GetTextureResolutionColor(FileType e)
{
	switch (e)
	{
	case FileType::kTexture4: return RGBA::Create(0, 255, 0, 255);
	case FileType::kTexture3: return RGBA::Create(89, 171, 0, 255);
	case FileType::kTexture2: return RGBA::Create(143, 118, 0, 255);
	case FileType::kTexture1: return RGBA::Create(201, 54, 0, 255);
	case FileType::kTexture0: return RGBA::Create(255, 0, 0, 255);
	default: return RGBA::Create(255, 0, 255, 255);
	};
}
#endif // /#if SEOUL_ENABLE_CHEATS

void Drawer::PostClip(
	TriangleListDescription eDescription,
	UInt32 uIndexCount,
	UInt32 uVertexCount)
{
	UInt32 const uVertices = m_vVertices.GetSize();

	// Early out if no vertices.
	if (0u == uVertices)
	{
		return;
	}

	// Starting vertex.
	UInt32 const uVertex = (uVertices - uVertexCount);

	// 3D depth was specified per vertex.
	auto const fDepth3D = m_pState->GetModifiedDepth3D();
	if (SEOUL_UNLIKELY(m_vDepths3D.GetSize() == uVertices))
	{
		// Clamp depth values.
		for (UInt32 i = uVertex; i < uVertices; ++i)
		{
			auto& rf = m_vDepths3D[i];
			rf = Clamp(rf, 0.0f, 0.999f);
		}

		// Compute shadow bounds here with a reprojection to account for larger size.
		if (0 != m_pState->m_iInPlanarShadowRender)
		{
			Vector2D const vCenter = m_pState->m_WorldCullRectangle.GetCenter();
			Float const fFactor = m_pState->GetPerspectiveFactor();
			for (UInt32 i = uVertex; i < uVertices; ++i)
			{
				auto const fD = m_vDepths3D[i];
				Float const fW = 1.0f / Clamp(1.0f - (fD * fFactor), 1e-4f, 1.0f);
				m_PlanarShadowBounds.AbsorbPoint((m_vVertices[i].m_vP - vCenter) * fW + vCenter);
			}
		}
	}
	// Post clipping, apply 3D projection to the vertices, if present, and also
	// calculate the shadow bounds with
	else if (SEOUL_UNLIKELY(fDepth3D > 1e-4f))
	{
		Float const fOneMinusW = Clamp(fDepth3D, 0.0f, 0.999f);

		// Depths are only expanded when applying a non-zero depth,
		// so resize now to the size of m_vVertices. We need to clear
		// any unallocated region up to uVertex to 0 - these would be
		// 2D vertices that have been intermixed with 3D vertices.
		UInt32 const uClearStart = m_vDepths3D.GetSize();
		m_vDepths3D.ResizeNoInitialize(uVertices);

		// 0 clear up to the range.
		if (uVertex > uClearStart)
		{
			memset(m_vDepths3D.Get(uClearStart), 0, (uVertex - uClearStart) * sizeof(Depths3D::ValueType));
		}

		// Apply the current depth to the appropriate range.
		for (UInt32 i = uVertex; i < uVertices; ++i)
		{
			m_vDepths3D[i] = fOneMinusW;
		}

		// Compute shadow bounds here with a reprojection to account for larger size.
		if (0 != m_pState->m_iInPlanarShadowRender)
		{
			Float const fW = 1.0f / Clamp(1.0f - (fDepth3D * m_pState->GetPerspectiveFactor()), 1e-4f, 1.0f);
			Vector2D const vCenter = m_pState->m_WorldCullRectangle.GetCenter();
			for (UInt32 i = uVertex; i < uVertices; ++i)
			{
				m_PlanarShadowBounds.AbsorbPoint((m_vVertices[i].m_vP - vCenter) * fW + vCenter);
			}
		}
	}
	// Accumulate shadow bounds.
	else if (SEOUL_UNLIKELY(0 != m_pState->m_iInPlanarShadowRender))
	{
		for (UInt32 i = uVertex; i < uVertices; ++i)
		{
			// Track overall bounds.
			m_PlanarShadowBounds.AbsorbPoint(m_vVertices[i].m_vP);
		}
	}

	// TODO: Cramming a bit too much stuff into the "clipper" functions, because it is
	// convenient to do so.
#if SEOUL_ENABLE_CHEATS
	// When enabled, force the color components to the appropriate values.
	if (m_eMode == Mode::kTextureResolution)
	{
		auto const c = GetTextureResolutionColor(m_eLastTextureType);
		for (UInt32 i = uVertex; i < uVertices; ++i)
		{
			m_vVertices[i].m_ColorMultiply *= c;
		}
	}
#endif // /#if SEOUL_ENABLE_CHEATS
}

void Drawer::ShadowProjectLastNVertices(UInt32 uVertexCount)
{
	auto const& settings = *m_pState->m_pStage3DSettings;

	auto const vPlane = settings.m_Shadow.ComputeShadowPlane(m_vPlanarShadowPosition);
	Float const fAlpha = settings.m_Shadow.GetAlpha();

	Vertices::SizeType const zSize = m_vVertices.GetSize();
	for (Vertices::SizeType i = (zSize - uVertexCount); i < zSize; ++i)
	{
		Vector4D const vProjection(settings.m_Shadow.ShadowProject(vPlane, Vector3D(m_vVertices[i].m_vP, 0.0f)));
		m_vVertices[i].m_ColorAdd = ColorAdd::Create(RGBA::TransparentBlack());
		m_vVertices[i].m_ColorMultiply = RGBA::Create(0, 0, 0, (UInt8)((m_vVertices[i].m_ColorMultiply.m_A * fAlpha) + 0.5f));
		m_vVertices[i].m_vP = vProjection.GetXY();
	}
}

} // namespace Render

} // namespace Seoul::Falcon
