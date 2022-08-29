/**
 * \file StreamBufferTest.h
 * \brief Unit tests for the serialization/deserialization StreamBuffer
 * class.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef	STREAM_BUFFER_TEST_H
#define STREAM_BUFFER_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class StreamBufferTest SEOUL_SEALED
{
public:
	void TestBasicStreamBuffer();
	void TestBigEndianStreamBuffer();
	void TestLittleEndianStreamBuffer();
	void TestRelinquishBuffer();
}; // class StreamBufferTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
