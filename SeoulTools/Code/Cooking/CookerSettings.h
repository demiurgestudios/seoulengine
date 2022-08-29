/**
 * \file CookerSettings.h
 * \brief Configuration of the overall cooker operation.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef COOKER_SETTINGS_H
#define COOKER_SETTINGS_H

#include "FilePath.h"
#include "SccPerforceClientParameters.h"
#include "SeoulString.h"

namespace Seoul::Cooking
{

struct CookerSettings SEOUL_SEALED
{
	/** Target platform of the cooking job. */
	Platform m_ePlatform = keCurrentPlatform;

	/** (Optional) Path to package cooker configuration file. */
	String m_sPackageCookConfig;

	/** (Optional) Perforce source control configuration. */
	Scc::PerforceClientParameters m_P4Parameters;

	/** (Optional) Single file path, runs the cooker in single-file mode. */
	FilePath m_SingleCookPath;

	/** (Optional) If true, scripts build debug scripts only, not ship scripts. */
	Bool m_bDebugOnly = false;

	/**
	 * (Optional) If true, local cook. Certain features are disabled to
	 * decrease cook time (e.g. dictionary compression) and certain
	 * behaviors are modified (e.g. local cook database is written
	 * instead of remote).
	 */
	Bool m_bLocal = false;

	/**
	 * (Optional) If true, when specified, compression dictionaries will be re-generated.
	 *
	 * This flag can be used to force a recomputation of the compression dictionary
	 * used for a data set. By default, the dictionary is only regenerated when it
	 * does not exist. Regeneration can be slow, but compression quality will also be
	 * reduced as the dictionary falls out-of-sync with the data being compressed.
	 *
	 * IMPORTANT: In branches with patching enabled, it is important that cdict generation
	 * be disabled or a new (full) build generated after patching has started can
	 * cause generation of a unintentionally large patch.
	 */
	Bool m_bForceGenCdict = false;
}; // struct CookerSettings

} // namespace Seoul::Cooking

#endif // include guard
