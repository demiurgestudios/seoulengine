/**
 * \file SaveLoadManagerSettings.h
 * \brief Configuration of the saving system (SaveLoadManager).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SAVE_LOAD_MANAGER_SETTINGS_H
#define SAVE_LOAD_MANAGER_SETTINGS_H

#include "HTTPCommon.h"
#include "Prereqs.h"
#include "SeoulHString.h"

namespace Seoul
{

/** Configuration of SaveLoadManager. */
struct SaveLoadManagerSettings SEOUL_SEALED
{
	typedef Delegate<
		HTTP::Request*(
			const String& sURL,
			const HTTP::ResponseDelegate& callback,
			HString sMethod,
			Bool bResendOnFailure,
			Bool bSuppressErrorMail)> CreateRequest;

	SaveLoadManagerSettings()
		: m_CreateRequest()
#if SEOUL_UNIT_TESTS
		, m_bEnableFirstTimeTests(false)
		, m_bEnableValidation(false)
#endif // /#if SEOUL_UNIT_TESTS
	{
	}

	CreateRequest m_CreateRequest;
#if SEOUL_UNIT_TESTS
	Bool m_bEnableFirstTimeTests;
	Bool m_bEnableValidation;
#endif // /#if SEOUL_UNIT_TESTS
}; // struct SaveLoadManagerSettings

} // namespace Seoul

#endif // include guard
