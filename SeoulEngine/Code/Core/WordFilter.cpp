/**
 * \file WordFilter.cpp
 *
 * \brief Provides whitelist/blacklist filtering of a string of text. Uses
 * a phonetic parser (based on double metaphone) and a collection of other
 * heuristic rules to normalize text, and a trie of whitelist/blacklist words
 * to perform filtering.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DataStore.h"
#include "Lexer.h"
#include "SeoulString.h"
#include "WordFilter.h"

namespace Seoul
{

/** Constants in trie configuration, used to set per-word options. */
static const HString ksDrop("Drop");
static const HString ksDropEntireInputString("DropEntireInputString");
static const HString ksEnableApplyDefaultSubstitutionToWholeWords("EnableApplyDefaultSubstitutionToWholeWords");
static const HString ksEnableLeetSpeak("EnableLeetSpeak");
static const HString ksLikely("Likely");
static const HString ksLikelyWordOnly("LikelyWordOnly");
static const HString ksSuffix("Suffix");
static const HString ksWhole("Whole");
static const HString ksWholeWordOnly("WholeWordOnly");

// Utility structure, describes an alternative to a character (e.g. 'y' -> "ie").
struct WordFilterAlternative
{
	UniChar const* m_sAlternative;
	Int32 m_iStartOffset;
}; // struct WordFilterAlternative

// Utility structure, collection of all configuration
// options recognized by WordFilter.
struct WordFilterOptionEntry
{
	HString const m_sOptionName;
	UInt32 const m_uOptionBitValue;
}; // struct WordFilterOptionEntry

// Array of all configuration options recognized by WordFilter.
static const WordFilterOptionEntry s_kaWordFilterOptions[] =
{
	{ ksEnableApplyDefaultSubstitutionToWholeWords, WordFilterOptions::kEnableApplyDefaultSubstitutionToWholeWords },
	{ ksEnableLeetSpeak, WordFilterOptions::kEnableLeetSpeak },
};

/** One of several possible matching options on individual words. */
struct WordFilterWordOptionEntry
{
	HString m_sName;
	UInt32 m_uValue;

	Bool operator==(HString name) const
	{
		return (m_sName == name);
	}

	Bool operator!=(HString name) const
	{
		return !(*this == name);
	}
};

/** The full set of matching options. */
static const WordFilterWordOptionEntry s_kaWordFilterWordOptionEntries[] =
{
	{ ksDrop, WordFilterWordOptions::kDropEntireInputString },
	{ ksDropEntireInputString, WordFilterWordOptions::kDropEntireInputString },
	{ ksLikely, WordFilterWordOptions::kLikelyWordOnly },
	{ ksLikelyWordOnly, WordFilterWordOptions::kLikelyWordOnly },
	{ ksSuffix, WordFilterWordOptions::kSuffix },
	{ ksWhole, WordFilterWordOptions::kWholeWordOnly },
	{ ksWholeWordOnly, WordFilterWordOptions::kWholeWordOnly },
};

/**
 * Special UniChar values in the Unicode Private-Use Characters main range (PUA),
 * used to encode special sequences during phonetic encoding (starts at the value of 0xE000).
 */
namespace SpecialCharacters
{
	enum Enum
	{
		FIRST_SPECIAL_CHARACTER = 0xE000,

		// Special handling for @ and &.
		kAtSymbol = FIRST_SPECIAL_CHARACTER,
		kAmpSymbol,
		kDotSymbol,

		// Extended.
		kLatinSmallLigatureOE = 0x0153, // œ
		kLatinSmallLetterSharpS = 0x00DF, // ß
		kLatinSmallLetterAWithAcute = 0x00E1, // á
		kLatinSmallLetterAWithGrave = 0x00E0, // à
		kLatinSmallLetterAWithCircumflex = 0x00E2, // â
		kLatinSmallLetterAWithDiaeresis = 0x00E4, // ä
		kLatinSmallLetterCWithCedilla = 0x00E7, // ç
		kLatinSmallLetterAE = 0x00E6, // æ
		kLatinSmallLetterEWithAcute = 0x00E9, // é
		kLatinSmallLetterEWithGrave = 0x00E8, // è
		kLatinSmallLetterEWithDiaeresis = 0x00EB, // ë
		kLatinSmallLetterEWithCircumflex = 0x00EA, // ê
		kLatinSmallLetterIWithAcute = 0x00ED, // í
		kLatinSmallLetterIWithGrave = 0x00EC, // ì
		kLatinSmallLetterIWithDiaeresis = 0x00EF, // ï
		kLatinSmallLetterIWithCircumflex = 0x00EE, // î
		kLatinSmallLetterNWithTilde = 0x00F1, // ñ
		kLatinSmallLetterOWithAcute = 0x00F3, // ó
		kLatinSmallLetterOWithGrave = 0x00F2, // ò
		kLatinSmallLetterOWithCircumflex = 0x00F4, // ô
		kLatinSmallLetterOWithDiaeresis = 0x00F6, // ö
		kLatinSmallLetterUWithGrave = 0x00F9, // ù
		kLatinSmallLetterUWithCircumflex = 0x00FB, // û
		kLatinSmallLetterUWithAcute = 0x00FA, // ú
		kLatinSmallLetterUWithDiaeresis = 0x00FC, // ü
		kLatinSmallLetterYWithDiaeresis = 0x00FF, // ÿ

		// Extended characters
		kBritishPound = 0x00A3,
		kEuro = 0x20AC,
		kYen = 0x00A5,

		// "1337" or leetspeak characters - converts to corresponding letter, then
		// becomes a skip character.
		kLeet0,
		kLeet1,
		kLeet3,
		kLeet5,
		kLeet7,
		kLeet8,
		kLeet9,
		kLeetEuro,
		kLeetPound,
		kLeetYen,
		LAST_SPECIAL_CHARACTER = kLeetYen,
		SPECIAL_CHARACTER_COUNT = (LAST_SPECIAL_CHARACTER - FIRST_SPECIAL_CHARACTER) + 1,

		FIRST_LEET_CHARACTER = kLeet0,
		LAST_LEET_CHARACTER = kLeetYen,
		LEET_CHARACTER_COUNT = (LAST_LEET_CHARACTER - FIRST_LEET_CHARACTER) + 1,
	}; // enum Enum
} // namespace SpecialCharacters

namespace Alternatives
{
	static const UniChar kSkip[] = { 0 };

	// Character sequences for alternatives.
	static const UniChar kA[]   = { 'a', 0 };
	static const UniChar kAH[]  = { 'a', 'h', 0 };
	static const UniChar kAND[] = { 'a', 'n', 'd', 0 };
	static const UniChar kAR[]  = { 'a', 'r', 0 };
	static const UniChar kAT[]  = { 'a', 't', 0 };
	static const UniChar kB[]   = { 'b', 0 };
	static const UniChar kC[]   = { 'c', 0 };
	static const UniChar kCK[]  = { 'c', 'k', 0 };
	static const UniChar kDOT[] = { 'd', 'o', 't', 0 };
	static const UniChar kE[]   = { 'e', 0 };
	static const UniChar kER[]  = { 'e', 'r', 0 };
	static const UniChar kG[]   = { 'g', 0 };
	static const UniChar kH[]   = { 'h', 0 };
	static const UniChar kI[]   = { 'i', 0 };
	static const UniChar kIE[]  = { 'i', 'e', 0 };
	static const UniChar kK[]   = { 'k', 0 };
	static const UniChar kL[]   = { 'l', 0 };
	static const UniChar kN[]   = { 'n', 0 };
	static const UniChar kO[]   = { 'o', 0 };
	static const UniChar kS[]   = { 's', 0 };
	static const UniChar kT[]   = { 't', 0 };
	static const UniChar kU[]   = { 'u', 0 };
	static const UniChar kUR[]  = { 'u', 'r', 0 };
	static const UniChar kY[]   = { 'y', 0 };

	static const WordFilterAlternative kTerminator = { nullptr, 0 };

