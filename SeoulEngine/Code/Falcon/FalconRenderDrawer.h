/**
 * \file FalconRenderDrawer.h
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

#pragma once
#ifndef FALCON_RENDER_DRAWER_H
#define FALCON_RENDER_DRAWER_H

#include "Delegate.h"
#include "FalconRenderFeatures.h"
#include "FalconRenderMode.h"
#include "FalconRenderState.h"
#include "FalconStage3DSettings.h"
#include "FalconTextChunk.h"
#include "FalconTexture.h"
#include "FalconTriangleListDescription.h"
#include "FalconTypes.h"
#include "Prereqs.h"
#include "UnsafeBuffer.h"
namespace Seoul { namespace Falcon { class ScalingGrid; } }
namespace Seoul { namespace Falcon { class TextureCache; } }
namespace Seoul { namespace Falcon { struct TextureCacheSettings; } }
namespace Seoul { namespace Falcon { namespace Render { class State; } } }

namespace Seoul::Falcon
{

namespace Render
{

/** Hi/low settings for SDF rendering used on text. */
struct SettingsSDF SEOUL_SEALED
{
	/** Base threshold - low is (this-value - blur). */
	static const UInt8 kuBaseThreshold = 192u;

	/** Base tolerance - this provides minimum anti-aliasing. */
	static const UInt8 kuBaseTolerance = 15u;

	explicit SettingsSDF(UInt8 uThreshold = kuBaseThreshold, UInt8 uTolerance = kuBaseTolerance)
		: m_uThreshold(uThreshold)
		, m_uTolerance(uTolerance)
	{
	}

	ColorAdd ToColorAdd(Float fTextHeightOnScreen) const
	{
		// Little odd - to maintain sharpness under normal
		// conditions, we scale the tolerance up to the base threshold,
		// then leave the rest at its specified value.
		Float32 const fBaseTolerance = (Float32)Min(m_uTolerance, kuBaseTolerance);

		Float32 const fTolerance =
			(fBaseTolerance * (kfGlyphHeightSDF / fTextHeightOnScreen)) + Max((Float)m_uTolerance - fBaseTolerance, 0.0f);

		UInt8 const uTolerance = (UInt8)Clamp(
			Round(fTolerance),
			1.0f,
			Min(255.0f - (Float)m_uThreshold, (Float)m_uThreshold));

		return ColorAdd::Create(
			m_uThreshold - uTolerance,
			m_uThreshold + uTolerance,
			0,
			128u); // Special value used to indicate alpha shape rendering.
	}

private:
	UInt8 m_uThreshold;
	UInt8 m_uTolerance;
}; // struct SettingsSDF

class Drawer SEOUL_SEALED
{
public:
	typedef UnsafeBuffer<Float32, MemoryBudgets::Falcon> Depths3D;
	typedef UnsafeBuffer<UInt16, MemoryBudgets::Falcon> Indices;
	typedef UnsafeBuffer<ShapeVertex, MemoryBudgets::Falcon> Vertices;

	Drawer();
	~Drawer();

	void Begin(State& rState);
	void End();

	/**
	 * @return Shared renderer state. The state
	 * instance is used across the Poser, Drawer, and
	 * Optimizer.
	 */
	const State& GetState() const
	{
		return *m_pState;
	}

	void DrawTextChunk(
		const TextChunk& textChunk,
		const Matrix2x3& mWorld,
		RGBA color,
		const Rectangle& objectClipRectangle,
		Bool bShouldClip,
		const SettingsSDF& settingsSDF = SettingsSDF(),
		TextEffectSettings const* pSettings = nullptr,
		TextureReference const* pDetailTex = nullptr)
	{
		DrawTextChunk(
			textChunk,
			mWorld,
			color,
			color,
			objectClipRectangle,
			bShouldClip,
			settingsSDF,
			pSettings,
			pDetailTex);
	}

	void DrawTextChunk(
		const TextChunk& textChunk,
		const Matrix2x3& mWorld,
		RGBA color,
		RGBA colorSecondary,
		const Rectangle& objectClipRectangle,
		Bool bShouldClip,
		const SettingsSDF& settingsSDF = SettingsSDF(),
		TextEffectSettings const* pSettings = nullptr,
		TextureReference const* pDetailTex = nullptr);

