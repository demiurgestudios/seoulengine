/**
 * \file GLSLFXLiteInternal.h
 * \brief Internal header implementing most methods of GLSLFXLite. Must
 * only be included by GLSLFXLite.h.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GLSLFX_LITE_INTERNAL_H
#define GLSLFX_LITE_INTERNAL_H

#include "GLSLFXLite.h"
#include "Logger.h"
#include "OGLES2RenderDevice.h"
#include "OGLES2StateManager.h"
#include "OGLES2Texture.h"
#include "OGLES2Util.h"
#include "UnsafeHandle.h"

namespace Seoul
{

/** OGLES2 attribute mappings - keep in sync with the values returned by GetVertexDataIndex() */
static Byte const* kaVertexAttribBindingNames[] =
{
	"seoul_attribute_Vertex",
	"seoul_attribute_Normal",
	"seoul_attribute_Color",
	"seoul_attribute_SecondaryColor",
	"seoul_attribute_MultiTexCoord0",
	"seoul_attribute_MultiTexCoord1",
	"seoul_attribute_MultiTexCoord2",
	"seoul_attribute_MultiTexCoord3",
	"seoul_attribute_MultiTexCoord4",
	"seoul_attribute_MultiTexCoord5",
	"seoul_attribute_MultiTexCoord6",
	"seoul_attribute_MultiTexCoord7",
};

using namespace GLSLFXLiteUtil;

inline GLSLFXLiteRuntimeShaderData::GLSLFXLiteRuntimeShaderData(
	Bool bVertexShader,
	const char* sShaderSource,
	Int zSourceLength)
	: m_Object(glCreateShader(bVertexShader ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER))
{
	if (0 != m_Object)
	{
		glShaderSource(m_Object, 1, &sShaderSource, &zSourceLength);
		glCompileShader(m_Object);

		GLint bSuccess = GL_FALSE;
		glGetShaderiv(m_Object, GL_COMPILE_STATUS, &bSuccess);
		if (GL_TRUE != bSuccess)
		{
			GLint nLogLength = 0;
			glGetShaderiv(m_Object, GL_INFO_LOG_LENGTH, &nLogLength);
			GLchar* sBuffer = (GLchar*)MemoryManager::Allocate(nLogLength + 1, MemoryBudgets::Rendering);
			glGetShaderInfoLog(m_Object, nLogLength + 1, nullptr, sBuffer);

			// Always null terminate - some platforms do not write out the null
			// terminator if the log length is 0.
			sBuffer[nLogLength] = '\0';

			SEOUL_WARN("Failed compiling %s shader, see log for more details, error \"%s\".\n",
				(bVertexShader ? "vertex" : "fragment"),
				sBuffer);

			SEOUL_LOG("Shader Source:");
			SEOUL_LOG("%s", sShaderSource);

			MemoryManager::Deallocate(sBuffer);

			glDeleteShader(m_Object);
			m_Object = 0;
		}
	}
}

inline GLSLFXLiteRuntimeShaderData::~GLSLFXLiteRuntimeShaderData()
{
	if (0 != m_Object)
	{
		glDeleteShader(m_Object);
		m_Object = 0;
	}
}

