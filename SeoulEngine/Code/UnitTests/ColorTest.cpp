/**
 * \file ColorTest.cpp
 * \brief Unit tests for structures and utilities in the Color.h header.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "ColorTest.h"
#include "Logger.h"
#include "ReflectionDefine.h"
#include "StandardVertex2D.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

SEOUL_BEGIN_TYPE(ColorTest)
	SEOUL_ATTRIBUTE(UnitTest)
	SEOUL_METHOD(TestAdd)
	SEOUL_METHOD(TestConvert)
	SEOUL_METHOD(TestDefault)
	SEOUL_METHOD(TestEqual)
	SEOUL_METHOD(TestGetData)
	SEOUL_METHOD(TestLerp)
	SEOUL_METHOD(TestModulate)
	SEOUL_METHOD(TestPremultiply)
	SEOUL_METHOD(TestPremultiply2)
	SEOUL_METHOD(TestSpecial)
	SEOUL_METHOD(TestStandard)
	SEOUL_METHOD(TestSubtract)
SEOUL_END_TYPE()

void ColorTest::TestAdd()
{
	{
		Color4 const c0(0.25f, 0.5f, 0.75f, 1.0f);
		Color4 const c1(0.75f, 0.5f, 0.25f, 0.0f);
		Color4 const cAdd(c0 + c1);

		SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, cAdd.R);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, cAdd.G);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, cAdd.B);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, cAdd.A);

		Color4 cAdd2 = c0;
		cAdd2 += c1;
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, cAdd2.R);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, cAdd2.G);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, cAdd2.B);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, cAdd2.A);
	}

	{
		RGBA const c0(RGBA::Create(64, 127, 191, 255));
		RGBA const c1(RGBA::Create(191, 128, 64, 0));
		RGBA const cAdd(c0 + c1);

		SEOUL_UNITTESTING_ASSERT_EQUAL(255, cAdd.m_R);
		SEOUL_UNITTESTING_ASSERT_EQUAL(255, cAdd.m_G);
		SEOUL_UNITTESTING_ASSERT_EQUAL(255, cAdd.m_B);
		SEOUL_UNITTESTING_ASSERT_EQUAL(255, cAdd.m_A);

		RGBA cAdd2 = c0;
		cAdd2 += c1;
		SEOUL_UNITTESTING_ASSERT_EQUAL(255, cAdd2.m_R);
		SEOUL_UNITTESTING_ASSERT_EQUAL(255, cAdd2.m_G);
		SEOUL_UNITTESTING_ASSERT_EQUAL(255, cAdd2.m_B);
		SEOUL_UNITTESTING_ASSERT_EQUAL(255, cAdd2.m_A);
	}
}

void ColorTest::TestConvert()
{
	// ColorARGBu8 from floats.
	{
		Float f = 0.0f;
		for (Int i = 0; i <= 255; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ColorARGBu8::Create(i, i, i, i),
				ColorARGBu8::CreateFromFloat(f, f, f, f));
			f += (1.0f / 255.0f);
		}
	}

	// ColorARGBu8 from Color4
	{
		Float f = 0.0f;
		for (Int i = 0; i <= 255; ++i)
		{
			SEOUL_UNITTESTING_ASSERT_EQUAL(
				ColorARGBu8::Create(i, i, i, i),
				Color4(f, f, f, f).ToColorARGBu8());
			f += (1.0f / 255.0f);
		}
	}
}

void ColorTest::TestDefault()
{
	{
		Color4 c;
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, c.R);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, c.G);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, c.B);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, c.A);
	}

	{
		ColorARGBu8 c;
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, c.m_R);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, c.m_G);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, c.m_B);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, c.m_A);
	}

	{
		RGBA c;
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, c.m_R);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, c.m_G);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, c.m_B);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, c.m_A);
	}
}

void ColorTest::TestEqual()
{
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(Color4(1.0f, 2.0f, 3.0f, 4.0f), Color4(1.0f, 2.0f, 3.0f, 4.0f));
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(Color4(1.0f, 2.0f, 3.0f, 4.0f), Color4(0.0f, 2.0f, 3.0f, 4.0f));
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(Color4(1.0f, 2.0f, 3.0f, 4.0f), Color4(1.0f, 0.0f, 3.0f, 4.0f));
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(Color4(1.0f, 2.0f, 3.0f, 4.0f), Color4(1.0f, 2.0f, 0.0f, 4.0f));
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(Color4(1.0f, 2.0f, 3.0f, 4.0f), Color4(1.0f, 2.0f, 3.0f, 0.0f));
	}

	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(ColorARGBu8::Create(1, 2, 3, 4), ColorARGBu8::Create(1, 2, 3, 4));
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(ColorARGBu8::Create(1, 2, 3, 4), ColorARGBu8::Create(0, 2, 3, 4));
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(ColorARGBu8::Create(1, 2, 3, 4), ColorARGBu8::Create(1, 0, 3, 4));
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(ColorARGBu8::Create(1, 2, 3, 4), ColorARGBu8::Create(1, 2, 0, 4));
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(ColorARGBu8::Create(1, 2, 3, 4), ColorARGBu8::Create(1, 2, 3, 0));
	}

	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::Create(1, 2, 3, 4), RGBA::Create(1, 2, 3, 4));
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(RGBA::Create(1, 2, 3, 4), RGBA::Create(0, 2, 3, 4));
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(RGBA::Create(1, 2, 3, 4), RGBA::Create(1, 0, 3, 4));
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(RGBA::Create(1, 2, 3, 4), RGBA::Create(1, 2, 0, 4));
		SEOUL_UNITTESTING_ASSERT_NOT_EQUAL(RGBA::Create(1, 2, 3, 4), RGBA::Create(1, 2, 3, 0));
	}
}

void ColorTest::TestGetData()
{
	{
		Color4 const c(1, 2, 3, 4);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, c.GetData()[0]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(2.0f, c.GetData()[1]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3.0f, c.GetData()[2]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(4.0f, c.GetData()[3]);
	}

	{
		Color4 c(1, 2, 3, 4);
		SEOUL_UNITTESTING_ASSERT_EQUAL(1.0f, c.GetData()[0]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(2.0f, c.GetData()[1]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(3.0f, c.GetData()[2]);
		SEOUL_UNITTESTING_ASSERT_EQUAL(4.0f, c.GetData()[3]);
	}
}

void ColorTest::TestLerp()
{
	SEOUL_UNITTESTING_ASSERT_EQUAL(
		ColorARGBu8::Create(128, 128, 128, 128),
		Lerp(
			ColorARGBu8::Create(0, 1, 2, 3),
			ColorARGBu8::Create(255, 254, 253, 252),
			0.5f));

	SEOUL_UNITTESTING_ASSERT_EQUAL(
		Color4(0.5f, 0.5f, 0.5f, 0.5f),
		Lerp(
			Color4(0, 0.1f, 0.2f, 0.3f),
			Color4(1, 0.9f, 0.8f, 0.7f),
			0.5f));

	SEOUL_UNITTESTING_ASSERT_EQUAL(
		RGBA::Create(128, 128, 128, 128),
		Lerp(
			RGBA::Create(0, 1, 2, 3),
			RGBA::Create(255, 254, 253, 252),
			0.5f));
}

void ColorTest::TestModulate()
{
	for (Int i = 0; i < 255; ++i)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			RGBA::Create(i, i, i, i),
			RGBA::Create(i, i, i, i) * RGBA::Create(255, 255, 255, 255));

		{
			RGBA c = RGBA::Create(255, 255, 255, 255);
			c *= RGBA::Create(i, i, i, i);
			SEOUL_UNITTESTING_ASSERT_EQUAL(
				RGBA::Create(i, i, i, i),
				c);
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(
			RGBA::Create(i / 2, i / 2, i / 2, i / 2),
			RGBA::Create(i, i, i, i) * RGBA::Create(127, 127, 127, 127));

		{
			RGBA c = RGBA::Create(127, 127, 127, 127);
			c *= RGBA::Create(i, i, i, i);
			SEOUL_UNITTESTING_ASSERT_EQUAL(
				RGBA::Create(i / 2, i / 2, i / 2, i / 2),
				c);
		}
	}
}

void ColorTest::TestPremultiply()
{
	// Function implemented in Color as well
	// as Image in TextureCookTask.cpp - verifying new vs.
	// old implementation (implementation changed for time optimization).
	for (Int32 iC = 0; iC <= 255; ++iC)
	{
		for (Int32 iA = 0; iA <= 255; ++iA)
		{
			UInt8 const uC = (UInt8)iC;
			UInt8 const uA = (UInt8)iA;

			auto const uNew = (UInt8)(((Int32)uC * (Int32)uA + 127) / 255);
			auto const uOld = (UInt8)(((((Float32)uC / 255.0f) * (((Float32)uA) / 255.0f)) * 255.0f) + 0.5f);

			SEOUL_UNITTESTING_ASSERT_EQUAL(uOld, uNew);
		}
	}
}

static RGBA OldPremultiply(RGBA c)
{
	Float const fA = c.m_A / 255.0f;

	RGBA ret;
	ret.m_R = (UInt8)Clamp(((c.m_R / 255.0f) * fA * 255.0f) + 0.5f, 0.0f, 255.0f);
	ret.m_G = (UInt8)Clamp(((c.m_G / 255.0f) * fA * 255.0f) + 0.5f, 0.0f, 255.0f);
	ret.m_B = (UInt8)Clamp(((c.m_B / 255.0f) * fA * 255.0f) + 0.5f, 0.0f, 255.0f);
	ret.m_A = c.m_A;
	return ret;
}

void ColorTest::TestPremultiply2()
{
	for (Int i = 0; i < 255; ++i)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			OldPremultiply(RGBA::Create(i, i, i, i)),
			PremultiplyAlpha(RGBA::Create(i, i, i, i)));
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			OldPremultiply(RGBA::Create(i, 2, 3, 4)),
			PremultiplyAlpha(RGBA::Create(i, 2, 3, 4)));
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			OldPremultiply(RGBA::Create(1, i, 3, 4)),
			PremultiplyAlpha(RGBA::Create(1, i, 3, 4)));
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			OldPremultiply(RGBA::Create(1, 2, i, 4)),
			PremultiplyAlpha(RGBA::Create(1, 2, i, 4)));
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			OldPremultiply(RGBA::Create(1, 2, 3, i)),
			PremultiplyAlpha(RGBA::Create(1, 2, 3, i)));
	}
}

void ColorTest::TestSpecial()
{
	for (Int i = 0; i < 255; ++i)
	{
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			RGBA::Create(i, i, i, i),
			PremultiplyAlpha(RGBA::Create(255, 255, 255, (UInt)i)));
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			OldPremultiply(RGBA::Create(i, 2, 3, 4)),
			PremultiplyAlpha(RGBA::Create(i, 2, 3, 4)));
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			OldPremultiply(RGBA::Create(1, i, 3, 4)),
			PremultiplyAlpha(RGBA::Create(1, i, 3, 4)));
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			OldPremultiply(RGBA::Create(1, 2, i, 4)),
			PremultiplyAlpha(RGBA::Create(1, 2, i, 4)));
		SEOUL_UNITTESTING_ASSERT_EQUAL(
			OldPremultiply(RGBA::Create(1, 2, 3, i)),
			PremultiplyAlpha(RGBA::Create(1, 2, 3, i)));
	}
}

void ColorTest::TestStandard()
{
	SEOUL_UNITTESTING_ASSERT_EQUAL(ColorARGBu8::Create(0, 0, 0, 255), ColorARGBu8::Black());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ColorARGBu8::Create(0, 0, 255, 255), ColorARGBu8::Blue());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ColorARGBu8::Create(0, 255, 255, 255), ColorARGBu8::Cyan());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ColorARGBu8::Create(0, 255, 0, 255), ColorARGBu8::Green());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ColorARGBu8::Create(255, 0, 255, 255), ColorARGBu8::Magenta());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ColorARGBu8::Create(255, 0, 0, 255), ColorARGBu8::Red());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ColorARGBu8::Create(0, 0, 0, 0), ColorARGBu8::TransparentBlack());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ColorARGBu8::Create(255, 255, 255, 255), ColorARGBu8::White());
	SEOUL_UNITTESTING_ASSERT_EQUAL(ColorARGBu8::Create(255, 255, 0, 255), ColorARGBu8::Yellow());

	SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::Create(0, 0, 0, 255), RGBA::Black());
	SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::Create(0, 0, 0, 0), RGBA::TransparentBlack());
	SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::Create(255, 255, 255, 0), RGBA::TransparentWhite());
	SEOUL_UNITTESTING_ASSERT_EQUAL(RGBA::Create(255, 255, 255, 255), RGBA::White());
}

void ColorTest::TestSubtract()
{
	{
		Color4 const c0(0.25f, 0.5f, 0.75f, 1.0f);
		Color4 const c1(0.25f, 0.5f, 0.75f, 1.0f);
		Color4 const cSub(c0 - c1);

		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, cSub.R);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, cSub.G);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, cSub.B);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, cSub.A);

		Color4 cSub2 = c0;
		cSub2 -= c1;
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, cSub2.R);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, cSub2.G);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, cSub2.B);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0.0f, cSub2.A);
	}

	{
		RGBA const c0(RGBA::Create(64, 127, 191, 255));
		RGBA const c1(RGBA::Create(64, 127, 191, 255));
		RGBA const cSub(c0 - c1);

		SEOUL_UNITTESTING_ASSERT_EQUAL(0, cSub.m_R);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, cSub.m_G);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, cSub.m_B);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, cSub.m_A);

		RGBA cSub2 = c0;
		cSub2 -= c1;
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, cSub2.m_R);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, cSub2.m_G);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, cSub2.m_B);
		SEOUL_UNITTESTING_ASSERT_EQUAL(0, cSub2.m_A);
	}
}

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