	void DrawTriangleList(
		const Rectangle& worldBoundsPreClip,
		const TextureReference& reference,
		const Matrix2x3& mWorldTransform,
		ShapeVertex const* pVertices,
		UInt32 uVertexCount,
		TriangleListDescription eDescription,
		Feature::Enum eFeature)
	{
		CheckForStateChange(worldBoundsPreClip, reference, nullptr, DeriveIndexCount(uVertexCount, eDescription), uVertexCount, eFeature);
		m_Features.SetFeature(eFeature);
		UInt32 const uIndexCount = AppendVertices(pVertices, uVertexCount, eDescription);
		TransformLastNVertices(mWorldTransform, uVertexCount);
		AdjustTexCoordsForLastNVertices(reference, uVertexCount);
		ClipLastN(eDescription, worldBoundsPreClip, uIndexCount, uVertexCount);
	}

	void DrawTriangleList(
		const Rectangle& worldBoundsPreClip,
		const TextureReference& reference,
		const Matrix2x3& mWorldTransform,
		const ColorTransformWithAlpha& cxTransform,
		ShapeVertex const* pVertices,
		UInt32 uVertexCount,
		TriangleListDescription eDescription,
		Feature::Enum eFeature)
	{
		CheckForStateChange(worldBoundsPreClip, reference, nullptr, DeriveIndexCount(uVertexCount, eDescription), uVertexCount, eFeature);
		m_Features.SetFeature(eFeature);
		UInt32 const uIndexCount = AppendVertices(pVertices, uVertexCount, eDescription);
		TransformLastNVertices(mWorldTransform, uVertexCount);
		TransformLastNVertices(cxTransform, uVertexCount);
		AdjustTexCoordsForLastNVertices(reference, uVertexCount);
		ClipLastN(eDescription, worldBoundsPreClip, uIndexCount, uVertexCount);
	}

	void DrawTriangleList(
		const Rectangle& worldBoundsPreClip,
		const TextureReference& reference,
		const Matrix2x3& mWorldTransform,
		UInt16 const* pIndices,
		UInt32 uIndexCount,
		ShapeVertex const* pVertices,
		UInt32 uVertexCount,
		TriangleListDescription eDescription,
		Feature::Enum eFeature)
	{
		CheckForStateChange(worldBoundsPreClip, reference, nullptr, uIndexCount, uVertexCount, eFeature);
		m_Features.SetFeature(eFeature);
		AppendIndicesAndVertices(pIndices, uIndexCount, pVertices, uVertexCount);
		TransformLastNVertices(mWorldTransform, uVertexCount);
		AdjustTexCoordsForLastNVertices(reference, uVertexCount);
		ClipLastN(eDescription, worldBoundsPreClip, uIndexCount, uVertexCount);
	}

	void DrawTriangleList(
		const Rectangle& worldBoundsPreClip,
		const TextureReference& reference,
		const Matrix2x3& mWorldTransform,
		const ColorTransformWithAlpha& cxTransform,
		UInt16 const* pIndices,
		UInt32 uIndexCount,
		ShapeVertex const* pVertices,
		UInt32 uVertexCount,
		TriangleListDescription eDescription,
		Feature::Enum eFeature)
	{
		CheckForStateChange(worldBoundsPreClip, reference, nullptr, uIndexCount, uVertexCount, eFeature);
		m_Features.SetFeature(eFeature);
		AppendIndicesAndVertices(pIndices, uIndexCount, pVertices, uVertexCount);
		TransformLastNVertices(mWorldTransform, uVertexCount);
		TransformLastNVertices(cxTransform, uVertexCount);
		AdjustTexCoordsForLastNVertices(reference, uVertexCount);
		ClipLastN(eDescription, worldBoundsPreClip, uIndexCount, uVertexCount);
	}