inline GLSLFXLite::GLSLFXLite(
	FilePath filePath,
	void* pRawEffectFileData,
	UInt32 zFileSizeInBytes)
	: m_hActivePass()
	, m_hActiveTechnique()
	, m_hPreviousPixelShader(0u)
	, m_hPreviousVertexShader(0u)
	, m_Data()
	, m_pDataSerialized(nullptr)
	, m_pShaderData(nullptr)
	, m_pTextureReferences(nullptr)
	, m_pSecondaryTextureData(nullptr)
{
	// Copy the serialized data.
	m_pDataSerialized = (GLSLFXLiteDataSerialized*)MemoryManager::AllocateAligned(
		zFileSizeInBytes,
		MemoryBudgets::Rendering,
		SEOUL_ALIGN_OF(GLSLFXLiteDataSerialized));
	memcpy(m_pDataSerialized, pRawEffectFileData, zFileSizeInBytes);

	// Process the serialized data.
	SetupSerializedData(m_pDataSerialized, m_Data);

	// Finish setup of other runtime state.
	m_pTextureReferences = (BaseTexture**)MemoryManager::AllocateAligned(
		m_Data.m_Description.m_uParameters * sizeof(BaseTexture*),
		MemoryBudgets::Rendering,
		SEOUL_ALIGN_OF(BaseTexture*));
	memset(m_pTextureReferences, 0, m_Data.m_Description.m_uParameters * sizeof(BaseTexture*));

	m_pSecondaryTextureData = (GLint*)MemoryManager::AllocateAligned(
		m_Data.m_Description.m_uParameters * sizeof(GLint),
		MemoryBudgets::Rendering,
		SEOUL_ALIGN_OF(GLint));
	for (UInt32 i = 0u; i < m_Data.m_Description.m_uParameters; ++i)
	{
		m_pSecondaryTextureData[i] = -1;
	}

	// If the effect has a secondary texture defined, cache the parameter
	// index for use when setting samplers.
	for (UInt32 i = 0u; i < m_Data.m_Description.m_uParameters; ++i)
	{
		const GLSLFXLiteParameterDescription& mainEntry = m_Data.m_pParameters[i];
		if (mainEntry.m_Class == GLSLFX_PARAMETERCLASS_SAMPLER)
		{
			String sEntryName(GetString(mainEntry.m_hName));
			for (UInt32 j = 0u; j < m_Data.m_Description.m_uParameters; ++j)
			{
				const GLSLFXLiteParameterDescription& secondaryEntry = m_Data.m_pParameters[j];
				if (secondaryEntry.m_Class == GLSLFX_PARAMETERCLASS_SAMPLER)
				{
					String sSecondaryEntryName(GetString(secondaryEntry.m_hName));
					if ((sEntryName + "_Secondary") == sSecondaryEntryName)
					{
						m_pSecondaryTextureData[i] = (Int32)j;
						break;
					}
				}
			}
		}
	}

	m_pShaderData = (GLSLFXLiteRuntimeShaderData*)MemoryManager::AllocateAligned(
		sizeof(GLSLFXLiteRuntimeShaderData) * m_Data.m_Description.m_uShaders,
		MemoryBudgets::Rendering,
		SEOUL_ALIGN_OF(GLSLFXLiteRuntimeShaderData));

	for (UInt32 i = 0u; i < m_Data.m_Description.m_uShaders; ++i)
	{
		const GLSLFXLiteShaderEntry& shader = m_Data.m_pShaderEntries[i];
		const UInt32 zShaderSizeInBytes = (shader.m_hShaderCodeLast - shader.m_hShaderCodeFirst) + 1u;
		const Bool bVertexShader = (0u != shader.m_bIsVertexShader);

		const char* sShaderData = (m_Data.m_pShaderCode + HandleToOffset(shader.m_hShaderCodeFirst));
		new (m_pShaderData + i) GLSLFXLiteRuntimeShaderData(bVertexShader, sShaderData, zShaderSizeInBytes - 1u);
	}

#if !SEOUL_SHIP
	Bool bFailed = false;
#endif

	for (UInt32 i = 0u; i < m_Data.m_Description.m_uPasses; ++i)
	{
		GLSLFXLitePassEntry& pass = m_Data.m_pPassEntries[i];
		pass.m_Program = glCreateProgram();

		if (0 != pass.m_Program)
		{
			Bool bLink = false;

			if (IsValid(pass.m_hPixelShader))
			{
				UInt const nShaderOffset = HandleToOffset(pass.m_hPixelShader);
				GLSLFXLiteRuntimeShaderData const& shader = m_pShaderData[nShaderOffset];
				if (0 != shader.m_Object)
				{
					glAttachShader(pass.m_Program, shader.m_Object);
					bLink = true;
				}
			}

			if (IsValid(pass.m_hVertexShader))
			{
				UInt const nShaderOffset = HandleToOffset(pass.m_hVertexShader);
				GLSLFXLiteRuntimeShaderData const& shader = m_pShaderData[nShaderOffset];
				if (0 != shader.m_Object)
				{
					glAttachShader(pass.m_Program, shader.m_Object);
					bLink = true;
				}
			}

			if (bLink)
			{
				// Set up attribute mappings - equivalent to gl mapping. Values
				// must match the indices in GetVertexDataIndex().

				// Only setup to the maximum supported on the current device.
				GLint nMaxAttribs = 0;
				SEOUL_OGLES2_VERIFY(glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nMaxAttribs));

				GLint const nAttribs = Min(nMaxAttribs, (GLint)SEOUL_ARRAY_COUNT(kaVertexAttribBindingNames));
				for (GLint i = 0; i < nAttribs; ++i)
				{
					SEOUL_OGLES2_VERIFY(glBindAttribLocation(pass.m_Program, i, kaVertexAttribBindingNames[i]));
				}

				glLinkProgram(pass.m_Program);

				GLint bSuccess = GL_FALSE;
				glGetProgramiv(pass.m_Program, GL_LINK_STATUS, &bSuccess);

				if (GL_TRUE != bSuccess)
				{
					GLint  nLogLength = 0;
					glGetProgramiv(pass.m_Program, GL_INFO_LOG_LENGTH, &nLogLength);

					GLchar* sBuffer = (GLchar*)MemoryManager::Allocate(nLogLength + 1, MemoryBudgets::Rendering);
					glGetProgramInfoLog(pass.m_Program, nLogLength, nullptr, sBuffer);

					SEOUL_WARN("Failed linking/validating program, see log for more details, error \"%s\".\n",
						sBuffer);

					MemoryManager::Deallocate(sBuffer);

					glDeleteProgram(pass.m_Program);
					pass.m_Program = 0;
				}
			}
			else
			{
				glDeleteProgram(pass.m_Program);
				pass.m_Program = 0;
			}

			if (0 != pass.m_Program)
			{
				SEOUL_OGLES2_VERIFY(glUseProgram(pass.m_Program));
				InternalSetupHardwareIndices(pass);
				SEOUL_OGLES2_VERIFY(glUseProgram(0));
			}
		}

#if !SEOUL_SHIP
		bFailed = bFailed || (0 == pass.m_Program);
#endif
	}

