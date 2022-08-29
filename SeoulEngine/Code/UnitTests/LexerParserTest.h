/**
 * \file LexerParserTest.h
 * \brief Test SeoulEngine lexers and parsers - specifically, json files,
 * data stores in json files, and data store alone.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef LEXER_PARSER_TEST_H
#define LEXER_PARSER_TEST_H

#include "Prereqs.h"
#include "DataStoreParser.h"
#include "Lexer.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class LexerParserTest SEOUL_SEALED
{
public:
	void TestDataStoreFromJsonFileBasic();
	void TestDataStoreFromJsonFileUnicode();
	void TestDataStoreBasic();
	void TestDataStoreNumbers();
	void TestDataStoreStrings();
	void TestDataStoreFromJsonFileErrors();
	void TestJSON();
	void TestDuplicateReject();
	void TestStringAsFilePath();
	void TestStringAsFilePathRegression();
}; // class LexerParserTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
