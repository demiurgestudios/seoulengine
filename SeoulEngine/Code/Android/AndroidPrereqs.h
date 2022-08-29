/**
 * \file AndroidPrereqs.h
 * \brief Utilities and headers necessary for working with JNI on Android.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef ANDROID_PREREQS_H
#define ANDROID_PREREQS_H

#include "CheckedPtr.h"
#include "CommerceManager.h"
#include "DataStore.h"
#include "Prereqs.h"
#include "SeoulHString.h"
#include "SeoulString.h"
#include "SeoulTypes.h"
#include "Thread.h"
#include "Vector.h"

#include <android/native_activity.h>
#include <jni.h>

namespace Seoul
{

// Forward declarations
class Thread;

/**
 * Utility class, creates a JavaEnvironment attached to the
 * current thread and release it on destruction.
 */
class ScopedJavaEnvironment SEOUL_SEALED
{
public:
	ScopedJavaEnvironment()
		: m_pJNIEnv(Thread::GetThisThreadJNIEnv())
	{
		if (nullptr != m_pJNIEnv)
		{
			// PushLocalFrame(0) - the argument is a capacity, it effectively reserves
			// n slots. Since we don't know how many slots the context will use, we pass
			// 0 here to guarantee PushLocalFrame() success.
			SEOUL_VERIFY(JNI_OK == m_pJNIEnv->PushLocalFrame(0));
		}
	}

	~ScopedJavaEnvironment()
	{
		if (nullptr != m_pJNIEnv)
		{
			// Don't care about the frame context, so ignore the return
			// and pass nullptr.
			(void)m_pJNIEnv->PopLocalFrame(nullptr);
		}
	}

	CheckedPtr<JNIEnv> GetJNIEnv() const
	{
		return m_pJNIEnv;
	}

private:
	JNIEnv* m_pJNIEnv;
}; // class ScopedJavaEnvironment

namespace Java
{

inline jmethodID GetMethodID(
	CheckedPtr<JNIEnv> pEnvironment,
	jobject pInstance,
	Byte const* sMethodName,
	Byte const* sMethodSignature)
{
	// Get the most derived class of the NativeActivity instance.
	jclass pInstanceClass = pEnvironment->GetObjectClass(pInstance);
	if (nullptr == pInstanceClass)
	{
		return nullptr;
	}

	// Get the method,
	jmethodID methodID = pEnvironment->GetMethodID(
		pInstanceClass,
		sMethodName,
		sMethodSignature);
	SEOUL_ASSERT_MESSAGE(methodID != nullptr, String::Printf("JNI: Could not find method %s %s", sMethodName, sMethodSignature).CStr());

	return methodID;
}

inline jstring ToJavaArgument(CheckedPtr<JNIEnv> pEnvironment, HString s)
{
	return pEnvironment->NewStringUTF(s.CStr());
}

inline jstring ToJavaArgument(CheckedPtr<JNIEnv> pEnvironment, const String& s)
{
	return pEnvironment->NewStringUTF(s.CStr());
}

inline jstring ToJavaArgument(CheckedPtr<JNIEnv> pEnvironment, Byte const* s)
{
	return pEnvironment->NewStringUTF(s);
}

inline jint ToJavaArgument(CheckedPtr<JNIEnv> /*pEnvironment*/, Int32 v)
{
	return v;
}

inline jint ToJavaArgument(CheckedPtr<JNIEnv> /*pEnvironment*/, UInt32 v)
{
	return (jint)v;
}

inline jlong ToJavaArgument(CheckedPtr<JNIEnv> /*pEnvironment*/, Int64 v)
{
	return v;
}

inline jlong ToJavaArgument(CheckedPtr<JNIEnv> /*pEnvironment*/, UInt64 v)
{
	return (jlong)v;
}

inline jfloat ToJavaArgument(CheckedPtr<JNIEnv> /*pEnvironment*/, Float f)
{
	return f;
}

inline jdouble ToJavaArgument(CheckedPtr<JNIEnv> /*pEnvironment*/, Double d)
{
	return d;
}

inline jobject ToJavaArgument(CheckedPtr<JNIEnv> /*pEnvironment*/, jobject o)
{
	return o;
}

template <int MEMORY_BUDGETS>
inline jobject ToJavaArgument(CheckedPtr<JNIEnv> pEnvironment, const Vector<String, MEMORY_BUDGETS>& v)
{
	auto const uSize = v.GetSize();
	auto const stringClass = pEnvironment->FindClass("java/lang/String");
	auto arr = pEnvironment->NewObjectArray(uSize, stringClass, 0);
	for (UInt32 i = 0u; i < uSize; ++i)
	{
		auto const str = pEnvironment->NewStringUTF(v[i].CStr());
		pEnvironment->SetObjectArrayElement(arr, i, str);
		pEnvironment->DeleteLocalRef(str);
	}
	pEnvironment->DeleteLocalRef(stringClass);
	return arr;
}

inline static void TestException(CheckedPtr<JNIEnv> pEnvironment)
{
	// Check for exceptions and crash if we get one.
	if (JNI_FALSE != pEnvironment->ExceptionCheck())
	{
		// Log the exception details.
		pEnvironment->ExceptionDescribe();

		// Handle the exception.
		pEnvironment->ExceptionClear();

		// Intentional crash. Could also be abort(), but that raises SIGABRT
		// which is less commonly used, and therefore may not behave
		// consistently across environments.
		(*((UInt volatile*)nullptr)) = 1;
	}
}

template <
	typename R,
	typename A1 = void,
	typename A2 = void,
	typename A3 = void,
	typename A4 = void,
	typename A5 = void,
	typename A6 = void,
	typename A7 = void,
	typename A8 = void,
	typename A9 = void>
struct Invoker {};

template <>
struct Invoker<void, void, void, void, void, void, void, void, void, void>
{
	static void Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID)
	{
		pEnvironment->CallVoidMethod(
			pInstance,
			methodID);
		TestException(pEnvironment);
	}
};

template <>
struct Invoker<Bool, void, void, void, void, void, void, void, void, void>
{
	static Bool Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID)
	{
		Bool const bResult = (0 != pEnvironment->CallBooleanMethod(
			pInstance,
			methodID));
		TestException(pEnvironment);
		return bResult;
	}
};

template <>
struct Invoker<Float, void, void, void, void, void, void, void, void, void>
{
	static Float Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID)
	{
		Float const fResult = pEnvironment->CallFloatMethod(
			pInstance,
			methodID);
		TestException(pEnvironment);
		return fResult;
	}
};

template <>
struct Invoker<Int, void, void, void, void, void, void, void, void, void>
{
	static Int Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID)
	{
		Int const iResult = pEnvironment->CallIntMethod(
			pInstance,
			methodID);
		TestException(pEnvironment);
		return iResult;
	}
};

template <>
struct Invoker<Int64, void, void, void, void, void, void, void, void, void>
{
	static Int64 Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID)
	{
		Int64 const iResult = (Int64)pEnvironment->CallLongMethod(
			pInstance,
			methodID);
		TestException(pEnvironment);
		return iResult;
	}
};

template <>
struct Invoker<String, void, void, void, void, void, void, void, void, void>
{
	static String Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID)
	{
		jstring value = (jstring)pEnvironment->CallObjectMethod(
			pInstance,
			methodID);
		TestException(pEnvironment);
		if (value == nullptr)
		{
			return String();
		}

		Byte const* sValue = pEnvironment->GetStringUTFChars(value, nullptr);
		String sReturn(sValue);
		pEnvironment->ReleaseStringUTFChars(value, sValue);
		return sReturn;
	}
};

template <>
struct Invoker<jobject, void, void, void, void, void, void, void, void, void>
{
	static jobject Invoke(
			CheckedPtr<JNIEnv> pEnvironment,
			jobject pInstance,
			jmethodID methodID)
	{
		jobject result = pEnvironment->CallObjectMethod(
				pInstance,
				methodID);
		TestException(pEnvironment);
		return result;
	}
};

template <typename A1>
struct Invoker<void, A1, void, void, void, void, void, void, void, void>
{
	static void Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID,
		const A1& a1)
	{
		pEnvironment->CallVoidMethod(
			pInstance,
			methodID,
			ToJavaArgument(pEnvironment, a1));
		TestException(pEnvironment);
	}
};

template <typename A1>
struct Invoker<Bool, A1, void, void, void, void, void, void, void, void>
{
	static Bool Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID,
		const A1& a1)
	{
		Bool const bResult = (0 != pEnvironment->CallBooleanMethod(
			pInstance,
			methodID,
			ToJavaArgument(pEnvironment, a1)));
		TestException(pEnvironment);
		return bResult;
	}
};

template <typename A1>
struct Invoker<Int, A1, void, void, void, void, void, void, void, void>
{
	static Int Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID,
		const A1& a1)
	{
		Int const iResult = pEnvironment->CallIntMethod(
			pInstance,
			methodID,
			ToJavaArgument(pEnvironment, a1));
		TestException(pEnvironment);
		return iResult;
	}
};

