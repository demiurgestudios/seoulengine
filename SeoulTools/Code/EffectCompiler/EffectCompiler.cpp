/**
 * \file EffectCompiler.cpp
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EffectConverter.h"
#include "EffectReceiverGLSLES2.h"
#include "FileManager.h"
#include "EffectCompiler.h"
#include "IncludeHandler.h"
#include "Logger.h"
#include "Path.h"
#include "ScopedAction.h"

namespace Seoul
{

using namespace EffectConverter;

typedef Vector<D3D_SHADER_MACRO> Macros;

static Bool ProcessToGLSLES2(
	UInt8 const* pInBytecode,
	UInt32 zInBytecodeSizeInBytes,
	Bool bBigEndianOutput,
	void*& rpData,
	UInt32& ruDataSizeInBytes)
{
	EffectReceiverGLSLES2 receiver;

	{
		Converter converter;
		if (!converter.ProcessBytecode(pInBytecode, zInBytecodeSizeInBytes))
		{
			return false;
		}

		if (!converter.ConvertTo(receiver))
		{
			return false;
		}
	}

	rpData = receiver.GetSerializeableData(bBigEndianOutput, ruDataSizeInBytes);
	return (nullptr != rpData);
}

static Macros ConvertMacros(const MacroTable& tMacros)
{
	// Capacity.
	Macros vMacros;
	vMacros.Reserve(tMacros.GetSize());

	// Conversion.
	for (auto const& pair : tMacros)
	{
		D3D_SHADER_MACRO macro;
		macro.Name = pair.First.CStr();
		macro.Definition = pair.Second.CStr();
		vMacros.PushBack(macro);
	}

	// Null terminate the macro array
	{
		D3D_SHADER_MACRO nullTerminator;
		memset(&nullTerminator, 0, sizeof(nullTerminator));
		vMacros.PushBack(nullTerminator);
	}

	return vMacros;
}

Bool CompileEffectFile(
	EffectTarget eTarget,
	FilePath filePath,
	const MacroTable& tMacros,
	void*& rp,
	UInt32& ru)
{
	// Resolve.
	auto const sInputFilename = filePath.GetAbsoluteFilenameInSource();
	// Macro vector.
	auto const vMacros = ConvertMacros(tMacros);

	// State.
	ScopedPtr<Converter> pEffectConverter;
	IncludeHandler includeHandler(sInputFilename);
	HRESULT hr = S_OK;
	ID3DBlob* pEffectBytecode = nullptr;
	ID3DBlob* pErrors = nullptr;
	Bool bReturn = true;

	// Compile the effect using the D3DCompiler.
	if (!SUCCEEDED(hr = D3DCompileFromFile(
		sInputFilename.WStr(),
		vMacros.Data(),
		&includeHandler,
		nullptr,
		(EffectTarget::D3D11 == eTarget ? "fx_5_0" : "fx_2_0"),
		D3DCOMPILE_ENABLE_STRICTNESS |
		D3DCOMPILE_NO_PRESHADER |
		D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR |
		D3DCOMPILE_OPTIMIZATION_LEVEL3 |
		D3DCOMPILE_WARNINGS_ARE_ERRORS,
		0u,
		&pEffectBytecode,
		&pErrors)))
	{
		SEOUL_LOG_COOKING("%s: failed loading '%s' (error 0x%08x): \"%s\"",
			filePath.CStr(),
			sInputFilename.CStr(),
			hr,
			(pErrors ? (char*) pErrors->GetBufferPointer() : "Unknown Error"));
		goto error;
	}
	SafeRelease(pErrors);

	// Ready to process - process the file, if successful, write the data
	// to disk.
	if (EffectTarget::GLSLES2 == eTarget)
	{
		if (!ProcessToGLSLES2(
			(UInt8 const*)pEffectBytecode->GetBufferPointer(),
			(UInt32)pEffectBytecode->GetBufferSize(),
			false, /* TODO: bBigEndian. */
			rp,
			ru))
		{
			SEOUL_LOG_COOKING("%s: failed converting the Microsoft FX to a GLSLFXLite.", filePath.CStr());
			goto error;
		}
	}
	else
	{
		ru = (UInt32)pEffectBytecode->GetBufferSize();
		rp = MemoryManager::Allocate(ru, MemoryBudgets::Cooking);
		memcpy(rp, pEffectBytecode->GetBufferPointer(), ru);
	}

done:
	SafeRelease(pErrors);
	SafeRelease(pEffectBytecode);
	pEffectConverter.Reset();
	return bReturn;

error:
	bReturn = false;
	goto done;
}

Bool GetEffectFileDependencies(
	FilePath filePath,
	const MacroTable& tMacros,
	EffectFileDependencies& rSet)
{
	// Resolve.
	auto const sInputFilename = filePath.GetAbsoluteFilenameInSource();
	// Macro vector.
	auto const vMacros = ConvertMacros(tMacros);

	// State.
	IncludeHandler includeHandler(sInputFilename);
	HRESULT hr = S_OK;
	ID3DBlob* pOutput = nullptr;
	ID3DBlob* pErrors = nullptr;

	// Read input.
	void* pInput = nullptr;
	UInt32 uInput = 0u;
	if (!FileManager::Get()->ReadAll(sInputFilename, pInput, uInput, 0u, MemoryBudgets::Cooking))
	{
		SEOUL_LOG_COOKING("%s: failed reading to gather dependencies.", filePath.CStr());
		return false;
	}
	auto const deferred(MakeDeferredAction([&]() { MemoryManager::Deallocate(pInput); }));

	// Compile the effect using the D3DCompiler.
	auto const bReturn = (SUCCEEDED(hr = D3DPreprocess(
		pInput,
		uInput,
		Path::GetFileName(sInputFilename).CStr(),
		vMacros.Data(),
		&includeHandler,
		&pOutput,
		&pErrors)));

	// Reporting.
	if (!bReturn)
	{
		SEOUL_LOG_COOKING("%s: failed loading %s (error 0x%08x): \"%s\"",
			filePath.CStr(),
			hr,
			(pErrors ? (char*) pErrors->GetBufferPointer() : "Unknown Error"));
	}
	SafeRelease(pErrors);
	SafeRelease(pOutput);

	// Early out no failure.
	if (!bReturn)
	{
		return bReturn;
	}

	// Assemble.
	EffectFileDependencies set;
	// Base file.
	set.Insert(filePath);
	// Deps.
	for (auto const& pair : includeHandler.GetFileData())
	{
		set.Insert(FilePath::CreateContentFilePath(pair.First));
	}

	// Done.
	rSet.Swap(set);
	return true;
}

} // namespace Seoul
