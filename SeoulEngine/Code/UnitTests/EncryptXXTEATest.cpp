/**
 * \file EncryptXXTEATest.cpp
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EncryptXXTEA.h"
#include "EncryptXXTEATest.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(EncryptXXTEATest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestBasic)
SEOUL_END_TYPE()

void EncryptXXTEATest::TestBasic()
{
	// Data to use for encrypt testing.
	static const UInt32 kauDataToEncrypt[] =
	{
		0x7f9a6505, 0x4894fdff,
		0x732de639, 0xc86ea46d,
		0x397c5863, 0x2e1efd72,
		0xc23b1275, 0x3a36c757,
		0x8da2ff17, 0xb94b2dc4,
		0x4143d9ff, 0x7dc7cb9f,
		0x4e785877, 0x09d517de,
		0xa21ff245, 0x2cca8d65,
		0xc31539a3, 0x20db12aa,
		0xc092fd07, 0xfc99f989,
		0xd342828c, 0x28a2ffff,
		0x7e7ec4c7, 0xa87b95a5,
		0xa1b744d2, 0xca8f2ac8,
		0x0089f24d, 0x3b5340c6,
		0xa21de157, 0x19286b74,
		0xd197f926, 0xfefcc63e,
	};

	// Key to use for encrypt testing.
	static const UInt32 kauKey[] =
	{
		0xe0f84317,
		0xa6a478f9,
		0x0104c374,
		0x659852c9,
	};

	// Make a copy of the data.
	UInt32 aData[SEOUL_ARRAY_COUNT(kauDataToEncrypt)];
	memcpy(aData, kauDataToEncrypt, sizeof(kauDataToEncrypt));

	// Encrypt the data.
	EncryptXXTEA::EncryptInPlace(aData, SEOUL_ARRAY_COUNT(aData), kauKey);

	// Check that it is not equal to the original data.
	SEOUL_UNITTESTING_ASSERT(0 != memcmp(aData, kauDataToEncrypt, sizeof(kauDataToEncrypt)));

	// Decrypt the data.
	EncryptXXTEA::DecryptInPlace(aData, SEOUL_ARRAY_COUNT(aData), kauKey);

	// Check that it is now equal to the original data.
	SEOUL_UNITTESTING_ASSERT(0 == memcmp(aData, kauDataToEncrypt, sizeof(kauDataToEncrypt)));
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