template <typename A1>
struct Invoker<String, A1, void, void, void, void, void, void, void, void>
{
	static String Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID,
		const A1& a1)
	{
		jstring value = (jstring)pEnvironment->CallObjectMethod(
			pInstance,
			methodID,
			ToJavaArgument(pEnvironment, a1));
		TestException(pEnvironment);
		if (value == nullptr)
		{
			return String();
		}

		Byte const* sValue = pEnvironment->GetStringUTFChars(value, nullptr);
		String sReturn(sValue);
		pEnvironment->ReleaseStringUTFChars(value, sValue);
		return sReturn;
	}
};

template <typename A1>
struct Invoker<jobject, A1, void, void, void, void, void, void, void, void>
{
	static jobject Invoke(
			CheckedPtr<JNIEnv> pEnvironment,
			jobject pInstance,
			jmethodID methodID,
			const A1& a1)
	{
		jobject result = pEnvironment->CallObjectMethod(
				pInstance,
				methodID,
				ToJavaArgument(pEnvironment, a1));
		TestException(pEnvironment);
		return result;
	}
};

template <typename A1, typename A2>
struct Invoker<void, A1, A2, void, void, void, void, void, void, void>
{
	static void Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID,
		const A1& a1,
		const A2& a2)
	{
		pEnvironment->CallVoidMethod(
			pInstance,
			methodID,
			ToJavaArgument(pEnvironment, a1),
			ToJavaArgument(pEnvironment, a2));
		TestException(pEnvironment);
	}
};

template <typename A1, typename A2>
struct Invoker<Bool, A1, A2, void, void, void, void, void, void, void>
{
	static Bool Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID,
		const A1& a1,
		const A2& a2)
	{
		Bool const bResult = (0 != pEnvironment->CallBooleanMethod(
			pInstance,
			methodID,
			ToJavaArgument(pEnvironment, a1),
			ToJavaArgument(pEnvironment, a2)));
		TestException(pEnvironment);
		return bResult;
	}
};

template <typename A1, typename A2>
struct Invoker<Int, A1, A2, void, void, void, void, void, void, void>
{
	static Int Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID,
		const A1& a1,
		const A2& a2)
	{
		Int const iResult = pEnvironment->CallIntMethod(
			pInstance,
			methodID,
			ToJavaArgument(pEnvironment, a1),
			ToJavaArgument(pEnvironment, a2));
		TestException(pEnvironment);
		return iResult;
	}
};

template <typename A1, typename A2>
struct Invoker<String, A1, A2, void, void, void, void, void, void, void>
{
	static String Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID,
		const A1& a1,
		const A2& a2)
	{
		jstring value = (jstring)pEnvironment->CallObjectMethod(
			pInstance,
			methodID,
			ToJavaArgument(pEnvironment, a1),
			ToJavaArgument(pEnvironment, a2));
		TestException(pEnvironment);
		if (value == nullptr)
		{
			return String();
		}

		Byte const* sValue = pEnvironment->GetStringUTFChars(value, nullptr);
		String sReturn(sValue);
		pEnvironment->ReleaseStringUTFChars(value, sValue);
		return sReturn;
	}
};

template <typename A1, typename A2>
struct Invoker<jobject, A1, A2, void, void, void, void, void, void, void>
{
	static jobject Invoke(
			CheckedPtr<JNIEnv> pEnvironment,
			jobject pInstance,
			jmethodID methodID,
			const A1& a1,
			const A2& a2)
	{
		jobject result = pEnvironment->CallObjectMethod(
				pInstance,
				methodID,
				ToJavaArgument(pEnvironment, a1),
				ToJavaArgument(pEnvironment, a2));
		TestException(pEnvironment);
		return result;
	}
};

template <typename A1, typename A2, typename A3>
struct Invoker<void, A1, A2, A3, void, void, void, void, void, void>
{
	static void Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID,
		const A1& a1,
		const A2& a2,
		const A3& a3)
	{
		pEnvironment->CallVoidMethod(
			pInstance,
			methodID,
			ToJavaArgument(pEnvironment, a1),
			ToJavaArgument(pEnvironment, a2),
			ToJavaArgument(pEnvironment, a3));
		TestException(pEnvironment);
	}
};

template <typename A1, typename A2, typename A3>
struct Invoker<Bool, A1, A2, A3, void, void, void, void, void, void>
{
	static Bool Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID,
		const A1& a1,
		const A2& a2,
		const A3& a3)
	{
		Bool const bResult = (0 != pEnvironment->CallBooleanMethod(
			pInstance,
			methodID,
			ToJavaArgument(pEnvironment, a1),
			ToJavaArgument(pEnvironment, a2),
			ToJavaArgument(pEnvironment, a3)));
		TestException(pEnvironment);
		return bResult;
	}
};

template <typename A1, typename A2, typename A3>
struct Invoker<Int, A1, A2, A3, void, void, void, void, void, void>
{
	static Int Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID,
		const A1& a1,
		const A2& a2,
		const A3& a3)
	{
		Int const iResult = pEnvironment->CallIntMethod(
			pInstance,
			methodID,
			ToJavaArgument(pEnvironment, a1),
			ToJavaArgument(pEnvironment, a2),
			ToJavaArgument(pEnvironment, a3));
		TestException(pEnvironment);
		return iResult;
	}
};

template <typename A1, typename A2, typename A3>
struct Invoker<String, A1, A2, A3, void, void, void, void, void, void>
{
	static String Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID,
		const A1& a1,
		const A2& a2,
		const A3& a3)
	{
		jstring value = (jstring)pEnvironment->CallObjectMethod(
			pInstance,
			methodID,
			ToJavaArgument(pEnvironment, a1),
			ToJavaArgument(pEnvironment, a2),
			ToJavaArgument(pEnvironment, a3));
		TestException(pEnvironment);
		if (value == nullptr)
		{
			return String();
		}

		Byte const* sValue = pEnvironment->GetStringUTFChars(value, nullptr);
		String sReturn(sValue);
		pEnvironment->ReleaseStringUTFChars(value, sValue);
		return sReturn;
	}
};

template <typename A1, typename A2, typename A3>
struct Invoker<jobject, A1, A2, A3, void, void, void, void, void, void>
{
	static jobject Invoke(
			CheckedPtr<JNIEnv> pEnvironment,
			jobject pInstance,
			jmethodID methodID,
			const A1& a1,
			const A2& a2,
			const A3& a3)
	{
		jobject result = pEnvironment->CallObjectMethod(
				pInstance,
				methodID,
				ToJavaArgument(pEnvironment, a1),
				ToJavaArgument(pEnvironment, a2),
				ToJavaArgument(pEnvironment, a3));
		TestException(pEnvironment);
		return result;
	}
};

template <typename A1, typename A2, typename A3, typename A4>
struct Invoker<void, A1, A2, A3, A4, void, void, void, void, void>
{
	static void Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4)
	{
		pEnvironment->CallVoidMethod(
			pInstance,
			methodID,
			ToJavaArgument(pEnvironment, a1),
			ToJavaArgument(pEnvironment, a2),
			ToJavaArgument(pEnvironment, a3),
			ToJavaArgument(pEnvironment, a4));
		TestException(pEnvironment);
	}
};

template <typename A1, typename A2, typename A3, typename A4>
struct Invoker<Bool, A1, A2, A3, A4, void, void, void, void, void>
{
	static Bool Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4)
	{
		Bool const bResult = (0 != pEnvironment->CallBooleanMethod(
			pInstance,
			methodID,
			ToJavaArgument(pEnvironment, a1),
			ToJavaArgument(pEnvironment, a2),
			ToJavaArgument(pEnvironment, a3),
			ToJavaArgument(pEnvironment, a4)));
		TestException(pEnvironment);
		return bResult;
	}
};

template <typename A1, typename A2, typename A3, typename A4>
struct Invoker<Int, A1, A2, A3, A4, void, void, void, void, void>
{
	static Int Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4)
	{
		Int const iResult = pEnvironment->CallIntMethod(
			pInstance,
			methodID,
			ToJavaArgument(pEnvironment, a1),
			ToJavaArgument(pEnvironment, a2),
			ToJavaArgument(pEnvironment, a3),
			ToJavaArgument(pEnvironment, a4));
		TestException(pEnvironment);
		return iResult;
	}
};

template <typename A1, typename A2, typename A3, typename A4>
struct Invoker<String, A1, A2, A3, A4, void, void, void, void, void>
{
	static String Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4)
	{
		jstring value = (jstring)pEnvironment->CallObjectMethod(
			pInstance,
			methodID,
			ToJavaArgument(pEnvironment, a1),
			ToJavaArgument(pEnvironment, a2),
			ToJavaArgument(pEnvironment, a3),
			ToJavaArgument(pEnvironment, a4));
		TestException(pEnvironment);
		if (value == nullptr)
		{
			return String();
		}

		Byte const* sValue = pEnvironment->GetStringUTFChars(value, nullptr);
		String sReturn(sValue);
		pEnvironment->ReleaseStringUTFChars(value, sValue);
		return sReturn;
	}
};

