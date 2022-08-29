/**
 * \file ScriptUIBitmapInstance.h
 * \brief Script binding around Falcon::BitmapInstance.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SCRIPT_UI_BITMAP_INSTANCE_H
#define SCRIPT_UI_BITMAP_INSTANCE_H

#include "ScriptUIInstance.h"
namespace Seoul { namespace Falcon { class BitmapInstance; } }

namespace Seoul
{

class ScriptUIBitmapInstance SEOUL_SEALED : public ScriptUIInstance
{
public:
	SEOUL_REFLECTION_POLYMORPHIC(ScriptUIBitmapInstance);

	ScriptUIBitmapInstance();
	~ScriptUIBitmapInstance();

	virtual HString GetClassName() const SEOUL_OVERRIDE;
	SharedPtr<Falcon::BitmapInstance> GetInstance() const;

	void ResetTexture();
	void SetIndirectTexture(const String& sSymbol, UInt32 uWidth, UInt32 uHeight);
	void SetTexture(FilePath filePath, UInt32 uWidth, UInt32 uHeight, Bool bPreload);

private:
	SEOUL_DISABLE_COPY(ScriptUIBitmapInstance);
}; // class ScriptUIBitmapInstance

} // namespace Seoul

#endif // include guard
