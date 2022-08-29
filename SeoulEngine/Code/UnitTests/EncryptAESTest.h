/**
 * \file EncryptAESTest.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ENCRYPT_AES_TEST_H
#define ENCRYPT_AES_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class EncryptAESTest SEOUL_SEALED
{
public:
	void TestBasic();
	void TestZero();
	void TestOneByte();
	void Test15Bytes();
	void Test16Bytes();
	void Test17Bytes();
	void Test31Bytes();
	void Test32Bytes();
	void Test33Bytes();

	void TestSHA512DigestZero();
	void TestSHA512DigestOneByte();
	void TestSHA512Digest63Bytes();
	void TestSHA512Digest64Bytes();
	void TestSHA512Digest65Bytes();
	void TestSHA512Digest127Bytes();
	void TestSHA512Digest128Bytes();
	void TestSHA512Digest129Bytes();
}; // class EncryptAESTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