	void DrawTriangleList(
		const Rectangle& worldBoundsPreClip,
		const TextureReference& reference,
		const Matrix2x3& mWorldTransform,
		const ColorTransformWithAlpha& cxTransform,
		UInt16 const* pIndices,
		UInt32 uIndexCount,
		Float32 const* pDepths3D,
		ShapeVertex const* pVertices,
		UInt32 uVertexCount,
		TriangleListDescription eDescription,
		Feature::Enum eFeature)
	{
		// Sanity check - this variation must only be called with valid pDepths3D.
		SEOUL_ASSERT(nullptr != pDepths3D);

		CheckForStateChange(worldBoundsPreClip, reference, nullptr, uIndexCount, uVertexCount, eFeature);
		m_Features.SetFeature(eFeature);
		AppendIndicesAndVertices(pIndices, uIndexCount, pDepths3D, pVertices, uVertexCount);
		TransformLastNVertices(mWorldTransform, uVertexCount);
		TransformLastNVertices(cxTransform, uVertexCount);
		AdjustTexCoordsForLastNVertices(reference, uVertexCount);

		// Unlike all other draw paths, vertices with explicit 3D depth variations
		// cannot be clipped with standard clipping/masking. So we just disable
		// the step in this case, and perform the non-clipping portions of ClipLastN()
		// manually.
		//
		// If clipping is desired for elements with arbitrary 3D depth, scissor clipping
		// can be enabled manually on a screen-aligned square max.
		PreClip(eDescription, uIndexCount, uVertexCount);
		PostClip(eDescription, uIndexCount, uVertexCount);

		// Sanity check - must be in-sync when we're done.
		SEOUL_ASSERT(m_vDepths3D.GetSize() == m_vVertices.GetSize());
	}

	void BeginPlanarShadows()
	{
		if (1 == ++m_pState->m_iInPlanarShadowRender)
		{
			m_PlanarShadowBounds = Rectangle::InverseMax();
		}
	}

	void EndPlanarShadows()
	{
		if (1 == m_pState->m_iInPlanarShadowRender)
		{
			--m_pState->m_iInPlanarShadowRender;
		}
	}

	void Flush()
	{
		if (!m_vIndices.IsEmpty())
		{
			// If 3D is enabled, but the last batches were 2D,
			// we need to fill the remainder with 0.
			if (!m_vDepths3D.IsEmpty())
			{
				auto const uFromSize = m_vDepths3D.GetSize();
				auto const uToSize = m_vVertices.GetSize();
				if (uFromSize < uToSize)
				{
					m_vDepths3D.ResizeNoInitialize(uToSize);
					memset(m_vDepths3D.Get(uFromSize), 0, (uToSize - uFromSize) * sizeof(Depths3D::ValueType));
				}
			}

			m_pState->m_Settings.m_DrawTriangleListRI(
				m_pActiveColorTexture,
				m_pActiveDetailTexture,
				m_vIndices.Data(),
				(UInt32)m_vIndices.GetSize(),
				(m_vDepths3D.IsEmpty() ? nullptr : m_vDepths3D.Data()),
				m_vVertices.Data(),
				(UInt32)m_vVertices.GetSize(),
				m_Features);
		}

		m_vVertices.Clear();
		m_vDepths3D.Clear();
		m_vIndices.Clear();
		m_pActiveDetailTexture.Reset();
		m_pActiveColorTexture.Reset();
		m_Features.Reset();
		m_fHighestCostInBatch = 0.0;
	}

	const Falcon::Rectangle& GetPlanarShadowBounds() const
	{
		return m_PlanarShadowBounds;
	}

	ScalingGrid& GetScalingGrid()
	{
		return *m_pScalingGrid;
	}

	void SetDepth3D(Float f)
	{
		m_pState->m_fRawDepth3D = f;
		m_pState->m_iIgnoreDepthProjection = 0;
	}

	void SetPlanarShadowPosition(const Vector2D& v)
	{
		m_vPlanarShadowPosition = v;
	}

#if SEOUL_ENABLE_CHEATS
	/** @return The current rendering mode. */
	Mode::Enum GetMode() const
	{
		return m_eMode;
	}

