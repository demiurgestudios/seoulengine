/**
 * \file FalconEditTextInstance.cpp
 * \brief The instance of an EditTextDefinition
 * in the Falcon scene graph.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FalconBitmapDefinition.h"
#include "FalconEditTextCommon.h"
#include "FalconEditTextDefinition.h"
#include "FalconEditTextInstance.h"
#include "FalconGlobalConfig.h"
#include "FalconMovieClipInstance.h"
#include "FalconRenderPoser.h"
#include "FalconRenderDrawer.h"
#include "Logger.h"
#include "ReflectionDefine.h"

namespace Seoul
{

namespace
{

Float GetIndent(const Falcon::EditTextInstance& edit)
{
	return edit.GetDefinition()->GetIndent();
}

Float GetLeading(const Falcon::EditTextInstance& edit)
{
	return edit.GetDefinition()->GetLeading();
}

Float GetLeftMargin(const Falcon::EditTextInstance& edit)
{
	return edit.GetDefinition()->GetLeftMargin();
}

Float GetRightMargin(const Falcon::EditTextInstance& edit)
{
	return edit.GetDefinition()->GetRightMargin();
}

} // namespace anonymous

SEOUL_BEGIN_TYPE(Falcon::EditTextInstance, TypeFlags::kDisableNew)
	SEOUL_PARENT(Falcon::Instance)
	SEOUL_PROPERTY_PAIR_N("AutoSizeBottom", GetAutoSizeBottom, SetAutoSizeBottom)
	SEOUL_PROPERTY_PAIR_N("AutoSizeContents", GetAutoSizeContents, SetAutoSizeContents)
	SEOUL_PROPERTY_PAIR_N("AutoSizeHorizontal", GetAutoSizeHorizontal, SetAutoSizeHorizontal)
	SEOUL_PROPERTY_N_Q("Indent", GetIndent)
	SEOUL_PROPERTY_N_Q("Leading", GetLeading)
	SEOUL_PROPERTY_N_Q("LeftMargin", GetLeftMargin)
	SEOUL_PROPERTY_N_Q("RightMargin", GetRightMargin)
	SEOUL_PROPERTY_PAIR_N("PlainText", GetPlainText, SetPlainText)
	SEOUL_PROPERTY_PAIR_N("VerticalCenter", GetVerticalCenter, SetVerticalCenter)
	SEOUL_PROPERTY_PAIR_N("XhtmlParsing", GetXhtmlParsing, SetXhtmlParsing)
	SEOUL_PROPERTY_PAIR_N("XhtmlText", GetXhtmlText, SetXhtmlText)
SEOUL_END_TYPE()

namespace Falcon
{

using namespace EditTextCommon;

#if !SEOUL_SHIP
static inline const char* ToString(HtmlAlign eAlignment)
{
	switch (eAlignment)
	{
	case HtmlAlign::kCenter: return "Center";
	case HtmlAlign::kJustify: return "Justify";
	case HtmlAlign::kLeft: return "Left";
	case HtmlAlign::kRight: return "Right";
	default:
		return "Unknown";
	};
};

static inline const char* ToString(HtmlImageAlign eAlignment)
{
	switch (eAlignment)
	{
	case HtmlImageAlign::kBottom: return "Bottom";
	case HtmlImageAlign::kLeft: return "Left";
	case HtmlImageAlign::kMiddle: return "Middle";
	case HtmlImageAlign::kRight: return "Right";
	case HtmlImageAlign::kTop: return "Top";
	default:
		return "Unknown";
	};
}
#endif // /#if !SEOUL_SHIP

EditTextInstance::ImageEntry::ImageEntry()
	: m_pBitmap()
	, m_vTextureCoordinates(0, 0, 1.0f, 1.0f)
	, m_fXOffset(0)
	, m_fYOffset(0)
	, m_fXMargin(0)
	, m_fYMargin(0)
	, m_iLinkIndex(-1)
	, m_iStartingTextLine(0)
	, m_eAlignment(HtmlAlign::kLeft)
	, m_eImageAlignment(HtmlImageAlign::kTop)
	, m_fRescale(1.0f)
{
}

EditTextInstance::ImageEntry::~ImageEntry()
{
}

Float EditTextInstance::ImageEntry::ComputeCenterY() const
{
	return (m_fYOffset + GetHeight() * 0.5f);
}

Float EditTextInstance::ImageEntry::GetHeight() const
{
	return (m_pBitmap.IsValid() ? (Float)m_pBitmap->GetHeight() : 0.0f) * m_fRescale;
}

Float EditTextInstance::ImageEntry::GetRightBorder() const
{
	return (m_fXOffset + GetWidth());
}

Float EditTextInstance::ImageEntry::GetWidth() const
{
	return (m_pBitmap.IsValid() ? (Float)m_pBitmap->GetWidth() : 0.0f) * m_fRescale;
}

Bool EditTextInstance::ImageEntry::IsValid() const
{
	return (
		m_pBitmap.IsValid() &&
		m_pBitmap->GetWidth() > 0u &&
		m_pBitmap->GetHeight() > 0u);
}

EditTextInstance::EditTextInstance(const SharedPtr<EditTextDefinition const>& pEditTextDefinition)
	: Instance(pEditTextDefinition->GetDefinitionID())
	, m_pEditTextDefinition(pEditTextDefinition)
	, m_vTextChunks()
	, m_sText()
	, m_sMarkupText()
	, m_fCursorBlinkTimer(0.0f)
	, m_CursorColor((pEditTextDefinition.IsValid() && pEditTextDefinition->HasTextColor()) ? pEditTextDefinition->GetTextColor() : RGBA::White())
	, m_fBottom(m_pEditTextDefinition->GetBounds().m_fBottom)
	, m_fLeft(m_pEditTextDefinition->GetBounds().m_fLeft)
	, m_fRight(m_pEditTextDefinition->GetBounds().m_fRight)
	, m_VisibleCharacters()
	, m_bNeedsFormatting(false)
	, m_bUseInitialText(true)
	, m_bVerticalCenter(false)
	, m_bAutoSizeBottom(false)
	, m_bXhtmlParsing(true)
	, m_bHasTextEditFocus(false)
	, m_bXhtmlVerticalCenter(false)
	, m_bAutoSizeContents(true)
	, m_bAutoSizeHorizontal(false)
	, m_uUnusedReserved(0)
{
}

EditTextInstance::~EditTextInstance()
{
}

void EditTextInstance::Advance(AdvanceInterface& rInterface)
{
	CheckFormatting(rInterface);
	if (m_bHasTextEditFocus)
	{
		m_fCursorBlinkTimer += rInterface.FalconGetDeltaTimeInSeconds();
		if (m_fCursorBlinkTimer > (2.0f * kfCursorBlinkIntervalInSeconds))
		{
			m_fCursorBlinkTimer -= (2.0f * kfCursorBlinkIntervalInSeconds);
		}
	}
}

Bool EditTextInstance::ComputeLocalBounds(Rectangle& rBounds)
{
	if (m_bAutoSizeBottom || m_bAutoSizeHorizontal)
	{
		CheckFormatting();
	}

	rBounds = GetLocalBounds();
	return true;
}

/**
 * If a chunk is configured with a detail/face texture,
 * this method resolves the TextureReference for that
 * texture. It will return false if the chunk does
 * not use a face texture, or if the resolve fails (e.g. streaming
 * texture load that is not yet ready).
 */
Bool EditTextInstance::GetDetailTexture(
	const Matrix2x3& mWorld,
	Render::Poser& rPoser,
	const TextChunk& chunk,
	TextureReference& detail)
{
	// Not configured.
	if (chunk.m_Format.m_TextEffectSettings.IsEmpty())
	{
		return false;
	}

	// Not configured.
	auto pSettings = g_Config.m_pGetTextEffectSettings(chunk.m_Format.m_TextEffectSettings);
	if (nullptr == pSettings)
	{
		return false;
	}

	// Not configured.
	if (!pSettings->m_bDetail)
	{
		return false;
	}

	// Overall bounds of the chunk.
	auto const localTightBounds(chunk.ComputeGlyphBounds());
	auto const worldTightBounds(TransformRectangle(mWorld, localTightBounds));

	// TODO: Need to factor in aspect mode, but currently
	// this involves a catch-22 (need to resolve the texture to
	// determine its aspect but need the aspect to resolve the texture).
	//
	// TODO: Measurement for character mode is wrong. Technically,
	// it should be different for each glyph. Width / # of characters
	// is effectively the mean width of a glyph, but could be wrong in outlier
	// cases (all 'w' with a single '.').
	auto const fRenderHeightLocal = (localTightBounds.GetHeight());
	auto fRenderWidthLocal = (localTightBounds.GetWidth());
	if (TextEffectDetailMode::Character == pSettings->m_eDetailMode)
	{
		fRenderWidthLocal /= (Float)Max(1u, chunk.m_uNumberOfCharacters);
	}

	// Resolve.
	return (Render::PoserResolveResult::kSuccess == rPoser.ResolveTextureReference(
		worldTightBounds,
		this,
		rPoser.GetRenderThreshold(fRenderWidthLocal, fRenderHeightLocal, mWorld),
		pSettings->m_DetailFilePath,
		detail,
		false,
		false));
}

namespace
{

/**
 * Internal utility used to encode various draw cases
 * into the Int32 iSubInstanceId that is carried from
 * Pose to Draw.
 */
class EncodedInstanceId
{
public:
	enum class Type
	{
		/** Draw is 1-n text chunks with no detail/face texture. */
		kTextChunks,

		/** Draw is an image embedded in a text box. */
		kImage,

		/** Draw is the editable text position cursor. */
		kCursor,

		/** Draw is a single text chunk with a detail/face texture. */
		kTextChunkWithDetail,
	};

	EncodedInstanceId()
		: m_uAllBits(0u)
	{
	}

	explicit EncodedInstanceId(Int32 iBits)
		: m_uAllBits((UInt32)iBits)
	{
	}

	/** Conversion to a packed Int32 value. */
	Int32 AsInt32() const { return (Int32)m_uAllBits; }
	/** Type of the op. */
	Type GetType() const { return (Type)m_uType; }

	/** Start index of the 1-n text chunks - FAIL if GetType() != kTextChunks. */
	UInt32 BeginTextChunk() const
	{
		SEOUL_ASSERT(Type::kTextChunks == GetType());
		return m_uBeginIndex;
	}

	/** End index of the 1-n text chunks - FAIL if GetType() != kTextChunks. 1 passed the last chunk. */
	UInt32 EndTextChunk() const
	{
		SEOUL_ASSERT(Type::kTextChunks == GetType());
		return m_uBeginIndex + m_uCount;
	}

	/** Index of cursor target - FAIL if GetType() != kCursor. */
	UInt32 GetCursorIndex() const
	{
		SEOUL_ASSERT(Type::kCursor == GetType());
		return m_uIndex;
	}

	/** Index of detail text chunk - FAIL if GetType() != kTextChunkWithDetail. */
	UInt32 GetDetailTextChunkIndex() const
	{
		SEOUL_ASSERT(Type::kTextChunkWithDetail == GetType());
		return m_uIndex;
	}

	/** Index of image in text box - FAIL if GetType() != kImage. */
	UInt32 GetImageIndex() const
	{
		SEOUL_ASSERT(Type::kImage == GetType());
		return m_uIndex;
	}

	/** Update the type of this draw operation - valid for all types except kTextChunks. */
	void Set(Type eType, UInt32 uIndex)
	{
		SEOUL_ASSERT((Int32)eType >= 0 && (Int32)eType < 4);
		SEOUL_ASSERT(uIndex <= (1 << 30));
		SEOUL_ASSERT(Type::kImage == eType || Type::kCursor == eType || Type::kTextChunkWithDetail == eType);

		m_uIndex = uIndex;
		m_uType = (UInt32)eType;
	}

	/** Update the type of this draw operation - valid only for kTextChunks. */
	void Set(Type eType, UInt32 uBeginIndex, UInt32 uCount)
	{
		SEOUL_ASSERT(Type::kTextChunks == eType);
		SEOUL_ASSERT(uBeginIndex <= (1 << 20));
		SEOUL_ASSERT(uCount <= (1 << 10));

		m_uBeginIndex = uBeginIndex;
		m_uCount = uCount;
		m_uType = (UInt32)eType;
	}