template <typename A1, typename A2, typename A3, typename A4>
struct Invoker<jobject, A1, A2, A3, A4, void, void, void, void, void>
{
	static jobject Invoke(
			CheckedPtr<JNIEnv> pEnvironment,
			jobject pInstance,
			jmethodID methodID,
			const A1& a1,
			const A2& a2,
			const A3& a3,
			const A4& a4)
	{
		jobject result = pEnvironment->CallObjectMethod(
				pInstance,
				methodID,
				ToJavaArgument(pEnvironment, a1),
				ToJavaArgument(pEnvironment, a2),
				ToJavaArgument(pEnvironment, a3),
				ToJavaArgument(pEnvironment, a4));
		TestException(pEnvironment);
		return result;
	}
};

template <typename A1, typename A2, typename A3, typename A4, typename A5>
struct Invoker<void, A1, A2, A3, A4, A5, void, void, void, void>
{
	static void Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5)
	{
		pEnvironment->CallVoidMethod(
			pInstance,
			methodID,
			ToJavaArgument(pEnvironment, a1),
			ToJavaArgument(pEnvironment, a2),
			ToJavaArgument(pEnvironment, a3),
			ToJavaArgument(pEnvironment, a4),
			ToJavaArgument(pEnvironment, a5));
		TestException(pEnvironment);
	}
};

template <typename A1, typename A2, typename A3, typename A4, typename A5>
struct Invoker<Bool, A1, A2, A3, A4, A5, void, void, void, void>
{
	static Bool Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5)
	{
		Bool const bResult = (0 != pEnvironment->CallBooleanMethod(
			pInstance,
			methodID,
			ToJavaArgument(pEnvironment, a1),
			ToJavaArgument(pEnvironment, a2),
			ToJavaArgument(pEnvironment, a3),
			ToJavaArgument(pEnvironment, a4),
			ToJavaArgument(pEnvironment, a5)));
		TestException(pEnvironment);
		return bResult;
	}
};

template <typename A1, typename A2, typename A3, typename A4, typename A5>
struct Invoker<Int, A1, A2, A3, A4, A5, void, void, void, void>
{
	static Int Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5)
	{
		Int const iResult = pEnvironment->CallIntMethod(
			pInstance,
			methodID,
			ToJavaArgument(pEnvironment, a1),
			ToJavaArgument(pEnvironment, a2),
			ToJavaArgument(pEnvironment, a3),
			ToJavaArgument(pEnvironment, a4),
			ToJavaArgument(pEnvironment, a5));
		TestException(pEnvironment);
		return iResult;
	}
};

template <typename A1, typename A2, typename A3, typename A4, typename A5>
struct Invoker<String, A1, A2, A3, A4, A5, void, void, void, void>
{
	static String Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5)
	{
		jstring value = (jstring)pEnvironment->CallObjectMethod(
			pInstance,
			methodID,
			ToJavaArgument(pEnvironment, a1),
			ToJavaArgument(pEnvironment, a2),
			ToJavaArgument(pEnvironment, a3),
			ToJavaArgument(pEnvironment, a4),
			ToJavaArgument(pEnvironment, a5));
		TestException(pEnvironment);
		if (value == nullptr)
		{
			return String();
		}

		Byte const* sValue = pEnvironment->GetStringUTFChars(value, nullptr);
		String sReturn(sValue);
		pEnvironment->ReleaseStringUTFChars(value, sValue);
		return sReturn;
	}
};

template <typename A1, typename A2, typename A3, typename A4, typename A5>
struct Invoker<jobject, A1, A2, A3, A4, A5, void, void, void, void>
{
	static jobject Invoke(
			CheckedPtr<JNIEnv> pEnvironment,
			jobject pInstance,
			jmethodID methodID,
			const A1& a1,
			const A2& a2,
			const A3& a3,
			const A4& a4,
			const A5& a5)
	{
		jobject result = pEnvironment->CallObjectMethod(
				pInstance,
				methodID,
				ToJavaArgument(pEnvironment, a1),
				ToJavaArgument(pEnvironment, a2),
				ToJavaArgument(pEnvironment, a3),
				ToJavaArgument(pEnvironment, a4),
				ToJavaArgument(pEnvironment, a5));
		TestException(pEnvironment);
		return result;
	}
};

template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
struct Invoker<void, A1, A2, A3, A4, A5, A6, void, void, void>
{
	static void Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6)
	{
		pEnvironment->CallVoidMethod(
			pInstance,
			methodID,
			ToJavaArgument(pEnvironment, a1),
			ToJavaArgument(pEnvironment, a2),
			ToJavaArgument(pEnvironment, a3),
			ToJavaArgument(pEnvironment, a4),
			ToJavaArgument(pEnvironment, a5),
			ToJavaArgument(pEnvironment, a6));
		TestException(pEnvironment);
	}
};

template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
struct Invoker<Bool, A1, A2, A3, A4, A5, A6, void, void, void>
{
	static Bool Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6)
	{
		Bool const bResult = (0 != pEnvironment->CallBooleanMethod(
			pInstance,
			methodID,
			ToJavaArgument(pEnvironment, a1),
			ToJavaArgument(pEnvironment, a2),
			ToJavaArgument(pEnvironment, a3),
			ToJavaArgument(pEnvironment, a4),
			ToJavaArgument(pEnvironment, a5),
			ToJavaArgument(pEnvironment, a6)));
		TestException(pEnvironment);
		return bResult;
	}
};

template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
struct Invoker<Int, A1, A2, A3, A4, A5, A6, void, void, void>
{
	static Int Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6)
	{
		Int const iResult = pEnvironment->CallIntMethod(
			pInstance,
			methodID,
			ToJavaArgument(pEnvironment, a1),
			ToJavaArgument(pEnvironment, a2),
			ToJavaArgument(pEnvironment, a3),
			ToJavaArgument(pEnvironment, a4),
			ToJavaArgument(pEnvironment, a5),
			ToJavaArgument(pEnvironment, a6));
		TestException(pEnvironment);
		return iResult;
	}
};

template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
struct Invoker<String, A1, A2, A3, A4, A5, A6, void, void, void>
{
	static String Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6)
	{
		jstring value = (jstring)pEnvironment->CallObjectMethod(
			pInstance,
			methodID,
			ToJavaArgument(pEnvironment, a1),
			ToJavaArgument(pEnvironment, a2),
			ToJavaArgument(pEnvironment, a3),
			ToJavaArgument(pEnvironment, a4),
			ToJavaArgument(pEnvironment, a5),
			ToJavaArgument(pEnvironment, a6));
		TestException(pEnvironment);
		if (value == nullptr)
		{
			return String();
		}

		Byte const* sValue = pEnvironment->GetStringUTFChars(value, nullptr);
		String sReturn(sValue);
		pEnvironment->ReleaseStringUTFChars(value, sValue);
		return sReturn;
	}
};

template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
struct Invoker<jobject, A1, A2, A3, A4, A5, A6, void, void, void>
{
	static jobject Invoke(
			CheckedPtr<JNIEnv> pEnvironment,
			jobject pInstance,
			jmethodID methodID,
			const A1& a1,
			const A2& a2,
			const A3& a3,
			const A4& a4,
			const A5& a5,
			const A6& a6)
	{
		jobject result = pEnvironment->CallObjectMethod(
				pInstance,
				methodID,
				ToJavaArgument(pEnvironment, a1),
				ToJavaArgument(pEnvironment, a2),
				ToJavaArgument(pEnvironment, a3),
				ToJavaArgument(pEnvironment, a4),
				ToJavaArgument(pEnvironment, a5),
				ToJavaArgument(pEnvironment, a6));
		TestException(pEnvironment);
		return result;
	}
};

template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
struct Invoker<void, A1, A2, A3, A4, A5, A6, A7, void, void>
{
	static void Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6,
		const A7& a7)
	{
		pEnvironment->CallVoidMethod(
			pInstance,
			methodID,
			ToJavaArgument(pEnvironment, a1),
			ToJavaArgument(pEnvironment, a2),
			ToJavaArgument(pEnvironment, a3),
			ToJavaArgument(pEnvironment, a4),
			ToJavaArgument(pEnvironment, a5),
			ToJavaArgument(pEnvironment, a6),
			ToJavaArgument(pEnvironment, a7));
		TestException(pEnvironment);
	}
};

template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
struct Invoker<Bool, A1, A2, A3, A4, A5, A6, A7, void, void>
{
	static Bool Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6,
		const A7& a7)
	{
		Bool const bResult = (0 != pEnvironment->CallBooleanMethod(
			pInstance,
			methodID,
			ToJavaArgument(pEnvironment, a1),
			ToJavaArgument(pEnvironment, a2),
			ToJavaArgument(pEnvironment, a3),
			ToJavaArgument(pEnvironment, a4),
			ToJavaArgument(pEnvironment, a5),
			ToJavaArgument(pEnvironment, a6),
			ToJavaArgument(pEnvironment, a7)));
		TestException(pEnvironment);
		return bResult;
	}
};

