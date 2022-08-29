/**
 * \file FalconFlaChecker.h
 * \brief For non-ship only, implements validation of Adobe Animate
 * (.FLA) files, which are .zip archives that contain Adobe Animate
 * data in .xml, image (.png and .jpg), and proprietary binary files.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef FALCON_FLA_CHECKER_H
#define FALCON_FLA_CHECKER_H

#include "Prereqs.h"

namespace Seoul::Falcon
{

#if !SEOUL_SHIP
Bool CheckFla(const String& sFilename, String* psRelativeSwfFilename = nullptr, Bool bFatalOnly = false);
#endif // /!SEOUL_SHIP

} // namespace Seoul::Falcon

#endif // include guard
