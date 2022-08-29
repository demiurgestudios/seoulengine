/**
 * \file ScriptUIBitmapInstance.cpp
 * \brief Script binding around Falcon::BitmapInstance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "FalconBitmapDefinition.h"
#include "FalconBitmapInstance.h"
#include "ReflectionDefine.h"
#include "ScriptUIBitmapInstance.h"

namespace Seoul
{

static const HString kDefaultBitmapClassName("Bitmap");

SEOUL_BEGIN_TYPE(ScriptUIBitmapInstance, TypeFlags::kDisableCopy)
	SEOUL_PARENT(ScriptUIInstance)
	SEOUL_METHOD(ResetTexture)
	SEOUL_METHOD(SetIndirectTexture)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "void", "string symbol, double iWidth, double iHeight")
	SEOUL_METHOD(SetTexture)
		SEOUL_DEV_ONLY_ATTRIBUTE(ScriptSignature, "void", "FilePath filePath, double iWidth, double iHeight, bool bPrefetch")
SEOUL_END_TYPE()

ScriptUIBitmapInstance::ScriptUIBitmapInstance()
{
}

ScriptUIBitmapInstance::~ScriptUIBitmapInstance()
{
}

HString ScriptUIBitmapInstance::GetClassName() const
{
	return kDefaultBitmapClassName;
}

SharedPtr<Falcon::BitmapInstance> ScriptUIBitmapInstance::GetInstance() const
{
	SEOUL_ASSERT(!m_pInstance.IsValid() || Falcon::InstanceType::kBitmap == m_pInstance->GetType());
	return SharedPtr<Falcon::BitmapInstance>((Falcon::BitmapInstance*)m_pInstance.GetPtr());
}

void ScriptUIBitmapInstance::ResetTexture()
{
	auto p(GetInstance());
	p->SetBitmapDefinition(SharedPtr<Falcon::BitmapDefinition>());
}

void ScriptUIBitmapInstance::SetIndirectTexture(const String& sSymbol, UInt32 uWidth, UInt32 uHeight)
{
	// Indirect is specified as a "pseudo" FilePath with no type or directory.
	FilePath filePath;
	filePath.SetRelativeFilenameWithoutExtension(FilePathRelativeFilename(sSymbol));

	SharedPtr<Falcon::BitmapDefinition> pDefinition(
		SEOUL_NEW(MemoryBudgets::UIRuntime) Falcon::BitmapDefinition(filePath, uWidth, uHeight, 0u /* TODO:*/, false));

	auto p(GetInstance());
	p->SetBitmapDefinition(pDefinition);
}

void ScriptUIBitmapInstance::SetTexture(FilePath filePath, UInt32 uWidth, UInt32 uHeight, Bool bPreload)
{
	// TODO: Makes sense to cache these on a hash of the details (file path and size).
	SharedPtr<Falcon::BitmapDefinition> pDefinition(
		SEOUL_NEW(MemoryBudgets::UIRuntime) Falcon::BitmapDefinition(filePath, uWidth, uHeight, 0u /* TODO:*/, bPreload));

	auto p(GetInstance());
	p->SetBitmapDefinition(pDefinition);
}

} // namespace Seoul