template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
struct Invoker<Int, A1, A2, A3, A4, A5, A6, A7, void, void>
{
	static Int Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6,
		const A7& a7)
	{
		Int const iResult = pEnvironment->CallIntMethod(
			pInstance,
			methodID,
			ToJavaArgument(pEnvironment, a1),
			ToJavaArgument(pEnvironment, a2),
			ToJavaArgument(pEnvironment, a3),
			ToJavaArgument(pEnvironment, a4),
			ToJavaArgument(pEnvironment, a5),
			ToJavaArgument(pEnvironment, a6),
			ToJavaArgument(pEnvironment, a7));
		TestException(pEnvironment);
		return iResult;
	}
};

template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
struct Invoker<String, A1, A2, A3, A4, A5, A6, A7, void, void>
{
	static String Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6,
		const A7& a7)
	{
		jstring value = (jstring)pEnvironment->CallObjectMethod(
			pInstance,
			methodID,
			ToJavaArgument(pEnvironment, a1),
			ToJavaArgument(pEnvironment, a2),
			ToJavaArgument(pEnvironment, a3),
			ToJavaArgument(pEnvironment, a4),
			ToJavaArgument(pEnvironment, a5),
			ToJavaArgument(pEnvironment, a6),
			ToJavaArgument(pEnvironment, a7));
		TestException(pEnvironment);
		if (value == nullptr)
		{
			return String();
		}

		Byte const* sValue = pEnvironment->GetStringUTFChars(value, nullptr);
		String sReturn(sValue);
		pEnvironment->ReleaseStringUTFChars(value, sValue);
		return sReturn;
	}
};

template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
struct Invoker<jobject, A1, A2, A3, A4, A5, A6, A7, void, void>
{
	static jobject Invoke(
			CheckedPtr<JNIEnv> pEnvironment,
			jobject pInstance,
			jmethodID methodID,
			const A1& a1,
			const A2& a2,
			const A3& a3,
			const A4& a4,
			const A5& a5,
			const A6& a6,
			const A7& a7)
	{
		jobject result = pEnvironment->CallObjectMethod(
				pInstance,
				methodID,
				ToJavaArgument(pEnvironment, a1),
				ToJavaArgument(pEnvironment, a2),
				ToJavaArgument(pEnvironment, a3),
				ToJavaArgument(pEnvironment, a4),
				ToJavaArgument(pEnvironment, a5),
				ToJavaArgument(pEnvironment, a6),
				ToJavaArgument(pEnvironment, a7));
		TestException(pEnvironment);
		return result;
	}
};

template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
struct Invoker<void, A1, A2, A3, A4, A5, A6, A7, A8, void>
{
	static void Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6,
		const A7& a7,
		const A8& a8)
	{
		pEnvironment->CallVoidMethod(
			pInstance,
			methodID,
			ToJavaArgument(pEnvironment, a1),
			ToJavaArgument(pEnvironment, a2),
			ToJavaArgument(pEnvironment, a3),
			ToJavaArgument(pEnvironment, a4),
			ToJavaArgument(pEnvironment, a5),
			ToJavaArgument(pEnvironment, a6),
			ToJavaArgument(pEnvironment, a7),
			ToJavaArgument(pEnvironment, a8));
		TestException(pEnvironment);
	}
};

template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
struct Invoker<Bool, A1, A2, A3, A4, A5, A6, A7, A8, void>
{
	static Bool Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6,
		const A7& a7,
		const A8& a8)
	{
		Bool const bResult = (0 != pEnvironment->CallBooleanMethod(
			pInstance,
			methodID,
			ToJavaArgument(pEnvironment, a1),
			ToJavaArgument(pEnvironment, a2),
			ToJavaArgument(pEnvironment, a3),
			ToJavaArgument(pEnvironment, a4),
			ToJavaArgument(pEnvironment, a5),
			ToJavaArgument(pEnvironment, a6),
			ToJavaArgument(pEnvironment, a7),
			ToJavaArgument(pEnvironment, a8)));
		TestException(pEnvironment);
		return bResult;
	}
};

template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
struct Invoker<Int, A1, A2, A3, A4, A5, A6, A7, A8, void>
{
	static Int Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6,
		const A7& a7,
		const A8& a8)
	{
		Int const iResult = pEnvironment->CallIntMethod(
			pInstance,
			methodID,
			ToJavaArgument(pEnvironment, a1),
			ToJavaArgument(pEnvironment, a2),
			ToJavaArgument(pEnvironment, a3),
			ToJavaArgument(pEnvironment, a4),
			ToJavaArgument(pEnvironment, a5),
			ToJavaArgument(pEnvironment, a6),
			ToJavaArgument(pEnvironment, a7),
			ToJavaArgument(pEnvironment, a8));
		TestException(pEnvironment);
		return iResult;
	}
};

template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
struct Invoker<String, A1, A2, A3, A4, A5, A6, A7, A8, void>
{
	static String Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6,
		const A7& a7,
		const A8& a8)
	{
		jstring value = (jstring)pEnvironment->CallObjectMethod(
			pInstance,
			methodID,
			ToJavaArgument(pEnvironment, a1),
			ToJavaArgument(pEnvironment, a2),
			ToJavaArgument(pEnvironment, a3),
			ToJavaArgument(pEnvironment, a4),
			ToJavaArgument(pEnvironment, a5),
			ToJavaArgument(pEnvironment, a6),
			ToJavaArgument(pEnvironment, a7),
			ToJavaArgument(pEnvironment, a8));
		TestException(pEnvironment);
		if (value == nullptr)
		{
			return String();
		}

		Byte const* sValue = pEnvironment->GetStringUTFChars(value, nullptr);
		String sReturn(sValue);
		pEnvironment->ReleaseStringUTFChars(value, sValue);
		return sReturn;
	}
};

template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
struct Invoker<jobject, A1, A2, A3, A4, A5, A6, A7, A8, void>
{
	static jobject Invoke(
			CheckedPtr<JNIEnv> pEnvironment,
			jobject pInstance,
			jmethodID methodID,
			const A1& a1,
			const A2& a2,
			const A3& a3,
			const A4& a4,
			const A5& a5,
			const A6& a6,
			const A7& a7,
			const A8& a8)
	{
		jobject result = pEnvironment->CallObjectMethod(
				pInstance,
				methodID,
				ToJavaArgument(pEnvironment, a1),
				ToJavaArgument(pEnvironment, a2),
				ToJavaArgument(pEnvironment, a3),
				ToJavaArgument(pEnvironment, a4),
				ToJavaArgument(pEnvironment, a5),
				ToJavaArgument(pEnvironment, a6),
				ToJavaArgument(pEnvironment, a7),
				ToJavaArgument(pEnvironment, a8));
		TestException(pEnvironment);
		return result;
	}
};

template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9>
struct Invoker<void, A1, A2, A3, A4, A5, A6, A7, A8, A9>
{
	static void Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6,
		const A7& a7,
		const A8& a8,
		const A9& a9)
	{
		pEnvironment->CallVoidMethod(
			pInstance,
			methodID,
			ToJavaArgument(pEnvironment, a1),
			ToJavaArgument(pEnvironment, a2),
			ToJavaArgument(pEnvironment, a3),
			ToJavaArgument(pEnvironment, a4),
			ToJavaArgument(pEnvironment, a5),
			ToJavaArgument(pEnvironment, a6),
			ToJavaArgument(pEnvironment, a7),
			ToJavaArgument(pEnvironment, a8),
			ToJavaArgument(pEnvironment, a9));
		TestException(pEnvironment);
	}
};

template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9>
struct Invoker<Bool, A1, A2, A3, A4, A5, A6, A7, A8, A9>
{
	static Bool Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6,
		const A7& a7,
		const A8& a8,
		const A9& a9)
	{
		Bool const bResult = (0 != pEnvironment->CallBooleanMethod(
			pInstance,
			methodID,
			ToJavaArgument(pEnvironment, a1),
			ToJavaArgument(pEnvironment, a2),
			ToJavaArgument(pEnvironment, a3),
			ToJavaArgument(pEnvironment, a4),
			ToJavaArgument(pEnvironment, a5),
			ToJavaArgument(pEnvironment, a6),
			ToJavaArgument(pEnvironment, a7),
			ToJavaArgument(pEnvironment, a8),
			ToJavaArgument(pEnvironment, a9)));
		TestException(pEnvironment);
		return bResult;
	}
};

