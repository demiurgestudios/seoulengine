/**
 * \file IncludeHandler.cpp
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "DiskFileSystem.h"
#include "HashTable.h"
#include "IncludeHandler.h"
#include "Path.h"
#include "SeoulFile.h"

namespace Seoul
{

IncludeHandler::IncludeHandler(const String& sInputFilename)
	: m_sInputFilename(sInputFilename)
	, m_tFilePaths()
{
}

IncludeHandler::~IncludeHandler()
{
	for (FileData::Iterator i = m_tFileData.Begin();
		m_tFileData.End() != i;
		++i)
	{
		MemoryManager::Deallocate((void*)i->Second.First);
	}

	m_tFileData.Clear();
}

HRESULT IncludeHandler::Open(
	D3D_INCLUDE_TYPE IncludeType,
	LPCSTR pFileName,
	LPCVOID pParentData,
	LPCVOID* ppData,
	UINT* pBytes)
{
	// Store the filename we're trying to include.
	String sIncludeFilename = pFileName;

	// Start with the FX file being compiled as the initial parent
	// filename.
	String sParentFilename = m_sInputFilename;

	// If the Microsoft FX system has provided parent data for
	// the file being included, it means this is a nested include.
	// Update the parent filename to the one associated with the
	// parent data.
	if (pParentData)
	{
		SEOUL_VERIFY(m_tFilePaths.GetValue(pParentData, sParentFilename));
	}

	// If path is relative, resolve it relative to its parent's file path.
	// But not if we're opening the top-level file.
	if (sIncludeFilename != sParentFilename &&
		!Path::IsRooted(sIncludeFilename))
	{
		SEOUL_VERIFY(Path::CombineAndSimplify(
			Path::GetDirectoryName(sParentFilename),
			sIncludeFilename,
			sIncludeFilename));
	}

	Data data;

	// If the data was previously loaded, return that, otherwise
	// load the file.
	if (!m_tFileData.GetValue(sIncludeFilename, data))
	{
		if (!DiskSyncFile::ReadAll(
			sIncludeFilename,
			data.First,
			data.Second,
			0u,
			MemoryBudgets::TBD))
		{
			return E_FAIL;
		}

		// Add the file as a potential parent to the parent file path table and
		// add the file data.
		m_tFilePaths.Insert(data.First, sIncludeFilename);
		m_tFileData.Insert(sIncludeFilename, data);
	}

	*ppData = data.First;
	*pBytes = data.Second;

	return S_OK;
}

HRESULT IncludeHandler::Close(LPCVOID pData)
{
	return S_OK;
}

} // namespace Seoul
