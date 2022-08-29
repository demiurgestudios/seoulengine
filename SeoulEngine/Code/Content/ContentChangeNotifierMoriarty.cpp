/**
 * \file ContentChangeNotifierMoriarty.cpp
 * \brief Specialization of Content::ChangeNotifier that receives content change
 * events from Moriarty.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "MoriartyClient.h"
#include "ContentChangeNotifierMoriarty.h"
#include "Thread.h"

namespace Seoul::Content
{

#if SEOUL_WITH_MORIARTY

ChangeNotifierMoriarty::ChangeNotifierMoriarty()
{
	MoriartyClient::Get()->RegisterContentChangeEventHandler(ChangeEventHandlerDelegate);
}

ChangeNotifierMoriarty::~ChangeNotifierMoriarty()
{
	MoriartyClient::Get()->RegisterContentChangeEventHandler(nullptr);
}

void ChangeNotifierMoriarty::ChangeEventHandler(
	FilePath oldFilePath,
	FilePath newFilePath,
	FileChangeNotifier::FileEvent eEvent)
{
	// Special handling for some one-to-many types.
	if (IsTextureFileType(newFilePath.GetType()))
	{
		newFilePath.SetType(FileType::kTexture0);
		ChangeEventHandlerImpl(oldFilePath, newFilePath, eEvent);
		newFilePath.SetType(FileType::kTexture1);
		ChangeEventHandlerImpl(oldFilePath, newFilePath, eEvent);
		newFilePath.SetType(FileType::kTexture2);
		ChangeEventHandlerImpl(oldFilePath, newFilePath, eEvent);
		newFilePath.SetType(FileType::kTexture3);
		ChangeEventHandlerImpl(oldFilePath, newFilePath, eEvent);
		newFilePath.SetType(FileType::kTexture4);
		ChangeEventHandlerImpl(oldFilePath, newFilePath, eEvent);
	}
	// Otherwise, dispatch normally.
	else
	{
		ChangeEventHandlerImpl(oldFilePath, newFilePath, eEvent);
	}
}

void ChangeNotifierMoriarty::ChangeEventHandlerImpl(
	FilePath oldFilePath,
	FilePath newFilePath,
	FileChangeNotifier::FileEvent eEvent)
{
	// Content change events are just passed through.
	ChangeEvent* pContentChangeEvent = SEOUL_NEW(MemoryBudgets::Content) ChangeEvent(
		oldFilePath,
		newFilePath,
		eEvent);
	SeoulGlobalIncrementReferenceCount(pContentChangeEvent);

	// Insert into the outgoing event queue.
	m_OutgoingContentChanges.Push(pContentChangeEvent);
}

#endif // /#if SEOUL_WITH_MORIARTY

} // namespace Seoul::Content
