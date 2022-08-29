/**
 * \file CookerSettings.cpp
 * \brief Configuration of the overall cooker operation.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "CookerSettings.h"
#include "ReflectionDefine.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(Cooking::CookerSettings)
	SEOUL_PROPERTY_N("Platform", m_ePlatform)
	SEOUL_PROPERTY_N("PackageCookConfig", m_sPackageCookConfig)
	SEOUL_PROPERTY_N("P4Parameters", m_P4Parameters)
	SEOUL_PROPERTY_N("SingleCookPath", m_SingleCookPath)
	SEOUL_PROPERTY_N("DebugOnly", m_bDebugOnly)
	SEOUL_PROPERTY_N("Local", m_bLocal)
	SEOUL_PROPERTY_N("ForceGenCdict", m_bForceGenCdict)
SEOUL_END_TYPE()

} // namespace Seoul
