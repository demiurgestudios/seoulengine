/**
 * \file HTTPExternalDefine.h
 * \brief Convenience header that wraps external dependencies of the HTTP
 * library. This header includes external dependencies.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef HTTP_EXTERNAL_DEFINE_H
#define HTTP_EXTERNAL_DEFINE_H

#if SEOUL_WITH_CURL
#include "curl.h"
#include "openssl/conf.h"
#include "openssl/ssl.h"

#if !SEOUL_ASSERTIONS_DISABLED
#define SEOUL_VERIFY_CURLE(expression) { auto e_curle_result__ = (expression); SEOUL_ASSERT_MESSAGE(CURLE_OK == e_curle_result__, curl_easy_strerror(e_curle_result__)); }
#define SEOUL_VERIFY_CURLM(expression) { auto e_curlm_result__ = (expression); SEOUL_ASSERT_MESSAGE(CURLM_OK == e_curlm_result__, curl_multi_strerror(e_curlm_result__)); }
#else
#define SEOUL_VERIFY_CURLE(expression) ((void)(expression))
#define SEOUL_VERIFY_CURLM(expression) ((void)(expression))
#endif

extern "C" { void Curl_demiurge_share_inside_lock_prune_idle_connections(CURLSH *share, long timeout_ms); }

#endif // /#if SEOUL_WITH_CURL

#endif // include guard
