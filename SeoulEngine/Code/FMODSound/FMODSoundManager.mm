/**
 * \file FMODSoundManager.mm
 * \brief IOS specific implementation.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#import <AVFoundation/AVFoundation.h>
#include "Prereqs.h"

#if SEOUL_WITH_FMOD

namespace Seoul
{

Bool FMODSoundManagerIsExternalAudioPlaying()
{
	// Deliberate double bang.
	return !!([[AVAudioSession sharedInstance] isOtherAudioPlaying]);
}

} // namespace Seoul

#endif // /#if SEOUL_WITH_FMOD