	/** Set the current renderer mode (default, overdraw, etc.). */
	void SetMode(Mode::Enum eMode)
	{
		m_eMode = eMode;
	}

	/** Enable or disable debug scanning. */
	void SetDebugScanning(Bool bDebugScanning)
	{
		m_bDebugScanning = bDebugScanning;
	}

	/** Get the current debug scanning offset. */
	Float GetDebugScanningOffset() const
	{
		return m_fDebugScanningOffset;
	}

	/** Set the current debug scanning offset. */
	void SetDebugScanningOffset(Float fDebugScanningOffset)
	{
		m_fDebugScanningOffset = fDebugScanningOffset;
	}

	Bool GetDebugEnableOverfillOptimizer() const { return m_bDebugEnableOverfillOptimizer; }
	void SetDebugEnableOverfillOptimizer(Bool bEnable) { m_bDebugEnableOverfillOptimizer = bEnable; }
#endif // /#if SEOUL_ENABLE_CHEATS

private:
	void AdjustTexCoordsForLastNVertices(
		const TextureReference& reference,
		UInt32 uVertexCount)
	{
		if (reference.m_pTexture.IsValid() &&
			reference.m_pTexture->IsAtlas())
		{
			Vertices::SizeType const zSize = m_vVertices.GetSize();
			for (Vertices::SizeType i = (zSize - uVertexCount); i < zSize; ++i)
			{
				auto& rvT = m_vVertices[i].m_vT;
				rvT.X = rvT.X * reference.m_vAtlasScale.X + reference.m_vAtlasOffset.X;
				rvT.Y = rvT.Y * reference.m_vAtlasScale.Y + reference.m_vAtlasOffset.Y;

				// Due to floating point error, we need to clamp the final texture coordinates so they
				// don't round down or up outside the intended area of the atlas, or they may sample
				// from adjacent atlas blocks.
				rvT.X = Clamp(rvT.X, reference.m_vAtlasMin.X, reference.m_vAtlasMax.X);
				rvT.Y = Clamp(rvT.Y, reference.m_vAtlasMin.Y, reference.m_vAtlasMax.Y);
			}
		}
	}

	void AdjustSecondaryTexCoordsForLastNVertices(
		const TextureReference& reference,
		UInt32 uVertexCount)
	{
		if (reference.m_pTexture.IsValid() &&
			reference.m_pTexture->IsAtlas())
		{
			Vertices::SizeType const zSize = m_vVertices.GetSize();
			for (Vertices::SizeType i = (zSize - uVertexCount); i < zSize; ++i)
			{
				auto& rvT = m_vVertices[i].m_vT;
				rvT.Z = rvT.Z * reference.m_vAtlasScale.X + reference.m_vAtlasOffset.X;
				rvT.W = rvT.W * reference.m_vAtlasScale.Y + reference.m_vAtlasOffset.Y;

				// Due to floating point error, we need to clamp the final texture coordinates so they
				// don't round down or up outside the intended area of the atlas, or they may sample
				// from adjacent atlas blocks.
				rvT.Z = Clamp(rvT.Z, reference.m_vAtlasMin.X, reference.m_vAtlasMax.X);
				rvT.W = Clamp(rvT.W, reference.m_vAtlasMin.Y, reference.m_vAtlasMax.Y);
			}
		}
	}

	void AppendCommon(
		Float32 const* pDepths3D,
		ShapeVertex const* pVertices,
		UInt32 uVertexCount)
	{
		// Vertices.
		{
			Vertices::SizeType const zOriginalSize = m_vVertices.GetSize();
			m_vDepths3D.ResizeNoInitialize(zOriginalSize + uVertexCount);
			memcpy(m_vDepths3D.Get(zOriginalSize), pDepths3D, sizeof(Float32) * uVertexCount);
			m_vVertices.ResizeNoInitialize(zOriginalSize + uVertexCount);
			memcpy(m_vVertices.Get(zOriginalSize), pVertices, sizeof(ShapeVertex) * uVertexCount);
		}
	}

