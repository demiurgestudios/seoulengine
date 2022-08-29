/**
 * \file SlimCSTest.h
 * \brief Unit tests of the SlimCS compiler and toolchain.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SLIM_CS_TEST_H
#define SLIM_CS_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class SlimCSTest SEOUL_SEALED
{
public:
	void TestFeatures();
	void TestMono();
	void TestSlimCSToLua();
	void TestSlimCSToLuaDebug();

	void TestSlimCSToLuaJapan() { DoTestSlimCSToLuaCulture("ja-JP"); }
	void TestSlimCSToLuaKorea() { DoTestSlimCSToLuaCulture("ko-KR"); }
	void TestSlimCSToLuaPoland() { DoTestSlimCSToLuaCulture("pl-PL"); }
	void TestSlimCSToLuaQatar() { DoTestSlimCSToLuaCulture("ar-QA"); }
	void TestSlimCSToLuaRussia() { DoTestSlimCSToLuaCulture("ru-RU"); }

private:
	void DoTestSlimCSToLuaCulture(Byte const* sCulture);
}; // class SlimCSTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