#if !SEOUL_SHIP
	for (UInt32 i = 0u; i < m_Data.m_Description.m_uShaders; ++i)
	{
		if (bFailed || m_pShaderData[i].m_Object == 0)
		{
			SEOUL_LOG("Failed compiling shaders for shader Effect: %s",
				filePath.CStr());
			break;
		}
	}
#endif
}

inline GLSLFXLite::~GLSLFXLite()
{
	for (Int i = (Int)m_Data.m_Description.m_uPasses - 1; i >= 0; --i)
	{
		GLSLFXLitePassEntry& pass = m_Data.m_pPassEntries[i];
		if (0 != pass.m_Program)
		{
			SEOUL_OGLES2_VERIFY(glDeleteProgram(pass.m_Program));
			pass.m_Program = 0;
		}
	}

	for (Int i = (Int)m_Data.m_Description.m_uShaders - 1; i >= 0; --i)
	{
		m_pShaderData[i].~GLSLFXLiteRuntimeShaderData();
	}

	if (m_pShaderData)
	{
		MemoryManager::Deallocate(m_pShaderData);
		m_pShaderData = nullptr;
	}

	if (m_pSecondaryTextureData)
	{
		MemoryManager::Deallocate(m_pSecondaryTextureData);
		m_pSecondaryTextureData = nullptr;
	}

	if (m_pTextureReferences)
	{
		MemoryManager::Deallocate(m_pTextureReferences);
		m_pTextureReferences = nullptr;
	}

	if (m_pDataSerialized)
	{
		MemoryManager::Deallocate(m_pDataSerialized);
		m_pDataSerialized = nullptr;
	}
}

inline void GLSLFXLite::BeginPassFromIndex(UInt32 nPassIndex)
{
	OGLES2RenderDevice& rd = GetOGLES2RenderDevice();
	OGLES2StateManager& sm = rd.GetStateManager();

	UInt32 const nTechniqueIndex = (UInt32)(StaticCast<size_t>(m_hActiveTechnique) - 1u);
	SEOUL_ASSERT(nTechniqueIndex < m_Data.m_Description.m_uTechniques);

	GLSLFXLiteTechniqueEntry const& technique =
		m_Data.m_pTechniqueEntries[nTechniqueIndex];

	UInt32 const nFirstPassOffset = HandleToOffset(technique.m_hFirstPass);
	UInt32 const nPassOffset = (nFirstPassOffset + nPassIndex);

	SEOUL_ASSERT(
		nPassOffset >= nFirstPassOffset &&
		nPassOffset <= HandleToOffset(technique.m_hLastPass));
	m_hActivePass = (nPassOffset + 1u);

	GLSLFXLitePassEntry const& pass =
		m_Data.m_pPassEntries[nPassOffset];

	if (IsValid(pass.m_hFirstRenderState))
	{
		SEOUL_ASSERT(IsValid(pass.m_hLastRenderState));
		GLSLFXLiteRenderState const* const pFirstRenderState =
			(m_Data.m_pRenderStates + HandleToOffset(pass.m_hFirstRenderState));
		GLSLFXLiteRenderState const* const pLastRenderState =
			(m_Data.m_pRenderStates + HandleToOffset(pass.m_hLastRenderState));

		for (GLSLFXLiteRenderState const* p = pFirstRenderState; p <= pLastRenderState; ++p)
		{
			sm.SetRenderState((RenderState)p->m_uState, p->m_uValue);
		}
	}

	if (0u != pass.m_Program)
	{
		SEOUL_OGLES2_VERIFY(glUseProgram(pass.m_Program));
	}
}

