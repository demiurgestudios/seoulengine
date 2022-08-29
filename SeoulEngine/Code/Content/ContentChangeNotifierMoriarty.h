/**
 * \file ContentChangeNotifierMoriarty.h
 * \brief Specialization of Content::ChangeNotifier that receives content change
 * events from Moriarty.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef CONTENT_CHANGE_NOTIFIER_MORIARTY_H
#define CONTENT_CHANGE_NOTIFIER_MORIARTY_H

#include "ContentChangeNotifier.h"

namespace Seoul::Content
{

#if SEOUL_WITH_MORIARTY

class ChangeNotifierMoriarty SEOUL_SEALED : public ChangeNotifier
{
public:
	/**
	 * @return The global singleton instance. Will be nullptr if that instance has not
	 * yet been created.
	 */
	static CheckedPtr<ChangeNotifierMoriarty> Get()
	{
		return (ChangeNotifierMoriarty*)ChangeNotifier::Get().Get();
	}

	ChangeNotifierMoriarty();
	~ChangeNotifierMoriarty();

protected:
	static void ChangeEventHandlerDelegate(
		FilePath oldFilePath,
		FilePath newFilePath,
		FileChangeNotifier::FileEvent eEvent)
	{
		ChangeNotifierMoriarty::Get()->ChangeEventHandler(oldFilePath, newFilePath, eEvent);
	}

	void ChangeEventHandler(
		FilePath oldFilePath,
		FilePath newFilePath,
		FileChangeNotifier::FileEvent eEvent);

	void ChangeEventHandlerImpl(
		FilePath oldFilePath,
		FilePath newFilePath,
		FileChangeNotifier::FileEvent eEvent);

	SEOUL_DISABLE_COPY(ChangeNotifierMoriarty);
}; // class Content::ChangeNotifierMoriarty

#endif // /#if SEOUL_WITH_MORIARTY

} // namespace Seoul::Content

#endif // include guard
