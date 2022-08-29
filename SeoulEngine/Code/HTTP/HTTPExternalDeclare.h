/**
 * \file HTTPExternalDeclare.h
 * \brief Convenience header that wraps external dependencies of the HTTP
 * library. This header includes pre-declares dependencies.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef HTTP_EXTERNAL_DECLARE_H
#define HTTP_EXTERNAL_DECLARE_H

#if SEOUL_WITH_CURL

typedef void CURL;
typedef void CURLM;
typedef void CURLSH;
struct curl_slist;
struct stack_st_X509_INFO;

#include "AtomicRingBuffer.h"

#elif SEOUL_WITH_URLSESSION

#if __has_feature(objc_arc)
#error "This code doesn't work with automatic reference counting"
#endif

#ifdef __OBJC__
@class NSOperationQueue;
@class NSURLSession;
@class NSURLSessionDataTask;
@class SeoulURLSessionDelegate;
#else
typedef void NSOperationQueue;
typedef void NSURLSession;
typedef void NSURLSessionDataTask;
typedef void SeoulURLSessionDelegate;
#endif

#endif

#endif // include guard