	// Extended alternatives.
	static const WordFilterAlternative kAltExtendedA[] = { { kA, 0 }, kTerminator };
	static const WordFilterAlternative kAltExtendedC[] = { { kC, 0 }, kTerminator };
	static const WordFilterAlternative kAltExtendedE[] = { { kE, 0 }, kTerminator };
	static const WordFilterAlternative kAltExtendedI[] = { { kI, 0 }, kTerminator };
	static const WordFilterAlternative kAltExtendedN[] = { { kN, 0 }, kTerminator };
	static const WordFilterAlternative kAltExtendedO[] = { { kO, 0 }, kTerminator };
	static const WordFilterAlternative kAltExtendedS[] = { { kS, 0 }, kTerminator };
	static const WordFilterAlternative kAltExtendedU[] = { { kU, 0 }, kTerminator };
	static const WordFilterAlternative kAltExtendedY[] = { { kY, 0 }, { kE, 0 }, { kI, 0 }, { kIE, -1 }, kTerminator };

	// Leet alternatives.
	static const WordFilterAlternative kAltLeet0[] = { { kO, 0 }, { kSkip, 1 }, kTerminator };
	static const WordFilterAlternative kAltLeet1[] = { { kL, 0 }, { kI, 0 }, { kSkip, 1 }, kTerminator };
	static const WordFilterAlternative kAltLeet3[] = { { kE, 0 }, { kSkip, 1 }, kTerminator };
	static const WordFilterAlternative kAltLeet5[] = { { kS, 0 }, { kSkip, 1 }, kTerminator };
	static const WordFilterAlternative kAltLeet7[] = { { kT, 0 }, { kSkip, 1 }, kTerminator };
	static const WordFilterAlternative kAltLeet8[] = { { kB, 0 }, { kSkip, 1 }, kTerminator };
	static const WordFilterAlternative kAltLeet9[] = { { kG, 0 }, { kSkip, 1 }, kTerminator };
	static const WordFilterAlternative kAltLeetEuro[] = { { kE, 0 }, { kSkip, 1 }, kTerminator };
	static const WordFilterAlternative kAltLeetPound[] = { { kH, 0 }, { kSkip, 1 }, kTerminator };
	static const WordFilterAlternative kAltLeetYen[] = { { kY, 0 }, { kSkip, 1 }, kTerminator };

	// Phonetic alternatives.
	static const WordFilterAlternative kAltAh[]   = { { kA, 1 }, { kU, 1 }, { kUR, 0 }, { kAR, 0 }, { kER, 0 }, kTerminator };
	static const WordFilterAlternative kAltAr[]   = { { kER, 0 }, kTerminator };
	static const WordFilterAlternative kAltAw[]   = { { kA, 1 }, { kU, 1 }, { kAH, 0 }, { kUR, 0 }, { kAR, 0 }, { kER, 0 }, kTerminator };
	static const WordFilterAlternative kAltCk[]   = { { kK, 1 }, { kC, 1 }, kTerminator };
	static const WordFilterAlternative kAltCend[] = { { kK, 0 }, { kCK, -1 }, kTerminator };
	static const WordFilterAlternative kAltK[]    = { { kC, 0 }, { kCK, -1 }, kTerminator };
	static const WordFilterAlternative kAltQ[]    = { { kK, 0 }, { kC, 0 }, { kCK, -1 }, kTerminator };
	static const WordFilterAlternative kAltUh[]   = { { kA, 1 }, { kU, 1 }, { kAH, 0 }, { kUR, 0 }, { kAR, 0 }, { kER, 0 }, kTerminator };
	static const WordFilterAlternative kAltUr[]   = { { kAR, 0 }, { kER, 0 }, kTerminator };
	static const WordFilterAlternative kAltY[]    = { { kE, 0 }, { kI, 0 }, { kIE, -1 }, kTerminator };

	// Symbol alternatives.
	static const WordFilterAlternative kAltAmpSymbol[] = { { kAND, -2 }, { kSkip, 1 }, kTerminator };
	static const WordFilterAlternative kAltAtSymbol[]  = { { kA, 0 }, { kAT, -1 }, { kSkip, 1 }, kTerminator };
	static const WordFilterAlternative kAltDotSymbol[] = { { kDOT, -2 }, { kSkip, 1 }, kTerminator };

	static inline WordFilterAlternative const* GetAlternatives(UniChar ch)
	{
		using namespace SpecialCharacters;

		switch (ch)
		{
			// Leet alternatives
		case kLeet0: return kAltLeet0;
		case kLeet1: return kAltLeet1;
		case kLeet3: return kAltLeet3;
		case kLeet5: return kAltLeet5;
		case kLeet7: return kAltLeet7;
		case kLeet8: return kAltLeet8;
		case kLeet9: return kAltLeet9;
		case kLeetEuro: return kAltLeetEuro;
		case kLeetPound: return kAltLeetPound;
		case kLeetYen: return kAltLeetYen;

			// Extended alternatives
		case kLatinSmallLigatureOE: return kAltExtendedE; // TODO: Not always E.
		case kLatinSmallLetterSharpS: return kAltExtendedS;
		case kLatinSmallLetterAWithAcute: return kAltExtendedA;
		case kLatinSmallLetterAWithGrave: return kAltExtendedA;
		case kLatinSmallLetterAWithCircumflex: return kAltExtendedA;
		case kLatinSmallLetterAWithDiaeresis: return kAltExtendedA;
		case kLatinSmallLetterCWithCedilla: return kAltExtendedC;
		case kLatinSmallLetterAE: return kAltExtendedA; // TODO: Not always A.
		case kLatinSmallLetterEWithAcute: return kAltExtendedE;
		case kLatinSmallLetterEWithGrave: return kAltExtendedE;
		case kLatinSmallLetterEWithDiaeresis: return kAltExtendedE;
		case kLatinSmallLetterEWithCircumflex: return kAltExtendedE;
		case kLatinSmallLetterIWithAcute: return kAltExtendedI;
		case kLatinSmallLetterIWithGrave: return kAltExtendedI;
		case kLatinSmallLetterIWithDiaeresis: return kAltExtendedI;
		case kLatinSmallLetterIWithCircumflex: return kAltExtendedI;
		case kLatinSmallLetterNWithTilde: return kAltExtendedN;
		case kLatinSmallLetterOWithAcute: return kAltExtendedO;
		case kLatinSmallLetterOWithGrave: return kAltExtendedO;
		case kLatinSmallLetterOWithCircumflex: return kAltExtendedO;
		case kLatinSmallLetterOWithDiaeresis: return kAltExtendedO;
		case kLatinSmallLetterUWithGrave: return kAltExtendedU;
		case kLatinSmallLetterUWithCircumflex: return kAltExtendedU;
		case kLatinSmallLetterUWithAcute: return kAltExtendedU;
		case kLatinSmallLetterUWithDiaeresis: return kAltExtendedU;
		case kLatinSmallLetterYWithDiaeresis: return kAltExtendedY;

			// Phonetic alternatives
		case 'k': return kAltK;
		case 'q': return kAltQ;
		case 'y': return kAltY;

			// Symbol alternatives
		case kAmpSymbol: return kAltAmpSymbol;
		case kAtSymbol: return kAltAtSymbol;
		case kDotSymbol: return kAltDotSymbol;

		default:
			return nullptr;
		};
	}

