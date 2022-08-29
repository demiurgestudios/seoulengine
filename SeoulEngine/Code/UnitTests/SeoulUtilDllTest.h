/**
 * \file SeoulUtilDll.h
 * \brief Tests of the SeoulUtil.dll in the tools codebase.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SEOUL_UTIL_DLL_H
#define SEOUL_UTIL_DLL_H

#include "Prereqs.h"
#include "ScopedPtr.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class SeoulUtilDllTest SEOUL_SEALED
{
public:
	SeoulUtilDllTest();
	~SeoulUtilDllTest();

	void TestAppendToJson();
	void TestCookJson();
	void TestMinifyJson();
	void TestGetModifiedTimeOfFileInSar();

private:
	SEOUL_DISABLE_COPY(SeoulUtilDllTest);
};

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
