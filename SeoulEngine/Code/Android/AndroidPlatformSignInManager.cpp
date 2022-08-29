/**
 * \file AndroidPlatformSignInManager.cpp
 * \brief Android-specific platform sign-in manager. Implemented
 * with Google Play Games API.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AndroidPlatformSignInManager.h"

#if SEOUL_WITH_GOOGLEPLAYGAMES

#include "AndroidEngine.h"
#include "AndroidMainThreadQueue.h"
#include "AndroidPrereqs.h"
#include "HTTPManager.h"
#include "HTTPRequest.h"
#include "HTTPRequestCancellationToken.h"
#include "Logger.h"
#include "ScopedAction.h"

namespace Seoul
{

/**
 * Runtime check for availability - must be called prior to instantiating a
 * AndroidPlatformSignInManager instance.
 */
Bool AndroidPlatformSignInManager::IsAvailable()
{
	ScopedJavaEnvironment scope;
	auto pEnvironment = scope.GetJNIEnv();
	auto pActivity = AndroidEngine::Get()->GetActivity();
	return Java::Invoke<Bool>(
		pEnvironment,
		pActivity->clazz,
		"GooglePlayGamesIsAvailable",
		"()Z");
}

AndroidPlatformSignInManager::AndroidPlatformSignInManager(const AndroidPlatformSignInManagerSettings& settings)
	: m_Settings(settings)
	, m_ChangeCount(0)
	, m_CancellationCount(0)
	, m_Mutex()
	, m_bSignedIn(false)
	// True by default, false once we get any sort of sign-in status update.
	, m_bSigningIn(true)
{
	ScopedJavaEnvironment scope;
	auto pEnvironment = scope.GetJNIEnv();
	auto pActivity = AndroidEngine::Get()->GetActivity();
	Java::Invoke<void>(
		pEnvironment,
		pActivity->clazz,
		"GooglePlayGamesInitialize",
		"(ZLjava/lang/String;)V",
		!SEOUL_SHIP,
		m_Settings.m_sOauthClientId);
}

AndroidPlatformSignInManager::~AndroidPlatformSignInManager()
{
	ScopedJavaEnvironment scope;
	auto pEnvironment = scope.GetJNIEnv();
	auto pActivity = AndroidEngine::Get()->GetActivity();
	Java::Invoke<void>(
		pEnvironment,
		pActivity->clazz,
		"GooglePlayGamesShutdown",
		"()V");
}

void AndroidPlatformSignInManager::PlatformSignInOnly(PlatformSignInDelegate delegate)
{
	// Simple case - early out if already signed in.
	if (IsSignedIn())
	{
		if (delegate) { delegate(true); }
		return;
	}

	// Copy the delegate.
	auto pDelegate = SEOUL_NEW(MemoryBudgets::TBD) PlatformSignInDelegate(delegate);

	// Dispatch to Java to handle the rest.
	ScopedJavaEnvironment scope;
	auto pEnvironment = scope.GetJNIEnv();
	auto pActivity = AndroidEngine::Get()->GetActivity();
	Java::Invoke<void>(
		pEnvironment,
		pActivity->clazz,
		"GooglePlayGamesPlatformSignInOnly",
		"(J)V",
		reinterpret_cast<jlong>(pDelegate));
}

void AndroidPlatformSignInManager::SignIn()
{
	ScopedJavaEnvironment scope;
	auto pEnvironment = scope.GetJNIEnv();
	auto pActivity = AndroidEngine::Get()->GetActivity();
	Java::Invoke<void>(
		pEnvironment,
		pActivity->clazz,
		"GooglePlayGamesSignIn",
		"()V");
}

void AndroidPlatformSignInManager::SignOut()
{
	ScopedJavaEnvironment scope;
	auto pEnvironment = scope.GetJNIEnv();
	auto pActivity = AndroidEngine::Get()->GetActivity();
	Java::Invoke<void>(
		pEnvironment,
		pActivity->clazz,
		"GooglePlayGamesSignOut",
		"()V");
}

namespace
{

struct CallbackData
{
	CheckedPtr<HTTP::Request> m_pRequest;
	AndroidPlatformSignInManager::OnTokenReceivedDelegate m_Callback;
	Bool m_bCallback = false;
};

} // namespace

void AndroidPlatformSignInManager::GetIDToken(const OnTokenReceivedDelegate& delegate)
{
	// Immediate complete with no id if not signed in.
	if (!m_bSignedIn || m_bSigningIn)
	{
		delegate(String(), false);
		return;
	}

	// Gather.
	auto pData = SEOUL_NEW(MemoryBudgets::TBD) CallbackData;
	pData->m_Callback = delegate;
	pData->m_bCallback = true;

	// Dispatch to Java, will invoke on callback.
	ScopedJavaEnvironment scope;
	auto pEnvironment = scope.GetJNIEnv();
	auto pActivity = AndroidEngine::Get()->GetActivity();
	Java::Invoke<void>(
		pEnvironment,
		pActivity->clazz,
		"GooglePlayGamesRequestIdToken",
		"(J)V",
		reinterpret_cast<jlong>(pData));
}

