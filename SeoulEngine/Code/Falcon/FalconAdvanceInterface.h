/**
 * \file FalconAdvanceInterface.h
 * \brief The AdvanceInterface provides the minimum
 * set of mutators that Instance::Advance() (and a few
 * related operations, like GotoAnd) require to
 * perform their processing.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_ADVANCE_INTERFACE_H
#define FALCON_ADVANCE_INTERFACE_H

#include "FalconLabelName.h"
#include "FalconTypes.h"
#include "SeoulHString.h"

namespace Seoul::Falcon
{

// Forward declarations
class Instance;
class MovieClipInstance;

class AddInterface SEOUL_ABSTRACT
{
public:
	virtual ~AddInterface()
	{
	}

	virtual void FalconOnAddToParent(
		MovieClipInstance* pParent,
		Instance* pInstance,
		const HString& sClassName) = 0;

	virtual void FalconOnClone(
		Instance const* pFromInstance,
		Instance* pToInstance) = 0;

protected:
	AddInterface()
	{
	}

private:
	SEOUL_DISABLE_COPY(AddInterface);
}; // class AddInterface

class AdvanceInterface SEOUL_ABSTRACT : public AddInterface
{
public:
	virtual ~AdvanceInterface()
	{
	}

	virtual void FalconDispatchEnterFrameEvent(
		Instance* pInstance) = 0;

	virtual void FalconDispatchEvent(
		const HString& sEventName,
		SimpleActions::EventType eType,
		Instance* pInstance) = 0;

	virtual Float FalconGetDeltaTimeInSeconds() const = 0;

	virtual Bool FalconLocalize(
		const HString& sLocalizationToken,
		String& rsLocalizedText) = 0;

protected:
	AdvanceInterface()
	{
	}

	Bool FalconDispatchGotoEvent(Instance* pInstance, const HString& sEventName);

	Bool FalconIsGotoAndPlayEvent(const HString& sEventName) const
	{
		// TODO: This is ugly, but is otherwise a nice
		// integration into the existing event handling.
		return (0 == strncmp(sEventName.CStr(), "gotoAndPlay:", 12));
	}

	UInt16 FalconGetGotoAndPlayFrameNumber(const HString& sEventName) const
	{
		char* sEndPtr = nullptr;
		return (UInt16)STRTOUINT64(sEventName.CStr() + 12, &sEndPtr, 0);
	}

	Bool FalconIsGotoAndPlayByLabelEvent(const HString& sEventName) const
	{
		// TODO: This is ugly, but is otherwise a nice
		// integration into the existing event handling.
		return (0 == strncmp(sEventName.CStr(), "gotoAndPlayByLabel:", 19));
	}

	LabelName FalconGetGotoAndPlayFrameLabel(const HString& sEventName) const
	{
		return LabelName(sEventName.CStr() + 19, (sEventName.GetSizeInBytes() - 19));
	}

	Bool FalconIsGotoAndStopEvent(const HString& sEventName) const
	{
		// TODO: This is ugly, but is otherwise a nice
		// integration into the existing event handling.
		return (0 == strncmp(sEventName.CStr(), "gotoAndStop:", 12));
	}

	UInt16 FalconGetGotoAndStopFrameNumber(const HString& sEventName) const
	{
		char* sEndPtr = nullptr;
		return (UInt16)STRTOUINT64(sEventName.CStr() + 12, &sEndPtr, 0);
	}

	Bool FalconIsGotoAndStopByLabelEvent(const HString& sEventName) const
	{
		// TODO: This is ugly, but is otherwise a nice
		// integration into the existing event handling.
		return (0 == strncmp(sEventName.CStr(), "gotoAndStopByLabel:", 19));
	}

	LabelName FalconGetGotoAndStopFrameLabel(const HString& sEventName) const
	{
		return LabelName(sEventName.CStr() + 19, (sEventName.GetSizeInBytes() - 19));
	}

private:
	SEOUL_DISABLE_COPY(AdvanceInterface);
}; // class AdvanceInterface

} // namespace Seoul::Falcon

#endif // include guard
