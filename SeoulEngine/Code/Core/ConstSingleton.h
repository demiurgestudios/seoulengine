/**
 * \file ConstSingleton.h
 * \brief Base class used to enforce the Singleton design pattern with a read only reference.
 * Ensures that a class which inherits from it can only have a single instance at any
 * time in the current application.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef CONST_SINGLETON_H
#define CONST_SINGLETON_H

#include "CheckedPtr.h"

namespace Seoul
{

/**
 * All classes that want to obey the Singleton pattern should inherit from
 * and specialize this template class.
 *
 * Singleton only enforces "single instance", it does not implement the Meyer
 * singleton pattern, which ensures one and only one instance always exists when it
 * is requested.
 */
template <typename T>
class ConstSingleton SEOUL_ABSTRACT
{
public:
	/**
	 * @return The global singleton instance. Will be nullptr if that instance has not
	 * yet been created.
	 */
	static CheckedPtr<const T> Get()
	{
		return s_pSingleton;
	}

	static CheckedPtr<const T> GetConst()
	{
		return s_pSingleton;
	}

protected:
	ConstSingleton()
	{
		// Sanity check that singletons are being handled as required.
		SEOUL_ASSERT(!s_pSingleton.IsValid());
		s_pSingleton = static_cast<const T*>(this);
	}

	// Intentionally not polymorphic - never delete subclasses with
	// a Singleton<> pointer.
	~ConstSingleton()
	{
		ReleaseSingleton();
	}

	// Can be used by subclasses which need to invalidate their Singleton
	// status before entering the base destructor. Typically this
	// is for Singletons that will be accessed from multiple threads.
	void ReleaseSingleton()
	{
		// Sanity check that singletons are being handled as required.
		SEOUL_ASSERT(nullptr == s_pSingleton || static_cast<const T*>(this) == s_pSingleton);
		s_pSingleton = nullptr;
	}

private:
	static CheckedPtr<const T> s_pSingleton;

	SEOUL_DISABLE_COPY(ConstSingleton);
}; // class ConstSingleton

// Initialize the global singleton to nullptr.
template <typename T> CheckedPtr<const T> ConstSingleton<T>::s_pSingleton(nullptr);

} // namespace Seoul

#endif // include guard