void AndroidPlatformSignInManager::StartWithIdToken(HTTP::Request& rRequest)
{
	// Immediate complete with no id if not signed in.
	if (!m_bSignedIn || m_bSigningIn)
	{
		rRequest.Start();
		return;
	}

	// Gather.
	auto pData = SEOUL_NEW(MemoryBudgets::TBD) CallbackData;
	pData->m_pRequest = &rRequest;
	pData->m_bCallback = false;

	// Dispatch to Java, will invoke on callback.
	ScopedJavaEnvironment scope;
	auto pEnvironment = scope.GetJNIEnv();
	auto pActivity = AndroidEngine::Get()->GetActivity();
	Java::Invoke<void>(
		pEnvironment,
		pActivity->clazz,
		"GooglePlayGamesRequestIdToken",
		"(J)V",
		reinterpret_cast<jlong>(pData));
}

void AndroidPlatformSignInManager::OnSessionStart()
{
	// Nop
}

void AndroidPlatformSignInManager::OnSessionEnd()
{
	// Nop
}

void AndroidPlatformSignInManager::HandleCancelled()
{
	auto pMan = AndroidPlatformSignInManager::Get();
	if (!pMan)
	{
		return;
	}

	pMan->OnCancelled();
}

void AndroidPlatformSignInManager::HandleChange(
	Bool bSignedIn)
{
	auto pMan = AndroidPlatformSignInManager::Get();
	if (!pMan)
	{
		return;
	}

	pMan->OnChange(bSignedIn);
}

void AndroidPlatformSignInManager::OnCancelled()
{
	auto const count = (++m_CancellationCount);
	(void)count;

	SEOUL_LOG_AUTH("Sign-in cancelled, cancellation count is now '%d'",
		(Int)count);
}

void AndroidPlatformSignInManager::OnChange(
	Bool bSignedIn)
{
	SEOUL_LOG_AUTH("%s: %s",
		__FUNCTION__,
		(bSignedIn ? "true" : "false"));

	// Always run the trigger sign in event when this function exits
	auto const signInEvent(MakeDeferredAction(
		[]() { RunOnMainThread(&PlatformSignInManager::TriggerSignInEvent); }));

	// Update signed-in state.
	{
		Lock lock(m_Mutex);

		// Now inside the lock, early out if no actual changes.
		if (bSignedIn == m_bSignedIn)
		{
			// Before return, update signing in status.
			m_bSigningIn = false;
			return;
		}

		m_bSignedIn = bSignedIn;
	}

	// Update status changed.
	SeoulMemoryBarrier();

	// State change.
	++m_ChangeCount;

	// Finally, update signing in status.
	SeoulMemoryBarrier();
	m_bSigningIn = false;
}

} // namespace Seoul

extern "C"
{

JNIEXPORT void JNICALL Java_com_demiurgestudios_seoul_AndroidNativeActivity_NativeOnPlatformOnlySignInResult(
	JNIEnv* pJniEnvironment,
	jclass,
	jlong pCallback,
	jboolean bSuccess)
{
	using namespace Seoul;

	// Cast, cleanup, and invoke.
	auto pSeoulCallback = reinterpret_cast<AndroidPlatformSignInManager::PlatformSignInDelegate*>(pCallback);
	auto callback = (nullptr == pSeoulCallback
		? AndroidPlatformSignInManager::PlatformSignInDelegate()
		: *pSeoulCallback);
	SafeDelete(pSeoulCallback);

	// Invoke and complete.
	if (callback)
	{
		callback((Bool)(!!bSuccess));
	}
}

JNIEXPORT void JNICALL Java_com_demiurgestudios_seoul_AndroidNativeActivity_NativeOnSignInCancelled(
	JNIEnv* pJniEnvironment,
	jclass)
{
	using namespace Seoul;

	AndroidPlatformSignInManager::HandleCancelled();
}

JNIEXPORT void JNICALL Java_com_demiurgestudios_seoul_AndroidNativeActivity_NativeOnSignInChange(
	JNIEnv* pJniEnvironment,
	jclass,
	jboolean bSignedIn)
{
	using namespace Seoul;

	AndroidPlatformSignInManager::HandleChange((Bool)(!!bSignedIn));
}

JNIEXPORT void JNICALL Java_com_demiurgestudios_seoul_AndroidNativeActivity_NativeOnRequestIdToken(
	JNIEnv* pJniEnvironment,
	jclass,
	jlong pUserData,
	jboolean bSuccess,
	jstring sJavaIdToken)
{
	using namespace Seoul;

	// Cast and invoke.
	auto pData = reinterpret_cast<CallbackData*>(pUserData);

	if (pData->m_bCallback)
	{
		// Acquire and cleanup.
		auto const callback = pData->m_Callback;
		SafeDelete(pData);

		if ((Bool)(!!bSuccess))
		{
			String sIdToken;
			SetStringFromJava(pJniEnvironment, sJavaIdToken, sIdToken);

			SEOUL_LOG_AUTH("Sending callback with id token: %s",
				sIdToken.CStr());

			callback(sIdToken, true);
		}
		else
		{
			callback(String(), false);
		}

		SafeDelete(pData);
		return;
	}

	// Acquire and cleanup.
	{
		auto const pRequest = pData->m_pRequest;
		SafeDelete(pData);

		// Handle.
		if ((Bool)(!!bSuccess))
		{
			String sIdToken;
			SetStringFromJava(pJniEnvironment, sJavaIdToken, sIdToken);
			pRequest->AddPostData("GooglePlayToken", sIdToken);

			SEOUL_LOG_AUTH("Sending HTTP request with id token: %s",
				sIdToken.CStr());
		}

		pRequest->Start();
	}
}

} // extern "C"

#endif // SEOUL_WITH_GOOGLEPLAYGAMES