inline void GLSLFXLite::BeginTechnique(UnsafeHandle hTechnique)
{
	m_hActiveTechnique = hTechnique;
	m_hPreviousPixelShader = 0u;
	m_hPreviousVertexShader = 0u;
}

inline void GLSLFXLite::Commit()
{
	UInt32 const nPassIndex = (UInt32)(StaticCast<size_t>(m_hActivePass) - 1u);
	SEOUL_ASSERT(nPassIndex < m_Data.m_Description.m_uPasses);

	GLSLFXLitePassEntry const& pass = m_Data.m_pPassEntries[nPassIndex];

	if (0 != pass.m_Program)
	{
		InternalCommitProgramParameters(pass);
	}
}

inline void GLSLFXLite::EndPass()
{
	// TODO: Disable all texture units that were bound and activated during this pass.

	m_hActivePass = UnsafeHandle();
}

inline void GLSLFXLite::EndTechnique()
{
	m_hActiveTechnique = UnsafeHandle();
}

inline void GLSLFXLite::GetEffectDescription(
	GLSLFXLiteEffectDescription* pDescription) const
{
	SEOUL_ASSERT(pDescription);

	memcpy(pDescription, &(m_Data.m_Description), sizeof(m_Data.m_Description));
}

inline void GLSLFXLite::GetParameterDescription(
	UnsafeHandle hParameter,
	GLSLFXLiteParameterDescription* pDescription) const
{
	SEOUL_ASSERT(pDescription);

	UInt32 const uParameterIndex = (UInt32)(StaticCast<size_t>(hParameter) - 1u);
	SEOUL_ASSERT(uParameterIndex < m_Data.m_Description.m_uParameters);

	memcpy(pDescription, &(m_Data.m_pParameters[uParameterIndex]), sizeof(*pDescription));
}

inline UnsafeHandle GLSLFXLite::GetParameterHandleFromIndex(
	UInt nParameterIndex) const
{
	return UnsafeHandle((size_t)(nParameterIndex + 1u));
}

inline Byte const* GLSLFXLite::GetString(GLSLFXLiteHandle hString) const
{
	return GLSLFXLiteUtil::GetString(m_Data.m_pStrings, hString);
}

inline void GLSLFXLite::GetTechniqueDescription(
	UnsafeHandle hTechnique,
	GLSLFXLiteTechniqueDescription* pDescription) const
{
	SEOUL_ASSERT(pDescription);

	UInt32 const uTechniqueIndex = (UInt32)(StaticCast<size_t>(hTechnique) - 1u);
	SEOUL_ASSERT(uTechniqueIndex < m_Data.m_Description.m_uTechniques);

	memcpy(pDescription, &(m_Data.m_pTechniques[uTechniqueIndex]), sizeof(*pDescription));
}

inline UnsafeHandle GLSLFXLite::GetTechniqueHandleFromIndex(
	UInt nTechniqueIndex) const
{
	return UnsafeHandle((size_t)(nTechniqueIndex + 1u));
}

inline void GLSLFXLite::OnLostDevice()
{
	// Nop
}

inline void GLSLFXLite::OnResetDevice()
{
	// Nop
}

inline void GLSLFXLite::InternalSetupHardwareIndices(const GLSLFXLitePassEntry& passEntry)
{
	if (IsValid(passEntry.m_hParameterFirst) &&
		IsValid(passEntry.m_hParameterLast))
	{
		UInt const nFirstParameter = HandleToOffset(passEntry.m_hParameterFirst);
		UInt const nLastParameter = HandleToOffset(passEntry.m_hParameterLast);

		for (UInt uParameter = nFirstParameter; uParameter <= nLastParameter; ++uParameter)
		{
			GLSLFXLiteProgramParameter& parameter = m_Data.m_pProgramParameters[uParameter];

			SEOUL_ASSERT(
				parameter.m_iHardwareIndex < 0 ||
				GLSLFX_PARAMETERCLASS_SAMPLER == parameter.m_uParameterClass);

			Byte const* sUniformName = GetString(parameter.m_hParameterLookupName);
			GLint uniformLocation = glGetUniformLocation(
				passEntry.m_Program,
				sUniformName);

			if (GLSLFX_PARAMETERCLASS_SAMPLER == parameter.m_uParameterClass)
			{
				if (parameter.m_iHardwareIndex >= 0)
				{
					SEOUL_OGLES2_VERIFY(glUniform1i(uniformLocation, parameter.m_iHardwareIndex));
				}
			}
			else
			{
				parameter.m_iHardwareIndex = uniformLocation;
			}
		}
	}
}

