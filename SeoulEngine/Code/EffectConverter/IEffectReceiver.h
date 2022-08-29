/**
 * \file IEffectReceiver.h
 * \brief Inherit from this interface to receive
 * events from an EffectConverter, in order to process parsed
 * effect data.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef IEFFECT_RECEIVER_H
#define IEFFECT_RECEIVER_H

#include "Prereqs.h"
namespace Seoul { namespace EffectConverter { class Converter; } }
namespace Seoul { namespace EffectConverter { namespace Util { struct Parameter; } } }
namespace Seoul { namespace EffectConverter { namespace Util { struct Technique; } } }

namespace Seoul::EffectConverter
{

class IEffectReceiver SEOUL_ABSTRACT
{
public:
	IEffectReceiver()
	{
	}

	virtual ~IEffectReceiver()
	{
	}

	virtual Bool AddParameter(const Converter& effectConverter, const Util::Parameter& parameter) = 0;
	virtual Bool AddTechnique(const Converter& effectConverter, const Util::Technique& technique) = 0;

private:
	SEOUL_DISABLE_COPY(IEffectReceiver);
}; // class IEffectReceiver

} // namespace Seoul::EffectConverter

#endif // include guard
