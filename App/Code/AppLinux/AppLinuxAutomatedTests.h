/**
 * \file AppLinuxAutomatedTests.h
 * \brief Defines the main function for a build run that will execute
 * automated tests.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef APP_LINUX_AUTOMATED_TESTS_H
#define APP_LINUX_AUTOMATED_TESTS_H

#include "Prereqs.h"
namespace Seoul { struct CustomCrashErrorState; }

namespace Seoul
{

#if SEOUL_AUTO_TESTS
Int AppLinuxRunAutomatedTests(const String& sAutomationScriptFileName, Bool bEnableDownloadableContent, Bool bPersistent);
#endif // /#if SEOUL_AUTO_TESTS

void LinuxSendCustomCrash(const CustomCrashErrorState& errorState);

} // namespace Seoul

#endif // include guard