template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9>
struct Invoker<Int, A1, A2, A3, A4, A5, A6, A7, A8, A9>
{
	static Int Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6,
		const A7& a7,
		const A8& a8,
		const A9& a9)
	{
		Int const iResult = pEnvironment->CallIntMethod(
			pInstance,
			methodID,
			ToJavaArgument(pEnvironment, a1),
			ToJavaArgument(pEnvironment, a2),
			ToJavaArgument(pEnvironment, a3),
			ToJavaArgument(pEnvironment, a4),
			ToJavaArgument(pEnvironment, a5),
			ToJavaArgument(pEnvironment, a6),
			ToJavaArgument(pEnvironment, a7),
			ToJavaArgument(pEnvironment, a8),
			ToJavaArgument(pEnvironment, a9));
		TestException(pEnvironment);
		return iResult;
	}
};

template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9>
struct Invoker<String, A1, A2, A3, A4, A5, A6, A7, A8, A9>
{
	static String Invoke(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		jmethodID methodID,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6,
		const A7& a7,
		const A8& a8,
		const A9& a9)
	{
		jstring value = (jstring)pEnvironment->CallObjectMethod(
			pInstance,
			methodID,
			ToJavaArgument(pEnvironment, a1),
			ToJavaArgument(pEnvironment, a2),
			ToJavaArgument(pEnvironment, a3),
			ToJavaArgument(pEnvironment, a4),
			ToJavaArgument(pEnvironment, a5),
			ToJavaArgument(pEnvironment, a6),
			ToJavaArgument(pEnvironment, a7),
			ToJavaArgument(pEnvironment, a8),
			ToJavaArgument(pEnvironment, a9));
		TestException(pEnvironment);
		if (value == nullptr)
		{
			return String();
		}

		Byte const* sValue = pEnvironment->GetStringUTFChars(value, nullptr);
		String sReturn(sValue);
		pEnvironment->ReleaseStringUTFChars(value, sValue);
		return sReturn;
	}
};

template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9>
struct Invoker<jobject, A1, A2, A3, A4, A5, A6, A7, A8, A9>
{
	static jobject Invoke(
			CheckedPtr<JNIEnv> pEnvironment,
			jobject pInstance,
			jmethodID methodID,
			const A1& a1,
			const A2& a2,
			const A3& a3,
			const A4& a4,
			const A5& a5,
			const A6& a6,
			const A7& a7,
			const A8& a8,
			const A9& a9)
	{
		jobject result = pEnvironment->CallObjectMethod(
				pInstance,
				methodID,
				ToJavaArgument(pEnvironment, a1),
				ToJavaArgument(pEnvironment, a2),
				ToJavaArgument(pEnvironment, a3),
				ToJavaArgument(pEnvironment, a4),
				ToJavaArgument(pEnvironment, a5),
				ToJavaArgument(pEnvironment, a6),
				ToJavaArgument(pEnvironment, a7),
				ToJavaArgument(pEnvironment, a8),
				ToJavaArgument(pEnvironment, a9));
		TestException(pEnvironment);
		return result;
	}
};

struct InvokerBinder SEOUL_ABSTRACT
{
	InvokerBinder(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		Byte const* sMethodName,
		Byte const* sMethodSignature)
		: m_pEnvironment(pEnvironment)
		, m_pInstance(pInstance)
		, m_sMethodName(sMethodName)
		, m_sMethodSignature(sMethodSignature)
	{
	}

	void ThreadDoInvoke();

protected:
	void PerformInvoke();
	virtual void DoInvoke() = 0;

	CheckedPtr<JNIEnv> m_pEnvironment;
	jobject m_pInstance;
	Byte const* m_sMethodName;
	Byte const* m_sMethodSignature;
}; // struct InvokerBinder

template <typename R>
struct InvokerBinder0 SEOUL_SEALED : public InvokerBinder
{
	InvokerBinder0(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		Byte const* sMethodName,
		Byte const* sMethodSignature)
		: InvokerBinder(pEnvironment, pInstance, sMethodName, sMethodSignature)
	{
	}

	R Invoke()
	{
		PerformInvoke();
		return m_Result;
	}

private:
	virtual void DoInvoke() SEOUL_OVERRIDE
	{
		m_Result = Invoker<R, void, void, void, void, void, void, void, void, void>::Invoke(
			m_pEnvironment,
			m_pInstance,
			GetMethodID(m_pEnvironment, m_pInstance, m_sMethodName, m_sMethodSignature));
	}

	R m_Result;
}; // struct InvokerBinder0

template <>
struct InvokerBinder0<void> SEOUL_SEALED : public InvokerBinder
{
	InvokerBinder0(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		Byte const* sMethodName,
		Byte const* sMethodSignature)
		: InvokerBinder(pEnvironment, pInstance, sMethodName, sMethodSignature)
	{
	}

	void Invoke()
	{
		PerformInvoke();
	}

private:
	virtual void DoInvoke() SEOUL_OVERRIDE
	{
		Invoker<void, void, void, void, void, void, void, void, void, void>::Invoke(
			m_pEnvironment,
			m_pInstance,
			GetMethodID(m_pEnvironment, m_pInstance, m_sMethodName, m_sMethodSignature));
	}
}; // struct InvokerBinder0

template <typename R, typename A1>
struct InvokerBinder1 SEOUL_SEALED : public InvokerBinder
{
	InvokerBinder1(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		Byte const* sMethodName,
		Byte const* sMethodSignature,
		const A1& a1)
		: InvokerBinder(pEnvironment, pInstance, sMethodName, sMethodSignature)
		, m_A1(a1)
	{
	}

	R Invoke()
	{
		PerformInvoke();
		return m_Result;
	}

private:
	virtual void DoInvoke() SEOUL_OVERRIDE
	{
		m_Result = Invoker<R, A1, void, void, void, void, void, void, void, void>::Invoke(
			m_pEnvironment,
			m_pInstance,
			GetMethodID(m_pEnvironment, m_pInstance, m_sMethodName, m_sMethodSignature),
			m_A1);
	}

	R m_Result;
	A1 m_A1;
}; // struct InvokerBinder1

template <typename A1>
struct InvokerBinder1<void, A1> SEOUL_SEALED : public InvokerBinder
{
	InvokerBinder1(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		Byte const* sMethodName,
		Byte const* sMethodSignature,
		const A1& a1)
		: InvokerBinder(pEnvironment, pInstance, sMethodName, sMethodSignature)
		, m_A1(a1)
	{
	}

	void Invoke()
	{
		PerformInvoke();
	}

private:
	virtual void DoInvoke() SEOUL_OVERRIDE
	{
		Invoker<void, A1, void, void, void, void, void, void, void, void>::Invoke(
			m_pEnvironment,
			m_pInstance,
			GetMethodID(m_pEnvironment, m_pInstance, m_sMethodName, m_sMethodSignature),
			m_A1);
	}

	A1 m_A1;
}; // struct InvokerBinder1

template <typename R, typename A1, typename A2>
struct InvokerBinder2 SEOUL_SEALED : public InvokerBinder
{
	InvokerBinder2(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		Byte const* sMethodName,
		Byte const* sMethodSignature,
		const A1& a1,
		const A2& a2)
		: InvokerBinder(pEnvironment, pInstance, sMethodName, sMethodSignature)
		, m_A1(a1)
		, m_A2(a2)
	{
	}

	R Invoke()
	{
		PerformInvoke();
		return m_Result;
	}

private:
	virtual void DoInvoke() SEOUL_OVERRIDE
	{
		m_Result = Invoker<R, A1, A2, void, void, void, void, void, void, void>::Invoke(
			m_pEnvironment,
			m_pInstance,
			GetMethodID(m_pEnvironment, m_pInstance, m_sMethodName, m_sMethodSignature),
			m_A1,
			m_A2);
	}

	R m_Result;
	A1 m_A1;
	A2 m_A2;
}; // struct InvokerBinder2

template <typename A1, typename A2>
struct InvokerBinder2<void, A1, A2> SEOUL_SEALED : public InvokerBinder
{
	InvokerBinder2(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		Byte const* sMethodName,
		Byte const* sMethodSignature,
		const A1& a1,
		const A2& a2)
		: InvokerBinder(pEnvironment, pInstance, sMethodName, sMethodSignature)
		, m_A1(a1)
		, m_A2(a2)
	{
	}

	void Invoke()
	{
		PerformInvoke();
	}

private:
	virtual void DoInvoke() SEOUL_OVERRIDE
	{
		Invoker<void, A1, A2, void, void, void, void, void, void, void>::Invoke(
			m_pEnvironment,
			m_pInstance,
			GetMethodID(m_pEnvironment, m_pInstance, m_sMethodName, m_sMethodSignature),
			m_A1,
			m_A2);
	}

	A1 m_A1;
	A2 m_A2;
}; // struct InvokerBinder2

template <typename R, typename A1, typename A2, typename A3>
struct InvokerBinder3 SEOUL_SEALED : public InvokerBinder
{
	InvokerBinder3(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		Byte const* sMethodName,
		Byte const* sMethodSignature,
		const A1& a1,
		const A2& a2,
		const A3& a3)
		: InvokerBinder(pEnvironment, pInstance, sMethodName, sMethodSignature)
		, m_A1(a1)
		, m_A2(a2)
		, m_A3(a3)
	{
	}

