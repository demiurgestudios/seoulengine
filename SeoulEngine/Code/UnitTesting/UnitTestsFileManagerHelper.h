/**
 * \file UnitTestsFileManagerHelper.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef UNIT_TESTS_FILE_MANAGER_HELPER_H
#define UNIT_TESTS_FILE_MANAGER_HELPER_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class UnitTestsFileManagerHelper SEOUL_SEALED
{
public:
	UnitTestsFileManagerHelper();
	~UnitTestsFileManagerHelper();

private:
	Bool m_bShutdownFileManager;
	Bool m_bShutdownGamePaths;
}; // class UnitTestsFileManagerHelper

#endif // /#if SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
