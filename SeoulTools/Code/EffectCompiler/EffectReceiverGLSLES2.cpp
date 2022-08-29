/**
 * \file EffectReceiverGLSLES2.cpp
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EffectReceiverGLSLES2.h"
#include "EndianSwap.h"
#include "ShaderReceiverGLSLES2.h"

namespace Seoul
{

using namespace EffectConverter;

inline static UInt32 HandleToOffset(GLSLFXLiteHandle h)
{
	return (h - 1u);
}

inline static Bool IsValid(GLSLFXLiteHandle h)
{
	return (0u != h);
}

template <typename T>
inline void AdvanceGLSLFXDataSizeInPreparationForSavingToDisk(
	UInt32& rzSize,
	const Vector<T>& vSource,
	UInt32 zAlignment)
{
	if (!vSource.IsEmpty())
	{
		// Align the size for the data type
		rzSize = (rzSize + zAlignment - 1) & ~(zAlignment - 1);

		// Advance the size by the size of the source buffer.
		rzSize += vSource.GetSizeInBytes();
	}
}

template <typename T>
inline void AdvanceGLSLFXDataSizeInPreparationForSavingToDisk(
	UInt32& rzSize,
	const Vector<T>& vSource)
{
	AdvanceGLSLFXDataSizeInPreparationForSavingToDisk<T>(rzSize, vSource, SEOUL_ALIGN_OF(T));
}


template <typename T>
inline void InitializeGLSLFXDataMemberForSavingToDisk(
	Bool bBigEndianOutput,
	Byte const* pBase,
	UInt32& ru,
	Byte*& rpOut,
	const Vector<T>& vSource,
	Byte const* pEnd,
	size_t zAlignment)
{
	// If there is data, copy the data and setup the pointers.
	if (!vSource.IsEmpty())
	{
		// Align the address for the data type.
		T* p = reinterpret_cast<T*>(
			(((size_t)rpOut) + zAlignment - 1) & ~(zAlignment - 1));

		// Sanity check that we're calculating our buffers correctly.
		SEOUL_ASSERT(reinterpret_cast<Byte*>(p) + vSource.GetSizeInBytes() <= pEnd);

		// Copy data from the source Vector<> buffer to the output area.
		memcpy(p, vSource.Get(0u), vSource.GetSizeInBytes());

		// Endian swap the data for serialization.
		if (bBigEndianOutput)
		{
			EndianSwap(p, vSource.GetSize());
		}

		// Advance the running output address - we need to use rp when
		// advancing due to the alignment step above.
		rpOut = reinterpret_cast<Byte*>(p) + vSource.GetSizeInBytes();

		// Output a relative offset from the head
		// of the Effect data - this will be fixed up at runtime.
		ru = (UInt32)((size_t)p - (size_t)pBase);

		// Endian swap the relative offset.
		if (bBigEndianOutput)
		{
			ru = EndianSwap32(ru);
		}
	}
	// Otherwise, the output pointer remains the 0 offset.
	else
	{
		ru = 0u;
	}
}

template <typename T>
inline void InitializeGLSLFXDataMemberForSavingToDisk(
	Bool bBigEndianOutput,
	Byte const* pBase,
	UInt32& ru,
	Byte*& rpOut,
	const Vector<T>& vSource,
	Byte const* pEnd)
{
	InitializeGLSLFXDataMemberForSavingToDisk<T>(
		bBigEndianOutput,
		pBase,
		ru,
		rpOut,
		vSource,
		pEnd,
		SEOUL_ALIGN_OF(T));
}

EffectReceiverGLSLES2::EffectReceiverGLSLES2()
{
}

EffectReceiverGLSLES2::~EffectReceiverGLSLES2()
{
}

Bool EffectReceiverGLSLES2::AddParameter(
	const Converter& effectConverter,
	const Util::Parameter& parameter)
{
	// Skip parameters that are not in use.
	if (!parameter.m_bInUse)
	{
		return true;
	}

	// Skip texture type parameters - we only care about sampler parameters.
	if (Util::ParameterType::kTexture == parameter.m_eType ||
		Util::ParameterType::kTexture1D == parameter.m_eType ||
		Util::ParameterType::kTexture2D == parameter.m_eType ||
		Util::ParameterType::kTexture3D == parameter.m_eType ||
		Util::ParameterType::kTextureCube == parameter.m_eType)
	{
		return true;
	}

	// Attempt to convert the parameter description to a GLSLFXLiteParameterDescription.
	// This may also add a string to m_vStrings to store the parameter Semantic.
	GLSLFXLiteParameterDescription glslfxLiteParameterDescription;
	memset(&glslfxLiteParameterDescription, 0, sizeof(glslfxLiteParameterDescription));
	if (!Convert(parameter, m_vStrings, glslfxLiteParameterDescription))
	{
		return false;
	}

	// Add the parameter description to the list of parameters.
	m_vParameters.PushBack(glslfxLiteParameterDescription);

	// Calculate the number of sizeof(GLSLFXLiteParameterData)-byte slots that will be needed
	// for the parameter's data - add 1 slot if the size is not a even
	// multiple of sizeof(GLSLFXLiteParameterData).
	UInt32 const nDataEntries =
		(glslfxLiteParameterDescription.m_uSize / sizeof(GLSLFXLiteParameterData)) +
		((0u != (glslfxLiteParameterDescription.m_uSize % sizeof(GLSLFXLiteParameterData))) ? 1u : 0u);

	// Insert the parameter data entry.
	GLSLFXLiteGlobalParameterEntry entry;
	memset(&entry, 0, sizeof(entry));

	entry.m_uCount = nDataEntries;
	entry.m_uIndex = m_vParameterData.GetSize();
	entry.m_uDirtyStamp = 1u; // Set to 1u to commit initial default values to program parameters.
	m_vParameterEntries.PushBack(entry);

	// Add space for the data and set its default value.
	m_vParameterData.Resize(m_vParameterData.GetSize() + nDataEntries);

	// Initialize the data to 0 for sanity sake, although all parameters should
	// be explicitly initialized.
	memset(m_vParameterData.Get(entry.m_uIndex), 0, glslfxLiteParameterDescription.m_uSize);

	// Get the default value for the parameter - must always be defined for parameters that
	// are in-use.
	if (!parameter.GetDefaultValue(
		m_vParameterData.Get(entry.m_uIndex),
		glslfxLiteParameterDescription.m_uSize))
	{
		return false;
	}

	return true;
}


Bool EffectReceiverGLSLES2::AddTechnique(
	const Converter& effectConverter,
	const Util::Technique& technique)
{
	// Convert the basic technique description to a GLSLFXLiteTechniqueDescription.
	GLSLFXLiteTechniqueDescription glslfxLiteTechniqueDescription;
	memset(&glslfxLiteTechniqueDescription, 0, sizeof(glslfxLiteTechniqueDescription));
	Convert(technique, m_vStrings, glslfxLiteTechniqueDescription);
	m_vTechniques.PushBack(glslfxLiteTechniqueDescription);

	// Setup the technique entry - this stores handles to the techniques
	// pass data.
	GLSLFXLiteTechniqueEntry entry;
	if (glslfxLiteTechniqueDescription.m_uPasses > 0u)
	{
		entry.m_hFirstPass = (m_vPasses.GetSize() + 1u);
		entry.m_hLastPass = entry.m_hFirstPass + glslfxLiteTechniqueDescription.m_uPasses - 1u;
	}
	else
	{
		entry.m_hFirstPass = 0u;
		entry.m_hLastPass = 0u;
	}
	m_vTechniqueEntries.PushBack(entry);

	// Process each pass of the technique.
	UInt32 const uPasses = technique.m_vPasses.GetSize();
	for (UInt32 iPass = 0u; iPass < uPasses; ++iPass)
	{
		const auto& pass = technique.m_vPasses[iPass];

		// Convert the basic pass description to a GLSLFXLitePassDescription.
		GLSLFXLitePassDescription glslfxLitePassDescription;
		memset(&glslfxLitePassDescription, 0, sizeof(glslfxLitePassDescription));
		Convert(pass, m_vStrings, glslfxLitePassDescription);
		m_vPasses.PushBack(glslfxLitePassDescription);

		// Setup the pass entry
		GLSLFXLitePassEntry passEntry;
		memset(&passEntry, 0, sizeof(passEntry));

		// Add the captured render states.
		InternalAddRenderStates(pass, passEntry);

		Bool bFoundPixelShader = false;
		String sPixelShaderSource;
		ConstantRegisterLookupTable tPixelShaderConstantRegisterLookupTable;
		Bool bFoundVertexShader = false;
		String sVertexShaderSource;
		ConstantRegisterLookupTable tVertexShaderConstantRegisterLookupTable;
		for (UInt32 i = 0u; i < pass.m_vShaders.GetSize(); ++i)
		{
			Util::Shader const* pShader = pass.m_vShaders.Get(i);

			// Shader is the vertex shader for the pass.
			if (pShader->m_eType == ShaderType::kVertex)
			{
				// If we already have a vertex shader, something terrible has happened.
				if (bFoundVertexShader)
				{
					fprintf(stderr, "More than one vertex shader associated with pass %d of technique %s.\n", iPass, technique.m_Name.CStr());
					return false;
				}

				// Compile and associated the shader.
				if (!InternalCompileShader(
					effectConverter,
					*pShader,
					true,
					passEntry,
					sVertexShaderSource,
					tVertexShaderConstantRegisterLookupTable))
				{
					fprintf(stderr, "Failed compiling vertex shader pass %d of technique %s.\n", iPass, technique.m_Name.CStr());
					return false;
				}

				bFoundVertexShader = true;
			}
			// Shader is the pixel shader for the pass.
			else if (pShader->m_eType == ShaderType::kPixel)
			{
				// If we already have a pixel shader, something terrible has happened.
				if (bFoundPixelShader)
				{
					fprintf(stderr, "More than one pixel shader associated with pass %d of technique %s.\n", iPass, technique.m_Name.CStr());
					return false;
				}

				// Compile and associate the shader.
				if (!InternalCompileShader(
					effectConverter,
					*pShader,
					false,
					passEntry,
					sPixelShaderSource,
					tPixelShaderConstantRegisterLookupTable))
				{
					fprintf(stderr, "Failed compiling pixel shader pass %d of technique %s.\n", iPass, technique.m_Name.CStr());
					return false;
				}

				bFoundPixelShader = true;
			}
		}

		// Validate pixel and vertex shaders if we have either.
		if (bFoundPixelShader || bFoundVertexShader)
		{
			if (!m_Validator.Validate(sPixelShaderSource, sVertexShaderSource))
			{
				fprintf(stderr, "Failed validating pixel and vertex shaders for pass %d of technique %s.\n", iPass, technique.m_Name.CStr());
				return false;
			}
		}

		// Hook up lookup names and hardware indictes for program parameters (uniform constants).
		if (!InternalSetupProgramParameters(
			tPixelShaderConstantRegisterLookupTable,
			tVertexShaderConstantRegisterLookupTable,
			passEntry))
		{
			fprintf(stderr, "Failed setting up parameters for pass %d of technique %s.\n", iPass, technique.m_Name.CStr());
			return false;
		}

		// Add the pass entry.
		m_vPassEntries.PushBack(passEntry);
	}

	return true;
}

UInt8* EffectReceiverGLSLES2::GetSerializeableData(Bool bBigEndianOutput, UInt32& rzDataSize) const
{
	const UInt32 zTotalDataSize = InternalCalculateSerializeableDataSize();
	UInt8* pReturn = (UInt8*)MemoryManager::AllocateAligned(
		zTotalDataSize,
		MemoryBudgets::Rendering,
		SEOUL_ALIGN_OF(GLSLFXLiteDataSerialized));
	memset(pReturn, 0, zTotalDataSize);

	// Effect description
	// Sanity check - make sure this conversion function is in-sync with
	// GLSLFXLiteEffectDescription structure.
	SEOUL_STATIC_ASSERT(sizeof(GLSLFXLiteEffectDescription) == 16u);

	GLSLFXLiteEffectDescription glslfxLiteEffectDescription;
	glslfxLiteEffectDescription.m_uParameters = m_vParameters.GetSize();
	glslfxLiteEffectDescription.m_uPasses = m_vPasses.GetSize();
	glslfxLiteEffectDescription.m_uShaders = m_vShaderEntries.GetSize();
	glslfxLiteEffectDescription.m_uTechniques = m_vTechniques.GetSize();

	GLSLFXLiteDataSerialized& rData = *reinterpret_cast<GLSLFXLiteDataSerialized*>(pReturn);
	memcpy(&(rData.m_Description), &glslfxLiteEffectDescription, sizeof(glslfxLiteEffectDescription));
	if (bBigEndianOutput)
	{
		rData.m_Description.m_uShaders = EndianSwap32(rData.m_Description.m_uShaders);
		rData.m_Description.m_uParameters = EndianSwap32(rData.m_Description.m_uParameters);
		rData.m_Description.m_uPasses = EndianSwap32(rData.m_Description.m_uPasses);
		rData.m_Description.m_uTechniques = EndianSwap32(rData.m_Description.m_uTechniques);
	}

	Byte const* pBase = (Byte*)pReturn;
	Byte const* pEnd = pBase + zTotalDataSize;
	Byte* pOut = (Byte*)pReturn + sizeof(rData);

	InitializeGLSLFXDataMemberForSavingToDisk(bBigEndianOutput, pBase, rData.m_uStrings, pOut, m_vStrings, pEnd);
	InitializeGLSLFXDataMemberForSavingToDisk(bBigEndianOutput, pBase, rData.m_uParameters, pOut, m_vParameters, pEnd);
	InitializeGLSLFXDataMemberForSavingToDisk(bBigEndianOutput, pBase, rData.m_uTechniques, pOut, m_vTechniques, pEnd);
	InitializeGLSLFXDataMemberForSavingToDisk(bBigEndianOutput, pBase, rData.m_uPasses, pOut, m_vPasses, pEnd);
	InitializeGLSLFXDataMemberForSavingToDisk(bBigEndianOutput, pBase, rData.m_uParameterData, pOut, m_vParameterData, pEnd);
	InitializeGLSLFXDataMemberForSavingToDisk(bBigEndianOutput, pBase, rData.m_uParameterEntries, pOut, m_vParameterEntries, pEnd);
	InitializeGLSLFXDataMemberForSavingToDisk(bBigEndianOutput, pBase, rData.m_uTechniqueEntries, pOut, m_vTechniqueEntries, pEnd);
	InitializeGLSLFXDataMemberForSavingToDisk(bBigEndianOutput, pBase, rData.m_uPassEntries, pOut, m_vPassEntries, pEnd);
	InitializeGLSLFXDataMemberForSavingToDisk(bBigEndianOutput, pBase, rData.m_uRenderStates, pOut, m_vRenderStates, pEnd);
	InitializeGLSLFXDataMemberForSavingToDisk(bBigEndianOutput, pBase, rData.m_uShaderEntries, pOut, m_vShaderEntries, pEnd);
	InitializeGLSLFXDataMemberForSavingToDisk(bBigEndianOutput, pBase, rData.m_uShaderCode, pOut, m_vShaderCode, pEnd, SEOUL_ALIGN_OF(GLSLFXLiteDataSerialized));
	InitializeGLSLFXDataMemberForSavingToDisk(bBigEndianOutput, pBase, rData.m_uProgramParameters, pOut, m_vProgramParameters, pEnd);

	// Final sanity check, make sure all the data was written as expected.
	SEOUL_ASSERT(pOut == pEnd);

	rzDataSize = zTotalDataSize;
	return pReturn;
}

UInt32 EffectReceiverGLSLES2::InternalCalculateSerializeableDataSize() const
{
	UInt32 zReturn = 0u;
	zReturn += sizeof(GLSLFXLiteDataSerialized);

	AdvanceGLSLFXDataSizeInPreparationForSavingToDisk(zReturn, m_vStrings);
	AdvanceGLSLFXDataSizeInPreparationForSavingToDisk(zReturn, m_vParameters);
	AdvanceGLSLFXDataSizeInPreparationForSavingToDisk(zReturn, m_vTechniques);
	AdvanceGLSLFXDataSizeInPreparationForSavingToDisk(zReturn, m_vPasses);
	AdvanceGLSLFXDataSizeInPreparationForSavingToDisk(zReturn, m_vParameterData);
	AdvanceGLSLFXDataSizeInPreparationForSavingToDisk(zReturn, m_vParameterEntries);
	AdvanceGLSLFXDataSizeInPreparationForSavingToDisk(zReturn, m_vTechniqueEntries);
	AdvanceGLSLFXDataSizeInPreparationForSavingToDisk(zReturn, m_vPassEntries);
	AdvanceGLSLFXDataSizeInPreparationForSavingToDisk(zReturn, m_vRenderStates);
	AdvanceGLSLFXDataSizeInPreparationForSavingToDisk(zReturn, m_vShaderEntries);
	AdvanceGLSLFXDataSizeInPreparationForSavingToDisk(zReturn, m_vShaderCode, SEOUL_ALIGN_OF(GLSLFXLiteDataSerialized));
	AdvanceGLSLFXDataSizeInPreparationForSavingToDisk(zReturn, m_vProgramParameters);

	return zReturn;
}

Bool EffectReceiverGLSLES2::InternalSetupProgramParameters(
	const ConstantRegisterLookupTable& tPixelShaderLookup,
	const ConstantRegisterLookupTable& tVertexShaderLookup,
	GLSLFXLitePassEntry& rPassEntry)
{
	GLSLFXLiteHandle hParameterFirst = (m_vProgramParameters.GetSize() + 1u);

	UInt32 const nParameters = m_vParameters.GetSize();
	for (UInt32 i = 0u; i < nParameters; ++i)
	{
		const UInt16 nGlobalParameterIndex = i;
		const GLSLFXLiteParameterDescription& parameterDescription =
			m_vParameters[nGlobalParameterIndex];

		// Check for a prameter name - we can't associate a parameter with
		// no name, so skip it if it doesn't have one.
		if (parameterDescription.m_hName == 0u)
		{
			continue;
		}

		// Get the parameter name.
		HString const parameterName(m_vStrings.Get(HandleToOffset(parameterDescription.m_hName)));

		// Get register info - if not defined, this parameter is not used for this pass,
		// so don't insert it.
		Int32 iConstantRegister = -1;
		if (tPixelShaderLookup.GetValue(parameterName, iConstantRegister))
		{
			// Check for a register mismatch between the pixel and vertex shaders
			// if a parameter is shared.
			Int32 iCheck = 0;
			if (tVertexShaderLookup.GetValue(parameterName, iCheck) &&
				iCheck != iConstantRegister)
			{
				fprintf(stderr, "Hardware register for sampler parameter \"%s\" is %d in "
					"the pixel shader but %d in the vertex shader, not compatible.",
					parameterName.CStr(),
					iConstantRegister,
					iCheck);
				return false;
			}
		}
		else if (tVertexShaderLookup.GetValue(parameterName, iConstantRegister))
		{
			// Check for a register mismatch between the pixel and vertex shaders
			// if a parameter is shared.
			Int32 iCheck = 0;
			if (tPixelShaderLookup.GetValue(parameterName, iCheck) &&
				iCheck != iConstantRegister)
			{
				fprintf(stderr, "Hardware register for sampler parameter \"%s\" is %d in "
					"the vertex shader but %d in the pixel shader, not compatible.",
					parameterName.CStr(),
					iConstantRegister,
					iCheck);
				return false;
			}
		}
		else
		{
			// Parameter is not used by this pass, so skip it.
			continue;
		}

		// Cache the global parameter info.
		const GLSLFXLiteGlobalParameterEntry& globalParameter =
			m_vParameterEntries[nGlobalParameterIndex];

		// Index and count of the entire parameter - may be subdivided further
		// depending on the type.
		UInt16 const nBaseParameterIndex = globalParameter.m_uIndex;
		UInt16 const nTotalParameterCount = globalParameter.m_uCount;

		// Insert the parameter.
		GLSLFXLiteProgramParameter parameter;
		memset(&parameter, 0, sizeof(parameter));
		parameter.m_uGlobalParameterIndex = nGlobalParameterIndex;
		parameter.m_uParameterIndex = nBaseParameterIndex;
		parameter.m_uParameterCount = nTotalParameterCount;
		parameter.m_uParameterClass = parameterDescription.m_Class;
		parameter.m_uDirtyStamp = 0u;
		parameter.m_hParameterLookupName = parameterDescription.m_hName;

		if (GLSLFX_PARAMETERCLASS_SAMPLER == parameter.m_uParameterClass)
		{
			// For sampler parameters, we must specify the sampler index at cook time.
			parameter.m_iHardwareIndex = iConstantRegister;
		}
		else
		{
			// Default hardware index of -1, this means it will be assigned at runtime.
			parameter.m_iHardwareIndex = -1;
		}

		m_vProgramParameters.PushBack(parameter);
	}

	GLSLFXLiteHandle hParameterLast = (m_vProgramParameters.GetSize());

	// If hParameterLast is less than hParameterFirst, the shader has
	// no parameters.
	if (hParameterLast < hParameterFirst)
	{
		rPassEntry.m_hParameterFirst = 0u;
		rPassEntry.m_hParameterLast = 0u;
	}
	else
	{
		rPassEntry.m_hParameterFirst = hParameterFirst;
		rPassEntry.m_hParameterLast = hParameterLast;
	}

	return true;
}

void EffectReceiverGLSLES2::InternalAddRenderStates(
	const Util::Pass& pass,
	GLSLFXLitePassEntry& rPassEntry)
{
	UInt32 const uStateCount = pass.m_vRenderStates.GetSize();

	if (uStateCount > 0u)
	{
		rPassEntry.m_hFirstRenderState = (m_vRenderStates.GetSize() + 1u);
		rPassEntry.m_hLastRenderState = rPassEntry.m_hFirstRenderState + uStateCount - 1u;
	}
	else
	{
		rPassEntry.m_hFirstRenderState = 0u;
		rPassEntry.m_hLastRenderState = 0u;
	}

	for (UInt32 iState = 0u; iState < uStateCount; ++iState)
	{
		m_vRenderStates.PushBack(Convert(pass.m_vRenderStates[iState]));
	}
}

Bool EffectReceiverGLSLES2::InternalCompileShader(
	const Converter& effectConverter,
	const Util::Shader& shader,
	Bool bVertexShader,
	GLSLFXLitePassEntry& rPassEntry,
	String& rsShaderSource,
	ConstantRegisterLookupTable& rtConstantRegisterLookupTable)
{
	GLSLFXLiteHandle& rhShaderHandle = ((bVertexShader)
		? rPassEntry.m_hVertexShader
		: rPassEntry.m_hPixelShader);

	rhShaderHandle = 0u;

	// Generate the shader code.
	ShaderReceiverGLSLES2 shaderReceiver(effectConverter);
	if (!shader.Convert(shaderReceiver))
	{
		fprintf(stderr, "Failed converting shader to target language.\n");
		return false;
	}
	rsShaderSource = shaderReceiver.GetCode();
	rtConstantRegisterLookupTable = shaderReceiver.GetConstantRegisterLookupTable();

	// Now look for an existing shader with the same code and reuse it if possible.
	{
		UInt32 const uShaderEntries = m_vShaderEntries.GetSize();
		for (UInt32 i = 0u; i < uShaderEntries; ++i)
		{
			const GLSLFXLiteShaderEntry& entry = m_vShaderEntries[i];
			if (entry.m_hShaderCodeFirst == 0u)
			{
				continue;
			}

			if (0 == memcmp(rsShaderSource.CStr(), m_vShaderCode.Get(HandleToOffset(entry.m_hShaderCodeFirst)), rsShaderSource.GetSize()))
			{
				rhShaderHandle = (i + 1u);
				return true;
			}
		}
	}

	// This is a new shader, add it.
	UInt32 const zSizeInBytes = rsShaderSource.GetSize() + 1u;

	// If we have data, finalize it.
	if (zSizeInBytes > 0u)
	{
		GLSLFXLiteShaderEntry entry;
		memset(&entry, 0, sizeof(entry));

		entry.m_bIsVertexShader = ((bVertexShader) ? 1u : 0u);

		// Align the size to make sure constants and addresses inside
		// the shader code are aligned correctly.
		UInt32 const zAlignedSizeInBytes =
			(zSizeInBytes + SEOUL_ALIGN_OF(GLSLFXLiteDataSerialized) - 1u) & ~(SEOUL_ALIGN_OF(GLSLFXLiteDataSerialized) - 1u);

		entry.m_hShaderCodeFirst = (m_vShaderCode.GetSize() + 1u);

		// Note that the aligned size is padding - the recorded code
		// size (first to last indices) must match the actual size.
		entry.m_hShaderCodeLast = entry.m_hShaderCodeFirst + zSizeInBytes - 1u;

		m_vShaderCode.Resize(m_vShaderCode.GetSize() + zAlignedSizeInBytes);
		memcpy(m_vShaderCode.Get(HandleToOffset(entry.m_hShaderCodeFirst)), rsShaderSource.CStr(), zSizeInBytes);

		rhShaderHandle = (m_vShaderEntries.GetSize() + 1u);
		m_vShaderEntries.PushBack(entry);

		return true;
	}
	else
	{
		fprintf(stderr, "Compilation succeeded but output shader is zero bytes.\n");
		return false;
	}
}

} // namespace Seoul