inline void GLSLFXLite::InternalStaticInlineSetValue(
	UnsafeHandle hParameter,
	UInt32 const nInputCount,
	void const* pData)
{
	static const UInt32 kMaximumComparisonCount = 8u;

	UInt32 const nParameterIndex = (UInt32)(StaticCast<size_t>(hParameter) - 1u);
	SEOUL_ASSERT(nParameterIndex < m_Data.m_Description.m_uParameters);

	GLSLFXLiteGlobalParameterEntry& entry = m_Data.m_pParameterEntries[nParameterIndex];
	UInt32 const nCount = Min((UInt16)nInputCount, entry.m_uCount);

	if (nCount > kMaximumComparisonCount ||
		0 != memcmp(m_Data.m_pParameterData + entry.m_uIndex, pData, sizeof(GLSLFXLiteParameterData) * nCount))
	{
		memcpy(
			m_Data.m_pParameterData + entry.m_uIndex,
			pData,
			sizeof(GLSLFXLiteParameterData) * nCount);

		entry.m_uDirtyStamp++;
	}
}

inline void GLSLFXLite::InternalCommitProgramParameters(const GLSLFXLitePassEntry& pass)
{
	OGLES2RenderDevice& rd = GetOGLES2RenderDevice();
	OGLES2StateManager& sm = rd.GetStateManager();

	if (IsValid(pass.m_hParameterFirst) &&
		IsValid(pass.m_hParameterLast))
	{
		UInt const nFirstParameter = HandleToOffset(pass.m_hParameterFirst);
		UInt const nLastParameter = HandleToOffset(pass.m_hParameterLast);

		for (UInt uParameter = nFirstParameter; uParameter <= nLastParameter; ++uParameter)
		{
			GLSLFXLiteProgramParameter& parameter = m_Data.m_pProgramParameters[uParameter];
			GLSLFXLiteGlobalParameterEntry const& globalParameter = m_Data.m_pParameterEntries[parameter.m_uGlobalParameterIndex];
			GLSLFXLiteParameterData const* pData = (m_Data.m_pParameterData + parameter.m_uParameterIndex);

			switch (parameter.m_uParameterClass)
			{
			case GLSLFX_PARAMETERCLASS_SAMPLER:
				if (parameter.m_iHardwareIndex >= 0)
				{
					sm.SetActiveTexture(GL_TEXTURE_2D, parameter.m_iHardwareIndex, pData->m_Texture);
				}
				break;
			default:
				if (parameter.m_uDirtyStamp != globalParameter.m_uDirtyStamp)
				{
					parameter.m_uDirtyStamp = globalParameter.m_uDirtyStamp;
					if (parameter.m_iHardwareIndex >= 0)
					{
						GLSLFXLiteParameterDescription const& desc = m_Data.m_pParameters[parameter.m_uGlobalParameterIndex];

						switch (desc.m_Type)
						{
						case GLSLFX_BOOL: // fall-through
						case GLSLFX_BOOL1:
							SEOUL_OGLES2_VERIFY(glUniform1iv(
								parameter.m_iHardwareIndex,
								parameter.m_uParameterCount,
								reinterpret_cast<GLint const*>(pData)));
							break;
						case GLSLFX_INT: // fall-through
						case GLSLFX_INT1:
							SEOUL_OGLES2_VERIFY(glUniform1iv(
								parameter.m_iHardwareIndex,
								parameter.m_uParameterCount,
								reinterpret_cast<GLint const*>(pData)));
							break;
						case GLSLFX_FLOAT: // fall-through
						case GLSLFX_FLOAT1:
							SEOUL_OGLES2_VERIFY(glUniform1fv(
								parameter.m_iHardwareIndex,
								parameter.m_uParameterCount,
								reinterpret_cast<Float const*>(pData)));
							break;
						case GLSLFX_FLOAT2:
							SEOUL_OGLES2_VERIFY(glUniform2fv(
								parameter.m_iHardwareIndex,
								parameter.m_uParameterCount / 2,
								reinterpret_cast<Float const*>(pData)));
							break;
						case GLSLFX_FLOAT3:
							SEOUL_OGLES2_VERIFY(glUniform3fv(
								parameter.m_iHardwareIndex,
								parameter.m_uParameterCount / 3,
								reinterpret_cast<Float const*>(pData)));
							break;
							// Float4x4 is setup in the cooker to be pushed to the program
							// using the same logic as a vector4.
						case GLSLFX_FLOAT4: // fall-through
						case GLSLFX_FLOAT4x4:
							SEOUL_OGLES2_VERIFY(glUniform4fv(
								parameter.m_iHardwareIndex,
								parameter.m_uParameterCount / 4,
								reinterpret_cast<Float const*>(pData)));
							break;
						default:
							SEOUL_LOG("Unknown parameter type: %d\n", desc.m_Type);
							SEOUL_FAIL("");
							break;
						};
					}
				}
				break;
			};
		}
	}
}

