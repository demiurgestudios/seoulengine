/**
 * \file ContentChangeNotifierLocal.h
 * \brief Specialization of Content::ChangeNotifier for monitoring local files. Uses
 * FileChangeNotifier to detect file changes.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef CONTENT_CHANGE_NOTIFIER_LOCAL_H
#define CONTENT_CHANGE_NOTIFIER_LOCAL_H

#include "ContentChangeNotifier.h"
#include "SeoulSignal.h"

namespace Seoul::Content
{

class ChangeNotifierLocal SEOUL_SEALED : public ChangeNotifier
{
public:
	SEOUL_DELEGATE_TARGET(ChangeNotifierLocal);

	// Construct this Content::ChangeNotifier so it is monitoring eDirectory.
	ChangeNotifierLocal();
	~ChangeNotifierLocal();

private:
	// Invoked when a file change occurs in the config folder.
	void OnConfigFileChanged(
		const String& sOldPath,
		const String& sNewPath,
		FileChangeNotifier::FileEvent eEvent)
	{
		OnFileChanged(true, sOldPath, sNewPath, eEvent);
	}

	// Invoked when a file change occurs in the source folder.
	void OnSourceFileChanged(
		const String& sOldPath,
		const String& sNewPath,
		FileChangeNotifier::FileEvent eEvent)
	{
		OnFileChanged(false, sOldPath, sNewPath, eEvent);
	}

	void HandleOnFileChanged(
		FilePath filePathOld,
		FilePath filePathNew,
		FileChangeNotifier::FileEvent eEvent);

	void OnFileChanged(
		Bool bConfigFile,
		const String& sOldPath,
		const String& sNewPath,
		FileChangeNotifier::FileEvent eEvent);

	Int ProcessIncomingChanges(const Thread& thread);

	ScopedPtr<Thread> m_pThread;
	Signal m_WorkerThreadSignal;
	ScopedPtr<FileChangeNotifier> m_pConfigFileChangeNotifier;
	ScopedPtr<FileChangeNotifier> m_pSourceFileChangeNotifier;
	Changes m_IncomingContentChanges;
	Atomic32Value<Bool> m_bShuttingDown;

	SEOUL_DISABLE_COPY(ChangeNotifierLocal);
}; // class Content::ChangeNotifierLocal

} // namespace Seoul::Content

#endif // include guard