	void AppendCommon(
		ShapeVertex const* pVertices,
		UInt32 uVertexCount)
	{
		// Vertices.
		{
			Vertices::SizeType const zOriginalSize = m_vVertices.GetSize();
			m_vVertices.ResizeNoInitialize(zOriginalSize + uVertexCount);
			memcpy(m_vVertices.Get(zOriginalSize), pVertices, sizeof(ShapeVertex) * uVertexCount);
		}
	}

	void AppendIndicesAndVertices(
		UInt16 const* pIndices,
		UInt32 uIndexCount,
		Float32 const* pDepths3D,
		ShapeVertex const* pVertices,
		UInt32 uVertexCount)
	{
		// Cache values.
		UInt16 const zOffset = (UInt16)m_vVertices.GetSize();
		Indices::SizeType const zOriginalSize = m_vIndices.GetSize();

		// Must be called after caching zOffset, inserts vertices and performs
		// material computations.
		AppendCommon(pDepths3D, pVertices, uVertexCount);

		// Append indices.
		m_vIndices.ResizeNoInitialize(zOriginalSize + uIndexCount);
		memcpy(m_vIndices.Get(zOriginalSize), pIndices, sizeof(UInt16) * uIndexCount);

		// Adjust offsets.
		Indices::SizeType const zNewSize = m_vIndices.GetSize();
		for (Indices::SizeType i = zOriginalSize; i < zNewSize; ++i)
		{
			m_vIndices[i] += zOffset;
		}
	}

	void AppendIndicesAndVertices(
		UInt16 const* pIndices,
		UInt32 uIndexCount,
		ShapeVertex const* pVertices,
		UInt32 uVertexCount)
	{
		// Cache values.
		UInt16 const zOffset = (UInt16)m_vVertices.GetSize();
		Indices::SizeType const zOriginalSize = m_vIndices.GetSize();

		// Must be called after caching zOffset, inserts vertices and performs
		// material computations.
		AppendCommon(pVertices, uVertexCount);

		// Append indices.
		m_vIndices.ResizeNoInitialize(zOriginalSize + uIndexCount);
		memcpy(m_vIndices.Get(zOriginalSize), pIndices, sizeof(UInt16) * uIndexCount);

		// Adjust offsets.
		Indices::SizeType const zNewSize = m_vIndices.GetSize();
		for (Indices::SizeType i = zOriginalSize; i < zNewSize; ++i)
		{
			m_vIndices[i] += zOffset;
		}
	}

	UInt32 AppendVertices(
		ShapeVertex const* pVertices,
		UInt32 uVertexCount,
		TriangleListDescription eDescription)
	{
		// Cache values.
		UInt16 const zOffset = (UInt16)m_vVertices.GetSize();
		Indices::SizeType const zOriginalSize = m_vIndices.GetSize();

		// Must be called after caching zOffset, inserts vertices and performs
		// material computations.
		AppendCommon(pVertices, uVertexCount);

		UInt32 uIndexCount = 0;

		// Generate indices based on the triangle list description.
		switch (eDescription)
		{
		case TriangleListDescription::kConvex:
			{
				// Must have at least 3 vertices for a convex vertex-only append.
				SEOUL_ASSERT(uVertexCount >= 3);

				uIndexCount = (uVertexCount - 2) * 3;
				m_vIndices.ResizeNoInitialize(zOriginalSize + uIndexCount);
				Indices::SizeType uIndex = zOriginalSize;
				for (UInt32 i = 2; i < uVertexCount; ++i)
				{
					m_vIndices[uIndex++] = (Indices::ValueType)(0 + zOffset);
					m_vIndices[uIndex++] = (Indices::ValueType)((i - 1) + zOffset);
					m_vIndices[uIndex++] = (Indices::ValueType)((i - 0) + zOffset);
				}
			}
			break;

		case TriangleListDescription::kNotSpecific:
			{
				// Must have a multiple of 3 vertices for a not specific vertex-only append.
				SEOUL_ASSERT(0 == (uVertexCount % 3));

				uIndexCount = uVertexCount;
				m_vIndices.ResizeNoInitialize(zOriginalSize + uIndexCount);
				for (UInt32 i = 0; i < uVertexCount; ++i)
				{
					m_vIndices[zOriginalSize + i] = (Indices::ValueType)(i + zOffset);
				}
			}
			break;

		case TriangleListDescription::kQuadList: // fall-through
		case TriangleListDescription::kTextChunk:
			{
				// Must have a multiple of 4 vertices for a quad list vertex-only append.
				SEOUL_ASSERT(0 == (uVertexCount % 4));

				uIndexCount = (uVertexCount / 4) * 6;
				m_vIndices.ResizeNoInitialize(zOriginalSize + uIndexCount);
				for (UInt32 i = 0; i < uVertexCount; i += 4)
				{
					Indices::SizeType const uIndex = zOriginalSize + ((i / 4) * 6);
					m_vIndices[uIndex+0] = (Indices::ValueType)(i + 0 + zOffset);
					m_vIndices[uIndex+1] = (Indices::ValueType)(i + 1 + zOffset);
					m_vIndices[uIndex+2] = (Indices::ValueType)(i + 2 + zOffset);
					m_vIndices[uIndex+3] = (Indices::ValueType)(i + 0 + zOffset);
					m_vIndices[uIndex+4] = (Indices::ValueType)(i + 2 + zOffset);
					m_vIndices[uIndex+5] = (Indices::ValueType)(i + 3 + zOffset);
				}
			}
			break;
		};

		return uIndexCount;
	}