	static const UInt32 kuMaxTextChunksPerDraw = (1 << 10); // Match width of m_uCount bit width below.

private:
	union
	{
		// Image, detailed text chunk, cursor.
		struct
		{
			UInt32 m_uIndex : 30;
			UInt32 m_uType : 2;
		};
		// Text chunks - range.
		struct
		{
			UInt32 m_uBeginIndex : 20;
			UInt32 m_uCount : 10;
			UInt32 m_uReserved : 2;
		};
		// Accumulation.
		UInt32 m_uAllBits;
	};
};
// Fundamental requirement, since the purpose of EncodedInstanceId is
// to pack configuration into an Int32.
SEOUL_STATIC_ASSERT(sizeof(EncodedInstanceId) == 4);

} // namespace anonymous

void EditTextInstance::Pose(
	Render::Poser& rPoser,
	const Matrix2x3& mParent,
	const ColorTransformWithAlpha& cxParent)
{
	if (!GetVisible())
	{
		return;
	}

	ColorTransformWithAlpha const cxWorld = (cxParent * GetColorTransformWithAlpha());
	if (cxWorld.m_fMulA == 0.0f)
	{
		return;
	}

	CheckFormatting();

	Matrix2x3 const mWorld = (mParent * GetTransform());

	// Text chunks.
	{
		TextureReference detail;
		TextureReference solidFill;

		// Tracking for current run - we submit 1-n text chunks per pose, to reduce
		// the number of pose operations we can generate for a single text box.
		UInt32 uBeginDrawTextChunk = 0u;
		UInt32 uDrawTextChunkCount = 0u;

		// Utility closure, submits the current run of text chunks.
		auto const submitTextChunks = [&](UInt32 uNext)
		{
			if (uDrawTextChunkCount > 0u)
			{
				// Compute the bounds of the current run.
				Rectangle localRenderBounds = Rectangle::InverseMax();
				for (UInt32 i = uBeginDrawTextChunk, iEnd = uBeginDrawTextChunk + uDrawTextChunkCount; iEnd != i; ++i)
				{
					localRenderBounds = Rectangle::Merge(localRenderBounds, m_vTextChunks[i].ComputeRenderBounds());
				}
				auto const worldRenderBounds(TransformRectangle(mWorld, localRenderBounds));

				// If we have not yet resolved the solid fill texture, do so now.
				if (!solidFill.m_pTexture.IsValid())
				{
					if (Render::PoserResolveResult::kSuccess != rPoser.ResolveTextureReference(
						worldRenderBounds,
						this,
						1.0f,
						FilePath(),
						solidFill))
					{
						// Unlikely, but if we fail to resolve the solid fill, nothing more to do.
						uBeginDrawTextChunk = uNext;
						uDrawTextChunkCount = 0u;
						return;
					}
				}

				// Generate the draw op encoded id and submit.
				EncodedInstanceId id;
				id.Set(EncodedInstanceId::Type::kTextChunks, uBeginDrawTextChunk, uDrawTextChunkCount);
				rPoser.Pose(
					worldRenderBounds,
					this,
					mWorld,
					cxWorld,
					solidFill,
					Rectangle(),
					Render::Feature::kAlphaShape,
					id.AsInt32());
			}

			// Advance.
			uBeginDrawTextChunk = uNext;
			uDrawTextChunkCount = 0u;
		};

		// State for culling during posing.
		Rectangle const objectSpaceCullRectangle(
			TransformRectangle(mWorld.Inverse(), rPoser.GetState().m_WorldCullRectangle));
		auto const textBoxLocalBounds = GetLocalBounds();
		Int32 iLastDrawLine = -1;
		Int32 iLastDrawTextChunk = -1;
		UInt32 uCharactersDrawn = 0u;

		// Iterate over chunks and process.
		auto const uSize = m_vTextChunks.GetSize();
		for (auto i = 0u; i < uSize; ++i)
		{
			// Process the chunk.
			auto const& chunk = m_vTextChunks[i];
			auto const renderBounds = chunk.ComputeRenderBounds();

			// Determine if the chunk is fully culled.
			Bool const bCulled = (
				renderBounds.m_fBottom < textBoxLocalBounds.m_fTop ||
				renderBounds.m_fTop > textBoxLocalBounds.m_fBottom ||
				renderBounds.m_fBottom < objectSpaceCullRectangle.m_fTop ||
				renderBounds.m_fTop > objectSpaceCullRectangle.m_fBottom ||
				renderBounds.m_fLeft > objectSpaceCullRectangle.m_fRight);

			if (bCulled)
			{
				// If culled and if we're on a new line, then we can
				// stop processing chunks.
				if (iLastDrawLine >= 0 && iLastDrawLine != chunk.m_iLine)
				{
					break;
				}

				// Need to submit chunks so far before continuing.
				submitTextChunks(i + 1u);

				// Need to advance to the next line before we can stop processing chunks
				// if a chunk is culled.
				continue;
			}

			// Add this chunk to the run - if we hit the max per run, submit the chunks.
			++uDrawTextChunkCount;
			if (EncodedInstanceId::kuMaxTextChunksPerDraw == uDrawTextChunkCount)
			{
				submitTextChunks(i + 1u);
			}

			// If the chunk has a detailed texture, the detail
			// portion must also be submitted separately.
			if (GetDetailTexture(mWorld, rPoser, chunk, detail))
			{
				// TODO: This submit is only necessary to avoid walking the text chunk
				// list twice. If we walked the list twice, we could add all detail texture
				// draws after all other text chunks instead, and avoid breaking into multiple
				// poses like this.
				submitTextChunks(i + 1u);

				// Configure the detail texture for draw and submit.
				auto const worldTightBounds(TransformRectangle(mWorld, chunk.ComputeGlyphBounds()));
				EncodedInstanceId id;
				id.Set(EncodedInstanceId::Type::kTextChunkWithDetail, i);
				rPoser.Pose(
					worldTightBounds,
					this,
					mWorld,
					cxWorld,
					detail,
					Rectangle(),
					Render::Feature::kDetail,
					id.AsInt32());
			}

			// Tracking and advance.
			iLastDrawLine = chunk.m_iLine;
			uCharactersDrawn += chunk.m_uNumberOfCharacters;

			iLastDrawTextChunk = (Int32)i;

			// Early out if configured - once we've exceeded the visible
			// count, we can stop processing text chunks.
			if (uCharactersDrawn >= m_VisibleCharacters.m_uVisibleCount)
			{
				break;
			}
		}

		// Submit the final run.
		submitTextChunks(0u);

		// Cursor.
		if (m_bHasTextEditFocus && m_fCursorBlinkTimer <= kfCursorBlinkIntervalInSeconds)
		{
			// TODO: Restrict to the actual cursor.
			auto const worldBounds(TransformRectangle(mWorld, textBoxLocalBounds));

			Bool bOk = true;

			// If we have not yet resolve the solid fill texture, do so now.
			if (!solidFill.m_pTexture.IsValid())
			{
				if (Render::PoserResolveResult::kSuccess != rPoser.ResolveTextureReference(
					worldBounds,
					this,
					1.0f,
					FilePath(),
					solidFill))
				{
					// Cannot draw if we fail to resolve the solid fill texture.
					bOk = false;
				}
			}

			if (bOk)
			{
				// Configure the draw for cursor and submit the draw.
				EncodedInstanceId id;
				id.Set(EncodedInstanceId::Type::kCursor, (iLastDrawTextChunk >= 0 ? (UInt32)iLastDrawTextChunk : m_vTextChunks.GetSize()));
				rPoser.Pose(
					worldBounds,
					this,
					mWorld,
					cxWorld,
					solidFill,
					Rectangle(),
					Render::Feature::kColorMultiply,
					id.AsInt32());
			}
		}
	}

	// Images
	{
		Bool bDrawnSome = false;
		auto const uSize = m_vImages.GetSize();
		for (auto i = 0u; i < uSize; ++i)
		{
			ImageEntry& entry = m_vImages[i];

			Float32 const fWidth = entry.GetWidth();
			Float32 const fHeight = entry.GetHeight();
			auto const bounds = Rectangle::Create(
				entry.m_fXOffset,
				entry.m_fXOffset + fWidth,
				entry.m_fYOffset,
				entry.m_fYOffset + fHeight);
			auto const worldBounds = TransformRectangle(mWorld, bounds);

			TextureReference reference;

			// Check for early out on pose failure (which
			// implies the draw call is outside the world culling
			// rectangle).
			auto const eResult = rPoser.ResolveTextureReference(
				worldBounds,
				this,
				rPoser.GetRenderThreshold(fWidth, fHeight, mWorld),
				entry.m_pBitmap,
				reference);
			if (Falcon::Render::PoserResolveResult::kSuccess != eResult)
			{
				if (Falcon::Render::PoserResolveResult::kNotReady == eResult && !rPoser.InPlanarShadow())
				{
					entry.m_vTextureCoordinates = Vector4D(0.0f, 0.0f, 1.0f, 1.0f);
				}
				else
				{
					// We assume all images, 0 to n-1, are ordered such that,
					// once we've hit an out of bounds image after drawing at
					// least one image, we can stop drawing images (all remaining
					// images will be clipped by the bottom of the text box bounds).
					if (bDrawnSome)
					{
						break;
					}
					else
					{
						continue;
					}
				}
			}
			else
			{
				// Configure the draw of images and submit the draw.
				EncodedInstanceId id;
				id.Set(EncodedInstanceId::Type::kImage, i);
				auto const worldOcclusion = ComputeOcclusionRectangle(mWorld, reference, bounds);
				rPoser.Pose(
					worldBounds,
					this,
					mWorld,
					cxWorld,
					reference,
					worldOcclusion,
					Render::Feature::kNone,
					id.AsInt32());
				bDrawnSome = true;
			}
		}
	}
}

#if SEOUL_ENABLE_CHEATS
void EditTextInstance::PoseInputVisualization(
	Render::Poser& rPoser,
	const Matrix2x3& mParent,
	RGBA color)
{
	// Text box hit testing is based around the content
	// bounds, not the local bounds.
	Rectangle bounds;
	if (!GetLocalTextBounds(bounds))
	{
		return;
	}

	// TODO: Draw the appropriate shape for exact hit testing.
	auto const mWorld(mParent * GetTransform());
	auto const worldBounds = TransformRectangle(mWorld, bounds);
	rPoser.PoseInputVisualization(
		worldBounds,
		bounds,
		mWorld,
		color);
}
#endif