	static inline WordFilterAlternative const* GetAlternatives(UniChar ch, UniChar chNext)
	{
		using namespace SpecialCharacters;

		switch (ch)
		{
		case kLatinSmallLetterAWithAcute: // fall-through
		case kLatinSmallLetterAWithGrave: // fall-through
		case kLatinSmallLetterAWithCircumflex: // fall-through
		case kLatinSmallLetterAWithDiaeresis: // fall-through
		case kLatinSmallLetterAE: // fall-through TODO: Not always A.
		case 'a':
			switch (chNext)
			{
			case 'h': return kAltAh;
			case 'r': return kAltAr;
			case 'w': return kAltAw;
			default:
				break;
			};
			break;

		case kLatinSmallLetterCWithCedilla: // fall-through
		case 'c':
			switch (chNext)
			{
			case 0: return kAltCend;
			case ' ': return kAltCend;
			case 'k': return kAltCk;
			default:
				break;
			}
			break;

		case kLatinSmallLetterUWithGrave: // fall-through
		case kLatinSmallLetterUWithCircumflex: // fall-through
		case kLatinSmallLetterUWithAcute: // fall-through
		case kLatinSmallLetterUWithDiaeresis: // fall-through
		case 'u':
			switch (chNext)
			{
			case 'h': return kAltUh;
			case 'r': return kAltUr;
			default:
				break;
			};
			break;

		default:
			break;
		};

		return nullptr;
	}
} // namespace Alternatives

/** @return True if ch0 is effectively equal to ch1, false otherwise. */
static inline Bool IsConsideredEqual(UniChar ch0, UniChar ch1)
{
	using namespace SpecialCharacters;

	// Equal if exactly equal.
	if (ch0 == ch1)
	{
		return true;
	}
	// Special handling for '@'.
	else if (
		(kAtSymbol == ch1 && 'a' == ch0) ||
		('a' == ch1 && kAtSymbol == ch0))
	{
		return true;
	}
	// Special handling for leet.
	else
	{
		// Make sure that leet, if present, is chPrevious.
		if (FIRST_LEET_CHARACTER <= ch1 && ch1 <= LAST_LEET_CHARACTER)
		{
			Swap(ch0, ch1);
		}

		switch (ch0)
		{
		case kLeet0: if ('o' == ch1) { return true; } break;
		case kLeet1: if ('i' == ch1 || 'l' == ch1) { return true; } break;
		case kLeet3: if ('e' == ch1) { return true; } break;
		case kLeet5: if ('s' == ch1) { return true; } break;
		case kLeet7: if ('t' == ch1) { return true; } break;
		case kLeet8: if ('b' == ch1) { return true; } break;
		case kLeet9: if ('g' == ch1) { return true; } break;
		case kLeetEuro: if ('e' == ch1) { return true; } break;
		case kLeetPound: if ('h' == ch1) { return true; } break;
		case kLeetYen: if ('y' == ch1) { return true; } break;
		default:
			break;
		};
	}

	return false;
}

/** @return True if ch can be skipped, based on the context of chPrevious and ch. */
static inline Bool CanSkip(UniChar chPrevious, UniChar ch)
{
	return IsConsideredEqual(chPrevious, ch);
}

/**
 * @return True if iWordFirst is likely a valid start of a word,
 * using heuristics based on the words contained in pWords.
 */
static inline Bool IsLikelyWordFirst(
	WordFilterWord const* /*pWord*/,
	WordFilterRootNode const* pWords,
	WordFilterEncodedString::ConstIterator const iBufferBegin,
	WordFilterEncodedString::ConstIterator const iBufferEnd,
	WordFilterEncodedString::ConstIterator const iWordFirst)
{
	// Immediate success if at the beginning of the buffer.
	if (iWordFirst <= iBufferBegin)
	{
		return true;
	}

	// Definitely true if iWordFirst is a whole word beginning.
	if (iWordFirst->IsWholeWordBegin())
	{
		return true;
	}

	// No words to use for reference, fail.
	if (nullptr == pWords)
	{
		return false;
	}

	// Iterate back to the furthest whole word character.
	auto iSearchFirst = iWordFirst;
	while (iSearchFirst > iBufferBegin)
	{
		if (iSearchFirst->IsWholeWordBegin())
		{
			break;
		}

		--iSearchFirst;
	}

	// Now search forward - must find a chain of known words
	// between the whole word position and the start of the word
	// in quesiton.
	auto iSearchLast = iSearchFirst;

	// Beginning of a word is never a valid place for a suffix.
	Bool bAllowSuffix = false;
	while (iSearchFirst < iWordFirst)
	{
		// Found a match, check and process.
		if (WordFilterWord const* pWord = pWords->FindLongestMatch(
			nullptr,
			iBufferBegin,
			iBufferEnd,
			iSearchFirst,
			iSearchLast,
			bAllowSuffix))
		{
			// Any position at or beyond our starting
			// character is a failure.
			if (iSearchLast >= iWordFirst)
			{
				return false;
			}
			// Finally, if the word ends right before our starting
			// character, success.
			else if (iSearchLast + 1 == iWordFirst)
			{
				return true;
			}

			// Allow suffixes the next time if pWord is not a suffix.
			bAllowSuffix = (!pWord->IsSetSuffix());

			// Start it at the next character.
			iSearchFirst = (iSearchLast + 1);
		}
		// No match, immediately fail.
		else
		{
			return false;
		}
	}

	return false;
}

/**
 * @return True if iWordLast is likely a valid end of a word,
 * using heuristics based on the words contained in pWords.
 */
static inline Bool IsLikelyWordLast(
	WordFilterWord const* pWord,
	WordFilterRootNode const* pWords,
	WordFilterEncodedString::ConstIterator const iBufferBegin,
	WordFilterEncodedString::ConstIterator const iBufferEnd,
	WordFilterEncodedString::ConstIterator const iWordLast,
	Bool bAllowSuffix)
{
	// Immediate success if at the end of the buffer.
	if (iWordLast >= iBufferEnd)
	{
		return true;
	}

	// Definitely done if we reached the (fuzzy) end of a whole word.
	// "Fuzzy" just allows for a whole word match against the
	// second to last 'e' in "tree" (for example), to allow
	// us to treat duplicate letters as a possible typo.
	if (iWordLast->IsFuzzyWholeWordEnd())
	{
		return true;
	}

	// No words to use for reference, fail.
	if (nullptr == pWords)
	{
		return false;
	}

	Bool bReturn = false;

	// Now recurse - try to reach a whole word end by chaining
	// together known words.
	{
		auto iSearchLast = iWordLast;
		if (WordFilterWord const* pSearchWord = pWords->FindLongestMatch(
			nullptr,
			iBufferBegin,
			iBufferEnd,
			(iWordLast + 1),
			iSearchLast,
			bAllowSuffix))
		{
			// Allow suffixes on the recursion if pSearchWord is not
			// a suffix.
			bAllowSuffix = !pSearchWord->IsSetSuffix();

			// Found at least one match, recurse to find the next.
			bReturn = IsLikelyWordLast(
				pSearchWord,
				pWords,
				iBufferBegin,
				iBufferEnd,
				iSearchLast,
				bAllowSuffix);
		}
	}

	// Special case - if the word ends with a character that is immediately
	// followed by the same character, allow a likely search starting at
	// the last character + 2.
	if (!bReturn &&
		(iWordLast + 1) < iBufferEnd &&
		IsConsideredEqual(iWordLast->GetCharacter(), (iWordLast + 1)->GetCharacter()))
	{
		auto iSearchLast = iWordLast;
		if (WordFilterWord const* pSearchWord = pWords->FindLongestMatch(
			nullptr,
			iBufferBegin,
			iBufferEnd,
			(iWordLast + 2),
			iSearchLast,
			bAllowSuffix))
		{
			// Allow suffixes on the recursion if pSearchWord is not
			// a suffix.
			bAllowSuffix = !pSearchWord->IsSetSuffix();

			// Found at least one match, recurse to find the next.
			bReturn = IsLikelyWordLast(
				pSearchWord,
				pWords,
				iBufferBegin,
				iBufferEnd,
				iSearchLast,
				bAllowSuffix);
		}
	}

	return bReturn;
}

/**
 * @return True if pFilterWord at the specified positions of
 * iWordFirst and iWordLast has all requirements fulfilled
 * (e.g. whole word) to be considered a valid word match.
 */
