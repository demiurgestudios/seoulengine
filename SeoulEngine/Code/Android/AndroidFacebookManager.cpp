/**
 * \file AndroidFacebookManager.cpp
 * \brief Specialization of FacebookManager for Android. Wraps
 * the Android Facebook (Java) SDK.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "AndroidFacebookManager.h"

#if SEOUL_WITH_FACEBOOK

#include "AndroidEngine.h"
#include "AndroidMainThreadQueue.h"
#include "AndroidPrereqs.h"

namespace Seoul
{

AndroidFacebookManager::AndroidFacebookManager()
{
}

AndroidFacebookManager::~AndroidFacebookManager()
{
}

/**
 * Begins the Facebook login flow.  The user is asked if he wants to allow the
 * app to have the given permissions.
 */
void AndroidFacebookManager::Login(const Vector<String>& vRequestedPermissions)
{
	// Attach a Java environment to the current thread.
	CheckedPtr<ANativeActivity> pActivity = AndroidEngine::Get()->GetActivity();
	ScopedJavaEnvironment scope;
	JNIEnv *pEnvironment = scope.GetJNIEnv();

	// Convert Vector<String> to java.lang.String[]
	jclass stringClass = pEnvironment->FindClass("java/lang/String");
	jobjectArray aJavaRequestedPermissions = pEnvironment->NewObjectArray(
		vRequestedPermissions.GetSize(),
		stringClass,
		nullptr);

	for (UInt i = 0; i < vRequestedPermissions.GetSize(); i++)
	{
		pEnvironment->SetObjectArrayElement(
			aJavaRequestedPermissions,
			i,
			pEnvironment->NewStringUTF(vRequestedPermissions[i].CStr()));
	}

	Java::Invoke<void>(
		pEnvironment,
		pActivity->clazz,
		"FacebookLogin",
		"([Ljava/lang/String;)V",
		aJavaRequestedPermissions);
}

/**
 * No-op, not called on Android
 */
void AndroidFacebookManager::RefreshAccessToken()
{
}

/**
 * Closes the current session but does not delete the user's current
 * authentication token.
 */
void AndroidFacebookManager::CloseSession()
{
	// Attach a Java environment to the current thread.
	CheckedPtr<ANativeActivity> pActivity = AndroidEngine::Get()->GetActivity();
	ScopedJavaEnvironment scope;
	JNIEnv *pEnvironment = scope.GetJNIEnv();

	Java::Invoke<void>(
		pEnvironment,
		pActivity->clazz,
		"FacebookCloseSession",
		"()V");
}

/**
 * Closes the current session and deletes the user's current authentication
 * token.
 */
void AndroidFacebookManager::LogOff()
{
	// Attach a Java environment to the current thread.
	CheckedPtr<ANativeActivity> pActivity = AndroidEngine::Get()->GetActivity();
	ScopedJavaEnvironment scope;
	JNIEnv *pEnvironment = scope.GetJNIEnv();

	Java::Invoke<void>(
		pEnvironment,
		pActivity->clazz,
		"FacebookLogOff",
		"()V");
}

/**
 * Tests if the user is currently logged into Facebook
 */
Bool AndroidFacebookManager::IsLoggedIn()
{
	// Attach a Java environment to the current thread.
	CheckedPtr<ANativeActivity> pActivity = AndroidEngine::Get()->GetActivity();
	ScopedJavaEnvironment scope;
	JNIEnv *pEnvironment = scope.GetJNIEnv();

	return Java::Invoke<Bool>(
		pEnvironment,
		pActivity->clazz,
		"FacebookIsLoggedIn",
		"()Z");
}

/**
 * Gets the current Facebook authentication token, or the empty string if we
 * don't have a token.
 */
String AndroidFacebookManager::GetAuthToken()
{
	// Attach a Java environment to the current thread.
	CheckedPtr<ANativeActivity> pActivity = AndroidEngine::Get()->GetActivity();
	ScopedJavaEnvironment scope;
	JNIEnv *pEnvironment = scope.GetJNIEnv();

	return Java::Invoke<String>(
		pEnvironment,
		pActivity->clazz,
		"FacebookGetAuthToken",
		"()Ljava/lang/String;");
}

void AndroidFacebookManager::GetFriendsList(const String& sFields, Int iUserLimit)
{
	// Attach a Java environment to the current thread.
	CheckedPtr<ANativeActivity> pActivity = AndroidEngine::Get()->GetActivity();
	ScopedJavaEnvironment scope;
	JNIEnv *pEnvironment = scope.GetJNIEnv();

	Java::Invoke<void>(
		pEnvironment,
		pActivity->clazz,
		"FacebookGetFriendsList",
		"(Ljava/lang/String;I)V",
		sFields,
		iUserLimit);
}