void EditTextInstance::Draw(
	Render::Drawer& rDrawer,
	const Rectangle& worldBoundsPreClip,
	const Matrix2x3& mWorld,
	const ColorTransformWithAlpha& cxWorld,
	const TextureReference& textureReference,
	Int32 iSubInstanceId)
{
	// Handle the draw based on type.
	EncodedInstanceId const id(iSubInstanceId);
	switch (id.GetType())
	{
	// Draw is a run of text chunks.
	case EncodedInstanceId::Type::kTextChunks:
	{
		auto const localBounds = GetLocalBounds();
		auto const uBegin = id.BeginTextChunk();
		auto const uEnd = id.EndTextChunk();

		for (auto i = uBegin; uEnd != i; ++i)
		{
			auto textChunk = m_vTextChunks[i];

			// Shorten if needed.
			if (SEOUL_UNLIKELY(m_VisibleCharacters.m_uPartiallyVisibleTextChunk == i))
			{
				InternalApplyVisibleToChunk(textChunk);
			}

			// Check if we need to apply effect settings.
			TextEffectSettings const* pSettings = nullptr;
			if (!textChunk.m_Format.m_TextEffectSettings.IsEmpty())
			{
				pSettings = g_Config.m_pGetTextEffectSettings(textChunk.m_Format.m_TextEffectSettings);

				// Override color now.
				if (nullptr != pSettings)
				{
					// Apply color from the text effect if it is defined.
					if (nullptr != pSettings->m_pTextColor)
					{
						auto const c = RGBA::Create(*pSettings->m_pTextColor);
						textChunk.m_Format.m_TextColor = c;
						textChunk.m_Format.m_SecondaryTextColor = c;
					}
					else
					{
						if (nullptr != pSettings->m_pTextColorTop)
						{
							auto const c = RGBA::Create(*pSettings->m_pTextColorTop);
							textChunk.m_Format.m_TextColor = c;
						}
						if (nullptr != pSettings->m_pTextColorBottom)
						{
							auto const c = RGBA::Create(*pSettings->m_pTextColorBottom);
							textChunk.m_Format.m_SecondaryTextColor = c;
						}
					}
				}
			}

			// If the text chunk has text settings, apply the shadow settings now, if enabled.
			if (nullptr != pSettings)
			{
				if (pSettings->m_bShadowEnable)
				{
					DrawOutline(
						rDrawer,
						localBounds,
						textChunk,
						mWorld,
						cxWorld,
						pSettings->m_vShadowOffset,
						pSettings->m_ShadowColor,
						pSettings->m_uShadowOutlineWidth,
						pSettings->m_uShadowBlur);
				}

				if (pSettings->m_bExtraOutlineEnable)
				{
					DrawOutline(
						rDrawer,
						localBounds,
						textChunk,
						mWorld,
						cxWorld,
						pSettings->m_vExtraOutlineOffset,
						pSettings->m_ExtraOutlineColor,
						pSettings->m_uExtraOutlineWidth,
						pSettings->m_uExtraOutlineBlur);
				}

				// If this chunk has detail, don't draw the body - that will be handled
				// by a separate detail render command.
				if (pSettings->m_bDetail)
				{
					continue;
				}
			}

			// Main/primary/top color, depending on the value of m_SecondaryColor.
			// Always used so always computed.
			RGBA const rgba = TransformColor(
				cxWorld,
				textChunk.m_Format.m_TextColor);

			// A more optimized draw function if the text chunk is a single
			// homogenous color top-to-bottom.
			if (textChunk.m_Format.m_TextColor == textChunk.m_Format.m_SecondaryTextColor)
			{
				rDrawer.DrawTextChunk(
					textChunk,
					mWorld,
					rgba,
					localBounds,
					!GetCanWordWrap());
			}
			// Otherwise, use a slightly more expensive draw function to handle
			// a separate secondary (bottom) color.
			else
			{
				RGBA const rgbaSecondary = TransformColor(
					cxWorld,
					textChunk.m_Format.m_SecondaryTextColor);

				rDrawer.DrawTextChunk(
					textChunk,
					mWorld,
					rgba,
					rgbaSecondary,
					localBounds,
					!GetCanWordWrap());
			}
		}
	}
	break;
	// Draw is the edit cursor for a text box that is editable.
	case EncodedInstanceId::Type::kCursor:
	{
		Float fXPosition = 0.0f;
		Float fYPosition = 0.0f;
		Float fLineHeight = 0.0f;

		auto const uCursorIndex = id.GetCursorIndex();
		if (uCursorIndex < m_vTextChunks.GetSize())
		{
			const TextChunk& textChunk = m_vTextChunks[uCursorIndex];

			// We must compute X manually since the right glyph border points at the last
			// break option of the chunk (it ignores trailing whitespace), and for the cursor,
			// we want to include trailing whitespace.
			fXPosition = textChunk.m_fRightGlyphBorder;
			if (textChunk.m_iBegin != textChunk.m_iEnd)
			{
				auto const& fontData = textChunk.m_Format.m_Font;
				auto const pFont = fontData.m_pData.GetPtr();

				Float fX0 = 0.0f;
				Float fY0 = 0.0f;
				Float fWidth = 0.0f;
				Float fHeight = 0.0f;
				if (pFont->Measure(
					textChunk.m_iBegin,
					textChunk.m_iEnd,
					fontData.m_Overrides,
					textChunk.m_Format.GetTextHeight(),
					fX0,
					fY0,
					fWidth,
					fHeight,
					true))
				{
					fXPosition = textChunk.m_fXOffset + (fX0 + fWidth);
				}
			}

			fYPosition = textChunk.m_fYOffset;
			fLineHeight = textChunk.m_Format.GetLineHeight();
		}
		else
		{
			switch (GetAlignment())
			{
			case HtmlAlign::kCenter:
				fXPosition = 0.5f * (m_fRight + m_fLeft);
				break;
			case HtmlAlign::kRight:
				fXPosition = m_fRight;
				break;
			case HtmlAlign::kLeft: // fall-through
			default:
				fXPosition = GetLineStart(false);
				break;
			};

			fYPosition = GetInitialY();
			if (m_pEditTextDefinition->GetFontDefinition().IsValid() &&
				m_pEditTextDefinition->GetFontDefinition()->GetFont().m_pData.IsValid())
			{
				auto const& fontData = m_pEditTextDefinition->GetFontDefinition()->GetFont();
				auto const fTextHeight = fontData.m_Overrides.m_fRescale * (Float)m_pEditTextDefinition->GetFontHeight();
				auto pFont = fontData.m_pData;
				fLineHeight = pFont->ComputeLineHeightFromTextHeight(fontData.m_Overrides, fTextHeight);
			}
			else
			{
				fLineHeight = m_pEditTextDefinition->GetFontHeight();
			}
		}

		FixedArray<ShapeVertex, 4> aVertices;

		Vector2D const v = Matrix2x3::TransformPosition(mWorld, Vector2D(fXPosition, fYPosition));
		Float const fHeight = (fLineHeight * mWorld.M11); // Adjust height by vertical scale.
		Float const fWidth = 1.0f / Max(rDrawer.GetState().m_fWorldWidthToScreenWidth, 1e-4f);

		Float const fX0 = v.X;
		Float const fX1 = v.X + fWidth;
		Float const fY0 = v.Y;
		Float const fY1 = v.Y + fHeight;

		aVertices[0] = ShapeVertex::Create(fX0, fY0, m_CursorColor, RGBA::TransparentBlack());
		aVertices[1] = ShapeVertex::Create(fX0, fY1, m_CursorColor, RGBA::TransparentBlack());
		aVertices[2] = ShapeVertex::Create(fX1, fY1, m_CursorColor, RGBA::TransparentBlack());
		aVertices[3] = ShapeVertex::Create(fX1, fY0, m_CursorColor, RGBA::TransparentBlack());

		auto const eFeature = (RGBA::White() != m_CursorColor ? Render::Feature::kColorMultiply : Render::Feature::kNone);
		rDrawer.DrawTriangleList(
			worldBoundsPreClip,
			textureReference,
			Matrix2x3::Identity(),
			cxWorld,
			aVertices.Data(),
			4,
			TriangleListDescription::kQuadList,
			eFeature);
	}
	break;
	// Draw is an image embedded in the text box.
	case EncodedInstanceId::Type::kImage:
	{
		FixedArray<ShapeVertex, 4> aVertices;

		auto const localBounds = GetLocalBounds();
		auto& entry = m_vImages[id.GetImageIndex()];

		// Refresh texcoord rectangle.
		entry.m_vTextureCoordinates.X = textureReference.m_vVisibleOffset.X;
		entry.m_vTextureCoordinates.Y = textureReference.m_vVisibleOffset.Y;
		entry.m_vTextureCoordinates.Z = textureReference.m_vVisibleOffset.X + textureReference.m_vVisibleScale.X;
		entry.m_vTextureCoordinates.W = textureReference.m_vVisibleOffset.Y + textureReference.m_vVisibleScale.Y;

		// Compute UV and position values.
		Float const fWidth = entry.GetWidth();
		Float const fHeight = entry.GetHeight();

		// Initial visible UV values.
		Float32 const fTU0 = entry.m_vTextureCoordinates.X;
		Float32 const fTV0 = entry.m_vTextureCoordinates.Y;
		Float32 fTU1 = entry.m_vTextureCoordinates.Z;
		Float32 fTV1 = entry.m_vTextureCoordinates.W;

		// Compute position values - X1 and Y1 can be clipped by the bounds.
		Float32 const fX0 = (fTU0 * fWidth) + entry.m_fXOffset;
		Float32 const fY0 = (fTV0 * fHeight) + entry.m_fYOffset;
		Float32 const fX1 = Min((fTU1 * fWidth) + entry.m_fXOffset, localBounds.m_fRight);
		Float32 const fY1 = Min((fTV1 * fHeight) + entry.m_fYOffset, localBounds.m_fBottom);

		// Recompute U1V1 values to factor in clipping.
		fTU1 = ((fX1 - fX0) / fWidth) + fTU0;
		fTV1 = ((fY1 - fY0) / fHeight) + fTV0;

		// Generate vertices.
		aVertices[0] = ShapeVertex::Create(fX0, fY0, RGBA::White(), RGBA::TransparentBlack(), fTU0, fTV0);
		aVertices[1] = ShapeVertex::Create(fX0, fY1, RGBA::White(), RGBA::TransparentBlack(), fTU0, fTV1);
		aVertices[2] = ShapeVertex::Create(fX1, fY1, RGBA::White(), RGBA::TransparentBlack(), fTU1, fTV1);
		aVertices[3] = ShapeVertex::Create(fX1, fY0, RGBA::White(), RGBA::TransparentBlack(), fTU1, fTV0);

		rDrawer.DrawTriangleList(
			worldBoundsPreClip,
			textureReference,
			mWorld,
			cxWorld,
			aVertices.Get(0),
			4,
			TriangleListDescription::kQuadList,
			Render::Feature::kNone);
	}
	break;
	// Draw is a text chunk that has a detail/face texture.
	case EncodedInstanceId::Type::kTextChunkWithDetail:
	{
		auto const localBounds = GetLocalBounds();
		// Get the chunk.
		auto const uChunkIndex = id.GetDetailTextChunkIndex();
		auto chunk = m_vTextChunks[uChunkIndex];
		// Shorten if needed.
		if (SEOUL_UNLIKELY(m_VisibleCharacters.m_uPartiallyVisibleTextChunk == uChunkIndex))
		{
			InternalApplyVisibleToChunk(chunk);
		}
		// Settings.
		auto const pSettings = g_Config.m_pGetTextEffectSettings(chunk.m_Format.m_TextEffectSettings);
		// Override color now.
		if (nullptr != pSettings)
		{
			// Apply color from the text effect if it is defined.
			if (nullptr != pSettings->m_pTextColor)
			{
				auto const c = RGBA::Create(*pSettings->m_pTextColor);
				chunk.m_Format.m_TextColor = c;
				chunk.m_Format.m_SecondaryTextColor = c;
			}
			else
			{
				if (nullptr != pSettings->m_pTextColorTop)
				{
					auto const c = RGBA::Create(*pSettings->m_pTextColorTop);
					chunk.m_Format.m_TextColor = c;
				}
				if (nullptr != pSettings->m_pTextColorBottom)
				{
					auto const c = RGBA::Create(*pSettings->m_pTextColorBottom);
					chunk.m_Format.m_SecondaryTextColor = c;
				}
			}
		}

		// Main/primary/top color, depending on the value of m_SecondaryColor.
		// Always used so always computed.
		RGBA const rgba = TransformColor(
			cxWorld,
			chunk.m_Format.m_TextColor);

		// A more optimized draw function if the text chunk is a single
		// homogenous color top-to-bottom.
		if (chunk.m_Format.m_TextColor == chunk.m_Format.m_SecondaryTextColor)
		{
			rDrawer.DrawTextChunk(
				chunk,
				mWorld,
				rgba,
				localBounds,
				!GetCanWordWrap(),
				Render::SettingsSDF(),
				pSettings,
				&textureReference);
		}
		// Otherwise, use a slightly more expensive draw function to handle
		// a separate secondary (bottom) color.
		else
		{
			RGBA const rgbaSecondary = TransformColor(
				cxWorld,
				chunk.m_Format.m_SecondaryTextColor);

			rDrawer.DrawTextChunk(
				chunk,
				mWorld,
				rgba,
				rgbaSecondary,
				localBounds,
				!GetCanWordWrap(),
				Render::SettingsSDF(),
				pSettings,
				&textureReference);
		}
	}
	break;
	default:
		SEOUL_FAIL("Out-of-sync enum.");
		break;
	}
}

void EditTextInstance::DrawOutline(Render::Drawer& rDrawer, const Rectangle & localBounds, const TextChunk& rTextChunk, const Matrix2x3& mWorld, const ColorTransformWithAlpha& cxWorld, const Vector2D& outlineOffset, const ColorARGBu8& outlineColor, const UInt8 outlineWidth, const UInt8 outlineBlur)
{
	TextChunk outlineTextChunk = rTextChunk;

	// Compute the x and y offsets applied to the outline
	Float const fXOffset = outlineOffset.X;
	Float const fYOffset = outlineOffset.Y;

	// Apply the offsets and color to the text chunk.
	outlineTextChunk.m_fLeftGlyphBorder += fXOffset;
	outlineTextChunk.m_fRightGlyphBorder += fXOffset;
	outlineTextChunk.m_fXOffset += fXOffset;
	outlineTextChunk.m_fTopGlyphBorder += fYOffset;
	outlineTextChunk.m_fBottomGlyphBorder += fYOffset;
	outlineTextChunk.m_fYOffset += fYOffset;

	RGBA const color(RGBA::Create(outlineColor));
	outlineTextChunk.m_Format.m_TextColor = color;
	outlineTextChunk.m_Format.m_SecondaryTextColor = color;

	// Compute the outline color.
	RGBA const rgba = TransformColor(
		cxWorld,
		outlineTextChunk.m_Format.m_TextColor);

	// Configure SDF settings for outline and blur.
	Render::SettingsSDF settingsSDF(
		(UInt8)Clamp((Int)Render::SettingsSDF::kuBaseThreshold - (Int)outlineWidth, 1, 254),
		(UInt8)Clamp((Int)Render::SettingsSDF::kuBaseTolerance + (Int)outlineBlur, 1, 254));

	// Draw the text chunk.
	rDrawer.DrawTextChunk(
		outlineTextChunk,
		mWorld,
		rgba,
		localBounds,
		!GetCanWordWrap(),
		settingsSDF);
}

