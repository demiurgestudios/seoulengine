/**
 * \file GameClient.h
 * \brief Common game client class for HTTP RESTful communication with a server.
 * Higher level utility functions around Seoul::HTTP that define specific, game
 * agnostic requests and handling.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GAME_CLIENT_H
#define GAME_CLIENT_H

#include "Engine.h"
#include "HTTPCommon.h"
#include "HashTable.h"
#include "Mutex.h"
#include "Prereqs.h"
#include "Singleton.h"
namespace Seoul { namespace HTTP { class HeaderTable; } }
namespace Seoul { namespace HTTP { class RequestList; } }
namespace Seoul { namespace HTTP { class Response; } }

namespace Seoul
{

// Forward declarations
class DataStore;
namespace HTTP { class Response; }
template <typename T> class SharedPtr;
namespace Reflection { class WeakAny; }

namespace Game
{

struct ClientVersionRequestData;

class Client SEOUL_SEALED : public Singleton<Client>
{
public:
	class CacheEntry SEOUL_SEALED
	{
	public:
		CacheEntry(HTTP::Response* pResponse);
		~CacheEntry();

		/** Replaces the cached body with new contents */
		void ReplaceBody(String const& newBody);

		/** @return A pointer to the body data of the cached request. */
		void const* GetBody() const
		{
			return m_pBody;
		}

		/** @return The size of the body data in bytes. */
		UInt32 GetBodySize() const
		{
			return m_uBodySize;
		}

		/** @return The table of headers that were returned with the cached request result. */
		const HTTP::HeaderTable& GetHeaders() const
		{
			return *m_pHeaders;
		}

	private:
		void* m_pBody;
		UInt32 m_uBodySize;
		ScopedPtr<HTTP::HeaderTable> m_pHeaders;

		SEOUL_DISABLE_COPY(CacheEntry);
	}; // class CacheEntry
	typedef HashTable<String, CacheEntry*, MemoryBudgets::Network> Cache;

	Client();
	~Client();

	// Cancel any HTTP requests issued by Game::Client that are still pending.
	void CancelPendingRequests();

	// Create a Game::Client customized request object.
	HTTP::Request& CreateRequest(
		const String& sURL,
		const HTTP::ResponseDelegate& callback,
		HString sMethod = HTTP::Method::ksGet,
		Bool bResendOnFailure = true,
		Bool bSuppressErrorMail = false);

	// Add standard API request headers, like auth and language
	void PrepareRequest(HTTP::Request& rRequest, Bool bSuppressErrorMail) const;

	// Sets the auth token for API requests
	void SetAuthToken(String const& sToken);
	String const& GetAuthToken() const;

#if SEOUL_WITH_REMOTE_NOTIFICATIONS
	void RemoteNotificationTokenChanged(RemoteNotificationType eType, Bool bIsDevelopmentEnvironment, const String& sDeviceToken);
	void RequestRemoteNotificationsIfSilent();
#endif

	// Helpers for serialization/deserialization
	static Bool SerializeToJson(const Reflection::WeakAny& object, String& sResult);
	static Bool DeserializeResponseJSON(HTTP::Response const * const pResponse, const Reflection::WeakAny& outObject, Bool bRequireProperties = false);
	static Bool DeserializeResponseJSON(HTTP::Response const * const pResponse, const Reflection::WeakAny& outObject, SharedPtr<DataStore>& pOutDataStore, Bool bRequireProperties = false);

	/**
	 * Returns the current time based on the timestamp last sent from the server.
	 *
	 * Due to usage of Engine::Get()->GetUptime() for a delta, the resolution of this
	 * time is in milliseconds, and on most platforms, advances only with advances to
	 * the engine frame (calls to Engine::Get()->Tick()).
	 *
	 * GetCurrentServerTime() is not guaranteed to be strictly monotonically increasing,
	 * as it is allowed to adjust backwards based on server events.
	 *
	 * In general, GetCurrentServerTime() is appropriate for coarse time stamping that
	 * needs to be reliable (not susceptible to "time cheats" from local clock changes)
	 * and also in sync with the game's server backend. It should not be used for precise
	 * delta timing or situations where very accurate, monotonically increasing time
	 * is needed.
	 */
	WorldTime GetCurrentServerTime() const
	{
		if (HasCurrentServerTime())
		{
			return m_ServerTimeStamp + (Engine::Get()->GetUptime() - m_ClientUptimeAtLastServerTimeStampUpdate);
		}
		else
		{
			return WorldTime::GetUTCTime();
		}
	}

	Bool HasCurrentServerTime() const
	{
		return (m_ServerTimeStamp != WorldTime());
	}

	static WorldTime StaticGetCurrentServerTime();
	static Bool StaticHasCurrentServerTime();

	/** Call with a request pointer; if it sets the right headers, updates the server time tracking */
	void UpdateCurrentServerTimeFromResponse(HTTP::Response *pResponse);

	/** Call with a new server time stamp value. */
	void UpdateCurrentServerTime(
		const WorldTime& newServerTimeStamp,
		Double fServerRequestFunctionTimeInSeconds,
		Double fRequestRoundTripTimeInSeconds,
		TimeInterval uptimeInMillisecondsAtRequestReceive);

	/**
	 * Wraps a request callback so that successful results (status code 200)
	 * are cached. Cached data can be accessed via Game::ClientCacheLock
	 * using the given sURL as a key.
	 *
	 * IMPORTANT: The returned callback *must* be invoked once and only once,
	 * or a memory leak will occur. Assigning it to an HTTP request with SetCallback()
	 * is the expected use case.
	 */
	HTTP::ResponseDelegate WrapCallbackForCache(const HTTP::ResponseDelegate& callback, const String& sURL);

	void OverrideCachedDataBody(String const& sURL, DataStore const& dataBodyTable);
	void ClearCache(String const& sURL);