inline void GLSLFXLite::SetBool(UnsafeHandle hParameter, Bool bValue)
{
	Int const iValue = (bValue) ? 1 : 0;
	InternalStaticInlineSetValue(
		hParameter,
		1u,
		&iValue);
}

inline void GLSLFXLite::SetFloat(UnsafeHandle hParameter, Float fValue)
{
	InternalStaticInlineSetValue(
		hParameter,
		1u,
		&fValue);
}

inline void GLSLFXLite::SetInt(UnsafeHandle hParameter, Int iValue)
{
	Float const fValue = (Float)iValue;
	InternalStaticInlineSetValue(
		hParameter,
		1u,
		&fValue);
}

inline void GLSLFXLite::SetMatrixF4x4(UnsafeHandle hParameter, Float const* pMatrix4D)
{
	// For sanity/consistency sake, SeoulEngine picks one of the original 2 graphics API
	// conventions (OpenGL) for matrices (columns as vectors with column major storage).
	// However, in the shader, parts of matrix multiplication can be reduced to
	// dot products if matrices are stored with either columns as vectors with row major
	// storage, or rows as vectors with column major storage.
	//
	// So, we apply that conversion here before submitting the 4D matrix.
	Matrix4D const mTransposed = ((Matrix4D const*)pMatrix4D)->Transpose();

	InternalStaticInlineSetValue(
		hParameter,
		16u,
		(Float const*)mTransposed.GetData());
}

inline void GLSLFXLite::SetSampler(UnsafeHandle hParameter, BaseTexture* pTexture)
{
	UInt32 const nParameterIndex = (UInt32)(StaticCast<size_t>(hParameter) - 1u);
	SEOUL_ASSERT(nParameterIndex < m_Data.m_Description.m_uParameters);

	GLSLFXLiteGlobalParameterEntry const& entry = m_Data.m_pParameterEntries[nParameterIndex];

	// If a nullptr texture is being set, also clear the OpenGL handle.
	if (nullptr == pTexture)
	{
		m_pTextureReferences[nParameterIndex] = nullptr;
		m_Data.m_pParameterData[entry.m_uIndex].m_Texture = 0u;
	}
	else
	{
		// Cache the main and secondary texture OpenGL objects.
		UnsafeHandle textureHandle = pTexture->GetTextureHandle();

		// Set the main texture object.
		m_pTextureReferences[nParameterIndex] = pTexture;
		m_Data.m_pParameterData[entry.m_uIndex].m_Texture = (GLuint)textureHandle.V;

		UnsafeHandle secondaryTextureHandle = pTexture->GetSecondaryTextureHandle();

		// If the current Effect uses a secondary texture and if
		// the texture being set references a secondary texture, set it
		// to the appropriate parameter.
		if (m_pSecondaryTextureData[nParameterIndex] >= 0)
		{
			GLSLFXLiteGlobalParameterEntry const& secondaryEntry = m_Data.m_pParameterEntries[m_pSecondaryTextureData[nParameterIndex]];
			m_Data.m_pParameterData[secondaryEntry.m_uIndex].m_Texture = (GLuint)secondaryTextureHandle.V;
		}
	}
}

inline void GLSLFXLite::SetScalarArrayF(UnsafeHandle hParameter, Float const* pValues, UInt nNumberOfFloats)
{
	InternalStaticInlineSetValue(
		hParameter,
		nNumberOfFloats,
		pValues);
}

} // namespace Seoul

#endif // include guard