Rectangle EditTextInstance::GetLocalBounds() const
{
	Rectangle ret = m_pEditTextDefinition->GetBounds();
	ret.m_fBottom = m_fBottom;
	ret.m_fLeft = m_fLeft;
	ret.m_fRight = m_fRight;
	return ret;
}

Bool EditTextInstance::GetLocalTextBounds(Rectangle& bounds) const
{
	if (m_vTextChunks.IsEmpty() && m_vImages.IsEmpty())
	{
		return false;
	}

	bounds.m_fLeft = ComputeContentsLeft();
	bounds.m_fRight = ComputeContentsRight();
	bounds.m_fTop = ComputeContentsTop();
	bounds.m_fBottom = ComputeContentsBottom();

	return true;
}

Bool EditTextInstance::GetTextBounds(Rectangle& bounds) const
{
	if (GetLocalTextBounds(bounds))
	{
		bounds = TransformRectangle(GetTransform(), bounds);
		return true;
	}
	return false;
}

Bool EditTextInstance::GetWorldTextBounds(Rectangle& bounds) const
{
	if (GetLocalTextBounds(bounds))
	{
		bounds = TransformRectangle(ComputeWorldTransform(), bounds);
		return true;
	}
	return false;
}

Bool EditTextInstance::HitTest(
	const Matrix2x3& mParent,
	Float32 fWorldX,
	Float32 fWorldY,
	Bool bIgnoreVisibility /*= false*/) const
{
	if (!bIgnoreVisibility)
	{
		if (!GetVisible())
		{
			return false;
		}
	}

	Matrix2x3 const mWorld = (mParent * GetTransform());
	Matrix2x3 const mInverseWorld = mWorld.Inverse();

	Vector2D const vObjectSpace = Matrix2x3::TransformPosition(mInverseWorld, Vector2D(fWorldX, fWorldY));

	// Text box hit testing is based around the content
	// bounds, not the local bounds.
	Rectangle localBounds;
	if (!GetLocalTextBounds(localBounds))
	{
		return false;
	}

	Float32 const fObjectSpaceX = vObjectSpace.X;
	Float32 const fObjectSpaceY = vObjectSpace.Y;

	if (fObjectSpaceX < localBounds.m_fLeft) { return false; }
	if (fObjectSpaceY < localBounds.m_fTop) { return false; }
	if (fObjectSpaceX > localBounds.m_fRight) { return false; }
	if (fObjectSpaceY > localBounds.m_fBottom) { return false; }

	return true;
}

Bool EditTextInstance::LinkHitTest(
	SharedPtr<EditTextLink>& rpLinkIndex,
	Float32 fWorldX,
	Float32 fWorldY)
{
	Matrix2x3 const mWorldTransform = ComputeWorldTransform();
	Matrix2x3 const mInverseWorldTransform = mWorldTransform.Inverse();

	Vector2D const vObjectSpace = Matrix2x3::TransformPosition(mInverseWorldTransform, Vector2D(fWorldX, fWorldY));

	UInt16 uLinkCount = m_vLinks.GetSize();
	for(UInt16 uLinkIndex = 0; uLinkIndex < uLinkCount; uLinkIndex++)
	{
		SharedPtr<EditTextLink> link = m_vLinks[uLinkIndex];
		UInt16 uChunkCount = link->m_vBounds.GetSize();
		for(UInt16 uChunkIndex = 0; uChunkIndex < uChunkCount; uChunkIndex++)
		{
			Rectangle bounds = link->m_vBounds[uChunkIndex];
			if(vObjectSpace.X > bounds.m_fLeft && vObjectSpace.X < bounds.m_fRight &&
				vObjectSpace.Y > bounds.m_fTop && vObjectSpace.Y < bounds.m_fBottom)
			{
				rpLinkIndex = link;
				return true;
			}
		}
	}
	return false;
}

HtmlAlign EditTextInstance::GetAlignment() const
{
	return m_pEditTextDefinition->GetAlignment();
}

Float EditTextInstance::AdvanceLine(Float fCurrentY, TextChunk& rTextChunk)
{
	// Apply image alignment and baselines before advancing.
	ApplyImageAlignmentAndFixupBaseline(rTextChunk.m_iLine);

	Float fY = fCurrentY;

	// Advance the initial Y.
	{
		Float const fYAdvance = rTextChunk.m_Format.GetLineHeight() + rTextChunk.m_Format.GetLineGap() + m_pEditTextDefinition->GetLeading();
		fY += fYAdvance;
	}

	// Check the computed Y against other text chunks on the same line.
	{
		Int32 const iSize = (Int32)m_vTextChunks.GetSize();
		for (Int32 i = (iSize - 1); i >= 0; --i)
		{
			const TextChunk& chunk = m_vTextChunks[i];

			// If we hit a text chunk on a previous line, we're done.
			if (chunk.m_iLine != rTextChunk.m_iLine)
			{
				break;
			}

			// Compute the next Y of the chunk.
			Float const fYAdvance =
				chunk.m_Format.GetLineHeight() +
				rTextChunk.m_Format.GetLineGap() +
				m_pEditTextDefinition->GetLeading();

			// Take the max.
			fY = Max(fY, (chunk.m_fYOffset + fYAdvance));
		}
	}

	// Check the computed Y against images.
	{
		Int32 const iSize = (Int32)m_vImages.GetSize();
		for (Int32 i = (iSize - 1); i >= 0; --i)
		{
			const ImageEntry& entry = m_vImages[i];

			// If we hit an image on a previous line, we're done.
			if (entry.m_iStartingTextLine != rTextChunk.m_iLine)
			{
				break;
			}

			// If the bottom of the image is a greater Y than the desired, update the Y.
			fY = Max(fY, (entry.m_fYOffset + (Float)entry.GetHeight() + entry.m_fYMargin));
		}
	}

	// Update the chunk line.
	++rTextChunk.m_iLine;

	// Return the computed Y.
	return fY;
}

void EditTextInstance::ComputeGlyphBounds()
{
	// Now that the text chunks have been created and formatted, compute top/bottom
	// borders for rendering and and associate them with their links, if any.
	for (auto& rChunk : m_vTextChunks)
	{
		Bool bOk = false;
		if (rChunk.m_iBegin != rChunk.m_iEnd)
		{
			rChunk.m_fTopGlyphBorder = FloatMax;
			rChunk.m_fBottomGlyphBorder = -FloatMax;

			Float fY = rChunk.m_fYOffset;

			auto const& fontData = rChunk.m_Format.m_Font;
			CookedTrueTypeFontData* pFont = fontData.m_pData.GetPtr();
			Float const fTextHeight = rChunk.m_Format.GetTextHeight();
			Float const fScaleForPixelHeight = pFont->GetScaleForPixelHeight(fTextHeight);
			Float ascent = fScaleForPixelHeight * fontData.m_pData->GetAscent(fontData.m_Overrides);
			for (auto i = rChunk.m_iBegin; rChunk.m_iEnd != i; ++i)
			{
				UniChar const c = *i;
				{
					Float fGlyphY0, fGlyphY1;
					if (GetGlyphY0Y1(fY, c, rChunk, fGlyphY0, fGlyphY1))
					{
						rChunk.m_fTopGlyphBorder = Min(rChunk.m_fTopGlyphBorder, fGlyphY0 + ascent);
						rChunk.m_fBottomGlyphBorder = Max(rChunk.m_fBottomGlyphBorder, fGlyphY1 + ascent);
						bOk = true;
					}
				}
			}
		}

		// In cases of (e.g.) characters that don't render (e.g. '\n'),
		// we must give reasonable values.
		if (!bOk)
		{
			rChunk.m_fTopGlyphBorder = rChunk.m_fYOffset;
			rChunk.m_fBottomGlyphBorder = rChunk.m_fYOffset + rChunk.m_Format.GetLineHeight();
		}
	}
}

void EditTextInstance::ApplyAlignmentAndCentering()
{
	// Apply horizontal alignment to text and images.
	{
		auto const bounds = m_pEditTextDefinition->GetBounds();
		auto const fRightMargin = GetRightMargin();
		auto const fLineCenter = bounds.GetCenter().X;
		TextChunks::SizeType const uChunks = m_vTextChunks.GetSize();
		Images::SizeType const uImages = m_vImages.GetSize();
		Int32 iLine = 0;
		TextChunks::SizeType uFirstChunk = 0;
		Images::SizeType uFirstImage = 0;

		// Process all text chunks and images, line by line.
		while (uFirstChunk < uChunks || uFirstImage < uImages)
		{
			// Find the last chunk and image.
			auto uEndChunk = uFirstChunk;
			while (uEndChunk < uChunks)
			{
				if (iLine != m_vTextChunks[uEndChunk].m_iLine) { break; }
				++uEndChunk;
			}
			auto uEndImage = uFirstImage;
			while (uEndImage < uImages)
			{
				if (iLine != m_vImages[uEndImage].m_iStartingTextLine) { break; }
				++uEndImage;
			}

			// The horizontal alignment mode for the line defaults to left always.
			HtmlAlign eAlignment = HtmlAlign::kLeft;
			// Scan images and line for other modes.
			for (UInt32 i = uFirstChunk; i < uEndChunk; ++i)
			{
				eAlignment = (HtmlAlign)Max((Int)eAlignment, (Int)m_vTextChunks[i].m_Format.GetAlignmentEnum());
			}
			for (UInt32 i = uFirstImage; i < uEndImage; ++i)
			{
				eAlignment = (HtmlAlign)Max((Int)eAlignment, (Int)m_vImages[i].m_eAlignment);
			}

			// Potentially apply an adjustment if not left and we have
			// at least one image or one text chunk.
			if (eAlignment != HtmlAlign::kLeft &&
				(uEndChunk > uFirstChunk || uEndImage > uFirstImage))
			{
				auto const fRight = (uFirstChunk < uEndChunk
					? (uFirstImage < uEndImage ? Max(m_vTextChunks[uEndChunk - 1u].m_fRightGlyphBorder, m_vImages[uEndImage - 1u].GetRightBorder()) : m_vTextChunks[uEndChunk - 1u].m_fRightGlyphBorder)
					: m_vImages[uEndImage - 1u].GetRightBorder());

				Float fAdjustment = 0.0f;

				// Matching behavior - Flash cancels any margin on the left side
				// built into a glyph or image (so, the x offset of the left most glyph
				// *or* the margin of the leftmost image).
				Float fImageX0 = FloatMax, fImageXMargin = 0.0f;
				Float fTextX0 = FloatMax, fTextXMargin = 0.0f;
				if (uFirstImage < uEndImage)
				{
					auto const& image = m_vImages[uFirstImage];
					fImageX0 = image.m_fXOffset;
					fImageXMargin = image.m_fXMargin;
				}
				if (uFirstChunk < uEndChunk)
				{
					auto const& chunk = m_vTextChunks[uFirstChunk];
					fTextX0 = chunk.m_fLeftGlyphBorder;
					fTextXMargin = Max(chunk.m_fLeftGlyphBorder - chunk.m_fXOffset, 0.0f);
				}

				if (fImageX0 < fTextX0) { fAdjustment = -fImageXMargin; }
				else if (fTextX0 < fImageX0) { fAdjustment = -fTextXMargin; }

				switch (eAlignment)
				{
				case HtmlAlign::kCenter:
				{
					auto const fLeft = (uFirstChunk < uEndChunk
						? (uFirstImage < uEndImage ? Min(m_vTextChunks[uFirstChunk].m_fLeftGlyphBorder, m_vImages[uFirstImage].m_fXOffset) : m_vTextChunks[uFirstChunk].m_fLeftGlyphBorder)
						: m_vImages[uFirstImage].m_fXOffset);

					fAdjustment += (fLineCenter - ((fLeft + fRight) * 0.5f));
				}
				break;
				case HtmlAlign::kRight:
				{
					fAdjustment += (fRightMargin - fRight);
				}
				break;
				default:
					SEOUL_WARN("'%s': Unsupported or unknown horizontal alignment mode '%s'.",
						GetName().CStr(),
						ToString(eAlignment));
					break;
				};

				// Flash appears to ignore centering or right alignment if a line
				// of text extends beyond the borders of the text box, and instead
				// falls back to left alignment. For text, we need to check for the
				// actual left border of the first glyph, since Flash allows the centering
				// unless it will actually clip the glyph.
				if (!m_bAutoSizeHorizontal && HtmlAlign::kLeft != eAlignment && fAdjustment < 0.0f)
				{
					// We can only apply this custom behavior if there is at least
					// one text chunk on the line.
					if (uFirstChunk < uEndChunk)
					{
						const TextChunk& firstChunk = m_vTextChunks[uFirstChunk];
						Float fGlyphLeft;
						if (GetGlyphX0(
							firstChunk.m_fXOffset,
							*firstChunk.m_iBegin,
							firstChunk,
							fGlyphLeft))
						{
							Float const fCheck =
								fGlyphLeft +
								fAdjustment +
								kfHorizontalAlignmentOutOfBoundsTolerance;

							if (fCheck < m_pEditTextDefinition->GetBounds().m_fLeft)
							{
								fAdjustment = 0.0f;
							}
						}
					}
				}

				if (fAdjustment != 0.0f)
				{
					for (auto i = uFirstChunk; i < uEndChunk; ++i)
					{
						m_vTextChunks[i].m_fXOffset += fAdjustment;
						m_vTextChunks[i].m_fLeftGlyphBorder += fAdjustment;
						m_vTextChunks[i].m_fRightGlyphBorder += fAdjustment;
					}

					for (auto i = uFirstImage; i < uEndImage; ++i)
					{
						m_vImages[i].m_fXOffset += fAdjustment;
					}
				}
			}

			// Advance.
			uFirstChunk = uEndChunk;
			uFirstImage = uEndImage;

			// Advance to next line.
			++iLine;
		}
	}

	// Apply vertical centering.
	if ((m_bVerticalCenter || m_bXhtmlVerticalCenter))
	{
		Float fMinY = (Float)ComputeContentsTopFromGlyphBounds();
		Float fMaxY = (Float)ComputeContentsBottomFromGlyphBounds();

		if (fMaxY >= fMinY)
		{
			Float const fTextCenter = (fMinY + fMaxY) * 0.5f;
			Float const fTextBoxCenter = m_pEditTextDefinition->GetBounds().GetCenter().Y;
			Float const fYOffset = (fTextBoxCenter - fTextCenter);

			if (fYOffset > 0.0f)
			{
				TextChunks::SizeType const uChunks = m_vTextChunks.GetSize();
				for (TextChunks::SizeType i = 0; i < uChunks; ++i)
				{
					m_vTextChunks[i].m_fTopGlyphBorder += fYOffset;
					m_vTextChunks[i].m_fBottomGlyphBorder += fYOffset;
					m_vTextChunks[i].m_fYOffset += fYOffset;
				}

				Images::SizeType const uImages = m_vImages.GetSize();
				for (Images::SizeType i = 0; i < uImages; ++i)
				{
					m_vImages[i].m_fYOffset += fYOffset;
				}
			}
		}
	}
}

