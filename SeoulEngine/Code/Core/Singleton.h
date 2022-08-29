/**
 * \file Singleton.h
 * \brief Base class used to enforce the Singleton design pattern. Ensures that
 * a class which inherits from it can only have a single instance at any
 * time in the current application.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef SINGLETON_H
#define SINGLETON_H

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
class Singleton SEOUL_ABSTRACT
{
public:
	/**
	 * @return The global singleton instance. Will be nullptr if that instance has not
	 * yet been created.
	 */
	static CheckedPtr<T> Get()
	{
		return s_pSingleton;
	}

	static CheckedPtr<const T> GetConst()
	{
		return s_pSingleton;
	}

protected:
	Singleton()
	{
		// Sanity check that singletons are being handled as required.
		SEOUL_ASSERT(!s_pSingleton.IsValid());
		s_pSingleton = static_cast<T*>(this);
	}

	// Intentionally not polymorphic - never delete subclasses with
	// a Singleton<> pointer.
	~Singleton()
	{
		ReleaseSingleton();
	}

	// Can be used by subclasses which need to invalidate their Singleton
	// status before entering the base destructor. Typically this
	// is for Singletons that will be accessed from multiple threads.
	void ReleaseSingleton()
	{
		// Sanity check that singletons are being handled as required.
		SEOUL_ASSERT(nullptr == s_pSingleton || this == s_pSingleton);
		s_pSingleton = nullptr;
	}

private:
	static CheckedPtr<T> s_pSingleton;

	SEOUL_DISABLE_COPY(Singleton);
}; // class Singleton

// Initialize the global singleton to nullptr.
template <typename T> CheckedPtr<T> Singleton<T>::s_pSingleton(nullptr);

} // namespace Seoul

#endif // include guard
