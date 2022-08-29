/**
 * \file EffectReceiverGLSLES2GlslangWrapper.cpp
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "EffectReceiverGLSLES2GlslangWrapper.h"
#include "EffectReceiverGLSLES2Util.h"
#include "Mutex.h"
#include "SeoulString.h"

#include <Public/ShaderLang.h>

namespace Seoul
{

static const Int32 kiGLSLES2LangVersion = 100;

static Mutex s_GlslangMutex;
static Int32 s_iGlslangActiveCount = 0;

static void EffectReceiverGLSLES2GlslangWrapperSetupBuiltInResource(TBuiltInResource& rResource)
{
	// Everything 0/false by default.
	memset(&rResource, 0, sizeof(rResource));

	rResource.maxClipPlanes = 6;
	rResource.maxLights = 8;
	rResource.maxTextureCoords = 4;
	rResource.maxTextureImageUnits = 4;
	rResource.maxTextureUnits = 4;
	rResource.maxVaryingComponents = 4;
	rResource.maxVaryingVectors = 4;
	rResource.maxVertexAttribs = 4;
}

static bool IsLogOk(Byte const* sLog)
{
	if (IsNullOrEmpty(sLog))
	{
		return true;
	}

	if (nullptr != strstr(sLog, "WARNING"))
	{
		return false;
	}

	if (nullptr != strstr(sLog, "ERROR"))
	{
		return false;
	}

	if (nullptr != strstr(sLog, "UNIMPLEMENTED"))
	{
		return false;
	}

	return true;
}

EffectReceiverGLSLES2GlslangWrapper::EffectReceiverGLSLES2GlslangWrapper()
{
	Lock lock(s_GlslangMutex);
	if (1 == ++s_iGlslangActiveCount)
	{
		SEOUL_VERIFY(glslang::InitializeProcess());
	}
}

EffectReceiverGLSLES2GlslangWrapper::~EffectReceiverGLSLES2GlslangWrapper()
{
	Lock lock(s_GlslangMutex);
	if (0 == --s_iGlslangActiveCount)
	{
		glslang::FinalizeProcess();
	}
}

Bool EffectReceiverGLSLES2GlslangWrapper::Validate(
	const String& sPixelShaderSource,
	const String& sVertexShaderSource) const
{
	TBuiltInResource builtInResource;
	EffectReceiverGLSLES2GlslangWrapperSetupBuiltInResource(builtInResource);

	Byte const* sPixelShader = sPixelShaderSource.CStr();
	Byte const* sVertexShader = sVertexShaderSource.CStr();

	glslang::TShader pixelShader(EShLangFragment);
	pixelShader.setStrings(&sPixelShader, 1);
	if (!pixelShader.parse(
		&builtInResource,
		kiGLSLES2LangVersion,
		false,
		EShMsgDefault) ||
		!IsLogOk(pixelShader.getInfoDebugLog()) ||
		!IsLogOk(pixelShader.getInfoLog()))
	{
		fprintf(stderr, "GLSLES2 fragment validation failed:\n%s\nErrors:\n%s\n%s",
			sPixelShaderSource.CStr(),
			pixelShader.getInfoDebugLog(),
			pixelShader.getInfoLog());
		return false;
	}

	glslang::TShader vertexShader(EShLangVertex);
	vertexShader.setStrings(&sVertexShader, 1);
	if (!vertexShader.parse(
		&builtInResource,
		kiGLSLES2LangVersion,
		false,
		EShMsgDefault) ||
		!IsLogOk(vertexShader.getInfoDebugLog()) ||
		!IsLogOk(vertexShader.getInfoLog()))
	{
		fprintf(stderr, "GLSLES2 vertex validation failed:\n%s\nErrors:\n%s\n%s",
			sVertexShaderSource.CStr(),
			vertexShader.getInfoDebugLog(),
			vertexShader.getInfoLog());
		return false;
	}

	glslang::TProgram program;
	program.addShader(&pixelShader);
	program.addShader(&vertexShader);
	if (!program.link(EShMsgDefault) ||
		!IsLogOk(program.getInfoDebugLog()) ||
		!IsLogOk(program.getInfoLog()))
	{
		fprintf(stderr, "GLSLES2 program validation/link failed:\nFragment:\n%s\nVertex:\n%s\nErrors:\n%s\n%s",
			sPixelShaderSource.CStr(),
			sVertexShaderSource.CStr(),
			program.getInfoDebugLog(),
			program.getInfoLog());
		return false;
	}

	return true;
}

} // namespace Seoul