// TODO: The behavior of this function is not expected based on the argument.
// It always processes the last line currently in the text chunk and images lists,
// if that line is equal to iLine, otherwise it processes nothing.

void EditTextInstance::ApplyImageAlignmentAndFixupBaseline(Int32 iLine)
{
	// Outside the fixup scope to be conditionally used for
	// image vertical alignment.
	Float fMaxBaseline = 0.0f;
	Bool bMaxBaseline = false;

	// Outside the fixup scope to be conditionally used
	// to fixup images that extend above the line top.
	Int32 const iTextChunksSize = (Int32)m_vTextChunks.GetSize();
	Int32 iFirstChunk = iTextChunksSize;

	// Fixup chunk base lines so that all text chunks on the specified line have the same
	// base line.
	{
		// Find the chunk range (the first chunk on the same line as the last chunk).
		for (Int32 i = (iTextChunksSize - 1); i >= 0; --i)
		{
			const TextChunk& chunk = m_vTextChunks[i];

			// If we hit a text chunk on a previous line, we're done.
			if (chunk.m_iLine != iLine)
			{
				break;
			}

			// Otherwise, update the start.
			iFirstChunk = i;
		}

		// Find the max baseline.
		for (Int32 i = iFirstChunk; i < iTextChunksSize; ++i)
		{
			const TextChunk& chunk = m_vTextChunks[i];

			auto const& fontData = chunk.m_Format.m_Font;
			CookedTrueTypeFontData* pFont = fontData.m_pData.GetPtr();
			Float const fTextHeight = chunk.m_Format.GetTextHeight();
			Float const fScaleForPixelHeight = pFont->GetScaleForPixelHeight(fTextHeight);
			Float const fFontAscent = fScaleForPixelHeight * (Float)pFont->GetAscent(fontData.m_Overrides);
			Float const fBaseline = chunk.m_fYOffset + fFontAscent;
			if (bMaxBaseline)
			{
				fMaxBaseline = Max(fMaxBaseline, fBaseline);
			}
			else
			{
				fMaxBaseline = fBaseline;
			}
			bMaxBaseline = true;
		}

		// Process chunks and adjust - only do so if there are at least 2 chunks.
		if (iFirstChunk + 1 < iTextChunksSize)
		{
			// Adjust.
			for (Int32 i = iFirstChunk; i < iTextChunksSize; ++i)
			{
				TextChunk& rChunk = m_vTextChunks[i];

				auto const& fontData = rChunk.m_Format.m_Font;
				CookedTrueTypeFontData* pFont = fontData.m_pData.GetPtr();
				Float const fTextHeight = rChunk.m_Format.GetTextHeight();
				Float const fScaleForPixelHeight = pFont->GetScaleForPixelHeight(fTextHeight);
				Float const fFontAscent = fScaleForPixelHeight * (Float)pFont->GetAscent(fontData.m_Overrides);
				Float const fBaseline = rChunk.m_fYOffset + fFontAscent;
				Float const fAdjust = (fMaxBaseline - fBaseline);

				rChunk.m_fTopGlyphBorder += fAdjust;
				rChunk.m_fBottomGlyphBorder += fAdjust;
				rChunk.m_fYOffset += fAdjust;
			}
		}
	}

	// Apply image vertical centering for the line, if there are some text chunks on the line
	// (bMaxBaseline has been set).
	if (bMaxBaseline)
	{
		// Find the image range (the first image on the same line as the last chunk).
		Int32 const iImagesSize = (Int32)m_vImages.GetSize();
		Int32 iFirstImage = iImagesSize;

		// Find the image range (the first image on the same line as the last chunk).
		for (Int32 i = (iImagesSize - 1); i >= 0; --i)
		{
			const ImageEntry& imageEntry = m_vImages[i];

			// If we hit an image on a previous line, we're done.
			if (imageEntry.m_iStartingTextLine != iLine)
			{
				break;
			}

			// Otherwise, update the start.
			iFirstImage = i;
		}

		// Process if we have at least one image.
		if (iFirstImage < iImagesSize)
		{
			for (Int32 i = iFirstImage; i < iImagesSize; ++i)
			{
				ImageEntry& rEntry = m_vImages[i];

				switch (rEntry.m_eImageAlignment)
				{
				case HtmlImageAlign::kTop:
					// Nop
					break;

				case HtmlImageAlign::kMiddle:
					// Nop
					break;

				case HtmlImageAlign::kBottom:
					{
						// Find the vertical bottom of the image, and then align it to the baseline
						// of the current line.
						Float const fImageVerticalBottom = rEntry.m_fYOffset + (Float)rEntry.GetHeight() + rEntry.m_fYMargin;
						Float const fAdjustment = (fMaxBaseline - fImageVerticalBottom);
						rEntry.m_fYOffset += fAdjustment;
					}
					break;

					// TODO: We don't support kLeft or kRight.
				default:
					SEOUL_WARN("'%s': Unsupported or unknown image alignment mode '%s'.",
						GetName().CStr(),
						ToString(rEntry.m_eImageAlignment));
					break;
				};
			}

			// Find the text and image bounds and apply centering as appropriate for image
			// modes.
			{
				// Min text y of the line.
				Float fMinTextY = FloatMax;
				Float fMaxTextY = -FloatMax;
				{
					for (Int32 i = iFirstChunk; i < iTextChunksSize; ++i)
					{
						const TextChunk& chunk = m_vTextChunks[i];
						fMinTextY = Min(fMinTextY, chunk.m_fYOffset);
						fMaxTextY = Max(fMaxTextY, chunk.m_fYOffset + chunk.m_Format.GetTextHeight());
					}
				}

				// Min image y of the line.
				Float fMinImageY = FloatMax;
				Float fMaxImageY = -FloatMax;
				HtmlImageAlign eMode = m_vImages[iFirstImage].m_eImageAlignment; // TODO: Consider others.
				{
					for (Int32 i = iFirstImage; i < iImagesSize; ++i)
					{
						const ImageEntry& imageEntry = m_vImages[i];
						fMinImageY = Min(fMinImageY, imageEntry.m_fYOffset - imageEntry.m_fYMargin);
						fMaxImageY = Max(fMaxImageY, imageEntry.m_fYOffset + imageEntry.GetHeight() + imageEntry.m_fYMargin);
					}
				}

				// Adjust if the image min is above (a smaller value compared to) the text min.
				if (HtmlImageAlign::kTop == eMode)
				{
					if (fMinImageY < fMinTextY)
					{
						Float const fAdjustment = (fMinTextY - fMinImageY);
						for (Int32 i = iFirstChunk; i < iTextChunksSize; ++i)
						{
							TextChunk& rChunk = m_vTextChunks[i];
							rChunk.m_fTopGlyphBorder += fAdjustment;
							rChunk.m_fBottomGlyphBorder += fAdjustment;
							rChunk.m_fYOffset += fAdjustment;
						}
						for (Int32 i = iFirstImage; i < iImagesSize; ++i)
						{
							ImageEntry& rEntry = m_vImages[i];
							rEntry.m_fYOffset += fAdjustment;
						}
					}
				}
				else if (HtmlImageAlign::kMiddle == eMode)
				{
					auto const fImageCenter = (fMinImageY + fMaxImageY) * 0.5f;
					for (Int32 i = iFirstChunk; i < iTextChunksSize; ++i)
					{
						TextChunk& rChunk = m_vTextChunks[i];
						auto const fCenter = rChunk.ComputeCenterY();
						auto const fAdjust = (fImageCenter - fCenter);

						rChunk.m_fTopGlyphBorder += fAdjust;
						rChunk.m_fBottomGlyphBorder += fAdjust;
						rChunk.m_fYOffset += fAdjust;
					}
					for (Int32 i = iFirstImage; i < iImagesSize; ++i)
					{
						ImageEntry& rEntry = m_vImages[i];
						auto const fCenter = rEntry.ComputeCenterY();
						rEntry.m_fYOffset += (fImageCenter - fCenter);
					}
				}
			}
		}
	}
}

void EditTextInstance::AutoSizeBottom()
{
	if (!m_bAutoSizeBottom)
	{
		m_fBottom = m_pEditTextDefinition->GetBounds().m_fBottom;
		return;
	}

	m_fBottom = ComputeContentsBottom();
}

void EditTextInstance::AutoSizeHorizontal()
{
	if (!m_bAutoSizeHorizontal)
	{
		m_fLeft = m_pEditTextDefinition->GetBounds().m_fLeft;
		m_fRight = m_pEditTextDefinition->GetBounds().m_fRight;
		return;
	}

	m_fLeft = ComputeContentsLeft();
	m_fRight = ComputeContentsRight();
}

