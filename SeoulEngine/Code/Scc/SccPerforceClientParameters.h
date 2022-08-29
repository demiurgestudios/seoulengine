/**
 * \file SccPerforceClientParameters.h
 * \brief Configuration structure for a Perforce source control
 * client.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCC_PERFORCE_CLIENT_PARAMETERS_H
#define SCC_PERFORCE_CLIENT_PARAMETERS_H

#include "SeoulString.h"

namespace Seoul::Scc
{

/**
 * Parameters that fully define a Perforce Client connection - connections
 * can still be established with only some of these values specified.
 */
struct PerforceClientParameters SEOUL_SEALED
{
	Int32 m_iP4Changelist = -1;
	String m_sP4ClientWorkspace{};
	String m_sP4Port{};
	String m_sP4User{};
	String m_sP4Password{};
	Int32 m_iTimeoutInSeconds = 60;

	Bool IsValid() const
	{
		return (
			m_iP4Changelist >= 0 &&
			!m_sP4Port.IsEmpty() &&
			!m_sP4User.IsEmpty() &&
			!m_sP4ClientWorkspace.IsEmpty());
	}
}; // struct PerforceClientParameters

} // namespace Seoul::Scc

#endif // include guard
