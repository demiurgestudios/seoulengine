/**
 * \file ContainerTestUtil.h
 * \brief Shared structures and functions for SeoulEngine
 * container testing.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Logger.h"
#include "SeoulString.h"
#include "UnitTesting.h"

namespace Seoul
{

#if SEOUL_UNIT_TESTS

struct ContainerTestSimple
{
	Int m_iA;
	Int m_B;

	static ContainerTestSimple Create(Int iA)
	{
		ContainerTestSimple ret;
		ret.m_iA = iA;
		ret.m_B = 33;
		return ret;
	}

	Bool operator==(const ContainerTestSimple& b) const
	{
		return m_B == b.m_B && m_iA == b.m_iA;
	}

	Bool operator!=(const ContainerTestSimple& b) const
	{
		return !(*this == b);
	}

	Bool operator<(const ContainerTestSimple& b) const
	{
		return (m_iA < b.m_iA);
	}
};
SEOUL_STATIC_ASSERT(CanMemCpy<ContainerTestSimple>::Value);
SEOUL_STATIC_ASSERT(CanZeroInit<ContainerTestSimple>::Value);

struct ContainerTestComplex
{
	static Int s_iCount;
	Int m_iFixedValue;
	Int m_iVariableValue;

	ContainerTestComplex()
		: m_iFixedValue(33)
		, m_iVariableValue(433)
	{
		++s_iCount;
	}

	ContainerTestComplex(const ContainerTestComplex& b)
		: m_iFixedValue(b.m_iFixedValue)
		, m_iVariableValue(b.m_iVariableValue)
	{
		++s_iCount;
		SEOUL_UNITTESTING_ASSERT_EQUAL(33, m_iFixedValue);
	}

	ContainerTestComplex(ContainerTestComplex&& b)
		: m_iFixedValue(b.m_iFixedValue)
		, m_iVariableValue(b.m_iVariableValue)
	{
		++s_iCount;
		SEOUL_UNITTESTING_ASSERT_EQUAL(33, m_iFixedValue);

		b.m_iVariableValue = 5235;
	}

	explicit ContainerTestComplex(Int iVariableValue)
		: m_iFixedValue(33)
		, m_iVariableValue(iVariableValue)
	{
		++s_iCount;
	}

	Bool operator==(const ContainerTestComplex& b) const
	{
		return m_iFixedValue == b.m_iFixedValue && m_iVariableValue == b.m_iVariableValue;
	}

	Bool operator!=(const ContainerTestComplex& b) const
	{
		return !(*this == b);
	}

	Bool operator==(Int i) const
	{
		return m_iVariableValue == i;
	}

	Bool operator!=(Int i) const
	{
		return !(*this == i);
	}

	Bool operator<(const ContainerTestComplex& b) const
	{
		return (m_iVariableValue < b.m_iVariableValue);
	}

	ContainerTestComplex& operator=(const ContainerTestComplex& b)
	{
		if (this == &b)
		{
			return *this;
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(33, b.m_iFixedValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(33, m_iFixedValue);

		m_iFixedValue = b.m_iFixedValue;
		m_iVariableValue = b.m_iVariableValue;
		return *this;
	}

	ContainerTestComplex& operator=(ContainerTestComplex&& b)
	{
		if (this == &b)
		{
			return *this;
		}

		SEOUL_UNITTESTING_ASSERT_EQUAL(33, b.m_iFixedValue);
		SEOUL_UNITTESTING_ASSERT_EQUAL(33, m_iFixedValue);

		m_iFixedValue = b.m_iFixedValue;
		m_iVariableValue = b.m_iVariableValue;

		b.m_iVariableValue = 5235;

		return *this;
	}

	~ContainerTestComplex()
	{
		SEOUL_UNITTESTING_ASSERT_LESS_THAN(0, s_iCount);

		--s_iCount;
		SEOUL_UNITTESTING_ASSERT_EQUAL(33, m_iFixedValue);
		m_iFixedValue = 1375;
	}
};

#endif // /SEOUL_UNIT_TESTS

} // namespace Seoul