void EditTextInstance::CheckFormatting(AdvanceInterface& rInterface)
{
	if (m_bUseInitialText)
	{
		HString const sName = GetName();
		if (!sName.IsEmpty() && sName.CStr()[0] == '$')
		{
			HString const sLocalizationToken(sName.CStr() + 1);

			String sLocalizedText;
			if (rInterface.FalconLocalize(sLocalizationToken, sLocalizedText))
			{
				SetText(sLocalizedText);
			}
			else if (!m_pEditTextDefinition->GetInitialText().IsEmpty())
			{
				SetText(m_pEditTextDefinition->GetInitialText());
			}
		}
		else
		{
			if (!m_pEditTextDefinition->GetInitialText().IsEmpty())
			{
				SetText(m_pEditTextDefinition->GetInitialText());
			}
		}

		m_bUseInitialText = false;
	}

	CheckFormatting();
}

void EditTextInstance::CheckFormatting()
{
	if (m_bNeedsFormatting)
	{
		FormatText();
		m_bNeedsFormatting = false;
	}
}

void EditTextInstance::CloneTo(AddInterface& rInterface, EditTextInstance& rClone) const
{
	Instance::CloneTo(rInterface, rClone);

	// First, copy all members through.
	rClone.m_vImages = m_vImages;
	rClone.m_vLinks = m_vLinks;
	rClone.m_vTextChunks = m_vTextChunks;
	rClone.m_sText = m_sText;
	rClone.m_sMarkupText = m_sMarkupText;
	rClone.m_fCursorBlinkTimer = m_fCursorBlinkTimer;
	rClone.m_CursorColor = m_CursorColor;
	rClone.m_fBottom = m_fBottom;
	rClone.m_fLeft = m_fLeft;
	rClone.m_fRight = m_fRight;
	rClone.m_uOptions = m_uOptions;

	// Next, fixup text chunks - need to rebase the pointers
	// against the new copy of m_sText.
	TextChunks::SizeType const uTextChunks = rClone.m_vTextChunks.GetSize();
	for (TextChunks::SizeType i = 0; i < uTextChunks; ++i)
	{
		TextChunk& rChunk = rClone.m_vTextChunks[i];
		rChunk.m_iBegin.SetPtr(rClone.m_sText.CStr());
		rChunk.m_iEnd.SetPtr(rClone.m_sText.CStr());
	}
}

Float EditTextInstance::ComputeContentsBottom() const
{
	// Initialize the bottom to the top (min reasonable value for the bottom).
	Float fBottom = m_pEditTextDefinition->GetBounds().m_fTop;

	// If there are text chunks, size to the bottom most line.
	if (!m_vTextChunks.IsEmpty())
	{
		Int const iLine = m_vTextChunks.Back().m_iLine;
		for (Int i = (Int)m_vTextChunks.GetSize() - 1; i >= 0; --i)
		{
			const TextChunk& chunk = m_vTextChunks[i];
			if (chunk.m_iLine != iLine)
			{
				break;
			}

			fBottom = Max(fBottom, (chunk.m_fYOffset + chunk.m_Format.GetLineHeight()));
		}
	}

	// If there are images, size to the bottom most line.
	if (!m_vImages.IsEmpty())
	{
		Int const iLine = m_vImages.Back().m_iStartingTextLine;
		for (Int i = (Int)m_vImages.GetSize() - 1; i >= 0; --i)
		{
			const ImageEntry& entry = m_vImages[i];
			if (entry.m_iStartingTextLine != iLine)
			{
				break;
			}

			fBottom = Max(fBottom, (entry.m_fYOffset + (Float)entry.GetHeight() + entry.m_fYMargin));
		}
	}

	return fBottom;
}

Float EditTextInstance::ComputeContentsTop() const
{
	// Initialize the top to the bottom (max reasonable value for the top).
	Float fTop = m_pEditTextDefinition->GetBounds().m_fBottom;

	if (!m_vTextChunks.IsEmpty())
	{
		Int const iLine = m_vTextChunks.Begin()->m_iLine;
		for (Int i = 0; i < (Int)m_vTextChunks.GetSize(); ++i)
		{
			const TextChunk& chunk = m_vTextChunks[i];
			if (chunk.m_iLine != iLine)
			{
				break;
			}

			fTop = Min(fTop, (chunk.m_fYOffset));
		}
	}

	if (!m_vImages.IsEmpty())
	{
		Int const iLine = m_vImages.Begin()->m_iStartingTextLine;
		for (Int i = 0; i < (Int)m_vImages.GetSize(); ++i)
		{
			const ImageEntry& entry = m_vImages[i];
			if (entry.m_iStartingTextLine != iLine)
			{
				break;
			}

			fTop = Min(fTop, (entry.m_fYOffset));
		}
	}

	return fTop;
}


Float EditTextInstance::ComputeContentsTopFromGlyphBounds() const
{
	// Initialize the top to the bottom (max reasonable value for the top).
	Float fTop = m_pEditTextDefinition->GetBounds().m_fBottom;
	if (!m_vTextChunks.IsEmpty())
	{
		Int const iLine = m_vTextChunks.Begin()->m_iLine;
		for (Int i = 0; i < (Int)m_vTextChunks.GetSize(); ++i)
		{
			const TextChunk& chunk = m_vTextChunks[i];
			if (chunk.m_iLine != iLine)
			{
				break;
			}

			fTop = Min(fTop, chunk.m_fTopGlyphBorder);
		}
	}

	if (!m_vImages.IsEmpty())
	{
		Int const iLine = m_vImages.Begin()->m_iStartingTextLine;
		for (Int i = 0; i < (Int)m_vImages.GetSize(); ++i)
		{
			const ImageEntry& entry = m_vImages[i];
			if (entry.m_iStartingTextLine != iLine)
			{
				break;
			}

			fTop = Min(fTop, (entry.m_fYOffset - entry.m_fYMargin));
		}
	}

	return fTop;
}

Float EditTextInstance::ComputeContentsBottomFromGlyphBounds() const
{
	// Initialize the bottom to the top (min reasonable value for the bottom).
	Float fBottom = m_pEditTextDefinition->GetBounds().m_fTop;

	// If there are text chunks, size to the bottom most line.
	if (!m_vTextChunks.IsEmpty())
	{
		Int const iLine = m_vTextChunks.Back().m_iLine;
		for (Int i = (Int)m_vTextChunks.GetSize() - 1; i >= 0; --i)
		{
			const TextChunk& chunk = m_vTextChunks[i];
			if (chunk.m_iLine != iLine)
			{
				break;
			}

			fBottom = Max(fBottom, chunk.m_fBottomGlyphBorder);
		}
	}

	// If there are images, size to the bottom most line.
	if (!m_vImages.IsEmpty())
	{
		Int const iLine = m_vImages.Back().m_iStartingTextLine;
		for (Int i = (Int)m_vImages.GetSize() - 1; i >= 0; --i)
		{
			const ImageEntry& entry = m_vImages[i];
			if (entry.m_iStartingTextLine != iLine)
			{
				break;
			}

			fBottom = Max(fBottom, (entry.m_fYOffset + (Float)entry.GetHeight() + entry.m_fYMargin));
		}
	}

	return fBottom;
}

Float EditTextInstance::ComputeContentsLeft() const
{
	// Initialize the left to the right (max reasonable value for the left).
	Float fLeft = m_fRight;

	// Accumulate text chunks.
	for (auto i = m_vTextChunks.Begin(); m_vTextChunks.End() != i; ++i)
	{
		fLeft = Min(fLeft, i->m_fLeftGlyphBorder);
	}

	// Accumulate images.
	for (auto i = m_vImages.Begin(); m_vImages.End() != i; ++i)
	{
		fLeft = Min(fLeft, i->m_fXOffset);
	}

	return fLeft;
}


Float EditTextInstance::ComputeContentsRight() const
{
	// Initialize the right to the left (min reasonable value for the right).
	Float fRight = m_fLeft;

	// Accumulate text chunks.
	for (auto i = m_vTextChunks.Begin(); m_vTextChunks.End() != i; ++i)
	{
		fRight = Max(fRight, i->m_fRightGlyphBorder);
	}

	// Accumulate images.
	for (auto i = m_vImages.Begin(); m_vImages.End() != i; ++i)
	{
		fRight = Max(fRight, i->m_fXOffset + i->GetWidth());
	}

	return fRight;
}

class EditTextInstance::PlainAutoSizeUtil SEOUL_SEALED
{
public:
	SEOUL_DELEGATE_TARGET(PlainAutoSizeUtil);

	PlainAutoSizeUtil(EditTextInstance& r)
		: m_r(r)
	{
	}

	void Func(Float fAutoSizeRescale)
	{
		m_r.FormatPlainTextInner(fAutoSizeRescale);
	}

private:
	EditTextInstance& m_r;

	SEOUL_DISABLE_COPY(PlainAutoSizeUtil);
}; // class PlainAutoSizeUtil

void EditTextInstance::FormatPlainText()
{
	// Perform formatting with auto sizing rescaling. Will be conditionally
	// enabled inside FormatWithAutoContentSizing().
	PlainAutoSizeUtil util(*this);
	FormatWithAutoContentSizing(SEOUL_BIND_DELEGATE(&PlainAutoSizeUtil::Func, &util));
}

void EditTextInstance::FormatPlainTextInner(Float fAutoSizeRescale)
{
	// Cleanup state.
	ResetFormattedState();

	TextChunk textChunk;
	if (!GetInitialTextChunk(textChunk, fAutoSizeRescale))
	{
		return;
	}

	LineBreakRecord noneRecord;
	FormatTextChunk(noneRecord, textChunk);

	// Apply image alignment/baseline fixup to the last line.
	{
		Int32 const iLastLine = (GetNumLines() - 1);
		if (iLastLine >= 0)
		{
			ApplyImageAlignmentAndFixupBaseline(iLastLine);
		}
	}
}

void EditTextInstance::FormatText()
{
	// Bulk of formatting.
	if (m_bXhtmlParsing)
	{
		FormatXhtmlText();
	}
	else
	{
		FormatPlainText();
	}

	ComputeGlyphBounds();
	ApplyAlignmentAndCentering();
	AutoSizeBottom();
	AutoSizeHorizontal();

	// Now that the text chunks have been created and formatted, go through
	// and associate them with their links.
	for (auto& rChunk : m_vTextChunks)
	{
		if (rChunk.m_Format.m_iLinkIndex >= 0)
		{
			m_vLinks[rChunk.m_Format.m_iLinkIndex]->m_vBounds.PushBack(rChunk.ComputeGlyphBounds());
		}
	}

	// Also associate the images with links
	Images::SizeType const zImagesSize = m_vImages.GetSize();
	for (Images::SizeType i = 0; i < zImagesSize; ++i)
	{
		ImageEntry& rImage = m_vImages[i];
		if (rImage.m_iLinkIndex >= 0)
		{
			Rectangle bounds;
			bounds.m_fLeft = (rImage.m_fXOffset);
			bounds.m_fTop = (rImage.m_fYOffset);
			bounds.m_fRight = (bounds.m_fLeft + rImage.GetWidth());
			bounds.m_fBottom = (bounds.m_fTop + rImage.GetHeight());
			m_vLinks[rImage.m_iLinkIndex]->m_vBounds.PushBack(bounds);
		}
	}

	// Refresh visible characters config now that
	// we've reformatted the text.
	InternalRefreshVisibleCharacters();
}

