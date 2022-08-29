/**
 * \file BuildChangelist.h
 * \brief Header file that contains the changelist component of the product version info
 *
 * NOTE: Changes made to this file only affect local builds.  The autobuilder overwrites it
 * in order to get the build changelist number into the product version number.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef BUILD_CHANGELIST_H
#define BUILD_CHANGELIST_H

/**
 * Note, local builds will always have a CL of 0 and a description of Local.
 * This means that a server will never consider the client out of date in terms
 * of patching. To fake a "live" build and be considered for patches, enter values
 * here to mimic the "live" build. That is, if CL 123456 is live, use:
 *     #define BUILD_CHANGELIST        (123456)
 *     #define BUILD_CHANGELIST_STR    "CL123456"
 * Note the "CL" at the begining of the string.
 **/

#define BUILD_CHANGELIST        (0)
#define BUILD_CHANGELIST_STR    "Local"

#endif // include guard