void AndroidFacebookManager::SendRequest(const String& sMessage, const String& sTitle, const String& sRecipients, const String& sSuggestedFriends, const String& sData)
{
	// Attach a Java environment to the current thread.
	CheckedPtr<ANativeActivity> pActivity = AndroidEngine::Get()->GetActivity();
	ScopedJavaEnvironment scope;
	JNIEnv *pEnvironment = scope.GetJNIEnv();

	Java::Invoke<void>(
		pEnvironment,
		pActivity->clazz,
		"FacebookSendRequest",
		"(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V",
		sMessage,
		sTitle,
		sRecipients,
		sSuggestedFriends,
		sData);
}

void AndroidFacebookManager::DeleteRequest(const String& sRequestID)
{
	// If we're not logged in, queue up the request to be deleted later, after we
	// do log in
	if (!IsLoggedIn())
	{
		m_vRequestsToDelete.PushBack(sRequestID);
		return;
	}

	// Attach a Java environment to the current thread.
	CheckedPtr<ANativeActivity> pActivity = AndroidEngine::Get()->GetActivity();
	ScopedJavaEnvironment scope;
	JNIEnv *pEnvironment = scope.GetJNIEnv();

	Java::Invoke<void>(
		pEnvironment,
		pActivity->clazz,
		"FacebookDeleteRequest",
		"(Ljava/lang/String;)V",
		sRequestID);
}

void AndroidFacebookManager::SendPurchaseEvent(Double fLocalPrice, const String& sCurrencyCode)
{
	// Attach a Java environment to the current thread.
	CheckedPtr<ANativeActivity> pActivity = AndroidEngine::Get()->GetActivity();
	ScopedJavaEnvironment scope;
	JNIEnv *pEnvironment = scope.GetJNIEnv();

	Java::Invoke<void>(
		pEnvironment,
		pActivity->clazz,
		"FacebookSendPurchaseEvent",
		"(DLjava/lang/String;)V",
		fLocalPrice,
		sCurrencyCode);
}

void AndroidFacebookManager::TrackEvent(const String& sEventID)
{
	// Attach a Java environment to the current thread.
	CheckedPtr<ANativeActivity> pActivity = AndroidEngine::Get()->GetActivity();
	ScopedJavaEnvironment scope;
	JNIEnv *pEnvironment = scope.GetJNIEnv();

	Java::Invoke<void>(
		pEnvironment,
		pActivity->clazz,
		"FacebookTrackEvent",
		"(Ljava/lang/String;)V",
		sEventID);
}

void AndroidFacebookManager::ShareLink(const String& sContentURL)
{
	// Attach a Java environment to the current thread.
	CheckedPtr<ANativeActivity> pActivity = AndroidEngine::Get()->GetActivity();
	ScopedJavaEnvironment scope;
	JNIEnv *pEnvironment = scope.GetJNIEnv();

	Java::Invoke<void>(
		pEnvironment,
		pActivity->clazz,
		"FacebookShareLink",
		"(Ljava/lang/String;)V",
		sContentURL);
}

// Called once the user has a player guid reported.
void AndroidFacebookManager::SetUserID(const String& sUserID)
{
	if (m_bInitialized)
	{
		return;
	}

	// Tracking.
	m_bInitialized = true;

	// Attach a Java environment to the current thread.
	CheckedPtr<ANativeActivity> pActivity = AndroidEngine::Get()->GetActivity();
	ScopedJavaEnvironment scope;
	JNIEnv *pEnvironment = scope.GetJNIEnv();

	Bool const bEnableDebugLogging = !SEOUL_SHIP;

	Java::Invoke<void>(
		pEnvironment,
		pActivity->clazz,
		"FacebookInitialize",
		"(Z)V",
		bEnableDebugLogging);
}

void AndroidFacebookManager::RequestBatchUserInfo(const Vector<String>& vUserIDs)
{
	// Attach a Java environment to the current thread.
	CheckedPtr<ANativeActivity> pActivity = AndroidEngine::Get()->GetActivity();
	ScopedJavaEnvironment scope;
	JNIEnv *pEnvironment = scope.GetJNIEnv();

	// Convert Vector<String> to java.lang.String[]
	jclass stringClass = pEnvironment->FindClass("java/lang/String");
	jobjectArray aJavaUserIDs = pEnvironment->NewObjectArray(
		vUserIDs.GetSize(),
		stringClass,
		nullptr);

	for (UInt i = 0; i < vUserIDs.GetSize(); i++)
	{
		pEnvironment->SetObjectArrayElement(
			aJavaUserIDs,
			i,
			pEnvironment->NewStringUTF(vUserIDs[i].CStr()));
	}

	Java::Invoke<void>(
		pEnvironment,
		pActivity->clazz,
		"FacebookRequestBatchUserInfo",
		"([Ljava/lang/String;)V",
		aJavaUserIDs);
}

static void HandleFacebookLoginStatusChanged()
{
	if (AndroidFacebookManager::Get())
	{
		AndroidFacebookManager::Get()->PublicOnFacebookLoginStatusChanged();
	}
}