	R Invoke()
	{
		PerformInvoke();
		return m_Result;
	}

private:
	virtual void DoInvoke() SEOUL_OVERRIDE
	{
		m_Result = Invoker<R, A1, A2, A3, void, void, void, void, void, void>::Invoke(
			m_pEnvironment,
			m_pInstance,
			GetMethodID(m_pEnvironment, m_pInstance, m_sMethodName, m_sMethodSignature),
			m_A1,
			m_A2,
			m_A3);
	}

	R m_Result;
	A1 m_A1;
	A2 m_A2;
	A3 m_A3;
}; // struct InvokerBinder3

template <typename A1, typename A2, typename A3>
struct InvokerBinder3<void, A1, A2, A3> SEOUL_SEALED : public InvokerBinder
{
	InvokerBinder3(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		Byte const* sMethodName,
		Byte const* sMethodSignature,
		const A1& a1,
		const A2& a2,
		const A3& a3)
		: InvokerBinder(pEnvironment, pInstance, sMethodName, sMethodSignature)
		, m_A1(a1)
		, m_A2(a2)
		, m_A3(a3)
	{
	}

	void Invoke()
	{
		PerformInvoke();
	}

private:
	virtual void DoInvoke() SEOUL_OVERRIDE
	{
		Invoker<void, A1, A2, A3, void, void, void, void, void, void>::Invoke(
			m_pEnvironment,
			m_pInstance,
			GetMethodID(m_pEnvironment, m_pInstance, m_sMethodName, m_sMethodSignature),
			m_A1,
			m_A2,
			m_A3);
	}

	A1 m_A1;
	A2 m_A2;
	A3 m_A3;
}; // struct InvokerBinder3

template <typename R, typename A1, typename A2, typename A3, typename A4>
struct InvokerBinder4 SEOUL_SEALED : public InvokerBinder
{
	InvokerBinder4(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		Byte const* sMethodName,
		Byte const* sMethodSignature,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4)
		: InvokerBinder(pEnvironment, pInstance, sMethodName, sMethodSignature)
		, m_A1(a1)
		, m_A2(a2)
		, m_A3(a3)
		, m_A4(a4)
	{
	}

	R Invoke()
	{
		PerformInvoke();
		return m_Result;
	}

private:
	virtual void DoInvoke() SEOUL_OVERRIDE
	{
		m_Result = Invoker<R, A1, A2, A3, A4, void, void, void, void, void>::Invoke(
			m_pEnvironment,
			m_pInstance,
			GetMethodID(m_pEnvironment, m_pInstance, m_sMethodName, m_sMethodSignature),
			m_A1,
			m_A2,
			m_A3,
			m_A4);
	}

	R m_Result;
	A1 m_A1;
	A2 m_A2;
	A3 m_A3;
	A4 m_A4;
}; // struct InvokerBinder4

template <typename A1, typename A2, typename A3, typename A4>
struct InvokerBinder4<void, A1, A2, A3, A4> SEOUL_SEALED : public InvokerBinder
{
	InvokerBinder4(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		Byte const* sMethodName,
		Byte const* sMethodSignature,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4)
		: InvokerBinder(pEnvironment, pInstance, sMethodName, sMethodSignature)
		, m_A1(a1)
		, m_A2(a2)
		, m_A3(a3)
		, m_A4(a4)
	{
	}

	void Invoke()
	{
		PerformInvoke();
	}

private:
	virtual void DoInvoke() SEOUL_OVERRIDE
	{
		Invoker<void, A1, A2, A3, A4, void, void, void, void, void>::Invoke(
			m_pEnvironment,
			m_pInstance,
			GetMethodID(m_pEnvironment, m_pInstance, m_sMethodName, m_sMethodSignature),
			m_A1,
			m_A2,
			m_A3,
			m_A4);
	}

	A1 m_A1;
	A2 m_A2;
	A3 m_A3;
	A4 m_A4;
}; // struct InvokerBinder4

template <typename R, typename A1, typename A2, typename A3, typename A4, typename A5>
struct InvokerBinder5 SEOUL_SEALED : public InvokerBinder
{
	InvokerBinder5(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		Byte const* sMethodName,
		Byte const* sMethodSignature,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5)
		: InvokerBinder(pEnvironment, pInstance, sMethodName, sMethodSignature)
		, m_A1(a1)
		, m_A2(a2)
		, m_A3(a3)
		, m_A4(a4)
		, m_A5(a5)
	{
	}

	R Invoke()
	{
		PerformInvoke();
		return m_Result;
	}

private:
	virtual void DoInvoke() SEOUL_OVERRIDE
	{
		m_Result = Invoker<R, A1, A2, A3, A4, A5, void, void, void, void>::Invoke(
			m_pEnvironment,
			m_pInstance,
			GetMethodID(m_pEnvironment, m_pInstance, m_sMethodName, m_sMethodSignature),
			m_A1,
			m_A2,
			m_A3,
			m_A4,
			m_A5);
	}

	R m_Result;
	A1 m_A1;
	A2 m_A2;
	A3 m_A3;
	A4 m_A4;
	A5 m_A5;
}; // struct InvokerBinder5

template <typename A1, typename A2, typename A3, typename A4, typename A5>
struct InvokerBinder5<void, A1, A2, A3, A4, A5> SEOUL_SEALED : public InvokerBinder
{
	InvokerBinder5(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		Byte const* sMethodName,
		Byte const* sMethodSignature,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5)
		: InvokerBinder(pEnvironment, pInstance, sMethodName, sMethodSignature)
		, m_A1(a1)
		, m_A2(a2)
		, m_A3(a3)
		, m_A4(a4)
		, m_A5(a5)
	{
	}

	void Invoke()
	{
		PerformInvoke();
	}

private:
	virtual void DoInvoke() SEOUL_OVERRIDE
	{
		Invoker<void, A1, A2, A3, A4, A5, void, void, void, void>::Invoke(
			m_pEnvironment,
			m_pInstance,
			GetMethodID(m_pEnvironment, m_pInstance, m_sMethodName, m_sMethodSignature),
			m_A1,
			m_A2,
			m_A3,
			m_A4,
			m_A5);
	}

	A1 m_A1;
	A2 m_A2;
	A3 m_A3;
	A4 m_A4;
	A5 m_A5;
}; // struct InvokerBinder5

template <typename R, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
struct InvokerBinder6 SEOUL_SEALED : public InvokerBinder
{
	InvokerBinder6(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		Byte const* sMethodName,
		Byte const* sMethodSignature,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6)
		: InvokerBinder(pEnvironment, pInstance, sMethodName, sMethodSignature)
		, m_A1(a1)
		, m_A2(a2)
		, m_A3(a3)
		, m_A4(a4)
		, m_A5(a5)
		, m_A6(a6)
	{
	}

	R Invoke()
	{
		PerformInvoke();
		return m_Result;
	}

private:
	virtual void DoInvoke() SEOUL_OVERRIDE
	{
		m_Result = Invoker<R, A1, A2, A3, A4, A5, A6, void, void, void>::Invoke(
			m_pEnvironment,
			m_pInstance,
			GetMethodID(m_pEnvironment, m_pInstance, m_sMethodName, m_sMethodSignature),
			m_A1,
			m_A2,
			m_A3,
			m_A4,
			m_A5,
			m_A6);
	}

	R m_Result;
	A1 m_A1;
	A2 m_A2;
	A3 m_A3;
	A4 m_A4;
	A5 m_A5;
	A6 m_A6;
}; // struct InvokerBinder6

template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
struct InvokerBinder6<void, A1, A2, A3, A4, A5, A6> SEOUL_SEALED : public InvokerBinder
{
	InvokerBinder6(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		Byte const* sMethodName,
		Byte const* sMethodSignature,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6)
		: InvokerBinder(pEnvironment, pInstance, sMethodName, sMethodSignature)
		, m_A1(a1)
		, m_A2(a2)
		, m_A3(a3)
		, m_A4(a4)
		, m_A5(a5)
		, m_A6(a6)
	{
	}

	void Invoke()
	{
		PerformInvoke();
	}

private:
	virtual void DoInvoke() SEOUL_OVERRIDE
	{
		Invoker<void, A1, A2, A3, A4, A5, A6, void, void, void>::Invoke(
			m_pEnvironment,
			m_pInstance,
			GetMethodID(m_pEnvironment, m_pInstance, m_sMethodName, m_sMethodSignature),
			m_A1,
			m_A2,
			m_A3,
			m_A4,
			m_A5,
			m_A6);
	}

	A1 m_A1;
	A2 m_A2;
	A3 m_A3;
	A4 m_A4;
	A5 m_A5;
	A6 m_A6;
}; // struct InvokerBinder6

