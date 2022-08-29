/**
 * \file FalconEditTextCommon.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_EDIT_TEXT_COMMON_H
#define FALCON_EDIT_TEXT_COMMON_H

#include "Prereqs.h"

namespace Seoul::Falcon
{

namespace EditTextCommon
{

/** Symbolic characters. */
static inline Bool IsIdeographic(UniChar c)
{
	return ((c) >= 0x2E80 && (c) < 0xFFFF);
}

// TODO: This can probably just replace IsSpace().
/** Special implementation of IsSpace(). */
static inline Bool IsWhiteSpace(UniChar c)
{
	switch (c)
	{
		case ' ': // Regular space
		case 0x0085: // NEXT LINE
		case 0x00A0: // NBSP
		case 0x1680: // OGHAM SPACE MARK
		case 0x180e: // MONGOLIAN VOWEL SEPARATOR
		case 0x200B: // ZERO WIDTH SPACE
		case 0x2028: // LINE SEPARATOR
		case 0x2029: // PARAGRAPH SEPARATOR
		case 0x202F: // NARROW NO-BREAK SPACE
		case 0x205f: // MEDIUM MATHEMATICAL SPACE
		case 0x3000: // IDEOGRAPHIC SPACE
			return true;
		default:
			return (c >= 0x2000 && c <= 0x200a) // EN QUAD through HAIR SPACE
				|| (c >= 0x0009 && c <= 0x000D); // ASCII Tab through Carriage return
	}
}

