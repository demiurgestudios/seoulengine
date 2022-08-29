/**
 * \file ScriptEngineRenderer.cpp
 * \brief Binder instance for exposing Renderer functionality into
 * a script VM.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FxManager.h"
#include "ReflectionDefine.h"
#include "RenderDevice.h"
#include "Renderer.h"
#include "ScriptEngineRenderer.h"
#include "ScriptFunctionInterface.h"

namespace Seoul
{

SEOUL_BEGIN_TYPE(ScriptEngineRenderer, TypeFlags::kDisableCopy)
	SEOUL_METHOD(GetViewportAspectRatio)
	SEOUL_METHOD(GetViewportDimensions)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "(double, double)")
	SEOUL_METHOD(PrefetchFx)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "void", "FilePath filePath")
SEOUL_END_TYPE()

ScriptEngineRenderer::ScriptEngineRenderer()
{
}

ScriptEngineRenderer::~ScriptEngineRenderer()
{
}

/** @return The back buffer viewport aspect ratio. */
Float ScriptEngineRenderer::GetViewportAspectRatio() const
{
	return RenderDevice::Get()->GetBackBufferViewport().GetViewportAspectRatio();
}

/** @return The back buffer viewport dimensions. */
void ScriptEngineRenderer::GetViewportDimensions(Script::FunctionInterface* pInterface) const
{
	Viewport viewport = RenderDevice::Get()->GetBackBufferViewport();
	Double dimX = viewport.m_iViewportWidth;
	Double dimY = viewport.m_iViewportHeight;

	pInterface->PushReturnNumber(dimX);
	pInterface->PushReturnNumber(dimY);
}

void ScriptEngineRenderer::PrefetchFx(Script::FunctionInterface* pInterface) const
{
	// Ignore non-FilePath for convenience, can be called with symbolic names.
	auto pFilePath = pInterface->GetUserData<FilePath>(1);
	if (nullptr != pFilePath)
	{
		if (FxManager::Get())
		{
			FxManager::Get()->Prefetch(*pFilePath);
		}
	}
}

} // namespace Seoul
