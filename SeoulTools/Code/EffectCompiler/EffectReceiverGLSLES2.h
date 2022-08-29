/**
 * \file EffectReceiverGLSLES2.h
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef EFFECT_RECEIVER_GLSLES2_H
#define EFFECT_RECEIVER_GLSLES2_H

#include "HashTable.h"
#include "EffectReceiverGLSLES2GlslangWrapper.h"
#include "EffectReceiverGLSLES2Util.h"
#include "IEffectReceiver.h"
#include "Vector.h"
namespace Seoul { namespace EffectConverter { class Converter; } }
namespace Seoul { namespace EffectConverter { namespace Util { struct Pass; } } }

namespace Seoul
{

typedef HashTable<HString, Int32> ConstantRegisterLookupTable;

class EffectReceiverGLSLES2 SEOUL_SEALED : public EffectConverter::IEffectReceiver
{
public:
	EffectReceiverGLSLES2();
	~EffectReceiverGLSLES2();

	virtual Bool AddParameter(const EffectConverter::Converter& effectConverter, const EffectConverter::Util::Parameter& parameter) SEOUL_OVERRIDE;
	virtual Bool AddTechnique(const EffectConverter::Converter& effectConverter, const EffectConverter::Util::Technique& technique) SEOUL_OVERRIDE;

	UInt8* GetSerializeableData(Bool bBigEndianOutput, UInt32& rzDataSize) const;

private:
	// Structures used to process parameters and techniques
	Vector<Byte> m_vStrings;
	Vector<GLSLFXLiteParameterDescription> m_vParameters;
	Vector<GLSLFXLiteTechniqueDescription> m_vTechniques;
	Vector<GLSLFXLitePassDescription> m_vPasses;
	Vector<GLSLFXLiteParameterData> m_vParameterData;
	Vector<GLSLFXLiteGlobalParameterEntry> m_vParameterEntries;
	Vector<GLSLFXLiteTechniqueEntry> m_vTechniqueEntries;
	Vector<GLSLFXLitePassEntry> m_vPassEntries;
	Vector<GLSLFXLiteRenderState> m_vRenderStates;
	Vector<GLSLFXLiteShaderEntry> m_vShaderEntries;
	Vector<Byte> m_vShaderCode;
	Vector<GLSLFXLiteProgramParameter> m_vProgramParameters;

private:
	EffectReceiverGLSLES2GlslangWrapper m_Validator;

	UInt32 InternalCalculateSerializeableDataSize() const;

	Bool InternalSetupProgramParameters(
		const ConstantRegisterLookupTable& tPixelShaderLookup,
		const ConstantRegisterLookupTable& tVertexShaderLookup,
		GLSLFXLitePassEntry& rPassEntry);

	void InternalAddRenderStates(
		const EffectConverter::Util::Pass& pass,
		GLSLFXLitePassEntry& rPassEntry);

	Bool InternalCompileShader(
		const EffectConverter::Converter& effectConverter,
		const EffectConverter::Util::Shader& shader,
		Bool bVertexShader,
		GLSLFXLitePassEntry& rPassEntry,
		String& rsShaderSource,
		ConstantRegisterLookupTable& rtConstantRegisterLookupTable);

	SEOUL_DISABLE_COPY(EffectReceiverGLSLES2);
}; // class EffectReceiverGLSLES2

} // namespace Seoul

#endif // include guard