static inline Bool WordRequirementsFulfilled(
	WordFilterWord const* pFilterWord,
	WordFilterRootNode const* pLikelyWords,
	WordFilterEncodedString::ConstIterator const iBufferBegin,
	WordFilterEncodedString::ConstIterator const iBufferEnd,
	WordFilterEncodedString::ConstIterator const iWordFirst,
	WordFilterEncodedString::ConstIterator const iWordLast,
	Bool bHasSkippedWhiteSpaceDuringSearch,
	Bool bAllowSuffix)
{
	using namespace WordFilterWordOptions;

	// Initially, we've reached the end of a valid word. May be
	// set to false depending on options and other context.
	Bool bRequirementsFulfilled = true;

	// If the word is a suffix and they're not allowed in the current
	// context, skip it.
	if (pFilterWord->IsSetSuffix())
	{
		if (!bAllowSuffix)
		{
			return false;
		}
	}

	// If the word requires whole word matching, check for it.
	if (pFilterWord->IsSetWholeWordOnly())
	{
		// The word is not a whole word if the end or beginning is not a whole word start and (fuzzy) whole word end.
		if (!iWordFirst->IsWholeWordBegin() || !iWordLast->IsFuzzyWholeWordEnd())
		{
			bRequirementsFulfilled = false;
		}
	}
	// If the word requires likely word matching, check for it.
	else if (bHasSkippedWhiteSpaceDuringSearch || pFilterWord->IsSetLikelyWordOnly())
	{
		// Must be a likely word start.
		if (!IsLikelyWordFirst(
			pFilterWord,
			pLikelyWords,
			iBufferBegin,
			iBufferEnd,
			iWordFirst))
		{
			bRequirementsFulfilled = false;
		}
		// Must be a likely word end.
		else if (!IsLikelyWordLast(
			pFilterWord,
			pLikelyWords,
			iBufferBegin,
			iBufferEnd,
			iWordLast,
			true))
		{
			bRequirementsFulfilled = false;
		}
	}

	return bRequirementsFulfilled;
}

WordFilterEncodedString::WordFilterEncodedString(
	Byte const* s,
	UInt32 zStringLengthInBytes,
	UInt32 uOptions)
	: m_vBuffer()
{
	// Populate the initial buffer - this just stores characters
	// with their offsets.
	InternalPopulate(s, zStringLengthInBytes);

	// Convert all characters to lowercase.
	InternalConvertToLowerCase();

	// Apply normalizations and combinations to ease with pattern matching.
	//
	// NOTE: Some of these steps are dependent, if you decide to remove or
	// change them. In particular:
	// - most processing depends on normalized white space (checks are made only
	//   against ' ').
	// - symbol conversion to specials (e.g. leetspeak conversion) must
	//   occur prior to duplicate collapse, for special alternatives to be
	//   considered during collapse.
	// - conversion to phonetics must occur last.
	InternalNormalizeWhiteSpace();

	// Conditionally apply leetspeak processing.
	if (WordFilterOptions::kEnableLeetSpeak == (WordFilterOptions::kEnableLeetSpeak & uOptions))
	{
		InternalConvertLeetSpeakToLetters();
	}

	// Convert a few symbols that we care about into special character.
	InternalConvertSpecialSymbols();

	// Remove extra symbols which will not contribute (and may impede)
	// further processing.
	InternalRemoveExtraSymbols();

	// Now reduce all sequences to (at most) 2 of the same
	// character (e.g. "ssss" is collapsed to "ss").
	InternalCollapseDuplicateRuns();

	// Mark whole word begin and end.
	InternalMarkWholeWordBeginAndEnd();
}

WordFilterEncodedString::~WordFilterEncodedString()
{
}

/**
 * Reduce runs of the same character to at most 2 of the same
 * character (e.g. "ssss" is collapsed to "ss").
 */
void WordFilterEncodedString::InternalCollapseDuplicateRuns()
{
	UniChar aSequence[2] = { 0 };

	// Populate the sequence buffer with the first 2 characters.
	Int32 iCharacters = (Int32)m_vBuffer.GetSize();
	for (Int32 i = 0; i < Min(iCharacters, 2); ++i)
	{
		aSequence[i] = m_vBuffer[i].GetCharacter();
	}

	// Now iterate over the sequence, starting at character 3.
	for (Int32 i = 2; i < iCharacters; ++i)
	{
		UniChar ch = m_vBuffer[i].GetCharacter();

		// We have a sequence if 3 in a row are the same character.
		if (IsConsideredEqual(aSequence[0], aSequence[1]) &&
			IsConsideredEqual(aSequence[1], ch))
		{
			// Erase the middle of the 3 characters to collapse.
			m_vBuffer.Erase(m_vBuffer.Begin() + i - 1);
			--i;
			--iCharacters;
			aSequence[1] = ch;
		}
		else
		{
			// Otherwise, update the sequence for tracking purposes.
			aSequence[0] = aSequence[1];
			aSequence[1] = ch;
		}
	}
}

/**
 * Converts numbers used in "1337 speak" into their
 * letter equivalents.
 */
void WordFilterEncodedString::InternalConvertLeetSpeakToLetters()
{
	using namespace SpecialCharacters;

	UInt32 const uCharacters = m_vBuffer.GetSize();
	for (UInt32 i = 0u; i < uCharacters; ++i)
	{
		UniChar ch = m_vBuffer[i].GetCharacter();
		switch (ch)
		{
		case '0': ch = SpecialCharacters::kLeet0; break;
		case '1': ch = SpecialCharacters::kLeet1; break;
		case '3': ch = SpecialCharacters::kLeet3; break;
		case '5': ch = SpecialCharacters::kLeet5; break;
		case '7': ch = SpecialCharacters::kLeet7; break;
		case '8': ch = SpecialCharacters::kLeet8; break;
		case '9': ch = SpecialCharacters::kLeet9; break;
		case '#': ch = SpecialCharacters::kLeetPound; break;
		case '!': ch = SpecialCharacters::kLeet1; break;
		case '$': ch = SpecialCharacters::kLeet5; break;
		case '+': ch = SpecialCharacters::kLeet7; break;
		case kEuro: ch = SpecialCharacters::kLeetEuro; break;
		case kYen: ch = SpecialCharacters::kLeetYen; break;

		default:
			break;
		};
		m_vBuffer[i].SetCharacter(ch);
	}
}

/**
 * Converts special symbols (e.g. '@' and '&') to special
 * characters for later processing.
 */
void WordFilterEncodedString::InternalConvertSpecialSymbols()
{
	// Iterate over all characters and convert certain
	// special symbols that we care about, which have
	// alternative interpretations.
	UInt32 const uCharacters = m_vBuffer.GetSize();
	for (UInt32 i = 0u; i < uCharacters; ++i)
	{
		UniChar ch = m_vBuffer[i].GetCharacter();
		switch (ch)
		{
		case '@': ch = SpecialCharacters::kAtSymbol; break;
		case '&': ch = SpecialCharacters::kAmpSymbol; break;

			// Dot symbol is handled specially - we only
			// treat a '.' as a dot symbol if it is not followed
			// by white space or buffer end, or if it is preceeded *and* followed
			// by white space.
		case '.':
			// If the '.' is at the end of the buffer or is followed by
			// white space, we potentially don't treat it as a "dot".
			if (i + 1 >= uCharacters || ' ' == m_vBuffer[i + 1].GetCharacter())
			{
				// Don't treat it as a dot if the '.' character is preceeded
				// by the buffer start or a non-whitespace character.
				if (0 == i || ' ' != m_vBuffer[i - 1].GetCharacter())
				{
					break;
				}
			}

			// Fall-through to here, consider the '.' character
			// a dot.
			ch = SpecialCharacters::kDotSymbol;
			break;

		default:
			break;
		};
		m_vBuffer[i].SetCharacter(ch);
	}
}

/**
 * Unicode aware conversion of all characters into their lower case
 * variation.
 */
