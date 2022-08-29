/**
 * \file PCScopedCom.h
 * \brief Utility to initialize and uninitialize COM services
 * for operations that depend on COM.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef PC_SCOPED_COM_H
#define PC_SCOPED_COM_H

#include "Prereqs.h"

namespace Seoul
{

/** Utility to handle initialization of COM support. */
class PCScopedCom SEOUL_SEALED
{
public:
	PCScopedCom();
	~PCScopedCom();

private:
	SEOUL_DISABLE_COPY(PCScopedCom);

	const Int32 m_eResult;
}; // class PCScopedCom

} // namespace Seoul

#endif // include guard