template <typename R, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
struct InvokerBinder7 SEOUL_SEALED : public InvokerBinder
{
	InvokerBinder7(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		Byte const* sMethodName,
		Byte const* sMethodSignature,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6,
		const A7& a7)
		: InvokerBinder(pEnvironment, pInstance, sMethodName, sMethodSignature)
		, m_A1(a1)
		, m_A2(a2)
		, m_A3(a3)
		, m_A4(a4)
		, m_A5(a5)
		, m_A6(a6)
		, m_A7(a7)
	{
	}

	R Invoke()
	{
		PerformInvoke();
		return m_Result;
	}

private:
	virtual void DoInvoke() SEOUL_OVERRIDE
	{
		m_Result = Invoker<R, A1, A2, A3, A4, A5, A6, A7, void, void>::Invoke(
			m_pEnvironment,
			m_pInstance,
			GetMethodID(m_pEnvironment, m_pInstance, m_sMethodName, m_sMethodSignature),
			m_A1,
			m_A2,
			m_A3,
			m_A4,
			m_A5,
			m_A6,
			m_A7);
	}

	R m_Result;
	A1 m_A1;
	A2 m_A2;
	A3 m_A3;
	A4 m_A4;
	A5 m_A5;
	A6 m_A6;
	A7 m_A7;
}; // struct InvokerBinder7

template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
struct InvokerBinder7<void, A1, A2, A3, A4, A5, A6, A7> SEOUL_SEALED : public InvokerBinder
{
	InvokerBinder7(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		Byte const* sMethodName,
		Byte const* sMethodSignature,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6,
		const A7& a7)
		: InvokerBinder(pEnvironment, pInstance, sMethodName, sMethodSignature)
		, m_A1(a1)
		, m_A2(a2)
		, m_A3(a3)
		, m_A4(a4)
		, m_A5(a5)
		, m_A6(a6)
		, m_A7(a7)
	{
	}

	void Invoke()
	{
		PerformInvoke();
	}

private:
	virtual void DoInvoke() SEOUL_OVERRIDE
	{
		Invoker<void, A1, A2, A3, A4, A5, A6, A7, void, void>::Invoke(
			m_pEnvironment,
			m_pInstance,
			GetMethodID(m_pEnvironment, m_pInstance, m_sMethodName, m_sMethodSignature),
			m_A1,
			m_A2,
			m_A3,
			m_A4,
			m_A5,
			m_A6,
			m_A7);
	}

	A1 m_A1;
	A2 m_A2;
	A3 m_A3;
	A4 m_A4;
	A5 m_A5;
	A6 m_A6;
	A7 m_A7;
}; // struct InvokerBinder7

template <typename R, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
struct InvokerBinder8 SEOUL_SEALED : public InvokerBinder
{
	InvokerBinder8(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		Byte const* sMethodName,
		Byte const* sMethodSignature,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6,
		const A7& a7,
		const A8& a8)
		: InvokerBinder(pEnvironment, pInstance, sMethodName, sMethodSignature)
		, m_A1(a1)
		, m_A2(a2)
		, m_A3(a3)
		, m_A4(a4)
		, m_A5(a5)
		, m_A6(a6)
		, m_A7(a7)
		, m_A8(a8)
	{
	}

	R Invoke()
	{
		PerformInvoke();
		return m_Result;
	}

private:
	virtual void DoInvoke() SEOUL_OVERRIDE
	{
		m_Result = Invoker<R, A1, A2, A3, A4, A5, A6, A7, A8, void>::Invoke(
			m_pEnvironment,
			m_pInstance,
			GetMethodID(m_pEnvironment, m_pInstance, m_sMethodName, m_sMethodSignature),
			m_A1,
			m_A2,
			m_A3,
			m_A4,
			m_A5,
			m_A6,
			m_A7,
			m_A8);
	}

	R m_Result;
	A1 m_A1;
	A2 m_A2;
	A3 m_A3;
	A4 m_A4;
	A5 m_A5;
	A6 m_A6;
	A7 m_A7;
	A8 m_A8;
}; // struct InvokerBinder8

template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
struct InvokerBinder8<void, A1, A2, A3, A4, A5, A6, A7, A8> SEOUL_SEALED : public InvokerBinder
{
	InvokerBinder8(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		Byte const* sMethodName,
		Byte const* sMethodSignature,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6,
		const A7& a7,
		const A8& a8)
		: InvokerBinder(pEnvironment, pInstance, sMethodName, sMethodSignature)
		, m_A1(a1)
		, m_A2(a2)
		, m_A3(a3)
		, m_A4(a4)
		, m_A5(a5)
		, m_A6(a6)
		, m_A7(a7)
		, m_A8(a8)
	{
	}

	void Invoke()
	{
		PerformInvoke();
	}

private:
	virtual void DoInvoke() SEOUL_OVERRIDE
	{
		Invoker<void, A1, A2, A3, A4, A5, A6, A7, A8, void>::Invoke(
			m_pEnvironment,
			m_pInstance,
			GetMethodID(m_pEnvironment, m_pInstance, m_sMethodName, m_sMethodSignature),
			m_A1,
			m_A2,
			m_A3,
			m_A4,
			m_A5,
			m_A6,
			m_A7,
			m_A8);
	}

	A1 m_A1;
	A2 m_A2;
	A3 m_A3;
	A4 m_A4;
	A5 m_A5;
	A6 m_A6;
	A7 m_A7;
	A8 m_A8;
}; // struct InvokerBinder8

template <typename R, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9>
struct InvokerBinder9 SEOUL_SEALED : public InvokerBinder
{
	InvokerBinder9(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		Byte const* sMethodName,
		Byte const* sMethodSignature,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6,
		const A7& a7,
		const A8& a8,
		const A9& a9)
		: InvokerBinder(pEnvironment, pInstance, sMethodName, sMethodSignature)
		, m_A1(a1)
		, m_A2(a2)
		, m_A3(a3)
		, m_A4(a4)
		, m_A5(a5)
		, m_A6(a6)
		, m_A7(a7)
		, m_A8(a8)
		, m_A9(a9)
	{
	}

	R Invoke()
	{
		PerformInvoke();
		return m_Result;
	}

private:
	virtual void DoInvoke() SEOUL_OVERRIDE
	{
		m_Result = Invoker<R, A1, A2, A3, A4, A5, A6, A7, A8, A9>::Invoke(
			m_pEnvironment,
			m_pInstance,
			GetMethodID(m_pEnvironment, m_pInstance, m_sMethodName, m_sMethodSignature),
			m_A1,
			m_A2,
			m_A3,
			m_A4,
			m_A5,
			m_A6,
			m_A7,
			m_A8,
			m_A9);
	}

	R m_Result;
	A1 m_A1;
	A2 m_A2;
	A3 m_A3;
	A4 m_A4;
	A5 m_A5;
	A6 m_A6;
	A7 m_A7;
	A8 m_A8;
	A9 m_A9;
}; // struct InvokerBinder9

template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9>
struct InvokerBinder9<void, A1, A2, A3, A4, A5, A6, A7, A8, A9> SEOUL_SEALED : public InvokerBinder
{
	InvokerBinder9(
		CheckedPtr<JNIEnv> pEnvironment,
		jobject pInstance,
		Byte const* sMethodName,
		Byte const* sMethodSignature,
		const A1& a1,
		const A2& a2,
		const A3& a3,
		const A4& a4,
		const A5& a5,
		const A6& a6,
		const A7& a7,
		const A8& a8,
		const A9& a9)
		: InvokerBinder(pEnvironment, pInstance, sMethodName, sMethodSignature)
		, m_A1(a1)
		, m_A2(a2)
		, m_A3(a3)
		, m_A4(a4)
		, m_A5(a5)
		, m_A6(a6)
		, m_A7(a7)
		, m_A8(a8)
		, m_A9(a9)
	{
	}

	void Invoke()
	{
		PerformInvoke();
	}

private:
	virtual void DoInvoke() SEOUL_OVERRIDE
	{
		Invoker<void, A1, A2, A3, A4, A5, A6, A7, A8, A9>::Invoke(
			m_pEnvironment,
			m_pInstance,
			GetMethodID(m_pEnvironment, m_pInstance, m_sMethodName, m_sMethodSignature),
			m_A1,
			m_A2,
			m_A3,
			m_A4,
			m_A5,
			m_A6,
			m_A7,
			m_A8,
			m_A9);
	}

	A1 m_A1;
	A2 m_A2;
	A3 m_A3;
	A4 m_A4;
	A5 m_A5;
	A6 m_A6;
	A7 m_A7;
	A8 m_A8;
	A9 m_A9;
}; // struct InvokerBinder9

template <typename R>
inline R Invoke(
	CheckedPtr<JNIEnv> pEnvironment,
	jobject pInstance,
	Byte const* sMethodName,
	Byte const* sMethodSignature)
{
	InvokerBinder0<R> binder(
		pEnvironment,
		pInstance,
		sMethodName,
		sMethodSignature);

	return binder.Invoke();
}

