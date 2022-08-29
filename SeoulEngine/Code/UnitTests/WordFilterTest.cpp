/**
 * \file WordFilterTest.cpp
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DataStoreParser.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "UnitTesting.h"
#include "UnitTestsFileManagerHelper.h"
#include "WordFilterTest.h"
#include "WordFilter.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(WordFilterTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestBasic)
	SEOUL_METHOD(TestComplexNegatives)
	SEOUL_METHOD(TestComplexPositives)
	SEOUL_METHOD(TestFalsePositives)
	SEOUL_METHOD(TestLeetSpeak)
	SEOUL_METHOD(TestKnownPositives)
	SEOUL_METHOD(TestPhonetics)
	SEOUL_METHOD(TestUglyChatLog)
SEOUL_END_TYPE()

void WordFilterTest::TestBasic()
{
	WordFilter filter;
	filter.SetDefaultSubstitution("***");

	// Load a blacklist and a whitelist
	{
		DataStore dataStore;
		dataStore.MakeArray();
		DataNode blacklist;
		SEOUL_UNITTESTING_ASSERT(dataStore.SetArrayToArray(dataStore.GetRootNode(), 0u));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(dataStore.GetRootNode(), 0u, blacklist));
		{
			DataNode const root = blacklist;
			dataStore.SetStringToArray(root, 0u, "fuck");
			dataStore.SetStringToArray(root, 1u, "ass");
			dataStore.SetStringToArray(root, 2u, "twat");
		}

		DataNode whitelist;
		SEOUL_UNITTESTING_ASSERT(dataStore.SetArrayToArray(dataStore.GetRootNode(), 1u));
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(dataStore.GetRootNode(), 1u, whitelist));
		{
			DataNode const root = whitelist;
			dataStore.SetStringToArray(root, 0u, "assassin");
			dataStore.SetStringToArray(root, 1u, "assimilate");
			dataStore.SetStringToArray(root, 2u, "assist");
			dataStore.SetStringToArray(root, 3u, "assume");
		}

		SEOUL_UNITTESTING_ASSERT(filter.LoadLists(
			dataStore,
			blacklist,
			DataNode(),
			whitelist));
	}

	// Test some basic cases.
	String s;

	// Should not filter.
	s = "I'm an assassin.";
	SEOUL_UNITTESTING_ASSERT(!filter.FilterString(s));
	s = "I will assimilate you.";
	SEOUL_UNITTESTING_ASSERT(!filter.FilterString(s));
	s = "I am here to assist you.";
	SEOUL_UNITTESTING_ASSERT(!filter.FilterString(s));
	s = "I assume full responsibility.";
	SEOUL_UNITTESTING_ASSERT(!filter.FilterString(s));

	// Should filter.
	s = "What the fuck.";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("What the ***."), s);
	s = "Stop being an ass.";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("Stop being an ***."), s);
	s = "You're such a tw@.";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("You're such a ***."), s);
	s = "Stupid @ss.";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("Stupid ***."), s);
}

static void InternalStaticLoadChatFilter(
	WordFilter& filter,
	String& sDefaultSubstitution,
	HashSet<String, MemoryBudgets::TBD>& baseBlacklistSet,
	HashSet<String, MemoryBudgets::TBD>& blacklistSet)
{
	// A list of suffixes that we add to our "expected bad words" array,
	// to reduce the need to manually generate a large number of "this is
	// not a false positive, just a variation of a valid match".
	static Byte const* s_kaSuffixes[] =
	{
		"ed",
		"er",
		"es",
		"ing",
		"s",
	};

	FilePath filePath = FilePath::CreateConfigFilePath("Chat/ChatFilter.json");

	DataStore dataStore;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromFile(filePath, dataStore, DataStoreParserFlags::kLogParseErrors));

	DataNode value;
	DataNode const root = dataStore.GetRootNode();

	// Set the default substitution.
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("DefaultSubstitution"), value));
	SEOUL_UNITTESTING_ASSERT(dataStore.AsString(value, sDefaultSubstitution));
	filter.SetDefaultSubstitution(sDefaultSubstitution);

	// Load the configuration.
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("Configuration"), value));
	SEOUL_UNITTESTING_ASSERT(filter.LoadConfiguration(dataStore, value));

	// Load the blacklist, knownWords, and whitelist.
	{
		// Blacklist
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("Blacklist"), value));
		DataNode const blacklist = value;

		// Generate our own set for the blacklist for testing.
		{
			UInt32 uArrayCount = 0u;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(value, uArrayCount));
			for (UInt32 i = 0u; i < uArrayCount; ++i)
			{
				DataNode blacklistEntry;
				SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, i, blacklistEntry));
				String sEntry;
				if (blacklistEntry.IsArray())
				{
					SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(blacklistEntry, 0u, blacklistEntry));
				}
				SEOUL_UNITTESTING_ASSERT(dataStore.AsString(blacklistEntry, sEntry));

				SEOUL_UNITTESTING_ASSERT(baseBlacklistSet.Insert(sEntry).Second);
				(void)blacklistSet.Insert(sEntry);

				// Add suffix variations.
				for (size_t i = 0u; i < SEOUL_ARRAY_COUNT(s_kaSuffixes); ++i)
				{
					String sWithSuffix(sEntry + s_kaSuffixes[i]);
					(void)blacklistSet.Insert(sWithSuffix);
				}
			}
		}

		// Known words
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("KnownWords"), value));
		DataNode const knownWords = value;

		// Whitelist
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("Whitelist"), value));
		DataNode const whitelist = value;

		SEOUL_UNITTESTING_ASSERT(filter.LoadLists(
			dataStore,
			blacklist,
			knownWords,
			whitelist));
	}

	// Load substitutions.
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("Substitutions"), value));
	SEOUL_UNITTESTING_ASSERT(filter.LoadSubstitutionTable(dataStore, value));
}

static void InternalStaticLoadKnownNegatives(HashSet<String, MemoryBudgets::TBD>& rKnownNegatives)
{
	FilePath filePath = FilePath::CreateConfigFilePath("UnitTests/WordFilter/KnownComplexNegatives.json");

	DataStore dataStore;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromFile(filePath, dataStore, DataStoreParserFlags::kLogParseErrors));

	DataNode value;
	DataNode const root = dataStore.GetRootNode();
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("KnownComplexNegatives"), value));

	UInt32 uArrayCount = 0u;
	SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(value, uArrayCount));
	for (UInt32 i = 0u; i < uArrayCount; ++i)
	{
		DataNode entry;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, i, entry));
		String sEntry;
		SEOUL_UNITTESTING_ASSERT(dataStore.AsString(entry, sEntry));
		SEOUL_UNITTESTING_ASSERT(rKnownNegatives.Insert(sEntry).Second);
	}
}

static void InternalStaticLoadKnownPositives(HashSet<String, MemoryBudgets::TBD>& rKnownPositives)
{
	FilePath filePath = FilePath::CreateConfigFilePath("UnitTests/WordFilter/KnownComplexPositives.json");

	DataStore dataStore;
	SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromFile(filePath, dataStore, DataStoreParserFlags::kLogParseErrors));

	DataNode value;
	DataNode const root = dataStore.GetRootNode();
	SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(root, HString("KnownComplexPositives"), value));

	UInt32 uArrayCount = 0u;
	SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(value, uArrayCount));
	for (UInt32 i = 0u; i < uArrayCount; ++i)
	{
		DataNode entry;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, i, entry));
		String sEntry;
		SEOUL_UNITTESTING_ASSERT(dataStore.AsString(entry, sEntry));
		SEOUL_UNITTESTING_ASSERT(rKnownPositives.Insert(sEntry).Second);
	}
}

void WordFilterTest::TestComplexNegatives()
{
	UnitTestsFileManagerHelper helper;

	WordFilter filter;
	String sDefaultSubstitution;
	HashSet<String, MemoryBudgets::TBD> baseBlacklistSet;
	HashSet<String, MemoryBudgets::TBD> blacklistSet;
	InternalStaticLoadChatFilter(
		filter,
		sDefaultSubstitution,
		baseBlacklistSet,
		blacklistSet);

	// Should all *not* filter.
	String s;
	s = "Do not it's a trap!";
	SEOUL_UNITTESTING_ASSERT(!filter.FilterString(s));
	s = "C'mon, push it.";
	SEOUL_UNITTESTING_ASSERT(!filter.FilterString(s));
	s = "push it";
	SEOUL_UNITTESTING_ASSERT(!filter.FilterString(s));
	s = "As someone who respects authority...";
	SEOUL_UNITTESTING_ASSERT(!filter.FilterString(s));
	s = "afk";
	SEOUL_UNITTESTING_ASSERT(!filter.FilterString(s));
	s = "but  the";
	SEOUL_UNITTESTING_ASSERT(!filter.FilterString(s));
	s = "Poop butt the";
	SEOUL_UNITTESTING_ASSERT(!filter.FilterString(s));
	s = "As soon as";
	SEOUL_UNITTESTING_ASSERT(!filter.FilterString(s));
}

void WordFilterTest::TestComplexPositives()
{
	UnitTestsFileManagerHelper helper;

	WordFilter filter;
	String sDefaultSubstitution;
	HashSet<String, MemoryBudgets::TBD> baseBlacklistSet;
	HashSet<String, MemoryBudgets::TBD> blacklistSet;
	InternalStaticLoadChatFilter(
		filter,
		sDefaultSubstitution,
		baseBlacklistSet,
		blacklistSet);

	// Should all filter.
	String s;
	s = "F u ckthis.";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::Printf("%s.", sDefaultSubstitution.CStr()), s);
	s = "Whatthef u ckis this?";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::Printf("%s this?", sDefaultSubstitution.CStr()), s);
	s = "What thef uck is this?";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::Printf("What %s is this?", sDefaultSubstitution.CStr()), s);
	s = "What thefuc kis this?";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::Printf("What %s this?", sDefaultSubstitution.CStr()), s);
	s = "ohf uck me";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::Printf("%s", sDefaultSubstitution.CStr()), s);
	s = "oh myfu cking god";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::Printf("oh %s god", sDefaultSubstitution.CStr()), s);
	s = "Sweett its";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::Printf("%s", sDefaultSubstitution.CStr()), s);
	s = "this is someshi t";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::Printf("this is %s", sDefaultSubstitution.CStr()), s);
	s = "this is someshit t";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::Printf("this is %s t", sDefaultSubstitution.CStr()), s);
	s = "gof uckyourself";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::Printf("%s", sDefaultSubstitution.CStr()), s);
	s = "bukake";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::Printf("%s", sDefaultSubstitution.CStr()), s);
	s = "You are ana ss.";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::Printf("You are %s.", sDefaultSubstitution.CStr()), s);
	s = "Reeks sof ucking much.";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::Printf("Reeks %s much.", sDefaultSubstitution.CStr()), s);
	s = "fck it";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::Printf("%s it", sDefaultSubstitution.CStr()), s);
	s = "asshat";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::Printf("%s", sDefaultSubstitution.CStr()), s);
	s = "Youknowmenigga";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String(), s);
	s = "fcuk";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::Printf("%s", sDefaultSubstitution.CStr()), s);
	s = "cumdumpster";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::Printf("%s", sDefaultSubstitution.CStr()), s);
	s = "cumguzzler";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::Printf("%s", sDefaultSubstitution.CStr()), s);
	s = "testicle";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::Printf("%s", sDefaultSubstitution.CStr()), s);
	s = "testicles";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::Printf("%s", sDefaultSubstitution.CStr()), s);
	s = "labia";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::Printf("%s", sDefaultSubstitution.CStr()), s);
	s = "labias";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::Printf("%s", sDefaultSubstitution.CStr()), s);
	s = "scrote";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::Printf("%s", sDefaultSubstitution.CStr()), s);
	s = "scrotes";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::Printf("%s", sDefaultSubstitution.CStr()), s);
	s = "www";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::Printf("%s", sDefaultSubstitution.CStr()), s);
	s = "xhamster";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::Printf("%s", sDefaultSubstitution.CStr()), s);
	s = "nazi";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::Printf("%s", sDefaultSubstitution.CStr()), s);
	s = "nazis";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::Printf("%s", sDefaultSubstitution.CStr()), s);
	s = "brummynazi";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::Printf("%s", sDefaultSubstitution.CStr()), s);
	s = "brummynazis";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String::Printf("%s", sDefaultSubstitution.CStr()), s);
}

static void LoadWordLists(DataStore& rDataStore)
{
	static Byte const* const s_kaFiles[] =
	{
		"UnitTests/WordFilter/CensusUS1990FemaleFirst.json",
		"UnitTests/WordFilter/CensusUS1990MaleFirst.json",
		"UnitTests/WordFilter/CensusUS1990Surnames.json",
		"UnitTests/WordFilter/2014_2_SubdivisionCodes.json",
		"UnitTests/WordFilter/2014_2_UNLOCODE_CodeListPart1.json",
		"UnitTests/WordFilter/2014_2_UNLOCODE_CodeListPart2.json",
		"UnitTests/WordFilter/2014_2_UNLOCODE_CodeListPart3.json",
	};

	for (size_t i = 0u; i < SEOUL_ARRAY_COUNT(s_kaFiles); ++i)
	{
		FilePath const filePath = FilePath::CreateConfigFilePath(s_kaFiles[i]);

		DataStore dataStore;
		SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromFile(filePath, dataStore, DataStoreParserFlags::kLogParseErrors));

		if (0u == i)
		{
			rDataStore.Swap(dataStore);
		}
		else
		{
			DataNode to;
			SEOUL_UNITTESTING_ASSERT(rDataStore.GetValueFromTable(rDataStore.GetRootNode(), HString("Words"), to));

			DataNode from;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("Words"), from));

			UInt32 uTo = 0u;
			SEOUL_UNITTESTING_ASSERT(rDataStore.GetArrayCount(to, uTo));
			UInt32 uFrom = 0u;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(from, uFrom));
			for (UInt32 i = 0u; i < uFrom; ++i)
			{
				Byte const* s = nullptr;
				UInt32 uSizeInBytes = 0u;
				DataNode value;
				SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(from, i, value));
				SEOUL_UNITTESTING_ASSERT(dataStore.AsString(value, s, uSizeInBytes));
				SEOUL_UNITTESTING_ASSERT(rDataStore.SetStringToArray(to, uTo, s, uSizeInBytes));
				++uTo;
			}
		}
	}
}

void WordFilterTest::TestFalsePositives()
{
	UnitTestsFileManagerHelper helper;

	WordFilter filter;
	String sDefaultSubstitution;
	HashSet<String, MemoryBudgets::TBD> baseBlacklistSet;
	HashSet<String, MemoryBudgets::TBD> blacklistSet;
	InternalStaticLoadChatFilter(
		filter,
		sDefaultSubstitution,
		baseBlacklistSet,
		blacklistSet);

	// Louad our set of additional "known negatives".
	HashSet<String, MemoryBudgets::TBD> additionalKnownNegatives;
	InternalStaticLoadKnownNegatives(additionalKnownNegatives);

	// Load our set of additional "known positives", in addition to the direct matches against bad words.
	HashSet<String, MemoryBudgets::TBD> additionalKnownPositives;
	InternalStaticLoadKnownPositives(additionalKnownPositives);

	// Load our large database of valid words.
	{
		DataStore dataStore;
		LoadWordLists(dataStore);

		DataNode wordList;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("Words"), wordList));

		UInt32 uArrayCount = 0u;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(wordList, uArrayCount));

		// Check that all bad words above do not match anything in our giant word list,
		// other than themselves.
		UInt32 uFalseNegatives = 0u;
		UInt32 uFalsePositives = 0u;
		for (UInt32 i = 0u; i < uArrayCount; ++i)
		{
			DataNode wordValue;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(wordList, i, wordValue));

			String sWord;
			if (!dataStore.AsString(wordValue, sWord))
			{
				SEOUL_UNITTESTING_FAIL("Failed to convert node %d to string", i);
			}

			// Make sure filtering produces the expected output -
			// filtering a string not in the blacklistSet is a false positive.
			String sFilteredWord(sWord);
			if (filter.FilterString(sFilteredWord))
			{
				Bool const bIsBadWord = blacklistSet.HasKey(sWord);
				if (!bIsBadWord)
				{
					// Check the additional known positives list.
					Bool const bIsKnownComplexPositives = additionalKnownPositives.HasKey(sWord);

					// Not a direct bad word and not a known positive, this is a false positive.
					if (!bIsKnownComplexPositives)
					{
						// False positive - record, and then report at the end.
						SEOUL_LOG("%s -> %s", sWord.CStr(), sFilteredWord.CStr());
						++uFalsePositives;
					}
				}
			}
			// On a failure, check if the word is in the baseBlacklistSet - this
			// means we have a guaranteed false negative.
			else
			{
				Bool const bIsBaseBadWord = baseBlacklistSet.HasKey(sWord);
				if (bIsBaseBadWord)
				{
					// Check the additional known negatives list.
					Bool const bIsKnownComplexNegative = additionalKnownNegatives.HasKey(sWord);

					// Not a known negative, this is a false negative.
					if (!bIsKnownComplexNegative)
					{
						// False negative - record, and then report at the end.
						SEOUL_LOG("False negative: %s", sWord.CStr());
						++uFalseNegatives;
					}
				}
			}
		}

		if (0u != uFalseNegatives)
		{
			SEOUL_LOG("%u False Negatives", uFalseNegatives);
		}

		if (0u != uFalsePositives)
		{
			SEOUL_LOG("%u False Positives", uFalsePositives);
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(0u, uFalseNegatives);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0u, uFalsePositives);
	}
}

void WordFilterTest::TestLeetSpeak()
{
	WordFilter filter;
	filter.SetDefaultSubstitution("***");

	// Load a blacklist
	{
		DataStore dataStore;
		dataStore.MakeArray();
		DataNode const root = dataStore.GetRootNode();
		dataStore.SetStringToArray(root, 0u, "ass");
		dataStore.SetStringToArray(root, 1u, "boob");
		dataStore.SetStringToArray(root, 2u, "shit");
		dataStore.SetStringToArray(root, 3u, "poop");
		dataStore.SetStringToArray(root, 4u, "tits");
		SEOUL_UNITTESTING_ASSERT(filter.LoadLists(dataStore, root, DataNode(), DataNode()));
	}

	// Test some leetspeak cases.
	String s;

	// Should filter.
	s = "a$$";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("***"), s);
	s = "p0op";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("***"), s);
	s = "po0p";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("***"), s);
	s = "p00p";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("***"), s);
	s = "p0o0ooo0ooop";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("***"), s);
	s = "7175";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("***"), s);
	s = "$#!+";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("***"), s);
	s = "sh!t";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("***"), s);
	s = "sh!t";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("***"), s);

	// Test skip character handling - the match here should be
	// attempted with the 0 removed.
	s = "71075";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("***"), s);

	// New additin (8 = B)
	s = "8008";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("***"), s);
	s = "80085";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("***"), s);
}

void WordFilterTest::TestKnownPositives()
{
	UnitTestsFileManagerHelper helper;

	WordFilter filter;
	String sDefaultSubstitution;
	HashSet<String, MemoryBudgets::TBD> baseBlacklistSet;
	HashSet<String, MemoryBudgets::TBD> blacklistSet;
	InternalStaticLoadChatFilter(
		filter,
		sDefaultSubstitution,
		baseBlacklistSet,
		blacklistSet);

	HashSet<String, MemoryBudgets::TBD> knownPositives;
	InternalStaticLoadKnownPositives(knownPositives);

	// Make sure all known positives are filtered.
	UInt32 uKnownPositiveMisses = 0u;
	auto const iBegin = knownPositives.Begin();
	auto const iEnd = knownPositives.End();
	for (auto i = iBegin; iEnd != i; ++i)
	{
		String s = *i;
		if (!filter.FilterString(s))
		{
			SEOUL_LOG("Missed Known Positive: %s", i->CStr());
			++uKnownPositiveMisses;
		}
	}

	if (0u != uKnownPositiveMisses)
	{
		SEOUL_LOG("%u Missed Known Positives", uKnownPositiveMisses);
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, uKnownPositiveMisses);
}

void WordFilterTest::TestPhonetics()
{
	WordFilter filter;
	filter.SetDefaultSubstitution("***");

	// Load a blacklist
	{
		DataStore dataStore;
		dataStore.MakeArray();
		DataNode const root = dataStore.GetRootNode();
		dataStore.SetStringToArray(root, 0u, "bigger");
		dataStore.SetStringToArray(root, 1u, "fuck");
		dataStore.SetStringToArray(root, 2u, "fuckit");
		SEOUL_UNITTESTING_ASSERT(filter.LoadLists(dataStore, root, DataNode(), DataNode()));
	}

	// Test some leetspeak cases.
	String s;

	// Should filter.
	s = "biggar";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("***"), s);
	s = "biggur";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("***"), s);
	s = "biggah";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("***"), s);
	s = "bigguh";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("***"), s);
	s = "fahk";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("***"), s);
	s = "fahkeet";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("***"), s);
	s = "fawk";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("***"), s);
	s = "fawkit";
	SEOUL_UNITTESTING_ASSERT(filter.FilterString(s));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("***"), s);
}

void WordFilterTest::TestUglyChatLog()
{
	UnitTestsFileManagerHelper helper;

	WordFilter filter;
	String sDefaultSubstitution;
	HashSet<String, MemoryBudgets::TBD> baseBlacklistSet;
	HashSet<String, MemoryBudgets::TBD> blacklistSet;
	InternalStaticLoadChatFilter(
		filter,
		sDefaultSubstitution,
		baseBlacklistSet,
		blacklistSet);

	// Load and process the chat log.
	UInt32 uLinesFailed = 0u;
	{
		FilePath filePath = FilePath::CreateConfigFilePath("UnitTests/WordFilter/UglyChatLog.json");

		DataStore dataStore;
		SEOUL_UNITTESTING_ASSERT(DataStoreParser::FromFile(filePath, dataStore, DataStoreParserFlags::kLogParseErrors));

		DataNode chatLog;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromTable(dataStore.GetRootNode(), HString("ChatLog"), chatLog));

		UInt32 uArrayCount = 0u;
		SEOUL_UNITTESTING_ASSERT(dataStore.GetArrayCount(chatLog, uArrayCount));
		for (UInt32 i = 0u; i < uArrayCount; ++i)
		{
			DataNode value;
			SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(chatLog, i, value));

			Bool bShouldFilter = false;
			String sLine;
			String sExpectedLine;
			if (value.IsArray())
			{
				DataNode sub;
				SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 0u, sub));
				SEOUL_UNITTESTING_ASSERT(dataStore.AsString(sub, sLine));
				SEOUL_UNITTESTING_ASSERT(dataStore.GetValueFromArray(value, 1u, sub));
				SEOUL_UNITTESTING_ASSERT(dataStore.AsString(sub, sExpectedLine));
				bShouldFilter = true;
			}
			else
			{
				SEOUL_UNITTESTING_ASSERT(dataStore.AsString(value, sLine));
			}

			String sLineCopy(sLine);
			if (bShouldFilter != filter.FilterString(sLineCopy))
			{
				if (bShouldFilter)
				{
					SEOUL_LOG("%u: Failed filtering: \"%s\"", (i + 1), sLine.CStr());
				}
				else
				{
					SEOUL_LOG("%u: Wrong filter: \"%s\" -> \"%s\"", (i + 1), sLine.CStr(), sLineCopy.CStr());
				}
				++uLinesFailed;
			}
		}
	}

	if (0u != uLinesFailed)
	{
		SEOUL_LOG("%u Failed Lines", uLinesFailed);
	}

	SEOUL_UNITTESTING_ASSERT_EQUAL(0u, uLinesFailed);
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