void WordFilterEncodedString::InternalConvertToLowerCase()
{
	// TODO: Use the user's current locale.
	String const sLocale("en");

	String sWorkArea;
	UInt32 const uCharacters = m_vBuffer.GetSize();
	for (UInt32 i = 0u; i < uCharacters; ++i)
	{
		sWorkArea.Assign(m_vBuffer[i].GetCharacter());
		sWorkArea = sWorkArea.ToLower(sLocale);
		m_vBuffer[i].SetCharacter(*sWorkArea.Begin());
	}
}

/**
 * Iterate over this WordFilterEncodedString and mark characters
 * which identify full, whole word begin and end points.
 */
void WordFilterEncodedString::InternalMarkWholeWordBeginAndEnd()
{
	UniChar chPrevious = (UniChar)' ';
	UInt32 const uCharacters = m_vBuffer.GetSize();
	for (UInt32 i = 0u; i < uCharacters; ++i)
	{
		UniChar const ch = m_vBuffer[i].GetCharacter();
		if (' ' != ch && ' ' == chPrevious)
		{
			m_vBuffer[i].SetWholeWordBegin(true);
		}
		else if (' ' == ch && ' ' != chPrevious)
		{
			m_vBuffer[i - 1].SetExactWholeWordEnd(true);
		}

		chPrevious = ch;
	}

	// If the last character is not white space, it is always
	// a whole word ending.
	if (uCharacters > 0u && m_vBuffer.Back().GetCharacter() != ' ')
	{
		m_vBuffer.Back().SetExactWholeWordEnd(true);
	}

	// Now set fuzzy whole word endings. Fuzzy endings
	// exist where Exact endings exist, and also immediately
	// before exact endings, if a double character is present
	// (e.g. "dd", the first 'd' will also be a fuzzy whole word
	// ending, if the second 'd' is a whole word ending).
	if (uCharacters > 0u && m_vBuffer[0].IsExactWholeWordEnd())
	{
		m_vBuffer[0].SetFuzzyWholeWordEnd(true);
	}

	for (UInt32 i = 1u; i < uCharacters; ++i)
	{
		if (m_vBuffer[i].IsExactWholeWordEnd())
		{
			m_vBuffer[i].SetFuzzyWholeWordEnd(true);

			if (IsConsideredEqual(m_vBuffer[i - 1].GetCharacter(), m_vBuffer[i].GetCharacter()))
			{
				m_vBuffer[i - 1].SetFuzzyWholeWordEnd(true);
			}
		}
	}
}

/**
 * Converts all white space characters to ' ', to simplify
 * further processing, and to allow easy detection of white space
 * duplicates.
 */
void WordFilterEncodedString::InternalNormalizeWhiteSpace()
{
	{
		// First convert all white space to a normalize ' ' character.
		UInt32 const uCharacters = m_vBuffer.GetSize();
		for (UInt32 i = 0u; i < uCharacters; ++i)
		{
			UniChar const character = m_vBuffer[i].GetCharacter();
			if (IsSpace(character))
			{
				m_vBuffer[i].SetCharacter((UniChar)' ');
			}
		}
	}

	{
		// Now collapse white space runs.
		Int32 iCharacters = (Int32)m_vBuffer.GetSize();
		for (Int32 i = 1; i < iCharacters; ++i)
		{
			UniChar const ch = m_vBuffer[i].GetCharacter();
			UniChar const chPrevious = m_vBuffer[i - 1].GetCharacter();
			if (' ' == ch && ch == chPrevious)
			{
				m_vBuffer.Erase(m_vBuffer.Begin() + i - 1);
				--i;
				--iCharacters;
			}
		}
	}
}

/**
 * Sets up the initial buffer - expands the UTF8 string
 * data into a buffer of UniChar entries with references
 * to their original position in the string.
 */
void WordFilterEncodedString::InternalPopulate(
	Byte const* s,
	UInt32 zStringLengthInBytes)
{
	m_vBuffer.Clear();
	m_vBuffer.Reserve(zStringLengthInBytes);

	LexerContext lexer;
	lexer.SetStream(s, zStringLengthInBytes);

	while (lexer.IsStreamValid())
	{
		UniChar const ch = lexer.GetCurrent();
		Character const character(ch, (UInt32)(lexer.GetStream() - s));
		m_vBuffer.PushBack(character);
		lexer.Advance();
	}
}

/**
 * Basic symbol elimination, prevent symbols that do not have
 * other meanings and have not been otherwise converted to specials
 * from contributing further to matching.
 */
void WordFilterEncodedString::InternalRemoveExtraSymbols()
{
	using namespace SpecialCharacters;

	Int32 iCharacters = (Int32)m_vBuffer.GetSize();
	for (Int32 i = 0u; i < iCharacters; ++i)
	{
		UniChar const ch = m_vBuffer[i].GetCharacter();
		Bool bErase = false;
		switch (ch)
		{
		// Symbols that we recognize and eliminate.
		case '^':
		case '-':
		case '_':
		case '.':
		case '{':
		case '}':
		case '(':
		case ')':
		case '[':
		case ']':
		case '<':
		case '>':
		case '+':
		case '$':
		case '?':
		case '!':
		case '@':
		case '#':
		case '&':
		case '*':
		case '%': // TODO: Probably a leetspeak meaning for this.
		case '=':
		case ',':
		case '/':
		case '`':
		case '\'':
		case '"':
		case '~':
		case ':':
		case ';':
		case '\\':
		case '|':
		case kBritishPound:
		case kEuro:
		case kYen:
		// Symbols that we recognize and eliminate.
			bErase = true;
			break;
		default:
			break;
		};

		// On a found symbol, erase it from the buffer and
		// reprocess the entry.
		if (bErase)
		{
			m_vBuffer.Erase(m_vBuffer.Begin() + i);
			--i;
			--iCharacters;
		}
	}
}

/**
 * Starting at iWordFirst, and advancing forward no further than iBufferEnd,
 * and backward no further than iBufferBegin, attempt to find the longest
 * matching word in the input string as defined by this trie.
 *
 * @return A non-null pointer to the cached WordFilterWord which is the longest
 * match starting at iWordFirst within this trie, or nullptr if no match
 * was found. If this method returns non-null, riWordLast will
 * point to the offset at which the last character of the match was found.
 */
WordFilterWord const* WordFilterRootNode::FindLongestMatch(
	WordFilterRootNode const* pLikelyWords,
	WordFilterEncodedString::ConstIterator const iBufferBegin,
	WordFilterEncodedString::ConstIterator const iBufferEnd,
	WordFilterEncodedString::ConstIterator const iWordFirst,
	WordFilterEncodedString::ConstIterator& riWordLast,
	Bool bAllowSuffix) const
{
	// Starting at the root.
	WordFilterTreeChildren const* ptChildren = &m_tChildren;
	return InternalFindLongestMatch(
		pLikelyWords,
		nullptr,
		ptChildren,
		iBufferBegin,
		iBufferEnd,
		iWordFirst,
		iWordFirst,
		riWordLast,
		false,
		bAllowSuffix);
}

/** Encode a new word into the trie. Builds a patch in the trie for fulfilling FindLongestMatch(). */
void WordFilterRootNode::InsertWord(Byte const* sWord, UInt32 zWordSizeInBytes, UInt32 uOptions)
{
	// Encode the string for further processing.
	WordFilterEncodedString const encodedString(sWord, zWordSizeInBytes, uOptions);

	// Cache the Iterator to the last non-vowel - all characters starting at this
	// will be considered word enders, so (for example), a check against "piec"
	// will match "piece".
	auto const iBegin = encodedString.Begin();
	auto const iEnd = encodedString.End();

	// Starting at the root.
	WordFilterTreeChildren* ptChildren = &m_tChildren;
	for (auto i = iBegin; iEnd != i; ++i)
	{
		// Cache the character at the current element of the input string.
		UniChar const ch = i->GetCharacter();

		// Search for a child of the trie that corresponds to that input character.
		WordFilterTreeNode** ppWordFilterTreeNode = ptChildren->Find(ch);
		if (nullptr == ppWordFilterTreeNode)
		{
			// No existing child, create one.
			ppWordFilterTreeNode = &ptChildren->Insert(
				ch,
				SEOUL_NEW(MemoryBudgets::Strings) WordFilterTreeNode).First->Second;

		}

		// This is the end of the word if this is the last character
		// of the word.
		if ((i + 1) == iEnd)
		{
			(*ppWordFilterTreeNode)->SetWord(sWord, zWordSizeInBytes, uOptions);
		}

		// Advance to the next node.
		ptChildren = &((*ppWordFilterTreeNode)->GetChildren());
	}
}

