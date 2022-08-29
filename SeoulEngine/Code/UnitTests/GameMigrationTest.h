/**
 * \file GameMigrationTest.h
 * \brief Test for save game migrations.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GAME_MIGRATION_TEST_H
#define GAME_MIGRATION_TEST_H

#include "Prereqs.h"
#include "ReflectionMethod.h"

namespace Seoul { namespace Reflection { namespace Attributes { class PersistenceDataMigrationTest; } } }
namespace Seoul { class UnitTestsGameHelper; }

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class GameMigrationTest SEOUL_SEALED
{
public:
	GameMigrationTest();
	~GameMigrationTest();

	void TestMigrations();

private:
	void RunMigrationTest(HString sTypeName, const Reflection::Method *pMethod, const Reflection::Attributes::PersistenceDataMigrationTest* attribute);

	ScopedPtr<UnitTestsGameHelper> m_pHelper;

	SEOUL_DISABLE_COPY(GameMigrationTest);
}; // class GameMigrationTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