	/**
	 * Inline utility function used to compute index counts from vertex counts for appropriate
	 * shape types.
	 */
	static inline UInt32 DeriveIndexCount(UInt32 uVertexCount, TriangleListDescription eDescription)
	{
		switch (eDescription)
		{
		case TriangleListDescription::kConvex: return (uVertexCount >= 2u ? (uVertexCount - 2u) * 3u : 0u);
		case TriangleListDescription::kNotSpecific: return 0u;
		case TriangleListDescription::kQuadList: // fall-through
		case TriangleListDescription::kTextChunk:
			return (uVertexCount / 4u) * 6u;
		default:
			SEOUL_FAIL("Out-of-sync enum.");
			return 0u;
		};
	}

	void CheckForStateChange(
		const Rectangle& worldBoundsPreClip,
		const SharedPtr<Texture>& pColorTexture,
		const SharedPtr<Texture>& pDetailTexture,
		UInt32 uIndexCount,
		UInt32 uVertexCount,
		Feature::Enum eFeature);

	void CheckForStateChange(
		const Rectangle& worldBoundsPreClip,
		const TextureReference& colorTex,
		TextureReference const* pDetailTex,
		UInt32 uIndexCount,
		UInt32 uVertexCount,
		Feature::Enum eFeature)
	{
		CheckForStateChange(worldBoundsPreClip, colorTex.m_pTexture, (nullptr == pDetailTex ? SharedPtr<Texture>() : pDetailTex->m_pTexture), uIndexCount, uVertexCount, eFeature);

#if SEOUL_ENABLE_CHEATS
		// This must happen last, as we override what is set by CheckForStateChange(pTexture,...).
		m_eLastTextureType = colorTex.m_eTextureType;
#endif // /#if SEOUL_ENABLE_CHEATS
	}

	void InternalFinalizeDrawText(
		const TextChunk& textChunk,
		UInt32 uBaseVertex,
		UInt32 uGlyphs,
		const Matrix2x3& mWorld,
		const Rectangle& tightGlyphBounds,
		TextEffectSettings const* pSettings,
		TextureReference const* pDetailTex);
	void InternalFinalizeDrawTextSecondaryTexCoords(
		const Matrix2x3& mWorld,
		const TextChunk& textChunk,
		UInt32 uBaseVertex,
		UInt32 uGlyphs,
		TextEffectSettings const* pSettings,
		TextureReference const* pDetailTex);

