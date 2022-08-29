/**
 * \file HTTPHeaderTableTest.cpp
 * \brief Unit tests for the HTTP::HeaderTable class.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "HTTPHeaderTable.h"
#include "HTTPHeaderTableTest.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(HTTPHeaderTableTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestBasic)
	SEOUL_METHOD(TestClone)
SEOUL_END_TYPE()

void HTTPHeaderTableTest::TestBasic()
{
	HTTP::HeaderTable tHeaders;

	String sValue;

	// Test parsing and query.
	SEOUL_UNITTESTING_ASSERT(!tHeaders.GetValue(HString(), sValue));
	SEOUL_UNITTESTING_ASSERT(!tHeaders.GetValue(HString("server"), sValue));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String(), sValue);

	SEOUL_UNITTESTING_ASSERT(tHeaders.ParseAndAddHeader(" Server  :  Apache"));
	SEOUL_UNITTESTING_ASSERT(!tHeaders.ParseAndAddHeader("  HTTP/1.1 200 OK		"));
	SEOUL_UNITTESTING_ASSERT(tHeaders.ParseAndAddHeader("X-Powered-By:	HHVM/3.3.1	"));

	SEOUL_UNITTESTING_ASSERT(!tHeaders.GetValue(HString(), sValue));
	SEOUL_UNITTESTING_ASSERT(tHeaders.GetValue(HString("server"), sValue));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("Apache"), sValue);
	SEOUL_UNITTESTING_ASSERT(!tHeaders.GetValue(HString("notakey"), sValue));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("Apache"), sValue);

	// ParseAndAddHeader test for concatenation beehavior - adding a header
	// with the same key as an existing header causes the value to be appended
	// to the existing value using a comma ',' separator, see RFC2616.
	SEOUL_UNITTESTING_ASSERT(tHeaders.ParseAndAddHeader("Cache-control:		s-maxage=3600  "));
	SEOUL_UNITTESTING_ASSERT(tHeaders.ParseAndAddHeader("Cache-control:		 must-revalidate	"));
	SEOUL_UNITTESTING_ASSERT(tHeaders.ParseAndAddHeader("Cache-control:	  max-age=0		"));
	SEOUL_UNITTESTING_ASSERT(tHeaders.ParseAndAddHeader("X-Content-Type-Options:	nosniff	"));
	SEOUL_UNITTESTING_ASSERT(tHeaders.ParseAndAddHeader("	  Content-Encoding:	gzip	"));
	SEOUL_UNITTESTING_ASSERT(tHeaders.ParseAndAddHeader(" Vary:	Accept-Encoding	"));
	SEOUL_UNITTESTING_ASSERT(tHeaders.ParseAndAddHeader(" Last-Modified:	Tue, 23 Dec 2014 12:12:00 GMT	"));

	SEOUL_UNITTESTING_ASSERT(tHeaders.GetValue(HString("server"), sValue));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("Apache"), sValue);
	SEOUL_UNITTESTING_ASSERT(!tHeaders.GetValue(HString("notakey"), sValue));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("Apache"), sValue);
	SEOUL_UNITTESTING_ASSERT(tHeaders.GetValue(HString("vary"), sValue));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("Accept-Encoding"), sValue);
	// Concatenated value test.
	SEOUL_UNITTESTING_ASSERT(tHeaders.GetValue(HString("cache-control"), sValue));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("s-maxage=3600,must-revalidate,max-age=0"), sValue);

	SEOUL_UNITTESTING_ASSERT(tHeaders.ParseAndAddHeader(" Content-Type:	text/html; charset=utf-8	"));
	SEOUL_UNITTESTING_ASSERT(tHeaders.ParseAndAddHeader("  X-Varnish:	442914063	"));
	SEOUL_UNITTESTING_ASSERT(tHeaders.ParseAndAddHeader("  X-Varnish:	3965944542	"));
	SEOUL_UNITTESTING_ASSERT(tHeaders.ParseAndAddHeader("  X-Varnish:	3965944189	"));
	SEOUL_UNITTESTING_ASSERT(tHeaders.ParseAndAddHeader("  X-Varnish:	3894464369	"));
	SEOUL_UNITTESTING_ASSERT(tHeaders.ParseAndAddHeader("  X-Varnish:	3893020709	"));

	SEOUL_UNITTESTING_ASSERT(tHeaders.GetValue(HString("server"), sValue));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("Apache"), sValue);
	SEOUL_UNITTESTING_ASSERT(!tHeaders.GetValue(HString("notakey"), sValue));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("Apache"), sValue);
	SEOUL_UNITTESTING_ASSERT(tHeaders.GetValue(HString("x-varnish"), sValue));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("442914063,3965944542,3965944189,3894464369,3893020709"), sValue);

	SEOUL_UNITTESTING_ASSERT(tHeaders.ParseAndAddHeader("Via:	1.1 varnish, 1.1 varnish, 1.1 varnish	"));
	SEOUL_UNITTESTING_ASSERT(tHeaders.ParseAndAddHeader(" Content-Length:	11249	"));
	SEOUL_UNITTESTING_ASSERT(tHeaders.ParseAndAddHeader("	Accept-Ranges:	bytes	"));
	SEOUL_UNITTESTING_ASSERT(tHeaders.ParseAndAddHeader("   Date:	Sat, 10 Jan 2015 23:10:49 GMT	"));

	SEOUL_UNITTESTING_ASSERT(tHeaders.GetValue(HString("server"), sValue));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("Apache"), sValue);
	SEOUL_UNITTESTING_ASSERT(!tHeaders.GetValue(HString("notakey"), sValue));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("Apache"), sValue);
	SEOUL_UNITTESTING_ASSERT(tHeaders.GetValue(HString("date"), sValue));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("Sat, 10 Jan 2015 23:10:49 GMT"), sValue);

	SEOUL_UNITTESTING_ASSERT(tHeaders.ParseAndAddHeader(" Age:	2547	"));
	SEOUL_UNITTESTING_ASSERT(tHeaders.ParseAndAddHeader("		Connection:	close	"));
	SEOUL_UNITTESTING_ASSERT(tHeaders.ParseAndAddHeader("  X-Cache:	cp1055 miss (0), amssq59 hit (24), amssq38 frontend hit (3449)	"));
	SEOUL_UNITTESTING_ASSERT(tHeaders.ParseAndAddHeader("	X-Analytics:	php=hhvm	"));
	SEOUL_UNITTESTING_ASSERT(tHeaders.ParseAndAddHeader(" Set-Cookie:	GeoIP=::::v4; Path=/; Domain=.wikipedia.org"));

	SEOUL_UNITTESTING_ASSERT(tHeaders.GetValue(HString("server"), sValue));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("Apache"), sValue);
	SEOUL_UNITTESTING_ASSERT(!tHeaders.GetValue(HString("notakey"), sValue));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("Apache"), sValue);
	SEOUL_UNITTESTING_ASSERT(tHeaders.GetValue(HString("set-cookie"), sValue));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("GeoIP=::::v4; Path=/; Domain=.wikipedia.org"), sValue);
}

void HTTPHeaderTableTest::TestClone()
{
	HTTP::HeaderTable tHeaders;
	SEOUL_UNITTESTING_ASSERT(tHeaders.ParseAndAddHeader(" Server  :  Apache"));
	SEOUL_UNITTESTING_ASSERT(tHeaders.ParseAndAddHeader("X-Powered-By:	HHVM/3.3.1	"));

	String sValue;
	SEOUL_UNITTESTING_ASSERT(tHeaders.GetValue(HString("server"), sValue));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("Apache"), sValue);
	SEOUL_UNITTESTING_ASSERT(tHeaders.GetValue(HString("x-powered-by"), sValue));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("HHVM/3.3.1"), sValue);

	HTTP::HeaderTable tHeaders2;
	SEOUL_UNITTESTING_ASSERT(tHeaders2.ParseAndAddHeader(" Age:	2547	"));
	SEOUL_UNITTESTING_ASSERT(tHeaders2.ParseAndAddHeader("		Connection:	close	"));
	SEOUL_UNITTESTING_ASSERT(tHeaders2.GetValue(HString("age"), sValue));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("2547"), sValue);
	SEOUL_UNITTESTING_ASSERT(tHeaders2.GetValue(HString("connection"), sValue));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("close"), sValue);

	tHeaders2.CloneFrom(tHeaders);
	SEOUL_UNITTESTING_ASSERT(tHeaders2.GetValue(HString("server"), sValue));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("Apache"), sValue);
	SEOUL_UNITTESTING_ASSERT(tHeaders2.GetValue(HString("x-powered-by"), sValue));
	SEOUL_UNITTESTING_ASSERT_EQUAL(String("HHVM/3.3.1"), sValue);
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
