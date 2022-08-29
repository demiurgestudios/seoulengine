/**
 * \file FalconEditTextInstanceXhtmlParsing.cpp
 * \brief Functions of a Falcon::EditTextInstance specific
 * to our XHTML parsing facilities.
 *
 * A point of divergence, Falcon supports XHTML in text boxes
 * instead of HTML as is expected by Flash.
 *
 * This allows us to use a more widely available (as well
 * as simpler and typically faster) XML parser than an HTML
 * parser.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FilePath.h"
#include "FalconBitmapDefinition.h"
#include "FalconEditTextCommon.h"
#include "FalconEditTextDefinition.h"
#include "FalconEditTextInstance.h"
#include "FalconGlobalConfig.h"
#include "FalconMovieClipInstance.h"
#include "HtmlReader.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "ReflectionUtil.h"

namespace Seoul::Falcon
{

using namespace EditTextCommon;

void EditTextInstance::FormatNode(
	HtmlReader& rReader,
	LineBreakRecord& rLastLineBreakOption,
	HtmlTag eTag,
	TextChunk& rTextChunk,
	Float fAutoSizeRescale)
{
	HtmlTag eNextTag = HtmlTag::kUnknown;
	HtmlTagStyle eNextTagStyle = HtmlTagStyle::kNone;
	while (true)
	{
		rReader.ReadTag(eNextTag, eNextTagStyle);
		if (HtmlTagStyle::kTerminator == eNextTagStyle)
		{
			if (eNextTag != eTag)
			{
				SEOUL_WARN("%s(%d): mismatched begin/end tag: %s != %s, full string: %s",
					GetPath(this).CStr(),
					rReader.GetColumn(),
					EnumToString<HtmlTag>(eTag),
					EnumToString<HtmlTag>(eNextTag),
					m_sMarkupText.CStr());
				continue;
			}
			else
			{
				return;
			}
		}

		// Text chunk or nested node.
		if (HtmlTag::kTextChunk == eNextTag)
		{
			// Termination is indicated by a failure to read a text chunk.
			if (!rReader.ReadTextChunk(rTextChunk.m_iBegin, rTextChunk.m_iEnd))
			{
				return;
			}

			FormatTextChunk(rLastLineBreakOption, rTextChunk);
		}
		else
		{
			// Backup the text chunk for partial restoration after possible
			// recursion.
			TextChunk oldChunk = rTextChunk;

			// Apply attributes - style may be updated after consuming
			// attributes.
			PreFormat(rReader, eNextTag, rTextChunk, fAutoSizeRescale, eNextTagStyle);

			// Self terminating, Pre/Post but no recursion.
			if (HtmlTagStyle::kSelfTerminating != eNextTagStyle)
			{
				// Recurse.
				FormatNode(rReader, rLastLineBreakOption, eNextTag, rTextChunk, fAutoSizeRescale);
			}

			// Text chunk post handling.
			PostFormat(rLastLineBreakOption, eNextTag, rTextChunk);

			// Only restore formatting, not positioning.
			rTextChunk.m_Format = oldChunk.m_Format;
		}
	}
}

class EditTextInstance::XhtmlAutoSizeUtil SEOUL_SEALED
{
public:
	SEOUL_DELEGATE_TARGET(XhtmlAutoSizeUtil);

	XhtmlAutoSizeUtil(EditTextInstance& r)
		: m_r(r)
	{
	}

	void Func(Float fAutoSizeRescale)
	{
		m_r.FormatXhtmlTextInner(fAutoSizeRescale);
	}

private:
	EditTextInstance& m_r;

	SEOUL_DISABLE_COPY(XhtmlAutoSizeUtil);
}; // class XhtmlAutoSizeUtil

void EditTextInstance::FormatXhtmlText()
{
	if (m_sMarkupText.IsEmpty())
	{
		// No formatted text or images.
		ResetFormattedState();
		return;
	}

	// Clear m_sText, in preparation for repopulation with from m_sMarkupText.
	m_sText.Clear();

	// Verify existence of GetFontDefinition() prior
	// to calling Inner*, it is required that GetInitialTextChunk()
	// will succeed inside Inner, and then ensures that requirement.
	if (!m_pEditTextDefinition->GetFontDefinition().IsValid())
	{
		SEOUL_WARN("'%s': error, could not get initial chunk, check for invalid font \"%s\".",
			GetPath(this).CStr(),
			m_pEditTextDefinition->GetFontDefinitionName().CStr());

		// Restore the original m_sText.
		m_sText = m_sMarkupText;

		return;
	}

	// Perform formatting with auto sizing rescaling. Will be conditionally
	// enabled inside FormatWithAutoContentSizing().
	XhtmlAutoSizeUtil util(*this);
	FormatWithAutoContentSizing(SEOUL_BIND_DELEGATE(&XhtmlAutoSizeUtil::Func, &util));
}

void EditTextInstance::FormatXhtmlTextInner(Float fAutoSizeRescale)
{
	// Initially no state in this case.
	ResetFormattedState();

	// Clear text - it will be reaccumulated by FormatNode().
	m_sText.Clear();

	// Assert - conditions for the success of GetInitialTextChunk() should
	// have been enforced by FormatXhtmlText().
	TextChunk textChunk;
	SEOUL_VERIFY(GetInitialTextChunk(textChunk, fAutoSizeRescale));

	// Perform the actual formatting.
	HtmlReader reader(m_sMarkupText.Begin(), m_sMarkupText.End(), m_sText);
	LineBreakRecord record;
	FormatNode(
		reader,
		record,
		HtmlTag::kRoot,
		textChunk,
		fAutoSizeRescale);

	// Need to fix up iterators, as m_sText.CStr() can be invalidated
	// while formatting text.
	TextChunks::SizeType const zSize = m_vTextChunks.GetSize();
	for (TextChunks::SizeType i = 0; i < zSize; ++i)
	{
		TextChunk& rChunk = m_vTextChunks[i];
		rChunk.m_iBegin = StringIterator(m_sText.CStr(), rChunk.m_iBegin.GetIndexInBytes());
		rChunk.m_iEnd = StringIterator(m_sText.CStr(), rChunk.m_iEnd.GetIndexInBytes());
	}

	// Apply image alignment/baseline fixup to the last line.
	{
		Int32 const iLastLine = (GetNumLines() - 1);
		if (iLastLine >= 0)
		{
			ApplyImageAlignmentAndFixupBaseline(iLastLine);
		}
	}
}

void EditTextInstance::PostFormat(
	LineBreakRecord& rLastLineBreakOption,
	HtmlTag eTag,
	TextChunk& rTextChunk)
{
	switch (eTag)
	{
	case HtmlTag::kBr: // fall-through
	case HtmlTag::kP:
		// Last valid line break invalidated on explicit newline.
		rLastLineBreakOption.Reset();

		rTextChunk.m_fXOffset = GetLineStart(true);
		rTextChunk.m_fYOffset = AdvanceLine(rTextChunk.m_fYOffset, rTextChunk);
		break;
	default:
		break;
	};
}

void EditTextInstance::PreFormat(
	HtmlReader& rReader,
	HtmlTag eTag,
	TextChunk& rTextChunk,
	Float fAutoSizeRescale,
	HtmlTagStyle& reStyle)
{
	HtmlAttribute eAttribute = HtmlAttribute::kUnknown;
	switch (eTag)
	{
	case HtmlTag::kB:
		if (!g_Config.m_pGetFont(
			rTextChunk.m_Format.m_Font.m_sName,
			true,
			rTextChunk.m_Format.m_Font.m_bItalic,
			rTextChunk.m_Format.m_Font))
		{
			SEOUL_WARN("'%s': Attempt to format text with undefined bold font '%s'",
				GetPath(this).CStr(),
				rTextChunk.m_Format.m_Font.m_sName.CStr());
		}
		else
		{
			rTextChunk.m_Format.m_Font.m_Overrides.m_fRescale *= fAutoSizeRescale;
		}
		break;

	case HtmlTag::kFont:
		while (rReader.ReadAttribute(eAttribute, reStyle))
		{
			switch (eAttribute)
			{
			case HtmlAttribute::kColor:
				{
					RGBA color{};
					rReader.ReadAttributeValue(color, rTextChunk.m_Format.m_TextColor);
					rTextChunk.m_Format.m_TextColor = color;
					rTextChunk.m_Format.m_SecondaryTextColor = color;
				}
				break;
			case HtmlAttribute::kColorTop:
				{
					RGBA color{};
					rReader.ReadAttributeValue(color, rTextChunk.m_Format.m_TextColor);
					rTextChunk.m_Format.m_TextColor = color;
				}
				break;
			case HtmlAttribute::kColorBottom:
				{
					RGBA color{};
					rReader.ReadAttributeValue(color, rTextChunk.m_Format.m_SecondaryTextColor);
					rTextChunk.m_Format.m_SecondaryTextColor = color;
				}
				break;
			case HtmlAttribute::kEffect:
				{
					rReader.ReadAttributeValue(rTextChunk.m_Format.m_TextEffectSettings);

					// Apply color from the text effect if it is defined.
					auto pSettings = g_Config.m_pGetTextEffectSettings(rTextChunk.m_Format.m_TextEffectSettings);
					if (nullptr == pSettings)
					{
						SEOUL_WARN("'%s': Attempt to format text with undefined text effect settings '%s'",
							GetPath(this).CStr(),
							rTextChunk.m_Format.m_TextEffectSettings.CStr());
					}
				}
				break;
			case HtmlAttribute::kFace:
				{
					HString name;
					rReader.ReadAttributeValue(name);
					if (!g_Config.m_pGetFont(
						name,
						rTextChunk.m_Format.m_Font.m_bBold,
						rTextChunk.m_Format.m_Font.m_bItalic,
						rTextChunk.m_Format.m_Font))
					{
						SEOUL_WARN("'%s': Attempt to format text with undefined font face '%s'",
							GetPath(this).CStr(),
							name.CStr());
					}
					else
					{
						rTextChunk.m_Format.m_Font.m_Overrides.m_fRescale *= fAutoSizeRescale;
					}
				}
				break;
			case HtmlAttribute::kLetterSpacing:
				{
					Float fLetterSpacing = 0.0f;
					rReader.ReadAttributeValue(fLetterSpacing, rTextChunk.m_Format.GetUnscaledLetterSpacing());
					rTextChunk.m_Format.SetUnscaledLetterSpacing(fLetterSpacing);
				}
				break;
			case HtmlAttribute::kSize:
				{
					Int32 iSize = 0;
					rReader.ReadAttributeValue(iSize, (Int32)rTextChunk.m_Format.GetUnscaledTextHeight());
					rTextChunk.m_Format.SetUnscaledTextHeight((Float)iSize);
				}
				break;
			default:
				break;
			};
		}
		break;
	case HtmlTag::kLink:
		{
			SharedPtr<EditTextLink> pLink(SEOUL_NEW(MemoryBudgets::Falcon) EditTextLink);
			while (rReader.ReadAttribute(eAttribute, reStyle))
			{
				switch (eAttribute)
				{
				case HtmlAttribute::kHref:
					{
						rReader.ReadAttributeValue(pLink->m_sLinkString);
					}
					break;
				case HtmlAttribute::kType:
					{
						rReader.ReadAttributeValue(pLink->m_sType);
					}
					break;
				default:
					break;
				}
			}

			if (m_vLinks.GetSize() <= (UInt32)Int16Max)
			{
				m_vLinks.PushBack(pLink);
				rTextChunk.m_Format.m_iLinkIndex = m_vLinks.GetSize() - 1;
			}
			else
			{
				SEOUL_WARN("'%s': Link count limit reached, all further links in this text box will be ignored.",
					GetPath(this).CStr());
			}
		}
		break;
	case HtmlTag::kI:
		if (!g_Config.m_pGetFont(
			rTextChunk.m_Format.m_Font.m_sName,
			rTextChunk.m_Format.m_Font.m_bBold,
			true,
			rTextChunk.m_Format.m_Font))
		{
			SEOUL_WARN("'%s': Attempt to format text with undefined italic font '%s'",
				GetPath(this).CStr(),
				rTextChunk.m_Format.m_Font.m_sName.CStr());
		}
		else
		{
			rTextChunk.m_Format.m_Font.m_Overrides.m_fRescale *= fAutoSizeRescale;
		}
		break;

	case HtmlTag::kImg:
		{
			ImageEntry entry;
			entry.m_fRescale = fAutoSizeRescale;
			Int32 iOptWidth = -1;
			Int32 iOptHeight = -1;
			Int32 iWidth = -1;
			Int32 iHeight = -1;
			Float fHOffset = 0.0f;
			Float fVOffset = 0.0f;

			// Note that the default hspace and vspace values in Flash are actually 8,
			// but this has been a PITA in practice - it's almost never the desired spacing
			// and often artists don't realize it/how to fix it. So we're defaulting to
			// 0 instead.
			Float fHSpace = 0.0f;
			Float fVSpace = 0.0f;

			// Matching Flash - I believe the standard alignment
			// in HTML would actually be bottom, but in Flash it appears
			// to be middle.
			HtmlImageAlign eImageAlignment = HtmlImageAlign::kMiddle;

			FilePath id;
			while (rReader.ReadAttribute(eAttribute, reStyle))
			{
				switch (eAttribute)
				{
				case HtmlAttribute::kAlign:
					rReader.ReadAttributeValue(eImageAlignment, eImageAlignment);
					break;
				case HtmlAttribute::kHeight:
					rReader.ReadAttributeValue(iHeight, -1);
					break;
				case HtmlAttribute::kHoffset:
					rReader.ReadAttributeValue(fHOffset, 0.0f);
					break;
				case HtmlAttribute::kHspace:
					rReader.ReadAttributeValue(fHSpace, 0.0f);
					break;
				case HtmlAttribute::kSrc:
					{
						String sUrl;
						rReader.ReadAttributeValue(sUrl);
						if (!g_Config.m_pResolveImageSource(m_pEditTextDefinition->GetFCNFileURL(), sUrl.CStr(), id, iOptWidth, iOptHeight))
						{
							SEOUL_WARN("'%s' text body contains invalid <img src='%s'...> tag, "
								"URL is invalid or source image does not exist.",
								GetPath(this).CStr(),
								sUrl.CStr());
						}
					}
					break;
				case HtmlAttribute::kVoffset:
					rReader.ReadAttributeValue(fVOffset, 0.0f);
					break;
				case HtmlAttribute::kVspace:
					rReader.ReadAttributeValue(fVSpace, 0.0f);
					break;
				case HtmlAttribute::kWidth:
					rReader.ReadAttributeValue(iWidth, -1);
					break;
				default:
					break;
				};
			}

			if (iWidth < 0) { iWidth = iOptWidth; }
			if (iHeight < 0) { iHeight = iOptHeight; }

			Bool bOk = true;
			if (iWidth <= 0)
			{
				SEOUL_WARN("'%s' text body contains invalid <img> tag, "
					"missing or invalid width= attribute.",
					GetPath(this).CStr());
				bOk = false;
			}
			if (iHeight <= 0)
			{
				SEOUL_WARN("'%s' text body contains invalid <img> tag, "
					"missing or invalid height= attribute.",
					GetPath(this).CStr());
				bOk = false;
			}
			if (!id.IsValid())
			{
				SEOUL_WARN("'%s' text body contains invalid <img> tag, "
					"missing or invalid src= attribute.",
					GetPath(this).CStr());
				bOk = false;
			}

			if (bOk)
			{
				// Unclear, but it appears that (possibly due to a math quirk
				// in our old Flash runtime) negative vspace values were clamped.
				//
				// We match this behavior for consistency.
				fVSpace = Max(fVSpace, 0.0f);

				entry.m_pBitmap.Reset(SEOUL_NEW(MemoryBudgets::Falcon) BitmapDefinition(id, (UInt32)iWidth, (UInt32)iHeight, 0));
				entry.m_fXOffset = rTextChunk.m_fXOffset + fAutoSizeRescale * (fHOffset + fHSpace);
				entry.m_fYOffset = rTextChunk.m_fYOffset + fAutoSizeRescale * (fVOffset + fVSpace);
				entry.m_fXMargin = fHSpace;
				entry.m_fYMargin = fVSpace;
				entry.m_iStartingTextLine = rTextChunk.m_iLine;
				entry.m_eAlignment = rTextChunk.m_Format.GetAlignmentEnum();
				entry.m_eImageAlignment = eImageAlignment;
				entry.m_iLinkIndex = rTextChunk.m_Format.m_iLinkIndex;

				// Handle data entry errors.
				if (entry.IsValid())
				{
					rTextChunk.m_fXOffset += fAutoSizeRescale * ((Float)iWidth + fHOffset + (2.0f * fHSpace));
					m_vImages.PushBack(entry);
				}
			}
		}
		break;

	case HtmlTag::kP:
		while (rReader.ReadAttribute(eAttribute, reStyle))
		{
			switch (eAttribute)
			{
			case HtmlAttribute::kAlign:
				{
				HtmlAlign eAlignment = HtmlAlign::kLeft;
					rReader.ReadAttributeValue(eAlignment, rTextChunk.m_Format.GetAlignmentEnum());
					rTextChunk.m_Format.SetAlignmentEnum(eAlignment);
				}
				break;
			default:
				break;
			};
		}
		break;

	case HtmlTag::kVerticalCentered:
		// Special case - any time this tag is encountered, we just set m_bXhtmlVerticalCenter of the current instance
		// to true.
		m_bXhtmlVerticalCenter = true;
		break;

	default:
		break;
	};
}

} // namespace Seoul::Falcon