void EditTextInstance::FormatTextChunk(
	LineBreakRecord& rLastLineBreakOption,
	TextChunk& rInOutTextChunk,
	Bool bAllowReflow /*= true*/)
{
	auto const& fmt = rInOutTextChunk.m_Format;
	auto const& pFont = fmt.m_Font.m_pData;
	auto const iBegin = rInOutTextChunk.m_iBegin;
	auto const iEnd = rInOutTextChunk.m_iEnd;
	auto const bCanWordWrap = GetCanWordWrap();
	auto const fWordWrapMargin = GetWordWrapX();
	auto const bMultiline = m_pEditTextDefinition->IsMultiline();
	auto const fTextHeight = fmt.GetTextHeight();
	auto const fOneGlyphPixel = pFont.IsValid() ? (pFont->GetOneEmForPixelHeight(fTextHeight)) : 0.0f;

	Bool bHasInternalBreakOption = false;

	// Account for a few code paths that can leave the cursor position outside the text bounds.
	if (bCanWordWrap && rInOutTextChunk.m_fXOffset > fWordWrapMargin)
	{
		rInOutTextChunk.m_fXOffset = GetLineStart(false);
		rInOutTextChunk.m_fYOffset = AdvanceLine(rInOutTextChunk.m_fYOffset, rInOutTextChunk);
	}

	Float fX = rInOutTextChunk.m_fXOffset;
	Float fX1 = fX;
	Float fY = rInOutTextChunk.m_fYOffset;

	UniChar lastChar = '\0';
	if (!m_vTextChunks.IsEmpty())
	{
		auto const& last = m_vTextChunks.Back();

		// TODO: Remove this, error prone.
		// Use raw indices since iterators may currently be invalid.
		auto const uBegin = last.m_iBegin.GetIndexInBytes();
		auto const uEnd = last.m_iEnd.GetIndexInBytes();
		if (uBegin != uEnd)
		{
			auto const iLast = String::Iterator(m_sText.CStr(), uEnd) - 1;
			lastChar = *iLast;
		}
	}

	TextChunk chunk = rInOutTextChunk;
	for (auto i = iBegin; iEnd != i; )
	{
		UniChar const c = *i;
		if ((c == '\r' || c == '\n') && bMultiline)
		{
			chunk.m_iEnd = i;

			// +1 so that (right - left) = width.
			chunk.m_fRightGlyphBorder = (fX1 + fOneGlyphPixel);
			if (chunk.m_iBegin != chunk.m_iEnd)
			{
				if (!GetGlyphX0(chunk.m_fXOffset, *chunk.m_iBegin, chunk, chunk.m_fLeftGlyphBorder))
				{
					chunk.m_fLeftGlyphBorder = chunk.m_fXOffset;
				}

				m_vTextChunks.PushBack(chunk);
			}

			if (c == '\r')
			{
				if ((i + 1) != iEnd && *(i + 1) == '\n')
				{
					++i;
				}
			}

			rLastLineBreakOption.Reset();
			bHasInternalBreakOption = false;
			fX = GetLineStart(false);
			fX1 = fX;
			fY = AdvanceLine(fY, chunk);

			chunk.m_iBegin = (i + 1);
			chunk.m_iEnd = iEnd;
			chunk.m_fXOffset = fX;
			chunk.m_fYOffset = fY;
			chunk.m_uNumberOfCharacters = 0;

			++i;
			lastChar = c;
			continue;
		}
		else if (CanBreak(lastChar, c))
		{
			// Commit the line break option now that we have a proper break.
			rLastLineBreakOption.m_f = fX;
			rLastLineBreakOption.m_uOffset = i.GetIndexInBytes();
			rLastLineBreakOption.m_uNumberOfCharacters = chunk.m_uNumberOfCharacters;
			rLastLineBreakOption.m_uTextChunk = m_vTextChunks.GetSize();
			bHasInternalBreakOption = true;
		}

		// Check if we need to wrap to the next line
		Float fGlyphX1;
		if (!GetGlyphX1(fX, c, chunk, fGlyphX1))
		{
			chunk.m_uNumberOfCharacters++;
			++i;
			lastChar = c;
			continue;
		}

		// Try reflow of the current line in this case. This
		// accounts for text chunks generated for formatting
		// only.
		if (!bHasInternalBreakOption &&
			bCanWordWrap &&
			fGlyphX1 > fWordWrapMargin &&
			rLastLineBreakOption.IsValid() &&
			bAllowReflow)
		{
			auto const fPrevX = fX;
			Reflow(rLastLineBreakOption, fX, fY, chunk);
			fGlyphX1 = (fGlyphX1 - fPrevX) + fX;

			// No more option unless we find a new one.
			rLastLineBreakOption.Reset();
			bAllowReflow = false;
		}

		if (bCanWordWrap &&
			fGlyphX1 > fWordWrapMargin &&
			chunk.m_uNumberOfCharacters > 1) // don't word-wrap if simply one character (e.g. a period after a formatted string, or a single number, etc).
		{
			// Must have an internal (within the current chunk) break option.
			// Configure on current state if we don't already
			// (can occur on very long lines).
			if (!bHasInternalBreakOption)
			{
				rLastLineBreakOption.m_f = fX;
				rLastLineBreakOption.m_uNumberOfCharacters = chunk.m_uNumberOfCharacters;
				rLastLineBreakOption.m_uOffset = i.GetIndexInBytes();
				rLastLineBreakOption.m_uTextChunk = m_vTextChunks.GetSize();
			}

			// Setup the chunk.
			auto const iLastBreakOption = String::Iterator(m_sText.CStr(), rLastLineBreakOption.m_uOffset);
			chunk.m_iEnd = iLastBreakOption;
			chunk.m_uNumberOfCharacters = rLastLineBreakOption.m_uNumberOfCharacters;

			// +1 so (right - left) = width.
			chunk.m_fRightGlyphBorder = (rLastLineBreakOption.m_f + fOneGlyphPixel); // +1 so (right - left) = width.

			// Commit the chunk.
			if (chunk.m_iBegin != chunk.m_iEnd)
			{
				if (!GetGlyphX0(chunk.m_fXOffset, *chunk.m_iBegin, chunk, chunk.m_fLeftGlyphBorder))
				{
					chunk.m_fLeftGlyphBorder = chunk.m_fXOffset;
				}

				m_vTextChunks.PushBack(chunk);
			}

			// Go back to the break. Space breaks, we exclude them, unless that would place i
			// at the end. Otherwise, they will be included after the break.
			i = (IsWhiteSpace(*iLastBreakOption) && (iLastBreakOption + 1) != iEnd) ? (iLastBreakOption + 1) : iLastBreakOption;

			// Newline
			rLastLineBreakOption.Reset();
			bHasInternalBreakOption = false;
			fX = GetLineStart(false);
			fX1 = fX;
			fY = AdvanceLine(fY, chunk);

			chunk.m_iBegin = i;
			chunk.m_iEnd = iEnd;
			chunk.m_fXOffset = fX;
			chunk.m_fYOffset = fY;
			chunk.m_uNumberOfCharacters = 0;

			lastChar = c;
			continue;
		}

		Float const fTextHeight = chunk.m_Format.GetTextHeight();
		Float const fAdvance = pFont->GetGlyphAdvance(c, fTextHeight) + chunk.m_Format.GetLetterSpacing();
		fX += fAdvance;
		fX1 = fGlyphX1;
		chunk.m_uNumberOfCharacters++;
		++i;
		lastChar = c;
	}

	if (chunk.m_iBegin != iEnd)
	{
		chunk.m_iEnd = iEnd;

		// +1 so (right - left) = width.
		chunk.m_fRightGlyphBorder = (fX1 + fOneGlyphPixel);

		if (chunk.m_iBegin != chunk.m_iEnd)
		{
			if (!GetGlyphX0(chunk.m_fXOffset, *chunk.m_iBegin, chunk, chunk.m_fLeftGlyphBorder))
			{
				chunk.m_fLeftGlyphBorder = chunk.m_fXOffset;
			}

			m_vTextChunks.PushBack(chunk);
		}
	}

	rInOutTextChunk.m_fXOffset = fX;
	rInOutTextChunk.m_fYOffset = fY;
	rInOutTextChunk.m_iLine = chunk.m_iLine;
}

/**
 * Auto-sizing, when enabled, attempts to (uniformally) scale the contents of
 * a text box, to avoid clipping it against the border.
 *
 * If m_bAutoSizeContents is false, this method calls formatter and returns.
 *
 * If m_bAutoSizeContents is true, this method may call formatter multiple times
 * to test various sizes. It will return when the best match under various constraints
 * has been found.
 *
 * Formatter must be implemented to behave correctly when called iteratively - e.g. it
 * must clear appropriate state to repopulate it with each iteration.
 */
void EditTextInstance::FormatWithAutoContentSizing(
	const Delegate<void(Float fAutoSizeRescale)>& formatter)
{
	// Our minimum rescale is 0.6 with these values - 0.5 step each
	// time with 8 steps, starting at 0.95f;
	static const Float kfStepSize = 0.05f;
	static const Int32 kiMaxSteps = 8;
	static const Float kfMinRescale = (1.0f - kfStepSize * (Float)kiMaxSteps);
	static const Float kfMaxOverlap = 4.0f;

	// Always format once with no resizing.
	formatter(1.0f);

	// If auto sizing is enabled, nothing more to do.
	if (!m_bAutoSizeContents)
	{
		return;
	}

	// Different possibilities depending on line mode.

	// Multiline and word wrap is handled uniquely - recompute
	// rescale needs based on bottom border, and reflow if necessary.
	if (GetCanWordWrap())
	{
		// With multiline and word wrapping, nothing to do
		// if we're also auto sizing the bottom border.
		if (m_bAutoSizeBottom)
		{
			return;
		}

		// Also nothing to do if we have no images or text chunks.
		if (m_vImages.IsEmpty() && m_vTextChunks.IsEmpty())
		{
			return;
		}

		// Compute the bottom of the contents area, and check it. If it
		// is already within the bounds, nothing to do. We give a little
		// wiggle to account for cases where text is slightly bigger than
		// the bounds, which is accounted for by various padding.
		Float const fContentsBottom = ComputeContentsBottom();
		if (fContentsBottom <= m_fBottom + kfMaxOverlap)
		{
			return;
		}

		// Rough approximation - use the amount we'd need
		// to reduce the scale to fit the bottom (without reflow),
		// rescaled by 2.0f to be conservative. Since the actual amount
		// will always be equal or less than this.
		//
		// An alternative would be to search backward from this value,
		// although that would mean we'd always need to reflow 1 extra
		// time (once we find the point where we no longer fit, we
		// need to revert back to the previous state).
		Int32 const iStart = Clamp(
			(Int32)((1.0f - (m_fBottom / fContentsBottom)) / (kfStepSize * 2.0f)),
			1,
			kiMaxSteps);

		// Iterate and reflow with a gradually increasing rescale size until the
		// contents fit. Stop immediately at that point.
		for (Int32 i = iStart; i <= kiMaxSteps; ++i)
		{
			Float const fAutoSizeRescale = (1.0f - kfStepSize * (Float)i);
			formatter(fAutoSizeRescale);
			Float const fNewBottom = ComputeContentsBottom();

			if (fNewBottom <= m_fBottom + kfMaxOverlap)
			{
				break;
			}
		}
	}
	// Multiline without wrap and single line can be handled with the same approach -
	// compute the right border, check it against the clip border, and if it
	// is greater, rescale to fit it.
	else
	{
		// If we're not multiline and/or not word wrapping, we need
		// to size contents with horizontal mode, unless auto size
		// horizontal is true.
		if (m_bAutoSizeHorizontal)
		{
			return;
		}

		Float const fContentsRight = ComputeContentsRight();
		Float const fMarginRight = GetRightMargin();

		// Contents already in bounds, early out.
		if (fContentsRight <= fMarginRight + kfMaxOverlap)
		{
			return;
		}

		Float const fContentsWidth = (fContentsRight - m_pEditTextDefinition->GetBounds().m_fLeft);
		Float const fBaseWidth = (
			fMarginRight - GetLineStart(false));
		Float const fAutoSizeRescale = Clamp(
			fBaseWidth / fContentsWidth,
			kfMinRescale,
			1.0f);

		formatter(fAutoSizeRescale);
	}
}

Bool EditTextInstance::GetCanWordWrap() const
{
	return !m_bAutoSizeHorizontal && (m_pEditTextDefinition->IsMultiline() && m_pEditTextDefinition->HasWordWrap());
}

Bool EditTextInstance::GetGlyphX0(Float fX, UniChar c, const TextChunk& textChunk, Float& rfX0) const
{
	auto pFont = textChunk.m_Format.m_Font.m_pData.GetPtr();
	if (!pFont) { return false; }

	auto const fTextHeight = textChunk.m_Format.GetTextHeight();
	auto const fScaleForPixelHeight = pFont->GetScaleForPixelHeight(fTextHeight);
	auto const pGlyph = pFont->GetGlyph(c);
	if (nullptr == pGlyph) { return false; }

	rfX0 = (fX + (Float)pGlyph->m_iX0 * fScaleForPixelHeight);
	return true;
}

Bool EditTextInstance::GetGlyphX1(Float fX, UniChar c, const TextChunk& textChunk, Float& rfX1) const
{
	auto pFont = textChunk.m_Format.m_Font.m_pData.GetPtr();
	if (!pFont) { return false; }

	auto const fTextHeight = textChunk.m_Format.GetTextHeight();
	auto const fScaleForPixelHeight = pFont->GetScaleForPixelHeight(fTextHeight);
	auto const pGlyph = pFont->GetGlyph(c);
	if (nullptr == pGlyph) { return false; }

	rfX1 = (fX + (Float)pGlyph->m_iX1 * fScaleForPixelHeight);
	return true;
}