static inline Bool CanBreakBefore(UniChar c)
{
	switch (c)
	{
		// Special case, to handle "no character" in various cases.
	case 0: return true;

		// Special non-breaking characters.
	case 0x00A0: // No-break space
	case 0x2011: // Non-breaking hyphen
	case 0x202F: // Narrow no-break space
	case 0x2060: // Word-joiner
	case 0xFEFF: // Zero width no-break space
		return false;

		// Japanese set of "do not start lines with" characters based on Kinsoku Shori from:
		// - http://msdn.microsoft.com/en-us/goglobal/bb688158.aspx
		// With additional reference:
		// - http://isthisthingon.org/unicode/
		// - http://www.unicode.org/reports/tr14/tr14-26.html
	case 0x0021: // Shift-JIS 0x21, Exclamation Mark
	case 0x0025: // Shift-JIS 0x25, Percent Sign
	case 0x0029: // Shift-JIS 0x29, Right Parenthesis
	case 0x002C: // Shift-JIS 0x2C, Comma
	case 0x002D: // Shift-JIS 0x2D, Hyphen-minus
	case 0x002E: // Shift-JIS 0x2E, Full Stop
	case 0x003A: // Shift-JIS 0x3A, Colon
	case 0x003B: // Shift-JIS 0x3B, Semicolon
	case 0x003F: // Shift-JIS 0x3F, Question Mark
	case 0x005D: // Shift-JIS 0x5D, Right Square Bracket
	case 0x007D: // Shift-JIS 0x7D, Right Curly Bracket
	case 0xFF61: // Shift-JIS 0xA1, Halfwidth Ideographic Full Stop
	case 0xFF63: // Shift-JIS 0xA3, Halfwidth Right Corner Bracket
	case 0xFF64: // Shift-JIS 0xA4, Halfwidth Ideographic Comma
	case 0xFF65: // Shift-JIS 0xA5, Halfwidth Katakana Middle Dot
	case 0xFF67: // Shift-JIS 0xA7, Halfwidth Katakana Letter Small A
	case 0xFF68: // Shift-JIS 0xA8, Halfwidth Katakana Letter Small I
	case 0xFF69: // Shift-JIS 0xA9, Halfwidth Katakana Letter Small U
	case 0xFF6A: // Shift-JIS 0xAA, Halfwidth Katakana Letter Small E
	case 0xFF6B: // Shift-JIS 0xAB, Halfwidth Katakana Letter Small O
	case 0xFF6C: // Shift-JIS 0xAC, Halfwidth Katakana Letter Small Ya
	case 0xFF6D: // Shift-JIS OxAD, Halfwidth Katakana Letter Small Yu
	case 0xFF6E: // Shift-JIS 0xAE, Halfwidth Katakana Letter Small Yo
	case 0xFF6F: // Shift-JIS 0xAF, Halfwidth Katakana Letter Small Tu
	case 0xFF70: // Shift-JIS 0xB0, Halfwidth Katakana-hiragana Prolonged Sound Mark
	case 0xFF9E: // Shift-JIS 0xDE, Halfwidth Katakana Voiced Sound Mark
	case 0xFF9F: // Shift-JIS 0xDF, Halfwidth Katakana Semi-voiced Sound Mark
	case 0x3001: // Shift-JIS 0x8141, Ideographic Comma
	case 0x3002: // Shift-JIS 0x8142, Ideographic Full Stop
	case 0xFF0C: // Shift-JIS 0x8143, Fullwidth Comma
	case 0xFF0E: // Shift-JIS 0x8144, Fullwidth Full Stop
	case 0x30FB: // Shift-JIS 0x8145, Katakana Middle Dot
	case 0xFF1A: // Shift-JIS 0x8146, Fullwidth Colon
	case 0xFF1B: // Shift-JIS 0x8147, Fullwidth Semicolon
	case 0xFF1F: // Shift-JIS 0x8148, Fullwidth Question Mark
	case 0xFF01: // Shift-JIS 0x8149, Fullwidth Exclamation Mark
	case 0x309B: // Shift-JIS 0x814A, Katakana-hiragana Voiced Sound Mark
	case 0x309C: // Shift-JIS 0x814B, Katakana-hiragana Semi-voiced Sound Mark
	case 0x30FD: // Shift-JIS 0x8152, Katakana Iteration Mark
	case 0x30FE: // Shift-JIS 0x8153, Katakana Voiced Iteration Mark
	case 0x309D: // Shift-JIS 0x8154, Hiragana Iteration Mark
	case 0x309E: // Shift-JIS 0x8155, Hiragana Voiced Iteration Mark
	case 0x3005: // Shift-JIS 0x8158, Ideographic Iteration Mark
	case 0x30FC: // Shift-JIS 0x815B, Katakana-hiragana Prolonged Sound Mark
	case 0x2010: // Shift-JIS 0x815D, Hyphen
	case 0x2019: // Shift-JIS 0x8166, Right Single Quotation Mark
	case 0x201D: // Shift-JIS 0x8168, Right Double Quotation Mark
	case 0xFF09: // Shift-JIS 0x816A, Fullwidth Right Parenthesis
	case 0x301C: // Shift-JIS 0x8160, Wave Dash
	case 0x3015: // Shift-JIS 0x816C, Right Tortoise Shell Bracket
	case 0xFF3D: // Shift-JIS 0x816E, Fullwidth Right Square Bracket
	case 0xFF5D: // Shift-JIS 0x8170, Fullwidth Right Curly Bracket
	case 0x3009: // Shift-JIS 0x8172, Right Angle Bracket
	case 0x300B: // Shift-JIS 0x8174, Right Double Angle Bracket
	case 0x300D: // Shift-JIS 0x8176, Right Corner Bracket
	case 0x300F: // Shift-JIS 0x8178, Right White Corner Bracket
	case 0x3011: // Shift-JIS 0x817A, Right Black Lenticular Bracket
	case 0x00B0: // Shift-JIS 0x818B, Degree Sign
	case 0x2032: // Shift-JIS 0x818C, Prime
	case 0x2033: // Shift-JIS 0x818D, Double Prime
	case 0x2103: // Shift-JIS 0x818E, Degree Celsius
	case 0x00A2: // Shift-JIS 0x8191, Cent Sign
	case 0xFF05: // Shift-JIS 0x8193, Fullwidth Percent Sign
	case 0x303B: // Shift-JIS 0x81B4, Vertical Ideographic Iteration Mark
	case 0xFF60: // Shift-JIS 0x81D5, Fullwidth Right White Parenthesis
	case 0x3019: // Shift-JIS 0x81D7, Right White Tortoise Shell Bracket
	case 0x3017: // Shift-JIS 0x81D9, Right White Lenticular Bracket
	case 0x2030: // Shift-JIS 0x81F1, Per Mille Sign
	case 0x30A0: // Shift-JIS 0x829B, Katakana-hiragana Double Hyphen
	case 0x2013: // Shift-JIS 0x829C, En Dash
	case 0x3041: // Shift-JIS 0x829F, Hiragana Letter Small A
	case 0x3043: // Shift-JIS 0x82A1, Hiragana Letter Small I
	case 0x3045: // Shift-JIS 0x82A3, Hiragana Letter Small U
	case 0x3047: // Shift-JIS 0x82A5, Hiragana Letter Small E
	case 0x3049: // Shift-JIS 0x82A7, Hiragana Letter Small O
	case 0x3063: // Shift-JIS 0x82C1, Hiragana Letter Small Tu
	case 0x3083: // Shift-JIS 0x82E1, Hiragana Letter Small Ya
	case 0x3085: // Shift-JIS 0x82E3, Hiragana Letter Small Yu
	case 0x3087: // Shift-JIS 0x82E5, Hiragana Letter Small Yo
	case 0x308E: // Shift-JIS 0x82EC, Hiragana Letter Small Wa
	case 0x3095: // Shift-JIS 0x82F3, Hiragana Letter Small Ka
	case 0x3096: // Shift-JIS 0x82F4, Hiragana Letter Small Ke
	case 0x30A1: // Shift-JIS 0x8340, Katakana Letter Small A
	case 0x30A3: // Shift-JIS 0x8342, Katakana Letter Small I
	case 0x30A5: // Shift-JIS 0x8344, Katakana Letter Small U
	case 0x30A7: // Shift-JIS 0x8346, Katakana Letter Small E
	case 0x30A9: // Shift-JIS 0x8348, Katakana Letter Small O
	case 0x30C3: // Shift-JIS 0x8362, Katakana Letter Small Tu
	case 0x30E3: // Shift-JIS 0x8383, Katakana Letter Small Ya
	case 0x30E5: // Shift-JIS 0x8385, Katakana Letter Small Yu
	case 0x30E7: // Shift-JIS 0x8387, Katakana Letter Small Yo
	case 0x30EE: // Shift-JIS 0x838E, Katakana Letter Small Wa
	case 0x30F5: // Shift-JIS 0x8395, Katakana Letter Small Ka
	case 0x30F6: // Shift-JIS 0x8396, Katakana Letter Small Ke
	case 0x31F0: // Shift-JIS 0x83EC, Katakana Letter Small Ku
	case 0x31F1: // Shift-JIS 0x83ED, Katakana Letter Small Si
	case 0x31F2: // Shift-JIS 0x83EE, Katakana Letter Small Su
	case 0x31F3: // Shift-JIS 0x83EF, Katakana Letter Small To
	case 0x31F4: // Shift-JIS 0x83F0, Katakana Letter Small Nu
	case 0x31F5: // Shift-JIS 0x83F1, Katakana Letter Small Ha
	case 0x31F6: // Shift-JIS 0x83F2, Katakana Letter Small Hi
	case 0x31F7: // Shift-JIS 0x83F3, Katakana Letter Small Hu
	case 0x31F8: // Shift-JIS 0x83F4, Katakana Letter Small He
	case 0x31F9: // Shift-JIS 0x83F5, Katakana Letter Small Ho
	case 0x31FA: // Shift-JIS 0x83F7, Katakana Letter Small Mu
	case 0x31FB: // Shift-JIS 0x83F8, Katakana Letter Small Ra
	case 0x31FC: // Shift-JIS 0x83F9, Katakana Letter Small Ri
	case 0x31FD: // Shift-JIS 0x83FA, Katakana Letter Small Ru
	case 0x31FE: // Shift-JIS 0x83FB, Katakana Letter Small Re
	case 0x31FF: // Shift-JIS 0x83FC, Katakana Letter Small Ro
	case 0x203C: // Shift-JIS 0x84E9, Double Exclamation Mark
	case 0x2047: // Shift-JIS 0x84EA, Double Question Mark
	case 0x2048: // Shift-JIS 0x84EB, Question Exclamation Mark
	case 0x2049: // Shift-JIS 0x84EC, Exclamation Question Mark
	case 0x00BB: // Shift-JIS 0x8551, Right-pointing Double Angle Quotation Mark
	case 0x301F: // Shift-JIS 0x8781, Low Double Prime Quotation Mark
		return false;
		// End Japanese set.

	default:
		break;
	};

	// Can always break before white space or an ideographic
	// that does not explicitly disallow it.
	return IsIdeographic(c) || IsWhiteSpace(c);
}

