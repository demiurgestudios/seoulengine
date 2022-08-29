/**
 * \file HTTPHeaderTableTest.h
 * \brief Unit tests for the HTTP::HeaderTable class.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef HTTP_HEADER_TABLE_TEST_H
#define HTTP_HEADER_TABLE_TEST_H

#include "Prereqs.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

class HTTPHeaderTableTest SEOUL_SEALED
{
public:
	void TestBasic();
	void TestClone();

private:
}; // class HTTPHeaderTableTest

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul

#endif // include guard