Bool EditTextInstance::GetGlyphY0Y1(Float fY, UniChar c, const TextChunk& textChunk, Float& rfY0, Float& rfY1) const
{
	auto pFont = textChunk.m_Format.m_Font.m_pData.GetPtr();
	if (!pFont) { return false; }

	auto const fTextHeight = textChunk.m_Format.GetTextHeight();
	auto const fScaleForPixelHeight = pFont->GetScaleForPixelHeight(fTextHeight);
	auto const pGlyph = pFont->GetGlyph(c);
	if (nullptr == pGlyph) { return false; }

	rfY0 = (fY + (Float)pGlyph->m_iY0 * fScaleForPixelHeight);
	rfY1 = (fY + (Float)pGlyph->m_iY1 * fScaleForPixelHeight);
	return true;
}

Bool EditTextInstance::GetGlyphY0(Float fY, UniChar c, const TextChunk& textChunk, Float& rfY0) const
{
	auto pFont = textChunk.m_Format.m_Font.m_pData.GetPtr();
	if (!pFont) { return false; }

	auto const fTextHeight = textChunk.m_Format.GetTextHeight();
	auto const fScaleForPixelHeight = pFont->GetScaleForPixelHeight(fTextHeight);
	auto const pGlyph = pFont->GetGlyph(c);
	if (nullptr == pGlyph) { return false; }

	rfY0 = (fY + (Float)pGlyph->m_iY0 * fScaleForPixelHeight);
	return true;
}

Bool EditTextInstance::GetGlyphY1(Float fY, UniChar c, const TextChunk& textChunk, Float& rfY1) const
{
	auto pFont = textChunk.m_Format.m_Font.m_pData.GetPtr();
	if (!pFont) { return false; }

	auto const fTextHeight = textChunk.m_Format.GetTextHeight();
	auto const fScaleForPixelHeight = pFont->GetScaleForPixelHeight(fTextHeight);
	auto const pGlyph = pFont->GetGlyph(c);
	if (nullptr == pGlyph) { return false; }

	rfY1 = (fY + (Float)pGlyph->m_iY1 * fScaleForPixelHeight);
	return true;
}

Bool EditTextInstance::GetInitialTextChunk(TextChunk& rTextChunk, Float fAutoSizeRescale) const
{
	SharedPtr<FontDefinition> pFontDefinition = m_pEditTextDefinition->GetFontDefinition();
	if (!pFontDefinition.IsValid())
	{
		SEOUL_WARN("'%s': Attempt to format text with undefined font '%s'",
			GetName().CStr(),
			m_pEditTextDefinition->GetFontDefinitionName().CStr());
		return false;
	}

	rTextChunk.m_fXOffset = GetLineStart(true);
	rTextChunk.m_fYOffset = GetInitialY();
	rTextChunk.m_iBegin = m_sText.Begin();
	rTextChunk.m_iEnd = m_sText.End();
	rTextChunk.m_uNumberOfCharacters = 0;
	rTextChunk.m_Format.SetAlignmentEnum(GetAlignment());
	rTextChunk.m_Format.SetUnscaledLetterSpacing(0.0f);
	rTextChunk.m_Format.m_Font = pFontDefinition->GetFont();

	// Matching Flash behavior - it appears that when text is set via
	// SetXhtml() (what was htmlText = in ActionScript), the default font is always supposed
	// to be the regular version of the font style, even if the default font is (e.g.) bold.
	if ((m_pEditTextDefinition->Html() || m_bXhtmlParsing) &&
		(rTextChunk.m_Format.m_Font.m_bBold || rTextChunk.m_Format.m_Font.m_bItalic))
	{
		if (!g_Config.m_pGetFont(
			rTextChunk.m_Format.m_Font.m_sName,
			false,
			false,
			rTextChunk.m_Format.m_Font))
		{
			SEOUL_WARN("'%s': Attempt to format text with undefined regular (non-bold, non-italic) font '%s'",
				GetPath(this).CStr(),
				rTextChunk.m_Format.m_Font.m_sName.CStr());

			// TODO: Return an error instead?
			//
			// fall-through
		}
	}

	rTextChunk.m_Format.m_Font.m_Overrides.m_fRescale *= fAutoSizeRescale;
	rTextChunk.m_Format.SetUnscaledTextHeight((Float)m_pEditTextDefinition->GetFontHeight());
	rTextChunk.m_Format.m_TextColor = (m_pEditTextDefinition->HasTextColor()
		? m_pEditTextDefinition->GetTextColor()
		: RGBA::White());
	rTextChunk.m_Format.m_SecondaryTextColor = (m_pEditTextDefinition->HasTextColor()
		? m_pEditTextDefinition->GetSecondaryTextColor()
		: RGBA::White());

	return true;
}

Float EditTextInstance::GetInitialY() const
{
	Rectangle const bounds = m_pEditTextDefinition->GetBounds();

	Float fReturn = (Float)bounds.m_fTop;
	fReturn += m_pEditTextDefinition->GetTopMargin();
	return fReturn;
}

Float EditTextInstance::GetLineStart(Bool bNewParagraph) const
{
	Float fReturn = (m_fLeft + m_pEditTextDefinition->GetLeftMargin());

	if (bNewParagraph)
	{
		fReturn += (Float)m_pEditTextDefinition->GetIndent();
	}

	return fReturn;
}

Float EditTextInstance::GetLineRightBorder(Int iLine, TextChunks::SizeType& ruTextChunk, Images::SizeType& ruImage) const
{
	// The initial value for the right border is the left border.
	Float fRightBorder = GetLineStart(false);

	// Scan text chunks for the right most border.
	{
		TextChunks::SizeType const uTextChunks = m_vTextChunks.GetSize();
		for (; ruTextChunk < uTextChunks; ++ruTextChunk)
		{
			const TextChunk& textChunk = m_vTextChunks[ruTextChunk];
			if (iLine != textChunk.m_iLine)
			{
				// Stop once we've hit a chunk on a different line.
				break;
			}

			// Take the max of the existing value and the text chunk's right border value.
			fRightBorder = Max(fRightBorder, textChunk.m_fRightGlyphBorder);
		}
	}

	// Scan images for the right most border.
	{
		Images::SizeType const uImages = m_vImages.GetSize();
		for (; ruImage < uImages; ++ruImage)
		{
			const ImageEntry& image = m_vImages[ruImage];
			if (iLine != image.m_iStartingTextLine)
			{
				// Stop once we've hit an image on a different line.
				break;
			}

			// Take the max of the existing value and the image's right border value.
			fRightBorder = Max(fRightBorder, image.m_fXOffset + image.GetWidth());
		}
	}

	// Return the result.
	return fRightBorder;
}

Float EditTextInstance::GetRightMargin() const
{
	Float fReturn = (m_fRight - m_pEditTextDefinition->GetRightMargin());

	return fReturn;
}

Float EditTextInstance::GetWordWrapX() const
{
	Float fReturn = (m_fRight - m_pEditTextDefinition->GetWordWrapMargin());

	return fReturn;
}

void EditTextInstance::ResetFormattedState()
{
	m_vImages.Clear();
	m_vLinks.Clear();
	m_vTextChunks.Clear();
	m_fBottom = m_pEditTextDefinition->GetBounds().m_fBottom;
	m_fLeft = m_pEditTextDefinition->GetBounds().m_fLeft;
	m_fRight = m_pEditTextDefinition->GetBounds().m_fRight;
	m_bXhtmlVerticalCenter = false;
}

void EditTextInstance::Reflow(
	const LineBreakRecord& option,
	Float& rfX,
	Float& rfY,
	TextChunk& rTextChunk)
{
	// TODO: This needs to update link references
	// and also reflow images.

	// Must not be called without an option.
	SEOUL_ASSERT(option.IsValid());

	// Split at the target text chunk, backup
	// trailing, then re-add them fixed.
	auto lastChunk = m_vTextChunks[option.m_uTextChunk];
	lastChunk.m_iBegin = String::Iterator(m_sText.CStr(), option.m_uOffset);

	// Split the last chunk.
	m_vTextChunks[option.m_uTextChunk].m_fRightGlyphBorder = option.m_f;
	m_vTextChunks[option.m_uTextChunk].m_iEnd = String::Iterator(m_sText.CStr(), option.m_uOffset);
	m_vTextChunks[option.m_uTextChunk].m_uNumberOfCharacters = option.m_uNumberOfCharacters;

	// Append to our buffer for reflow.
	TextChunks vTextChunks;
	vTextChunks.Append(
		m_vTextChunks.Begin() + option.m_uTextChunk + 1,
		m_vTextChunks.End());
	m_vTextChunks.Erase(m_vTextChunks.Begin() + option.m_uTextChunk + 1, m_vTextChunks.End());

	// Process the new chunk.
	lastChunk.m_fXOffset = GetLineStart(true);
	lastChunk.m_fYOffset = AdvanceLine(lastChunk.m_fYOffset, lastChunk);
	lastChunk.m_uNumberOfCharacters = 0u;
	lastChunk.m_iBegin = String::Iterator(m_sText.CStr(), lastChunk.m_iBegin.GetIndexInBytes());
	lastChunk.m_iEnd = String::Iterator(m_sText.CStr(), lastChunk.m_iEnd.GetIndexInBytes());

	// Space breaks exclude the space unless that would push us past the end.
	if (lastChunk.m_iBegin != lastChunk.m_iEnd && IsWhiteSpace(*lastChunk.m_iBegin))
	{
		++lastChunk.m_iBegin;
	}

	// No last break when reflow.
	LineBreakRecord noneRecord;
	FormatTextChunk(noneRecord, lastChunk);

	// Now reappend the new start and reflowed text chunks.
	for (auto& e : vTextChunks)
	{
		// Carry through.
		e.m_fXOffset = lastChunk.m_fXOffset;
		e.m_fYOffset = lastChunk.m_fYOffset;
		e.m_iLine = lastChunk.m_iLine;
		e.m_uNumberOfCharacters = 0u;
		e.m_iBegin = String::Iterator(m_sText.CStr(), e.m_iBegin.GetIndexInBytes());
		e.m_iEnd = String::Iterator(m_sText.CStr(), e.m_iEnd.GetIndexInBytes());

		// Update and submit.
		noneRecord = LineBreakRecord{};
		lastChunk = e;
		FormatTextChunk(noneRecord, lastChunk);
	}

	// Finally, apply the fixups to rTextChunk.
	rfX = (rfX - rTextChunk.m_fXOffset) + lastChunk.m_fXOffset;
	rfY = (rfY - rTextChunk.m_fYOffset) + lastChunk.m_fYOffset;
	rTextChunk.m_fXOffset = lastChunk.m_fXOffset;
	rTextChunk.m_fYOffset = lastChunk.m_fYOffset;
	rTextChunk.m_iLine = lastChunk.m_iLine;
}

void EditTextInstance::InternalRefreshVisibleCharacters()
{
	// Nothing to do if max value.
	if (UIntMax == m_VisibleCharacters.m_uVisibleCount)
	{
		m_VisibleCharacters.m_uPartiallyVisibleCharacterCount = 0u;
		m_VisibleCharacters.m_uPartiallyVisibleTextChunk = UIntMax;
		return;
	}

	// Compute.
	UInt32 uCharacters = 0u;
	auto const uSize = m_vTextChunks.GetSize();
	for (UInt32 i = 0u; i < uSize; ++i)
	{
		auto const uNext = m_vTextChunks[i].m_uNumberOfCharacters;
		if (uCharacters + uNext > m_VisibleCharacters.m_uVisibleCount)
		{
			m_VisibleCharacters.m_uPartiallyVisibleTextChunk = i;
			m_VisibleCharacters.m_uPartiallyVisibleCharacterCount = (m_VisibleCharacters.m_uVisibleCount - uCharacters);
			return;
		}

		// Advance.
		uCharacters += uNext;
	}

	// If we get here, no partially visible chunk.
	m_VisibleCharacters.m_uPartiallyVisibleCharacterCount = 0u;
	m_VisibleCharacters.m_uPartiallyVisibleTextChunk = UIntMax;
}

void EditTextInstance::InternalApplyVisibleToChunk(TextChunk& rChunk) const
{
	auto iNewEnd = rChunk.m_iBegin;
	for (UInt32 i = 0u; i < m_VisibleCharacters.m_uPartiallyVisibleCharacterCount; ++i)
	{
		++iNewEnd;
	}

	rChunk.m_uNumberOfCharacters = m_VisibleCharacters.m_uPartiallyVisibleCharacterCount;
	rChunk.m_iEnd = iNewEnd;
}

} // namespace Falcon

} // namespace Seoul