template <typename R, typename A1>
inline R Invoke(
	CheckedPtr<JNIEnv> pEnvironment,
	jobject pInstance,
	Byte const* sMethodName,
	Byte const* sMethodSignature,
	const A1& a1)
{
	InvokerBinder1<R, A1> binder(
		pEnvironment,
		pInstance,
		sMethodName,
		sMethodSignature,
		a1);

	return binder.Invoke();
}

template <typename R, typename A1, typename A2>
inline R Invoke(
	CheckedPtr<JNIEnv> pEnvironment,
	jobject pInstance,
	Byte const* sMethodName,
	Byte const* sMethodSignature,
	const A1& a1,
	const A2& a2)
{
	InvokerBinder2<R, A1, A2> binder(
		pEnvironment,
		pInstance,
		sMethodName,
		sMethodSignature,
		a1,
		a2);

	return binder.Invoke();
}

template <typename R, typename A1, typename A2, typename A3>
inline R Invoke(
	CheckedPtr<JNIEnv> pEnvironment,
	jobject pInstance,
	Byte const* sMethodName,
	Byte const* sMethodSignature,
	const A1& a1,
	const A2& a2,
	const A3& a3)
{
	InvokerBinder3<R, A1, A2, A3> binder(
		pEnvironment,
		pInstance,
		sMethodName,
		sMethodSignature,
		a1,
		a2,
		a3);

	return binder.Invoke();
}

template <typename R, typename A1, typename A2, typename A3, typename A4>
inline R Invoke(
	CheckedPtr<JNIEnv> pEnvironment,
	jobject pInstance,
	Byte const* sMethodName,
	Byte const* sMethodSignature,
	const A1& a1,
	const A2& a2,
	const A3& a3,
	const A4& a4)
{
	InvokerBinder4<R, A1, A2, A3, A4> binder(
		pEnvironment,
		pInstance,
		sMethodName,
		sMethodSignature,
		a1,
		a2,
		a3,
		a4);

	return binder.Invoke();
}

template <typename R, typename A1, typename A2, typename A3, typename A4, typename A5>
inline R Invoke(
	CheckedPtr<JNIEnv> pEnvironment,
	jobject pInstance,
	Byte const* sMethodName,
	Byte const* sMethodSignature,
	const A1& a1,
	const A2& a2,
	const A3& a3,
	const A4& a4,
	const A5& a5)
{
	InvokerBinder5<R, A1, A2, A3, A4, A5> binder(
		pEnvironment,
		pInstance,
		sMethodName,
		sMethodSignature,
		a1,
		a2,
		a3,
		a4,
		a5);

	return binder.Invoke();
}

template <typename R, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
inline R Invoke(
	CheckedPtr<JNIEnv> pEnvironment,
	jobject pInstance,
	Byte const* sMethodName,
	Byte const* sMethodSignature,
	const A1& a1,
	const A2& a2,
	const A3& a3,
	const A4& a4,
	const A5& a5,
	const A6& a6)
{
	InvokerBinder6<R, A1, A2, A3, A4, A5, A6> binder(
		pEnvironment,
		pInstance,
		sMethodName,
		sMethodSignature,
		a1,
		a2,
		a3,
		a4,
		a5,
		a6);

	return binder.Invoke();
}

template <typename R, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7>
inline R Invoke(
	CheckedPtr<JNIEnv> pEnvironment,
	jobject pInstance,
	Byte const* sMethodName,
	Byte const* sMethodSignature,
	const A1& a1,
	const A2& a2,
	const A3& a3,
	const A4& a4,
	const A5& a5,
	const A6& a6,
	const A7& a7)
{
	InvokerBinder7<R, A1, A2, A3, A4, A5, A6, A7> binder(
		pEnvironment,
		pInstance,
		sMethodName,
		sMethodSignature,
		a1,
		a2,
		a3,
		a4,
		a5,
		a6,
		a7);

	return binder.Invoke();
}

template <typename R, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8>
inline R Invoke(
	CheckedPtr<JNIEnv> pEnvironment,
	jobject pInstance,
	Byte const* sMethodName,
	Byte const* sMethodSignature,
	const A1& a1,
	const A2& a2,
	const A3& a3,
	const A4& a4,
	const A5& a5,
	const A6& a6,
	const A7& a7,
	const A8& a8)
{
	InvokerBinder8<R, A1, A2, A3, A4, A5, A6, A7, A8> binder(
		pEnvironment,
		pInstance,
		sMethodName,
		sMethodSignature,
		a1,
		a2,
		a3,
		a4,
		a5,
		a6,
		a7,
		a8);

	return binder.Invoke();
}

template <typename R, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6, typename A7, typename A8, typename A9>
inline R Invoke(
	CheckedPtr<JNIEnv> pEnvironment,
	jobject pInstance,
	Byte const* sMethodName,
	Byte const* sMethodSignature,
	const A1& a1,
	const A2& a2,
	const A3& a3,
	const A4& a4,
	const A5& a5,
	const A6& a6,
	const A7& a7,
	const A8& a8,
	const A9& a9)
{
	InvokerBinder9<R, A1, A2, A3, A4, A5, A6, A7, A8, A9> binder(
		pEnvironment,
		pInstance,
		sMethodName,
		sMethodSignature,
		a1,
		a2,
		a3,
		a4,
		a5,
		a6,
		a7,
		a8,
		a9);

	return binder.Invoke();
}

}  // namespace Java

inline static void SetStringFromJava(
	JNIEnv* pJniEnvironment,
	jstring inputString,
	String& rOutputString)
{
	if (inputString == nullptr)
	{
		rOutputString.Clear();
		return;
	}

	const Byte *sInputString = pJniEnvironment->GetStringUTFChars(inputString, nullptr);
	rOutputString.Assign(sInputString);

	pJniEnvironment->ReleaseStringUTFChars(inputString, sInputString);
}

inline static void SetStringFromJavaObjectField(
	JNIEnv* pJniEnvironment,
	jobject obj,
	Byte const* sName,
	String& rs)
{
	// Get the class of the object.
	auto const cls = pJniEnvironment->GetObjectClass(obj);

	// Get the field identifier.
	auto const fid = pJniEnvironment->GetFieldID(cls, sName, "Ljava/lang/String;");
	SEOUL_ASSERT(0 != fid);

	// Get the field data.
	auto const jstr = (jstring)pJniEnvironment->GetObjectField(obj, fid);
	SetStringFromJava(pJniEnvironment, jstr, rs);
	
	// Clean up local references
	pJniEnvironment->DeleteLocalRef(jstr);
	pJniEnvironment->DeleteLocalRef(cls);
}

inline static Int64 GetInt64FromJavaObjectField(
	JNIEnv* pJniEnvironment,
	jobject obj,
	Byte const* sName)
{
	// Get the class of the object.
	auto const cls = pJniEnvironment->GetObjectClass(obj);

	// Get the field identifier.
	auto const fid = pJniEnvironment->GetFieldID(cls, sName, "J");
	SEOUL_ASSERT(0 != fid);

	// Get the field data.
	auto const jlongTemp = pJniEnvironment->GetLongField(obj, fid);
	// Cast to Int64
	Int64 lRet = (Int64)jlongTemp;
	
	// Clean up local references
	pJniEnvironment->DeleteLocalRef(cls);

	return lRet;
}

inline static void SetProductIDFromJava(
	JNIEnv* pJniEnvironment,
	jstring inputString,
	CommerceManager::ProductID& rProductID)
{
	String sProductID;
	SetStringFromJava(pJniEnvironment, inputString, sProductID);
	if (!sProductID.IsEmpty())
	{
		rProductID = CommerceManager::ProductID(sProductID);
	}
	else
	{
		rProductID = CommerceManager::ProductID();
	}
}

inline static void GetEnumOrdinalFromJavaObjectField(
	JNIEnv* pJniEnvironment,
	jobject obj,
	Byte const* sName,
	Byte const* sEnumSig,
	Int& ri)
{
	// Get the class of the object.
	auto const cls = pJniEnvironment->GetObjectClass(obj);
	
	// Get the field identifier for the enum.
	auto const fid = pJniEnvironment->GetFieldID(cls, sName, sEnumSig);
	SEOUL_ASSERT(0 != fid);
	
	// Get the enum field.
	jobject jenum = pJniEnvironment->GetObjectField(obj, fid);
	
	// Get the ordinal method for the enum field.
	auto ordinalMethod = Java::GetMethodID(pJniEnvironment, jenum, "ordinal", "()I");
	
	// Invoke the ordinal method
	jint value = pJniEnvironment->CallIntMethod(jenum, ordinalMethod);
	
	// Cast to Int.
	ri = (Int)value;
	
	// Clean up local references
	pJniEnvironment->DeleteLocalRef(jenum);
	pJniEnvironment->DeleteLocalRef(cls);
}

} // namespace Seoul

#endif // include guard
