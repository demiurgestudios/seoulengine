/**
* \file CompressionTest.h
* \brief Unit test header file for general Seoul utility functions
*
* Copyright (c) Demiurge Studios, Inc.
* 
* This source code is licensed under the MIT license.
* Full license details can be found in the LICENSE file
* in the root directory of this source tree.
*/

#pragma once
#ifndef COMPRESSION_TEST_H
#define COMPRESSION_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class CompressionTest SEOUL_SEALED
{
public:
	void TestGzipCompress();
	void TestGzipCompressSmall();
	void TestLZ4Compress();
	void TestLZ4CompressSmall();
	void TestZlibCompress();
	void TestZlibCompressSmall();
	void TestZSTDCompress();
	void TestZSTDCompressSmall();
	void TestZSTDCompressDict();

};

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
