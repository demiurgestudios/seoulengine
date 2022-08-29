/**
 * \file SccPerforceClientParameters.cpp
 * \brief Configuration structure for a Perforce source control
 * client.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ReflectionDefine.h"
#include "SccPerforceClientParameters.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(Scc::PerforceClientParameters)
	SEOUL_PROPERTY_N("P4Changelist", m_iP4Changelist)
	SEOUL_PROPERTY_N("P4ClientWorkspace", m_sP4ClientWorkspace)
	SEOUL_PROPERTY_N("P4Port", m_sP4Port)
	SEOUL_PROPERTY_N("P4User", m_sP4User)
	SEOUL_PROPERTY_N("P4Password", m_sP4Password)
	SEOUL_PROPERTY_N("TimeoutInSeconds", m_iTimeoutInSeconds)
SEOUL_END_TYPE()

} // namespace Seoul
