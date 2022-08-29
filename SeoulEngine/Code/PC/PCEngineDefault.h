/**
 * \file PCEngineDefault.h
 * \brief Specialization of Engine for the PC platform with default
 * implementations of various online functionality (profiles, sessions, etc.).
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef PC_ENGINE_DEFAULT_H
#define PC_ENGINE_DEFAULT_H

#include "PCEngine.h"

namespace Seoul
{

/**
 * Default implementation of the PC engine.  Uses the default
 * implementation of various online functionality (profiles, sessions, etc.).
 */
class PCEngineDefault SEOUL_SEALED : public PCEngine
{
public:
	PCEngineDefault(const PCEngineSettings& settings);
	virtual ~PCEngineDefault();

	virtual EngineType GetType() const SEOUL_OVERRIDE { return EngineType::kPCDefault; }

private:
}; // class PCEngineDefault

} // namespace Seoul

#endif // include guard