/**
 * Inner variation of FindLongestMatch(), passed a starting
 * character (used at iBegin instead of iBegin->GetCharacter())
 * at a children table within the filter tree, to allow recursive
 * restarts while finding, to handle character alternatives.
 */
WordFilterWord const* WordFilterRootNode::InternalFindLongestMatch(
	WordFilterRootNode const* pLikelyWords,
	UniChar const* sAlternativeCharacters,
	WordFilterTreeChildren const* ptChildren,
	WordFilterEncodedString::ConstIterator const iBufferBegin,
	WordFilterEncodedString::ConstIterator const iBufferEnd,
	WordFilterEncodedString::ConstIterator const iWordFirst,
	WordFilterEncodedString::ConstIterator const iStartSearch,
	WordFilterEncodedString::ConstIterator& riWordLast,
	Bool bHasSkippedWhiteSpaceDuringSearch,
	Bool bAllowSuffix) const
{
	// Enumerate all characters of the input string range.
	WordFilterWord const* pReturn = nullptr;
	auto iWordLast = (iBufferBegin - 1);
	UniChar chPrevious = (UniChar)0;
	for (auto i = iStartSearch; iBufferEnd != i; ++i)
	{
		// Cache the character at the current element of the input string.
		// Use an alternative if it's available.
		Bool const bTryAlternatives = (nullptr == sAlternativeCharacters || (0 == *sAlternativeCharacters));
		UniChar const ch = ((sAlternativeCharacters && *sAlternativeCharacters)
			? (*sAlternativeCharacters++)
			: (i->GetCharacter()));

		if (bTryAlternatives)
		{
			// Try alternatives based on 1 character.
			if (WordFilterAlternative const* pAlternatives = Alternatives::GetAlternatives(ch))
			{
				InternalTryAlternatives(
					pLikelyWords,
					pAlternatives,
					ptChildren,
					iBufferBegin,
					iBufferEnd,
					iWordFirst,
					i,
					pReturn,
					iWordLast,
					bHasSkippedWhiteSpaceDuringSearch,
					bAllowSuffix);
			}

			// Try alternatives based on 2 characters.
			UniChar const chNext = (i + 1 < iBufferEnd ? (i + 1)->GetCharacter() : (UniChar)0);
			if (WordFilterAlternative const* pAlternatives = Alternatives::GetAlternatives(ch, chNext))
			{
				InternalTryAlternatives(
					pLikelyWords,
					pAlternatives,
					ptChildren,
					iBufferBegin,
					iBufferEnd,
					iWordFirst,
					i,
					pReturn,
					iWordLast,
					bHasSkippedWhiteSpaceDuringSearch,
					bAllowSuffix);
			}

			// Try alternatives based on 2 characters, skipping white space.
			if (' ' == chNext && i + 2 < iBufferEnd)
			{
				UniChar const chNextNext = (i + 2)->GetCharacter();
				if (WordFilterAlternative const* pAlternatives = Alternatives::GetAlternatives(ch, chNextNext))
				{
					InternalTryAlternatives(
						pLikelyWords,
						pAlternatives,
						ptChildren,
						iBufferBegin,
						iBufferEnd,
						iWordFirst,
						// Offset i by 1, since pair alternatives expect 2 input characters, not 3.
						(i + 1),
						pReturn,
						iWordLast,
						// Always skipping white space in this case.
						true,
						bAllowSuffix);
				}
			}
		}

		// If the current character can be skipped, evaluate that
		// as an option as well. Note that we implement this separately,
		// because we don't want a match against the skipped character,
		// only against at least one character after the skipped
		// character (e.g. butt should not match but, while motther should
		// match mother).
		if (CanSkip(chPrevious, ch))
		{
			auto iMatchWithSkip = iWordLast;
			WordFilterWord const* pMatchWithSkip = InternalFindLongestMatch(
				pLikelyWords,
				nullptr,
				ptChildren,
				iBufferBegin,
				iBufferEnd,
				iWordFirst,
				(i + 1),
				iMatchWithSkip,
				bHasSkippedWhiteSpaceDuringSearch,
				bAllowSuffix);

			if (nullptr != pMatchWithSkip &&
				(nullptr == pReturn || iMatchWithSkip > iWordLast))
			{
				pReturn = pMatchWithSkip;
				iWordLast = iMatchWithSkip;
			}
		}

		// Search for a child of the trie that corresponds to the main
		// (not an alternative) input character.
		WordFilterTreeNode* const* ppWordFilterTreeNode = ptChildren->Find(ch);
		if (nullptr == ppWordFilterTreeNode)
		{
			// The input character is normalized white space, on a failed match,
			// skip the white space and record that we have done so.
			if (ch == ' ')
			{
				// Deliberately don't set chPrevious here.

				bHasSkippedWhiteSpaceDuringSearch = true;
				continue;
			}
			// Finally, break out of the loop, since we can make
			// no more matches along the main "not an alternative" trace.
			else
			{
				break;
			}
		}

		// Progress to the next child.
		ptChildren = &((*ppWordFilterTreeNode)->GetChildren());

		// Only check if the current terminator forms a word, if it will
		// produce a better (longer) match. Alternatives may already have produced
		// a longer match.
		if (i > iWordLast)
		{
			// If the current node is a word terminator, we have a potential match.
			if (WordFilterWord const* pFilterWord = (*ppWordFilterTreeNode)->GetWord())
			{
				// Check requirements on the word match.
				if (WordRequirementsFulfilled(
					pFilterWord,
					pLikelyWords,
					iBufferBegin,
					iBufferEnd,
					iWordFirst,
					i,
					bHasSkippedWhiteSpaceDuringSearch,
					bAllowSuffix))
				{
					// Tag that we've reached the end of a word in the trie.
					pReturn = (*ppWordFilterTreeNode)->GetWord();
					iWordLast = i;
				}
			}
		}

		chPrevious = ch;
	}

	// If iWordLast has been set, we have a match.
	if (nullptr != pReturn)
	{
		riWordLast = iWordLast;
		return pReturn;
	}
	else
	{
		// No match, do not modify riLastCharacterOfMatch.
		return nullptr;
	}
}

/**
 * Internal utility function, called to evaluate and try
 * alternatives associated with the current character. This
 * triggers a recursive evaluation of another possible path
 * in the current trie.
 */
void WordFilterRootNode::InternalTryAlternatives(
	WordFilterRootNode const* pLikelyWords,
	WordFilterAlternative const* pAlternatives,
	WordFilterTreeChildren const* ptChildren,
	WordFilterEncodedString::ConstIterator const iBufferBegin,
	WordFilterEncodedString::ConstIterator const iBufferEnd,
	WordFilterEncodedString::ConstIterator const iWordFirst,
	WordFilterEncodedString::ConstIterator const i,
	WordFilterWord const*& rpWord,
	WordFilterEncodedString::ConstIterator& riWordLast,
	Bool bHasSkippedWhiteSpaceDuringSearch,
	Bool bAllowSuffix) const
{
	// Keep looping until we hit the nullptr alternative.
	while (nullptr != pAlternatives->m_sAlternative)
	{
		// Cache the alternative definition.
		WordFilterAlternative const alternative = (*pAlternatives);

		// Try the alternative.
		auto iAlternativeWordLast = riWordLast;
		WordFilterWord const* pAlternative = InternalFindLongestMatch(
			pLikelyWords,
			alternative.m_sAlternative,
			ptChildren,
			iBufferBegin,
			iBufferEnd,
			iWordFirst,
			(i + alternative.m_iStartOffset),
			iAlternativeWordLast,
			bHasSkippedWhiteSpaceDuringSearch,
			bAllowSuffix);

		// If we got a valid match with the alternative,
		// check if it is better than our best match and if so,
		// use it.
		if (nullptr != pAlternative)
		{
			// Due to expansion when evaluating alternatives (e.g. '.' to "dot"),
			// an alternative match can sometimes be a partial against the expansion
			// (e.g. "do" in "dot" expanded from '.'). Don't consider this case
			// (a match must at minimum end at the position of the start character).
			if (iAlternativeWordLast >= iWordFirst)
			{
				if (nullptr == rpWord || (iAlternativeWordLast > riWordLast))
				{
					rpWord = pAlternative;
					riWordLast = iAlternativeWordLast;
				}
			}
		}

		// Advance to the next alternative.
		++pAlternatives;
	}
}

