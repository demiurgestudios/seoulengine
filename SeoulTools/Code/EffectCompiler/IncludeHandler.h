/**
 * \file IncludeHandler.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef INCLUDE_HANDLER_H
#define INCLUDE_HANDLER_H

#include "D3DUtil.h"
#include "HashTable.h"
#include "Pair.h"

namespace Seoul
{

class IncludeHandler SEOUL_SEALED : public ID3DInclude
{
public:
	typedef Pair<void*, UInt32> Data;
	typedef HashTable<String, Data > FileData;
	typedef HashTable<void const*, String> FilePaths;

	IncludeHandler(const String& sInputFilename);
	virtual ~IncludeHandler();

	virtual HRESULT SEOUL_STD_CALL Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData, UINT* pBytes);
	virtual HRESULT SEOUL_STD_CALL Close(LPCVOID pData);

	/** @return All file data encountered while processing includes. */
	const FileData& GetFileData() const { return m_tFileData; }

private:
	String m_sInputFilename;
	FilePaths m_tFilePaths;
	FileData m_tFileData;

	SEOUL_DISABLE_COPY(IncludeHandler);
}; // class IncludeHandler

} // namespace Seoul

#endif // include guard