static inline Bool CanBreakAfter(UniChar c)
{
	switch (c)
	{
		// Special case, to handle "no character" in various cases.
	case 0: return true;

		// Special non-breaking whitespace characters.
	case 0x00A0: // No-break space
	case 0x202F: // Narrow no-break space
		return false;

		// Japanese set of "do not separate" characters based on Kinsoku Shori from:
		// - http://isthisthingon.org/unicode/
		//
		// We accomplish "do not separate" by not allowing a break after.
	case 0x2014: // Shift-JIS 0x815C, Em Dash
	case 0x2025: // Shift-JIS 0x8164, Two Dot Leader
	case 0x3033: // Shift-JIS 0x81B1, Vertical Kana Repeat Mark Upper Half
	case 0x3034: // Shift-JIS 0x81B2, Vertical Kana Repeat With Voiced Sound Mark Upper Half
	case 0x3035: // Shift-JIS 0x81B3, Vertical Kana Repeat Mark Lower Half
		return false;

		// Japanese set of "do not start lines with" characters based on Kinsoku Shori from:
		// - http://msdn.microsoft.com/en-us/goglobal/bb688158.aspx
		// With additional reference:
		// - http://isthisthingon.org/unicode/
		// - http://www.unicode.org/reports/tr14/tr14-26.html
	case 0x0024: // Shift-JIS 0x24, Dollar Sign
	case 0x0028: // Shift-JIS 0x28, Left Parenthesis
	case 0x005B: // Shift-JIS 0x5B, Left Square Bracket
	case 0x00A5: // Shift-JIS 0x5C, Yen Sign
	case 0x007B: // Shift-JIS 0x7B, Left Curly Brace
	case 0xFF62: // Shift-JIS 0xA2, Halfwidth Left Corner Bracket
	case 0x2018: // Shift-JIS 0x8165, Left Single Quotation Mark
	case 0x201C: // Shift-JIS 0x8167, Left Double Quotation Mark
	case 0xFF08: // Shift-JIS 0x8169, Fullwidth Left Parenthesis
	case 0x3014: // Shift-JIS 0x816B, Left Tortoise Shell Bracket
	case 0xFF3B: // Shift-JIS 0x816D, Fullwidth Left Square Bracket
	case 0xFF5B: // Shift-JIS 0x816F, Fullwidth Left Curly Bracket
	case 0x3008: // Shift-JIS 0x8171, Left Angle Bracket
	case 0x300A: // Shift-JIS 0x8173, Left Double Angle Bracket
	case 0x300C: // Shift-JIS 0x8175, Left Corner Bracket
	case 0x300E: // Shift-JIS 0x8177, Left White Corner Bracket
	case 0x3010: // Shift-JIS 0x8179, Left Black Lenticular Bracket
	case 0xFFE5: // Shift-JIS 0x818F, Fullwidth Yen Sign
	case 0xFF04: // Shift-JIS 0x8190, Fullwidth Dollar Sign
	case 0x00A3: // Shift-JIS 0x8192, Pound Sign
	case 0xFF5F: // Shift-JIS 0x81D4, Fullwidth Left White Parenthesis
	case 0x3018: // Shift-JIS 0x81D6, Left White Tortoise Shell Bracket
	case 0x3016: // Shift-JIS 0x81D8, Left White Lenticular Bracket
	case 0x00AB: // Shift-JIS 0x8547, Left-pointing Double Angle Quotation Mark
	case 0x301D: // Shift-JIS 0x8780, Reversed Double Prime Quotation Mark
	   // End Japanese set.
		return false;

	default:
		break;
	};

	// Default to true unless a character explicitly disallows it.
	return true;
}

static inline Bool CanBreak(UniChar prev, UniChar next)
{
	return CanBreakAfter(prev) && CanBreakBefore(next);
}

// TODO: Configurable?
static const Float kfCursorBlinkIntervalInSeconds = 0.265f;

// Derived by A/B testing with Flash Player.
static const Float kfTextPaddingLeft = 3.0f;
static const Float kfTextPaddingTopAbs = 2.0f;
static const Float kfTextPaddingRight = 3.0f;
static const Float kfTextPaddingWordWrap = 1.0f;
static const Float kfTextPaddingBottom = 2.0f;

// Derived by A/B testing with Flash Player - we appear to have to allot
// for 3-pixels (60 twips) of "slop" when deciding if a horizontal alignment
// will result in a left border outside a text box, likely to account for
// anti-aliasing pixels.
static const Float kfHorizontalAlignmentOutOfBoundsTolerance = 3.0f;

} // namespace EditTextCommon

} // namespace Seoul::Falcon

#endif // include guard