static void HandleFacebookFriendsListReturned(String sStr)
{
	if (AndroidFacebookManager::Get())
	{
		AndroidFacebookManager::Get()->PublicOnReturnFriendsList(sStr);
	}
}

static void HandleOnSentRequest(String sRequestID, String sRecipients, String sData)
{
	if (AndroidFacebookManager::Get())
	{
		AndroidFacebookManager::Get()->PublicOnSentRequest(sRequestID, sRecipients, sData);
	}
}

static void HandleOnSendLinkFinished(Bool bSent)
{
	if (AndroidFacebookManager::Get())
	{
		// TODO: If we need a callback from ShareLink, add one here
	}
}

static void HandleUpdateFacebookUserInfo(String sID)
{
	if (AndroidFacebookManager::Get())
	{
		AndroidFacebookManager::Get()->PublicSetFacebookID(sID);
	}
}

static void HandleOnGetBatchUserInfo(String sID, String sName)
{
	if (AndroidFacebookManager::Get())
	{
		AndroidFacebookManager::Get()->PublicOnGetBatchUserInfo(sID, sName);
	}
}

static void HandleOnGetBatchUserInfoFailed(String sID)
{
	if (AndroidFacebookManager::Get())
	{
		AndroidFacebookManager::Get()->PublicOnGetBatchUserInfoFailed(sID);
	}
}

} // namespace Seoul

extern "C"
{

JNIEXPORT void JNICALL Java_com_demiurgestudios_seoul_AndroidNativeActivity_NativeOnFacebookLoginStatusChanged(
	JNIEnv* pJniEnvironment,
	jclass)
{
	using namespace Seoul;

	RunOnMainThread(&HandleFacebookLoginStatusChanged);
}

JNIEXPORT void JNICALL Java_com_demiurgestudios_seoul_AndroidNativeActivity_NativeOnReturnFriendsList(
	JNIEnv* pJniEnvironment,
	jclass,
	jstring sJavaMessage)
{
	using namespace Seoul;

	String sMessage;
	SetStringFromJava(pJniEnvironment, sJavaMessage, sMessage);

	RunOnMainThread(&HandleFacebookFriendsListReturned, sMessage);
}


JNIEXPORT void JNICALL Java_com_demiurgestudios_seoul_AndroidNativeActivity_NativeOnSentRequest(
	JNIEnv* pJniEnvironment,
	jclass,
	jstring sJavaRequestID,
	jstring sJavaRecipients,
	jstring sJavaData)
{
	using namespace Seoul;

	String sRequestID, sRecipients, sData;
	SetStringFromJava(pJniEnvironment, sJavaRequestID, sRequestID);
	SetStringFromJava(pJniEnvironment, sJavaRecipients, sRecipients);
	SetStringFromJava(pJniEnvironment, sJavaData, sData);

	RunOnMainThread(&HandleOnSentRequest, sRequestID, sRecipients, sData);
}

JNIEXPORT void JNICALL Java_com_demiurgestudios_seoul_AndroidNativeActivity_NativeOnSentShareLink(
	JNIEnv* pJniEnvironment,
	jclass,
	jboolean bShared)
{
	using namespace Seoul;;

	RunOnMainThread<Bool>(&HandleOnSendLinkFinished, bShared);
}

JNIEXPORT void JNICALL Java_com_demiurgestudios_seoul_AndroidNativeActivity_NativeUpdateFacebookUserInfo(
	JNIEnv* pJniEnvironment,
	jclass,
	jstring sJavaID)
{
	using namespace Seoul;

	String sID;
	SetStringFromJava(pJniEnvironment, sJavaID, sID);

	RunOnMainThread(&HandleUpdateFacebookUserInfo, sID);
}


JNIEXPORT void JNICALL Java_com_demiurgestudios_seoul_AndroidNativeActivity_NativeOnGetBatchUserInfo(
	JNIEnv* pJniEnvironment,
	jclass,
	jstring sJavaID,
	jstring sJavaName)
{
	using namespace Seoul;

	String sID;
	SetStringFromJava(pJniEnvironment, sJavaID, sID);
	String sName;
	SetStringFromJava(pJniEnvironment, sJavaName, sName);

	RunOnMainThread(&HandleOnGetBatchUserInfo, sID, sName);
}


JNIEXPORT void JNICALL Java_com_demiurgestudios_seoul_AndroidNativeActivity_NativeOnGetBatchUserInfoFailed(
	JNIEnv* pJniEnvironment,
	jclass,
	jstring sJavaID)
{
	using namespace Seoul;

	String sID;
	SetStringFromJava(pJniEnvironment, sJavaID, sID);

	RunOnMainThread(&HandleOnGetBatchUserInfoFailed, sID);
}

} // extern "C"

#endif  // SEOUL_WITH_FACEBOOK