private:
	/** List used to track all HTTP requests instantiated by Game::Client code paths. */
	ScopedPtr<HTTP::RequestList> m_plPendingRequests;
	WorldTime m_ServerTimeStamp;
	TimeInterval m_ClientUptimeAtLastServerTimeStampUpdate;

	/**
	 * How much tolerance we allow when updating server time - this is to account for
	 * round trip. This should be replaced with a method that considers the round trip
	 * of the request.
	 */
	Double m_fLastConfidenceIntervalInSeconds;
	String m_sAuthToken;

	static HTTP::CallbackResult OnCachedRequest(void* pUserData, HTTP::Result eResult, HTTP::Response* pResponse);

	friend class ClientCacheLock;
	Mutex m_CacheMutex;
	Cache m_tCache;

	friend class ClientLifespanLock;
	static Mutex s_LifespanMutex;
	Atomic32 m_LifespanCount;

	SEOUL_DISABLE_COPY(Client);
}; // class Game::Client

/** Used to prevent destruction of Game::Client - typically for multi-threaded access. */
class ClientLifespanLock SEOUL_SEALED
{
public:
	ClientLifespanLock()
	{
		// We lock the lifespan mutex for increment,
		// but not for decrement. Likewise, Game::Client's
		// destructor will lock this mutex, then wait
		// for the lifespan-count to reach 0 before
		// clearing the singleton pointer.
		Lock lock(Client::s_LifespanMutex);
		if (Client::Get())
		{
			++(Client::Get()->m_LifespanCount);
		}
	}

	~ClientLifespanLock()
	{
		// Decrement without the lock - see comment in constructor.
		if (Client::Get())
		{
			// Do not access Game::Client::Get() after this decrement,
			// as it can be destroyed as soon as
			// the lifespan count reaches 0.
			--(Client::Get()->m_LifespanCount);
		}
	}

private:
	SEOUL_DISABLE_COPY(ClientLifespanLock);
}; // class Game::ClientLifespanLock

/** Similar to Lock, but specifically for entries in the Game::Client get request cache. */
class ClientCacheLock SEOUL_SEALED
{
public:
	ClientCacheLock(const String& sURL)
		: m_pEntry(AcquireEntry(sURL))
	{
	}

	~ClientCacheLock()
	{
		// Non-null data means we must release the mutex lock.
		if (nullptr != m_pEntry)
		{
			m_pEntry = nullptr;
			Client::Get()->m_CacheMutex.Unlock();
		}
	}

	/** @return A read-only pointer to the locked cache data. */
	Client::CacheEntry const* GetData() const { return m_pEntry; }

	/** @return true if data was available for the given URL, false otherwise. */
	Bool HasData() const { return (nullptr != m_pEntry); }

private:
	Client::CacheEntry const* m_pEntry;

	/**
	 * Acquire an entry from the cache table - if non-nil, the table mutex remains
	 * locked and must be released on Game::ClientCacheLock destruction.
	 */
	static inline Client::CacheEntry const* AcquireEntry(const String& sURL)
	{
		// Nop if no Game::Client singleton.
		auto pClient(Client::Get());
		if (!pClient)
		{
			return nullptr;
		}

		// Lock and acquire.
		pClient->m_CacheMutex.Lock();
		Client::CacheEntry* pReturn = nullptr;
		if (!pClient->m_tCache.GetValue(sURL, pReturn))
		{
			// Failed to acquire, release the lock.
			pClient->m_CacheMutex.Unlock();
		}

		// Done.
		return pReturn;
	}

	SEOUL_DISABLE_COPY(ClientCacheLock);
}; // class Game::ClientCacheLock

} // namespace Game

} // namespace Seoul

#endif // include guard