	void PreClip(
		TriangleListDescription eDescription,
		UInt32 uIndexCount,
		UInt32 uVertexCount);
	void ClipLastN(
		TriangleListDescription eDescription,
		UInt32 uIndexCount,
		UInt32 uVertexCount);
	void ClipLastN(
		TriangleListDescription eDescription,
		const Rectangle& vertexBounds,
		UInt32 uIndexCount,
		UInt32 uVertexCount);
	void PostClip(
		TriangleListDescription eDescription,
		UInt32 uIndexCount,
		UInt32 uVertexCount);

	void ShadowProjectLastNVertices(UInt32 uVertexCount);

	void TransformLastNVertices(
		const Matrix2x3& mWorldTransform,
		UInt32 uVertexCount)
	{
		Vertices::SizeType const zSize = m_vVertices.GetSize();
		for (Vertices::SizeType i = (zSize - uVertexCount); i < zSize; ++i)
		{
			m_vVertices[i].m_vP = Matrix2x3::TransformPosition(mWorldTransform, m_vVertices[i].m_vP);
		}
	}

	void TransformLastNVertices(
		const ColorTransformWithAlpha& cxTransform,
		UInt32 uVertexCount)
	{
		// Ranges.
		Vertices::SizeType const zSize = m_vVertices.GetSize();
		auto const iBegin = m_vVertices.Get(zSize - uVertexCount);
		auto const iEnd = m_vVertices.End();

		// Add
		{
			auto const uR = cxTransform.m_AddR;
			auto const uG = cxTransform.m_AddG;
			auto const uB = cxTransform.m_AddB;

			// Check and set.
			if (uR != 0 || uG != 0 || uB != 0)
			{
				m_Features.SetColorAdd();
				for (auto i = iBegin; iEnd != i; ++i)
				{
					auto& r = i->m_ColorAdd;
					r.m_R += uR;
					r.m_G += uG;
					r.m_B += uB;
				}
			}
		}

		// Factor
		if (cxTransform.m_uBlendingFactor != 0)
		{
			UInt8 const uF = cxTransform.m_uBlendingFactor;
			for (auto i = iBegin; iEnd != i; ++i)
			{
				auto& r = i->m_ColorAdd;
				r.m_BlendingFactor = Max(r.m_BlendingFactor, uF);
			}
		}

		// Multiply
		{
			auto const fR = cxTransform.m_fMulR;
			auto const fG = cxTransform.m_fMulG;
			auto const fB = cxTransform.m_fMulB;
			auto const fA = cxTransform.m_fMulA;

			// Check and set.
			if (fR != 1.0f ||
				fG != 1.0f ||
				fB != 1.0f ||
				fA != 1.0f)
			{
				m_Features.SetColorMultiply();
				for (auto i = iBegin; iEnd != i; ++i)
				{
					auto& r = i->m_ColorMultiply;
					r.m_R = (UInt8)Min(fR * (Float)r.m_R + 0.5f, 255.0f);
					r.m_G = (UInt8)Min(fG * (Float)r.m_G + 0.5f, 255.0f);
					r.m_B = (UInt8)Min(fB * (Float)r.m_B + 0.5f, 255.0f);
					r.m_A = (UInt8)Min(fA * (Float)r.m_A + 0.5f, 255.0f);
				}
			}
		}
	}

	CheckedPtr<State> m_pState;
	ScopedPtr<ScalingGrid> m_pScalingGrid;
	SharedPtr<Texture> m_pActiveColorTexture;
	SharedPtr<Texture> m_pActiveDetailTexture;
	Depths3D m_vDepths3D;
	Vertices m_vVertices;
	Indices m_vIndices;
	Vector2D m_vPlanarShadowPosition;
	Rectangle m_PlanarShadowBounds;
	Features m_Features;
	Double m_fHighestCostInBatch;
#if SEOUL_ENABLE_CHEATS
	Mode::Enum m_eMode;
	FileType m_eLastTextureType;
	Float m_fDebugScanningOffset;
	Bool m_bDebugScanning;
	Bool m_bDebugEnableOverfillOptimizer = true;
#endif // /#if SEOUL_ENABLE_CHEATS

	SEOUL_DISABLE_COPY(Drawer);
}; // class Drawer

} // namespace Render

} // namespace Seoul::Falcon

#endif // include guard