WordFilter::WordFilter()
	: m_sDefaultSubstitution()
	, m_tSubstitutions()
	, m_BlacklistRoot()
	, m_KnownWordsRoot()
	, m_WhitelistRoot()
	// All options are enabled by default.
	, m_uOptions(
		WordFilterOptions::kEnableApplyDefaultSubstitutionToWholeWords |
		WordFilterOptions::kEnableLeetSpeak)
{
}

WordFilter::~WordFilter()
{
}

/**
 * Apply filtering, based on black and white lists, to rsString. Modifies
 * the String in place if filtering occurs.
 *
 * @param[inout] rsString The word to filter. Modified in place on filtering
 * success.
 * @param[out] psLastMatch (optional) If non-null, this String will be set
 * to the last match in the input string.
 *
 * @return True if rsString was filtered/modified, false otherwise.
 */
Bool WordFilter::FilterString(String& rsString, String* psLastMatch) const
{
	// Encode the input string for processing.
	WordFilterEncodedString const encodedString(rsString.CStr(), rsString.GetSize(), m_uOptions);

	// Cache start and end iterators.
	auto const iBufferBegin = encodedString.Begin();
	auto const iBufferEnd = encodedString.End();
	auto iWordFirst = iBufferBegin;

	UInt32 uLastOffsetInBytes = 0u;
	String sOutput;
	Bool bReturn = false;

	// Keep a running track of the most last ending of the longest/last found
	// whitelist word. If a blacklist word is completely contained within a whitelist
	// word, it is not blacklisted.
	//
	// Must be initialized to iBufferBegin, so it can be replaced by any valid whitelist
	// match in the input string.
	auto iWhitelistWordLast = iBufferBegin;

	while (iWordFirst < iBufferEnd)
	{
		// Don't start word matching at white space in the input filtered string.
		if (iWordFirst->GetCharacter() == ' ')
		{
			++iWordFirst;
			continue;
		};

		// Look for a new whitelist word. If a match is found with a greater
		// offset, use it as the new iLastCharacterOfLongestWhitelistMatch.
		{
			auto i = iBufferBegin;
			if (nullptr != m_WhitelistRoot.FindLongestMatch(
				&m_KnownWordsRoot,
				iBufferBegin,
				iBufferEnd,
				iWordFirst,
				i,
				false) &&
				i > iWhitelistWordLast)
			{
				// Found a whitelisted word with a greater ending offset than the current,
				// use it.
				iWhitelistWordLast = i;
			}
		}

		// Now check the black list and perform a substitution if found.
		{
			auto iBlacklistWordLast = iBufferBegin;
			WordFilterWord const* pFoundMatch = m_BlacklistRoot.FindLongestMatch(
				&m_KnownWordsRoot,
				iBufferBegin,
				iBufferEnd,
				iWordFirst,
				iBlacklistWordLast,
				false);

			// We only use the blacklist match if it is beyond the end of the current whitelist match.
			if (nullptr != pFoundMatch && iBlacklistWordLast > iWhitelistWordLast)
			{
				// Set the last match.
				if (nullptr != psLastMatch)
				{
					*psLastMatch = pFoundMatch->GetWord();
				}

				// If pFoundMatch->IsSetDropEntireInputString() is true, it means
				// the match that was just found should drop or suppress the entire input string.
				// Immediately clear rsString and return true.
				if (pFoundMatch->IsSetDropEntireInputString())
				{
					rsString.Clear();
					return true;
				}

				// At least one match has been found and at least one substitution
				// will occur.
				bReturn = true;

				// Original start offset value. May be adjusted if whole word
				// substitution is enabled.
				UInt32 uStartingOffsetInBytes = iWordFirst->GetOriginalStartOffsetInBytes();

				// Get the desired substitution
				String const* psSubstitution = m_tSubstitutions.Find(pFoundMatch->GetWord());

				// Use the default substitution if a specified substitution was not found.
				if (nullptr == psSubstitution)
				{
					psSubstitution = &m_sDefaultSubstitution;

					// If specified for the default substitution, apply filtering to
					// the entire whole word of the match portion.
					if (WordFilterOptions::kEnableApplyDefaultSubstitutionToWholeWords == (WordFilterOptions::kEnableApplyDefaultSubstitutionToWholeWords & m_uOptions))
					{
						// Search for the character, backwards, that is the closest whole word beginning.
						auto iFirst = iWordFirst;
						while (iFirst > iBufferBegin)
						{
							if (iFirst->IsWholeWordBegin())
							{
								break;
							}

							--iFirst;
						}

						uStartingOffsetInBytes = iFirst->GetOriginalStartOffsetInBytes();

						// Advance iLastCharacterOfMatch to the next exact whole word ending.
						// Must use exact here, otherwise we might overlap start/end regions
						// between iterations.
						while (iBlacklistWordLast < iBufferEnd)
						{
							if (iBlacklistWordLast->IsExactWholeWordEnd())
							{
								break;
							}

							++iBlacklistWordLast;
						}
					}
				}

				// Calculate ending offset, now possibly adjusted for full matches.
				UInt32 const uEndingOffsetInBytes = iBlacklistWordLast->GetOriginalEndOffsetInBytes();

				// Copy through existing characters that haven't been filtered first.
				sOutput.Append(rsString.CStr() + uLastOffsetInBytes, (uStartingOffsetInBytes - uLastOffsetInBytes));
				uLastOffsetInBytes = uEndingOffsetInBytes;

				// Apply the substitution.
				sOutput.Append(*psSubstitution);

				// Start at the next character.
				iWordFirst = (iBlacklistWordLast + 1);
			}
			// Otherwise and finally, just advance to the next character and perform
			// another match.
			else
			{
				// Start the next match at the next character.
				++iWordFirst;
			}
		}
	}

	// If filtering occurred, complete and set the substitution.
	if (bReturn)
	{
		sOutput.Append(rsString.CStr() + uLastOffsetInBytes, (rsString.GetSize() - uLastOffsetInBytes));
		uLastOffsetInBytes = rsString.GetSize();

		rsString.Swap(sOutput);
	}

	// Return whether we filtered the string or not.
	return bReturn;
}

/**
 * Helper function used by WordFilter::LoadConfiguration().
 * Checks the table dataNode for an option. If set, applies
 * the value to uOptions
 *
 * @return True if the check was successful, false if an error occured. Errors:
 * - the option is defined in the table but the value is not a boolean.
 */
static inline Bool InternalStaticGetOption(
	const DataStore& dataStore,
	const DataNode& dataNode,
	HString optionName,
	UInt32 uOptionValue,
	UInt32& ruOptions)
{
	DataNode value;
	if (!dataStore.GetValueFromTable(dataNode, optionName, value))
	{
		// Option is not defined, so no error.
		return true;
	}

	Bool bValue = false;
	if (!dataStore.AsBoolean(value, bValue))
	{
		// Option flag must be a boolean, so return an error.
		return false;
	}

	// Add or remove the option flag.
	if (bValue)
	{
		ruOptions |= uOptionValue;
	}
	else
	{
		ruOptions &= ~uOptionValue;
	}

	// No errors, success.
	return true;
}

/**
 * Reconfigure filtering based on values set in the dataNode table.
 *
 * @return True on success, false on failure. On failure, the existing
 * configuration state is left unmodified.
 */
Bool WordFilter::LoadConfiguration(const DataStore& dataStore, const DataNode& dataNode)
{
	// Node is expected to be a table.
	if (!dataNode.IsTable())
	{
		return false;
	}

	UInt32 uOptions = 0u;

	// Enumerate and set options.
	for (size_t i = 0u; i < SEOUL_ARRAY_COUNT(s_kaWordFilterOptions); ++i)
	{
		const WordFilterOptionEntry& entry = s_kaWordFilterOptions[i];

		// A false return here means "parse error", so fail immediately.
		if (!InternalStaticGetOption(
			dataStore,
			dataNode,
			entry.m_sOptionName,
			entry.m_uOptionBitValue,
			uOptions))
		{
			return false;
		}
	}

	// Set the new options and return success.
	m_uOptions = uOptions;
	return true;
}

/**
 * Load the whitelist, the blacklist, and the known words list.
 * - whitelistNode - explicitly allowed words.
 * - blacklistNode - explicitly disallowed words.
 * - knownWordNodes - additional words used for sentence structure understanding
 *   in the absence of valid spaces. The knownWords list also appends/includes
 *   the whitelist and blacklist, so specified knownWords are in addition to those
 *   2 lists.
 *
 * @return True on success, false on failure. On failure, the existing lists
 * are left umodified.
 */
Bool WordFilter::LoadLists(
	const DataStore& dataStore,
	const DataNode& blacklistNode,
	const DataNode& knownWordsNode,
	const DataNode& whitelistNode)
{
	WordFilterRootNode knownWords;
	WordFilterRootNode blacklist;
	WordFilterRootNode whitelist;

	// Process the specified blacklist.
	if (!blacklistNode.IsNull())
	{
		// Failure, return immediately.
		if (!InternalBuildFilterTree(blacklist, dataStore, blacklistNode, m_uOptions, true))
		{
			return false;
		}

		// Also process the blacklist into the known words trie.
		if (!InternalBuildFilterTree(knownWords, dataStore, blacklistNode, m_uOptions, false))
		{
			return false;
		}
	}

	// Process the specified whitelist.
	if (!whitelistNode.IsNull())
	{
		// Failure, return immediately.
		if (!InternalBuildFilterTree(whitelist, dataStore, whitelistNode, m_uOptions, true))
		{
			return false;
		}

		// Also process the whitelist into the known words trie.
		if (!InternalBuildFilterTree(knownWords, dataStore, whitelistNode, m_uOptions, false))
		{
			return false;
		}
	}

	// Finally process any explicitly specified known words list.
	if (!knownWordsNode.IsNull())
	{
		// Failure, return immediately.
		if (!InternalBuildFilterTree(knownWords, dataStore, knownWordsNode, m_uOptions, true))
		{
			return false;
		}
	}

	// Success - swap in results and return true.
	blacklist.Swap(m_BlacklistRoot);
	knownWords.Swap(m_KnownWordsRoot);
	whitelist.Swap(m_WhitelistRoot);
	return true;
}

/**
 * Reset the internal substitution table state to one defined at dataNode
 *
 * @return True on success, false on failure. On failure, the existing
 * substitution table state is left unmodified.
 */
Bool WordFilter::LoadSubstitutionTable(const DataStore& dataStore, const DataNode& dataNode)
{
	// Node is expected to be an array.
	UInt32 uArrayCount = 0u;
	if (!dataStore.GetArrayCount(dataNode, uArrayCount))
	{
		return false;
	}

	Substitutions tSubstitutions;

	DataNode arrayValue;
	DataNode fromValue;
	DataNode toValue;
	for (UInt32 i = 0u; i < uArrayCount; ++i)
	{
		// Get each element of the array.
		if (!dataStore.GetValueFromArray(dataNode, i, arrayValue))
		{
			return false;
		}

		// Each element must be convertible to an array, with 2 elements,
		// each a string ("to", "from").
		if (!arrayValue.IsArray())
		{
			return false;
		}

		// Get the from and to values from the array.
		if (!dataStore.GetValueFromArray(arrayValue, 0u, fromValue))
		{
			return false;
		}
		if (!dataStore.GetValueFromArray(arrayValue, 1u, toValue))
		{
			return false;
		}

		// Convert both to string values.
		Byte const* sFrom = nullptr;
		UInt32 zFromSizeInBytes = 0u;
		if (!dataStore.AsString(fromValue, sFrom, zFromSizeInBytes))
		{
			return false;
		}
		Byte const* sTo = nullptr;
		UInt32 zToSizeInBytes = 0u;
		if (!dataStore.AsString(toValue, sTo, zToSizeInBytes))
		{
			return false;
		}

		// Insert the pair into the substitutions table.
		SEOUL_VERIFY(tSubstitutions.Overwrite(String(sFrom, zFromSizeInBytes), String(sTo, zToSizeInBytes)).Second);
	}

	// Success - swap our constructed table into the result and return success.
	tSubstitutions.Swap(m_tSubstitutions);
	return true;
}

/** Common helper, populates a trie from data defined in dataNode. */
Bool WordFilter::InternalBuildFilterTree(
	WordFilterRootNode& rRoot,
	const DataStore& dataStore,
	const DataNode& dataNode,
	UInt32 uGeneralOptions,
	Bool bProcessOptions)
{
	// Node is expected to be an array.
	UInt32 uArrayCount = 0u;
	if (!dataStore.GetArrayCount(dataNode, uArrayCount))
	{
		return false;
	}

	DataNode value;
	for (UInt32 i = 0u; i < uArrayCount; ++i)
	{
		// Get each element of the array.
		if (!dataStore.GetValueFromArray(dataNode, i, value))
		{
			return false;
		}

		Byte const* sFilterWord = nullptr;
		UInt32 zFilterWordSizeInBytes = 0u;
		UInt32 uOptions = 0u;

		// Each elements is an array or a string. If an array, elements after the first are options.
		if (value.IsArray())
		{
			DataNode subValue;

			// First element must be convertible to a string.
			if (!dataStore.GetValueFromArray(value, 0u, subValue) ||
				!dataStore.AsString(subValue, sFilterWord, zFilterWordSizeInBytes))
			{
				return false;
			}

			// Process options, unless specified to skip them (used to remove options
			// when merging whitelist and blacklist into the known words list).
			if (bProcessOptions)
			{
				// Now process options.
				UInt32 uOptionArrayCount = 0u;
				if (!dataStore.GetArrayCount(value, uOptionArrayCount))
				{
					return false;
				}

				// Iterate over each option.
				for (UInt32 uOption = 1u; uOption < uOptionArrayCount; ++uOption)
				{
					// Each option field must be convertible to an identifier.
					DataNode optionValue;
					String optionStringValue;
					if (!dataStore.GetValueFromArray(value, uOption, optionValue) ||
						!dataStore.AsString(optionValue, optionStringValue))
					{
						return false;
					}

					// Convert known options.
					WordFilterWordOptionEntry const* pEntry = Find(
						&s_kaWordFilterWordOptionEntries[0],
						&s_kaWordFilterWordOptionEntries[SEOUL_ARRAY_COUNT(s_kaWordFilterWordOptionEntries)],
						HString(optionStringValue));
					if (nullptr == pEntry)
					{
						return false;
					}

					uOptions |= pEntry->m_uValue;
				}
			}
		}
		else
		{
			// Each element must be convertible to a string.
			if (!dataStore.AsString(value, sFilterWord, zFilterWordSizeInBytes))
			{
				return false;
			}
		}

		// Encode the word for the trie, then insert it.
		rRoot.InsertWord(sFilterWord, zFilterWordSizeInBytes, uGeneralOptions | uOptions);
	}

	// Success.
	return true;
}

} // namespace Seoul
