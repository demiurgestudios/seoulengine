/**
 * \file FxBankCook.cpp
 * \brief Implement cooking of .xfx files into SeoulEngine .fxb files.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Color.h"
#include "FilePath.h"
#include "FromString.h"
#include "GamePaths.h"
#include "Logger.h"
#include "Path.h"
#include "Point2DInt.h"
#include "Prereqs.h"
#include "ReflectionAny.h"
#include "ReflectionCoreTemplateTypes.h"
#include "ReflectionDefine.h"
#include "SeoulMath.h"
#include "SeoulPugiXml.h"
#include "SeoulUUID.h"
#include "SimpleCurve.h"
#include "StackOrHeapArray.h"
#include "StreamBuffer.h"
#include "StringUtil.h"

 // TODO: Big endian support.
SEOUL_STATIC_ASSERT(SEOUL_LITTLE_ENDIAN == 1);

namespace Seoul
{

namespace Cooking
{

namespace FxBankCookDetail
{

static const String ksEmpty;

// Fixed guids for builtin/implicit properties.
static const UUID kNameUUID(UUID::FromString("EF1D7D1E-02B6-4548-80D9-5EF2FBCDA237"));

// Generic, not platform specific.
static const UUID kNoPlatformUUID(UUID::FromString("00000000-0000-0000-0000-000000000000"));

// Must match equivalent values in FxStudio C# source.
static const UUID kaPlatformUUIDs[(UInt32)Platform::SEOUL_PLATFORM_COUNT] =
{
	UUID::FromString("38C3409D-8620-449a-ABE7-824D99AF44CB"), // kPC,
	UUID::FromString("03543EC0-2235-11E2-81C1-0800200C9A66"), // kIOS,
	UUID::FromString("03543EC0-2235-11E2-78C1-0831200C7866"), // kAndroid,
	UUID::FromString("03543EC0-2235-11E2-78C1-0831200C7866"), // kLinux - use Android.
};

struct PhaseDef SEOUL_SEALED
{
	UUID m_Id;
	String m_sName;
	ColorARGBu8 m_Color;
	Float m_fInitialDuration;
	Int m_iInitialPlayCount;
};

static Bool LoadPhaseDef(const pugi::xml_node& node, PhaseDef& r)
{
	r.m_Id = UUID::FromString(node.attribute("id").as_string());
	r.m_sName = node.attribute("name").as_string();
	r.m_Color.m_Value = (UInt32)node.attribute("color").as_int();
	r.m_fInitialDuration = node.attribute("initialduration").as_float(5.0f);
	r.m_iInitialPlayCount = node.attribute("initialplaycount").as_int(1);
	return true;
}

enum class ConstraintType
{
	Unknown,

	MaximumChannels,
	MaximumFloat,
	MaximumInteger,
	MinimumFloat,
	MinimumInteger,
};

// ConstraintType lookup.
typedef HashTable<UUID, ConstraintType, MemoryBudgets::Cooking> ConstraintTypeFromGuidTable;
static ConstraintTypeFromGuidTable GetConstraintTypeFromGuidTable()
{
	ConstraintTypeFromGuidTable t;
#	define SEOUL_CTE(uuid, value) t.Insert(UUID::FromString(uuid), ConstraintType::value)
	SEOUL_CTE("93b62b05-582c-4379-925b-8cfc78962b9a", MaximumChannels);
	SEOUL_CTE("dac71a24-ca97-40dd-93b5-306579b73197", MaximumFloat);
	SEOUL_CTE("0232292f-143c-41cc-8a50-6c8da0951cbd", MaximumInteger);
	SEOUL_CTE("4944057f-9671-4e17-b3ae-f65b0bacff41", MinimumFloat);
	SEOUL_CTE("1a0ec3ec-dcdc-4fcd-b947-f9daac975f53", MinimumInteger);
#	undef SEOUL_CTE
	return t;
}
static const ConstraintTypeFromGuidTable s_tConstraintTypeFromGuid(GetConstraintTypeFromGuidTable());
static ConstraintType ConstraintTypeFromGuid(const UUID& uuid)
{
	ConstraintType e = ConstraintType::Unknown;
	s_tConstraintTypeFromGuid.GetValue(uuid, e);
	return e;
}

enum class KeyframeType
{
	None,
	ColorKeyframe,
	FloatKeyframe,
};

enum class PropType
{
	Unknown,

	Boolean,
	ColorRamp,
	CustomImage,
	CustomString,
	DropDownList,
	FloatRangeSlider,
	FloatSlider,
	IntegerRangeSlider,
	IntegerSlider,
	Ramp,
	Text,
	Vector3,
};

enum class RampType
{
	Linear,
	Spline,
};

// PropType lookup.
typedef HashTable<UUID, PropType, MemoryBudgets::Cooking> PropTypeFromGuidTable;
static PropTypeFromGuidTable GetPropTypeFromGuidTable()
{
	PropTypeFromGuidTable t;
#	define SEOUL_CTE(uuid, value) t.Insert(UUID::FromString(uuid), PropType::value)
	SEOUL_CTE("fcf65cf3-39b6-4bf9-9bd7-b941a6460519", Boolean);
	SEOUL_CTE("22c6b703-4b0e-4944-9d37-43fd436f9c71", ColorRamp);
	SEOUL_CTE("2485e9e1-e864-48f1-8e48-fbd51b3994f0", CustomImage);
	SEOUL_CTE("2afe2610-12a0-4301-a438-102a5d982d75", CustomString);
	SEOUL_CTE("fbb27f2a-c942-4840-8df1-0372bb898477", DropDownList);
	SEOUL_CTE("497241d5-8dcf-49f4-9f77-596b3f3c09a1", FloatRangeSlider);
	SEOUL_CTE("1a0cc0c6-9f3f-4e24-aa3e-115c1dd2d798", FloatSlider);
	SEOUL_CTE("e449ac44-e15d-42ea-bc30-7892c77b42d4", IntegerRangeSlider);
	SEOUL_CTE("999607c1-f678-4767-9b93-2f54e2924642", IntegerSlider);
	SEOUL_CTE("da8c974a-fe5b-415e-ae28-56c76d31094f", Ramp);
	SEOUL_CTE("f71ff166-5e06-47f4-a843-e0f9f08de542", Text);
	SEOUL_CTE("321d4c50-4a05-45f4-a356-ec011b49c01c", Vector3);
#	undef SEOUL_CTE
	return t;
}
static const PropTypeFromGuidTable s_tPropTypeFromGuid(GetPropTypeFromGuidTable());
static PropType PropTypeFromGuid(const UUID& uuid)
{
	PropType e = PropType::Unknown;
	s_tPropTypeFromGuid.GetValue(uuid, e);
	return e;
}

struct ConstraintDef SEOUL_SEALED
{
	ConstraintType m_eType;
	Double m_fValue;
	UUID m_PlatformId;
};

static Bool LoadConstraintDef(const pugi::xml_node& node, ConstraintDef& r)
{
	// Values.
	auto const guid = UUID::FromString(node.attribute("classid").as_string());
	r.m_eType = ConstraintTypeFromGuid(guid);
	if (ConstraintType::Unknown == r.m_eType)
	{
		SEOUL_LOG_COOKING("unknown constraint type guid '%s'", guid.ToString().CStr());
		return false;
	}
	r.m_fValue = node.attribute("value").as_double();
	r.m_PlatformId = UUID::FromString(node.attribute("platform").as_string());

	return true;
}

struct RampChannelDef SEOUL_SEALED
{
	String m_sName;
	ColorARGBu8 m_Color;
	UUID m_Id;
	Bool m_bHidden;
};

struct Keyframe SEOUL_SEALED
{
	Float m_fTime;
	Float m_fValueOrAlpha;
	ColorARGBu8 m_RGB;

	Bool operator==(const Keyframe& b) const
	{
		return
			(m_fTime == b.m_fTime) &&
			(m_fValueOrAlpha == b.m_fValueOrAlpha) &&
			(m_RGB == b.m_RGB);
	}

	Bool operator!=(const Keyframe& b) const
	{
		return !(*this == b);
	}
};

static Bool LoadKeyframe(PropType eType, const pugi::xml_node& node, Keyframe& r)
{
	r.m_fTime = node.attribute("time").as_float();
	if (PropType::ColorRamp == eType)
	{
		r.m_RGB.m_Value = node.attribute("value").as_int();
		r.m_fValueOrAlpha = (Float)r.m_RGB.m_A;
		r.m_RGB.m_A = 0;
	}
	else
	{
		r.m_fValueOrAlpha = node.attribute("value").as_float();
		r.m_RGB = ColorARGBu8::TransparentBlack();
	}
	return true;
}

static Bool LoadRampChannelDef(const pugi::xml_node& node, RampChannelDef& r)
{
	r.m_sName = node.attribute("name").as_string();
	r.m_Color.m_Value = (UInt32)node.attribute("color").as_int();
	r.m_Id = UUID::FromString(node.attribute("id").as_string());
	r.m_bHidden = node.attribute("hidden").as_bool();
	return true;
}

struct Cp SEOUL_SEALED
{
	Float m_fTimeOffset;
	Float m_fValueOffset;
	Bool m_bChanged;

	Bool operator==(const Cp& b) const
	{
		return
			(m_fTimeOffset == b.m_fTimeOffset) &&
			(m_fValueOffset == b.m_fValueOffset) &&
			(m_bChanged == b.m_bChanged);
	}

	Bool operator!=(const Cp& b) const
	{
		return !(*this == b);
	}

	Vector2D ComputeStartTimeAndValue(const Vector2D& a, const Vector2D& b) const
	{
		// Changed is used to indicate whether a control point on a curve has been
		// manually modified. If not, we want to use the legacy default behavior,
		// which is to compute midpoints.
		if (m_bChanged)
		{
			// Compute the control point's position, then clamp it.
			auto v = Vector2D(a.X + m_fTimeOffset, a.Y + m_fValueOffset);
			v.X = Min(v.X, b.X);
			return v;
		}

		// This is the unchanged case, which means we want to compute
		// the midpoint on the line and use that for the position.
		else
		{
			Float fMidTime = (b.X - a.X) / 2.0f;
			return Vector2D(a.X + fMidTime, a.Y);
		}
	}

	Vector2D ComputeEndTimeAndValue(const Vector2D& a, const Vector2D& b) const
	{
		// Changed is used to indicate whether a control point on a curve has been
		// manually modified. If not, we want to use the legacy default behavior,
		// which is to compute midpoints.
		if (m_bChanged)
		{
			// Compute the control point's position, then clamp it.
			auto v = Vector2D(b.X + m_fTimeOffset, b.Y + m_fValueOffset);
			v.X = Max(v.X, a.X);
			return v;
		}

		// This is the unchanged case, which means we want to compute
		// the midpoint on the line and use that for the position.
		else
		{
			Float fMidTime = (b.X - a.X) / 2.0f;
			return Vector2D(b.X - fMidTime, b.Y);
		}
	}
};

static Bool LoadCp(const pugi::xml_node& node, Cp& r)
{
	r.m_fTimeOffset = node.attribute("time").as_float();
	r.m_fValueOffset = node.attribute("value").as_float();
	r.m_bChanged = node.attribute("changed").as_bool();
	return true;
}

struct CpPair SEOUL_SEALED
{
	Cp m_Start;
	Cp m_End;

	Bool operator==(const CpPair& b) const
	{
		return
			(m_Start == b.m_Start) &&
			(m_End == b.m_End);
	}

	Bool operator!=(const CpPair& b) const
	{
		return !(*this == b);
	}
};

static Bool LoadCpPair(const pugi::xml_node& node, CpPair& r)
{
	if (!LoadCp(node.select_node("startcp").node(), r.m_Start))
	{
		return false;
	}
	if (!LoadCp(node.select_node("endcp").node(), r.m_End))
	{
		return false;
	}
	return true;
}

struct RampChannel SEOUL_SEALED
{
	UUID m_Id;
	RampType m_eType;
	typedef Vector<Keyframe, MemoryBudgets::Cooking> Keyframes;
	Keyframes m_vKeyframes;
	typedef Vector<CpPair, MemoryBudgets::Cooking> CpPairs;
	CpPairs m_vCpPairs;

	Bool operator==(const RampChannel& b) const
	{
		return
			(m_Id == b.m_Id) &&
			(m_eType == b.m_eType) &&
			(m_vKeyframes == b.m_vKeyframes) &&
			(m_vCpPairs == b.m_vCpPairs);
	}

	Bool operator!=(const RampChannel& b) const
	{
		return !(*this == b);
	}
};

static Bool LoadRampChannel(PropType eType, const pugi::xml_node& node, RampChannel& r)
{
	r.m_Id = UUID::FromString(node.attribute("id").as_string());
	r.m_eType = RampType::Linear;
	if (0 == STRCMP_CASE_INSENSITIVE(node.attribute("type").as_string(), "Spline"))
	{
		r.m_eType = RampType::Spline;
	}

	for (auto const& keyframe : node.select_nodes("keyframes/keyframe"))
	{
		auto const keyframeNode = keyframe.node();
		r.m_vKeyframes.PushBack(Keyframe{});
		if (!LoadKeyframe(eType, keyframeNode, r.m_vKeyframes.Back()))
		{
			return false;
		}
		if (RampType::Spline == r.m_eType)
		{
			r.m_vCpPairs.PushBack(CpPair{});
			if (!LoadCpPair(keyframeNode, r.m_vCpPairs.Back()))
			{
				return false;
			}
		}
	}

	return true;
}

struct RampChannelData SEOUL_SEALED
{
	typedef Vector<RampChannel, MemoryBudgets::Cooking> RampChannels;
	RampChannels m_vRampChannels;

	Bool operator==(const RampChannelData& b) const
	{
		return (m_vRampChannels == b.m_vRampChannels);
	}

	Bool operator!=(const RampChannelData& b) const
	{
		return !(*this == b);
	}
};

static Bool PropDataEqual(PropType eType, const Reflection::Any& a, const Reflection::Any& b)
{
	switch (eType)
	{
	case PropType::Boolean: return a.Cast<Int32>() == b.Cast<Int32>();
	case PropType::ColorRamp: return a.Cast<RampChannelData>() == b.Cast<RampChannelData>();
	case PropType::CustomImage: return a.Cast<String>() == b.Cast<String>();
	case PropType::CustomString: return a.Cast<String>() == b.Cast<String>();
	case PropType::DropDownList: return a.Cast<Int32>() == b.Cast<Int32>();
	case PropType::FloatRangeSlider: return a.Cast<Vector2D>() == b.Cast<Vector2D>();
	case PropType::FloatSlider: return a.Cast<Float32>() == b.Cast<Float32>();
	case PropType::IntegerRangeSlider: return a.Cast<Point2DInt>() == b.Cast<Point2DInt>();
	case PropType::IntegerSlider: return a.Cast<Int32>() == b.Cast<Int32>();
	case PropType::Ramp: return a.Cast<RampChannelData>() == b.Cast<RampChannelData>();
	case PropType::Text: return a.Cast<String>() == b.Cast<String>();
	case PropType::Vector3: return a.Cast<Vector3D>() == b.Cast<Vector3D>();
	default:
		SEOUL_FAIL("Out-of-sync enum.");
		return false;
	};
}

struct PropDef SEOUL_SEALED
{
	// Config.
	String m_sFullName;
	String m_sName;
	UUID m_Id;
	HString m_Type;
	PropType m_eType = PropType::Unknown;
	Bool m_bReadOnly = false;
	Bool m_bHidden = false;
	Bool m_bNoDefaultValue = false;
	Bool m_bCanSpecialize = false;

	// Constraints.
	typedef Vector<ConstraintDef, MemoryBudgets::Cooking> ConstraintDefs;
	ConstraintDefs m_vConstraintDefs;

	// Only applies to props of type Ramp.
	KeyframeType m_eKeyframeType;
	typedef Vector<RampChannelDef, MemoryBudgets::Cooking> RampChannelDefs;
	RampChannelDefs m_vRampChannelDefs;

	// Data.
	Reflection::Any m_Data;
};

static PropDef BuiltinPropDef(const UUID& uuid, const String& sName, PropType eType, const Reflection::Any& value)
{
	PropDef prop;
	prop.m_Id = uuid;
	prop.m_sName = sName;
	prop.m_sFullName = sName;
	prop.m_eType = eType;
	prop.m_Data = value;
	return prop;
}

// Special case, every component has an implicit "Name" prop.
static const PropDef kNamePropDef(BuiltinPropDef(kNameUUID, "Name", PropType::Text, String("Component")));

static Bool LoadPropDefSpecialized(const pugi::xml_node& node, PropDef& r)
{
	if (PropType::ColorRamp != r.m_eType && PropType::Ramp != r.m_eType)
	{
		r.m_eKeyframeType = KeyframeType::None;
		return true;
	}

	r.m_eKeyframeType = KeyframeType::ColorKeyframe;
	if (0 == STRCMP_CASE_INSENSITIVE(node.attribute("keyframetype").as_string(), "FloatKeyframe"))
	{
		r.m_eKeyframeType = KeyframeType::FloatKeyframe;
	}

	for (auto const& channel : node.select_nodes("channels/channel"))
	{
		r.m_vRampChannelDefs.PushBack(RampChannelDef{});
		if (!LoadRampChannelDef(channel.node(), r.m_vRampChannelDefs.Back()))
		{
			return false;
		}
	}

	return true;
}

static Bool GetPropValuePoint2DInt(Byte const* sValue, Reflection::Any& rValue)
{
	auto const sComma = strchr(sValue, ',');
	if (nullptr == sComma)
	{
		SEOUL_LOG_COOKING("invalid float range value");
		return false;
	}

	Point2DInt pt;
	String sTmp(TrimWhiteSpace(String(sValue, (UInt)(sComma - sValue))));
	if (!FromString(sTmp, pt.X))
	{
		SEOUL_LOG_COOKING("invalid float range value");
		return false;
	}
	sTmp = TrimWhiteSpace(String(sComma + 1, (UInt)(sValue + strlen(sValue) - (sComma + 1))));
	if (!FromString(sTmp, pt.Y))
	{
		SEOUL_LOG_COOKING("invalid float range value");
		return false;
	}

	// Done.
	rValue = pt;
	return true;
}

static Bool GetPropValueRamp(PropType eType, const pugi::xml_node& node, Reflection::Any& rValue)
{
	RampChannelData data;
	for (auto const& channel : node.select_nodes("rampchanneldata/rampchannel"))
	{
		data.m_vRampChannels.PushBack(RampChannel{});
		if (!LoadRampChannel(eType, channel.node(), data.m_vRampChannels.Back()))
		{
			return false;
		}
	}

	rValue = data;
	return true;
}

static Bool GetPropValueVector2D(Byte const* sValue, Reflection::Any& rValue)
{
	auto const sComma = strchr(sValue, ',');
	if (nullptr == sComma)
	{
		SEOUL_LOG_COOKING("invalid float range value");
		return false;
	}

	Vector2D v;
	String sTmp(TrimWhiteSpace(String(sValue, (UInt)(sComma - sValue))));
	if (!FromString(sTmp, v.X))
	{
		SEOUL_LOG_COOKING("invalid float range value");
		return false;
	}
	sTmp = TrimWhiteSpace(String(sComma + 1, (UInt)(sValue + strlen(sValue) - (sComma + 1))));
	if (!FromString(sTmp, v.Y))
	{
		SEOUL_LOG_COOKING("invalid float range value");
		return false;
	}

	// Done.
	rValue = v;
	return true;
}

static Bool GetPropValueVector3D(Byte const* sValue, Reflection::Any& rValue)
{
	auto const sComma1 = strchr(sValue, ',');
	if (nullptr == sComma1)
	{
		SEOUL_LOG_COOKING("invalid Vector3 value");
		return false;
	}
	auto const sComma2 = strchr(sComma1 + 1, ',');
	if (nullptr == sComma2)
	{
		SEOUL_LOG_COOKING("invalid Vector3 value");
		return false;
	}

	Vector3D v;
	String sTmp(TrimWhiteSpace(String(sValue, (UInt)(sComma1 - sValue))));
	if (!FromString(sTmp, v.X))
	{
		SEOUL_LOG_COOKING("invalid Vector3 value");
		return false;
	}
	sTmp = TrimWhiteSpace(String(sComma1 + 1, (UInt)(sComma2 - (sComma1 + 1))));
	if (!FromString(sTmp, v.Y))
	{
		SEOUL_LOG_COOKING("invalid Vector3 value");
		return false;
	}
	sTmp = TrimWhiteSpace(String(sComma2 + 1, (UInt)(sValue + strlen(sValue) - (sComma2 + 1))));
	if (!FromString(sTmp, v.Z))
	{
		SEOUL_LOG_COOKING("invalid Vector3 value");
		return false;
	}

	// Done.
	rValue = v;
	return true;
}

static Bool GetPropValue(PropType eType, const pugi::xml_node& node, Reflection::Any& rValue)
{
	auto const value = node.attribute("value");
	switch (eType)
	{
	case PropType::Boolean: rValue = (Int32)value.as_bool() ? 1 : 0; return true;
	case PropType::ColorRamp: return GetPropValueRamp(eType, node, rValue);
	case PropType::CustomImage: rValue = String(value.as_string()); return true;
	case PropType::CustomString: rValue = String(value.as_string()); return true;
	case PropType::DropDownList: rValue = (Int32)value.as_int(); return true;
	case PropType::FloatRangeSlider: return GetPropValueVector2D(value.as_string(), rValue);
	case PropType::FloatSlider: rValue = (Float32)value.as_float(); return true;
	case PropType::IntegerRangeSlider: return GetPropValuePoint2DInt(value.as_string(), rValue);
	case PropType::IntegerSlider: rValue = (Int32)value.as_int(); return true;
	case PropType::Ramp: return GetPropValueRamp(eType, node, rValue);
	case PropType::Text: rValue = String(value.as_string()); return true;
	case PropType::Vector3: return GetPropValueVector3D(value.as_string(), rValue);

	default:
		SEOUL_LOG_COOKING("Unsupported prop type: %d\n", (Int)eType);
		return false;
	};
}

static Bool LoadPropxDatum(Platform ePlatform, const pugi::xml_node& node, PropType eType, const String& sName, Reflection::Any& r)
{
	for (auto const& datum : node.select_nodes("data/datum"))
	{
		auto const datumNode = datum.node();

		// No platform attribute.
		auto const platformStr = datumNode.attribute("platform").as_string(nullptr);
		if (!platformStr)
		{
			continue;
		}

		// Either no platform, or platform specific. Stop immediately
		// on platform specific.
		auto const platform = UUID::FromString(platformStr);
		if (platform != kNoPlatformUUID && platform != kaPlatformUUIDs[(UInt32)ePlatform])
		{
			continue;
		}

		// Value.
		Reflection::Any value;
		if (!GetPropValue(eType, datumNode, value))
		{
			return false;
		}

		// Store.
		r = value;

		// Done if platform specific.
		if (platform != kNoPlatformUUID)
		{
			break;
		}
	}

	if (!r.IsValid())
	{
		SEOUL_LOG_COOKING("'%s' has no datum", sName.CStr());
		return false;
	}

	return true;
}

static Bool LoadPropDefNested(Platform ePlatform, const pugi::xml_node& node, PropDef& r)
{
	r.m_Type = HString(node.attribute("type").as_string());
	r.m_eType = PropTypeFromGuid(UUID::FromString(node.attribute("typeid").as_string()));
	if (PropType::Unknown == r.m_eType)
	{
		SEOUL_LOG_COOKING("Unknown prop type \"%s\", failed loading AppComponentDefinition.xcd", r.m_Type.CStr());
		return false;
	}

	// TODO: Inputs are not supported.
	if (node.attribute("input") || node.attribute("acceptsinput").as_bool())
	{
		SEOUL_LOG_COOKING("Inputs are not supported.");
		return false;
	}

	r.m_bReadOnly = node.attribute("readonly").as_bool();
	r.m_bHidden = node.attribute("hidden").as_bool();
	r.m_bNoDefaultValue = node.attribute("nodefaultvalue").as_bool();
	r.m_bCanSpecialize = node.attribute("specializable").as_bool();

	// Constraints.
	for (auto const& constraint : node.select_nodes("constraints/constraint"))
	{
		r.m_vConstraintDefs.PushBack(ConstraintDef{});
		if (!LoadConstraintDef(constraint.node(), r.m_vConstraintDefs.Back()))
		{
			return false;
		}
	}

	// Specialized loading based on type.
	if (!LoadPropDefSpecialized(node, r))
	{
		return false;
	}

	// Property data.
	return LoadPropxDatum(ePlatform, node, r.m_eType, r.m_sName, r.m_Data);
}

static Bool LoadPropDef(Platform ePlatform, const pugi::xml_node& node, PropDef& r)
{
	r.m_sName = node.attribute("name").as_string();
	r.m_Id = UUID::FromString(node.attribute("id").as_string());
	return LoadPropDefNested(ePlatform, node.select_node("definition").node(), r);
}

struct PropsDef SEOUL_SEALED
{
	typedef Vector<PropDef, MemoryBudgets::Cooking> PropDefs;
	PropDefs m_vProps;
};

static Bool LoadPropGroups(const String& sNamespace, const pugi::xml_node& node, PropsDef& r)
{
	// Nested properties.
	if (!sNamespace.IsEmpty())
	{
		for (auto const& propNode : node.select_nodes("properties/property"))
		{
			auto const id = UUID::FromString(propNode.node().attribute("id").as_string());
			for (auto& prop : r.m_vProps)
			{
				if (prop.m_Id == id)
				{
					prop.m_sFullName = sNamespace + "." + prop.m_sName;
					break;
				}
			}
		}
	}
	// Nested and root groups.
	for (auto const& propGroup : node.select_nodes("children/propertygroup"))
	{
		auto const propGroupNode = propGroup.node();
		String sNested(propGroupNode.attribute("name").as_string());
		if (sNested.IsEmpty())
		{
			continue;
		}

		if (!sNamespace.IsEmpty())
		{
			sNested = sNamespace + "." + sNested;
		}

		if (!LoadPropGroups(sNested, propGroupNode, r))
		{
			return false;
		}
	}

	return true;
}

static Bool LoadPropsDef(Platform ePlatform, const pugi::xml_node& node, PropsDef& r)
{
	// Always included, special case "Name" property.
	r.m_vProps.PushBack(kNamePropDef);

	// Properties.
	for (auto const& prop : node.select_nodes("property"))
	{
		r.m_vProps.PushBack(PropDef{});
		if (!LoadPropDef(ePlatform, prop.node(), r.m_vProps.Back()))
		{
			return false;
		}
	}
	// Apply groups - roots are ignored.
	for (auto const& propGroup : node.select_nodes("propertygroup"))
	{
		if (!LoadPropGroups(String(), propGroup.node(), r))
		{
			return false;
		}
	}
	// Final step - any properties with an empty m_sFullName field, just assign m_sName.
	for (auto& prop : r.m_vProps)
	{
		if (prop.m_sFullName.IsEmpty())
		{
			prop.m_sFullName = prop.m_sName;
		}
	}
	return true;
}

struct CompDef SEOUL_SEALED
{
	HString m_Class;
	ColorARGBu8 m_Color;
	PropsDef m_Props;
};

static Bool LoadCompDef(Platform ePlatform, const pugi::xml_node& node, CompDef& r)
{
	r.m_Class = HString(node.attribute("name").as_string(), true);
	r.m_Color.m_Value = (UInt32)node.attribute("color").as_int();
	return LoadPropsDef(ePlatform, node.child("properties"), r.m_Props);
}

struct CategoryDef
{
	String m_sName;
	UUID m_Id;
	Float m_fMin;
	Float m_fMax;
};

struct ComponentDefinition SEOUL_SEALED
{
	Platform m_ePlatform = keCurrentPlatform;
	String m_sVersion;

	typedef Vector<PhaseDef, MemoryBudgets::Cooking> PhaseDefs;
	PhaseDefs m_vPhases;
	typedef Vector<CompDef, MemoryBudgets::Cooking> CompDefs;
	CompDefs m_vComponents;
	typedef HashTable<HString, UInt32, MemoryBudgets::Cooking> CompDefTable;
	CompDefTable m_tComponents;
	typedef HashTable<UUID, CheckedPtr<PropDef const>, MemoryBudgets::Cooking> Props;
	Props m_tProps;

	void Swap(ComponentDefinition& r)
	{
		Seoul::Swap(m_ePlatform, r.m_ePlatform);
		m_sVersion.Swap(r.m_sVersion);
		m_vPhases.Swap(r.m_vPhases);
		m_vComponents.Swap(r.m_vComponents);
		m_tComponents.Swap(r.m_tComponents);
		m_tProps.Swap(r.m_tProps);
	}
};

static Bool LoadComponentDefinition(Platform ePlatform, ComponentDefinition& r)
{
	// Always the same.
	auto const sXcdPath(Path::Combine(GamePaths::Get()->GetSourceDir(), "AppComponentDefinition.xcd"));

	pugi::xml_document root;
	pugi::xml_parse_result result = root.load_file(
		sXcdPath.CStr(),
		pugi::parse_default,
		pugi::encoding_utf8);

	// Check and return failure on error.
	if (result.status != pugi::status_ok)
	{
		SEOUL_LOG_COOKING("Failed loading AppComponentDefinition.xcd: %s", result.description());
		return false;
	}

	// Output.
	ComponentDefinition data;

	// Cache.
	data.m_ePlatform = ePlatform;

	// Version.
	data.m_sVersion = root.select_node("root/@version").attribute().as_string();

	// TODO: inputs are not supported.
	if (!root.select_nodes("root/inputs/input").empty())
	{
		SEOUL_LOG_COOKING("AppComponentDefinition.xcd: inputs are not supported.");
		return false;
	}

	// Phases.
	for (auto const& phase : root.select_nodes("root/phases/object/data"))
	{
		data.m_vPhases.PushBack(PhaseDef{});
		if (!LoadPhaseDef(phase.node(), data.m_vPhases.Back()))
		{
			return false;
		}
	}

	// Components.
	for (auto const& component : root.select_nodes("root/components/component"))
	{
		data.m_vComponents.PushBack(CompDef{});
		if (!LoadCompDef(ePlatform, component.node(), data.m_vComponents.Back()))
		{
			return false;
		}

		auto const clazz = data.m_vComponents.Back().m_Class;
		auto const e = data.m_tComponents.Insert(clazz, data.m_vComponents.GetSize() - 1u);
		if (!e.Second)
		{
			SEOUL_LOG_COOKING("AppComponentDefinition.xcd: '%s' appears twice as component class.", clazz.CStr());
			return false;
		}
	}

	// Global prop table.
	for (auto const& component : data.m_vComponents)
	{
		for (auto const& prop : component.m_Props.m_vProps)
		{
			auto const e = data.m_tProps.Insert(prop.m_Id, &prop);
			if (!e.Second)
			{
				// Annoying - GUID is globally unique with this one exception. Every
				// component has a "Name" property and it always has the same GUID.
				//
				// Since m_tProps is used only to resolve to an appropriate definition,
				// and the definition of "Name" is identical across components, this is ok.
				if (prop.m_Id != kNamePropDef.m_Id)
				{
					SEOUL_LOG_COOKING("AppComponentDefinition.xcd: '%s' and '%s' both share UUID '%s'",
						e.First->Second->m_sName.CStr(),
						prop.m_sName.CStr(),
						prop.m_Id.ToString().CStr());
					return false;
				}
			}
		}
	}

	r.Swap(data);
	return true;
}

struct Phase SEOUL_SEALED
{
	UUID m_DefinitionId;
	Float m_fDuration;
	Int m_iPlayCount;
};

static Bool LoadPhase(const ComponentDefinition& def, const pugi::xml_node& node, Phase& r)
{
	r.m_DefinitionId = UUID::FromString(node.attribute("definitionid").as_string());
	r.m_fDuration = node.attribute("duration").as_float(5.0f);
	r.m_iPlayCount = node.attribute("playcount").as_int(1);
	return true;
}

struct Prop SEOUL_SEALED
{
	UUID m_Id;
	CheckedPtr<PropDef const> m_pDef;
	Reflection::Any m_Data;
};

enum class LoadPropRes
{
	Success,
	NoDef,
	Fail,
};

static LoadPropRes LoadProp(const  ComponentDefinition& def, const pugi::xml_node& node, Prop& r)
{
	r.m_Id = UUID::FromString(node.attribute("id").as_string());

	// This case is allowed - stale values are allowed in XFX files
	// and are stripped at build time.
	if (!def.m_tProps.GetValue(r.m_Id, r.m_pDef))
	{
		return LoadPropRes::NoDef;
	}

	// Property data.
	return LoadPropxDatum(def.m_ePlatform, node, r.m_pDef->m_eType, r.m_pDef->m_sName, r.m_Data) ? LoadPropRes::Success : LoadPropRes::Fail;
}

struct Props SEOUL_SEALED
{
	typedef Vector<Prop, MemoryBudgets::Cooking> PropsContainer;
	PropsContainer m_vProps;
};

static Bool LoadProps(const ComponentDefinition& def, const pugi::xml_node& node, Props& r)
{
	for (auto const& prop : node.select_nodes("property"))
	{
		r.m_vProps.PushBack(Prop{});
		auto const e = LoadProp(def, prop.node(), r.m_vProps.Back());

		// Allow, missing from def strips it from .xfx files.
		if (LoadPropRes::NoDef == e)
		{
			r.m_vProps.PopBack();
			continue;
		}
		else if (LoadPropRes::Success != e)
		{
			return false;
		}
	}

	return true;
}

struct Component SEOUL_SEALED
{
	HString m_Class;
	Float m_fStart;
	Float m_fEnd;
	Props m_Props;
};

static Bool LoadComponent(const ComponentDefinition& def, const pugi::xml_node& node, Component& r)
{
	r.m_Class = HString(node.attribute("class").as_string());
	r.m_fStart = node.attribute("start").as_float();
	r.m_fEnd = node.attribute("end").as_float();

	return LoadProps(def, node.select_node("properties").node(), r.m_Props);
}

struct Track SEOUL_SEALED
{
	String m_sName;
	Bool m_bMuted;
	Bool m_bLocked;
	typedef Vector<Component, MemoryBudgets::Cooking> Components;
	Components m_vComponents;
};

static Bool LoadTrack(const ComponentDefinition& def, const pugi::xml_node& node, Track& r)
{
	r.m_sName = node.attribute("name").as_string();
	r.m_bMuted = node.attribute("muted").as_bool();
	r.m_bLocked = node.attribute("locked").as_bool();

	for (auto const& comp : node.select_nodes("component"))
	{
		r.m_vComponents.PushBack(Component{});
		if (!LoadComponent(def, comp.node(), r.m_vComponents.Back()))
		{
			return false;
		}
	}

	return true;
}

struct TrackGroup
{
	String m_sName;
	typedef Vector<Track, MemoryBudgets::Cooking> Tracks;
	Tracks m_vTracks;
};

static Bool LoadTrackGroup(const ComponentDefinition& def, const pugi::xml_node& node, TrackGroup& r)
{
	r.m_sName = node.attribute("name").as_string();
	for (auto const& track : node.select_nodes("track"))
	{
		r.m_vTracks.PushBack(Track{});
		if (!LoadTrack(def, track.node(), r.m_vTracks.Back()))
		{
			return false;
		}
	}

	return true;
}

struct PackedComponent SEOUL_SEALED
{
	CheckedPtr<Component const> m_pComponent;
	UInt32 m_uNonDefaultProps = 0u;
	UInt32 m_uTrackGroupIndex = 0u;
};

struct XfxData SEOUL_SEALED
{
	String m_sBankName;
	String m_sEffectName;
	UUID m_Id;
	String m_sVersion;
	String m_sEffectVersion;

	Float ComputeDuration() const
	{
		Float fComponentDuration = 0.0f;
		for (auto const& group : m_vTrackGroups)
		{
			for (auto const& track : group.m_vTracks)
			{
				for (auto const& comp : track.m_vComponents)
				{
					fComponentDuration = Max(comp.m_fEnd, fComponentDuration);
				}
			}
		}

		Float fPhaseDuration = 0.0f;
		for (auto const& phase : m_vPhases)
		{
			fPhaseDuration += phase.m_fDuration;
		}

		return Max(fComponentDuration, fPhaseDuration);
	}

	typedef Vector<Phase, MemoryBudgets::Cooking> Phases;
	Phases m_vPhases;
	typedef Vector<ColorARGBu8, MemoryBudgets::Cooking> Colors;
	Colors m_vColors;
	typedef Vector<TrackGroup, MemoryBudgets::Cooking> TrackGroups;
	TrackGroups m_vTrackGroups;
	typedef Vector<PackedComponent, MemoryBudgets::Cooking> PackedComponents;
	PackedComponents m_vPackedComponents;
	typedef Vector<CheckedPtr<Prop const>, MemoryBudgets::Cooking> PackedNotDefaultProps;
	PackedNotDefaultProps m_vPackedNotDefaultProps;
};

static Bool LoadXfxData(FilePath filePath, const ComponentDefinition& def, const pugi::xml_node& node, XfxData& r)
{
	r.m_sBankName = Path::GetFileName(filePath.GetRelativeFilenameWithoutExtension().ToString());
	r.m_sEffectName = r.m_sBankName.ToLowerASCII();
	r.m_Id = UUID::FromString(node.select_node("effect/@id").attribute().as_string());
	r.m_sVersion = node.select_node("effect/@version").attribute().as_string();
	r.m_sEffectVersion = node.select_node("effect/@effectversion").attribute().as_string();

	// Load phases
	for (auto const& phaseNode : node.select_nodes("effect/phases/object/data"))
	{
		r.m_vPhases.PushBack(Phase{});
		if (!LoadPhase(def, phaseNode.node(), r.m_vPhases.Back()))
		{
			return false;
		}
	}

	// Colors.
	for (auto const& colorNode : node.select_nodes("effect/colors/color"))
	{
		ColorARGBu8 color;
		color.m_Value = colorNode.node().text().as_int();
		r.m_vColors.PushBack(color);
	}

	// Track Groups.
	for (auto const& trackGroup : node.select_nodes("effect/trackgroups/trackgroup"))
	{
		r.m_vTrackGroups.PushBack(TrackGroup{});
		if (!LoadTrackGroup(def, trackGroup.node(), r.m_vTrackGroups.Back()))
		{
			return false;
		}
	}

	// Create a flat component list.
	auto const uTrackGroups = r.m_vTrackGroups.GetSize();
	for (UInt32 uTrackGroupIndex = 0u; uTrackGroupIndex < uTrackGroups; ++uTrackGroupIndex)
	{
		auto const& trackGroup = r.m_vTrackGroups[uTrackGroupIndex];
		for (auto const& track : trackGroup.m_vTracks)
		{
			for (auto const& comp : track.m_vComponents)
			{
				if (track.m_bMuted)
				{
					continue;
				}

				PackedComponent packed;
				packed.m_pComponent = &comp;
				packed.m_uTrackGroupIndex = uTrackGroupIndex;
				for (auto const& prop : comp.m_Props.m_vProps)
				{
					if (!PropDataEqual(prop.m_pDef->m_eType, prop.m_Data, prop.m_pDef->m_Data))
					{
						r.m_vPackedNotDefaultProps.PushBack(&prop);
						packed.m_uNonDefaultProps++;
					}
				}

				r.m_vPackedComponents.PushBack(packed);
			}
		}
	}

	// Successful load
	return true;
}

static Bool LoadXfxData(const ComponentDefinition& def, FilePath filePath, XfxData& r)
{
	pugi::xml_document root;
	pugi::xml_parse_result result = root.load_file(
		filePath.GetAbsoluteFilenameInSource().CStr(),
		pugi::parse_default,
		pugi::encoding_utf8);

	// Check and return failure on error.
	if (result.status != pugi::status_ok)
	{
		SEOUL_LOG_COOKING("%s: failed loading: %s", filePath.CStr(), result.description());
		return false;
	}

	return LoadXfxData(filePath, def, root.root(), r);
}

SEOUL_DEFINE_PACKED_STRUCT(struct Header
{
	// Main header data.
	Byte m_aMagicID[4];
	UInt32 m_uVersion;
	UInt32 m_uPlatformFourCC;
	UInt32 m_uBankSize;
	UInt32 m_uBankNameID;

	// Offsets
	UInt32 m_uStringTableOffset;
	UInt32 m_uVector3TableOffset;
	UInt32 m_uVector4TableOffset;
	UInt32 m_uFloatRangeTableOffset;
	UInt32 m_uIntegerRangeTableOffset;
	UInt32 m_uFixedFunctionTableOffset;
	UInt32 m_uColorARGBChannelDataOffset;
	UInt32 m_uFloatChannelDataOffset;
	UInt32 m_uChannelTableOffset;
	UInt32 m_uLODTableOffset;
	// Defs
	UInt32 m_uComponentDefinitions;
	UInt32 m_uComponentDefinitionsOffset;
	UInt32 m_uInputDefinitions;
	UInt32 m_uInputDefinitionsOffset;
	UInt32 m_uLODCategoryOffset;
	UInt32 m_uNameTOCOffset;
	UInt32 m_uIdTOCOffset;
	UInt32 m_uEffects;
	UInt32 m_uEffectsOffset;
});

static UInt32 GetPlatformFourCC(Platform ePlatform)
{
	switch (ePlatform)
	{
	case Platform::kLinux: // Fall-through
	case Platform::kAndroid: return (UInt32)(('N' << 0) | ('D' << 8) | ('R' << 16) | ('D' << 24));
	case Platform::kIOS: return (UInt32)(('I' << 0) | ('O' << 8) | ('S' << 16) | (' ' << 24));
	case Platform::kPC: return (UInt32)(('W' << 0) | ('n' << 8) | ('3' << 16) | ('2' << 24));
	default:
		SEOUL_FAIL("out-of-sync enum.");
		return 0u;
	};
}

static Header DefaultHeader(Platform ePlatform)
{
	Header ret;
	memset(&ret, 0, sizeof(ret));
	ret.m_aMagicID[0] = 'F';
	ret.m_aMagicID[1] = 'x';
	ret.m_aMagicID[2] = 'B';
	ret.m_aMagicID[3] = 'k';
	ret.m_uVersion = 7u;
	ret.m_uPlatformFourCC = GetPlatformFourCC(ePlatform);
	return ret;
}

namespace PropertyDataType
{

// NOTE: Order here cannot change, iterated
// to emit bank data.
enum Enum
{
	String,
	Vector3,
	Vector4,
	FloatRange,
	IntegerRange,
	FixedFunction,
	ColorKeyFrame,
	FloatKeyFrame,

	// Special - contains 4-byte per channel offsets into
	// either the ColorKeyFrame or FloatKeyFrame channels.
	ChannelTable,

	Integer,
	Float,

	INDIRECT_COUNT = ChannelTable + 1,
};

} // namespace PropertyDataType

static PropertyDataType::Enum GetPropertyDataType(PropType eType)
{
	switch (eType)
	{
	case PropType::Boolean: return PropertyDataType::Integer;
	case PropType::ColorRamp: return PropertyDataType::ColorKeyFrame;
	case PropType::CustomImage: return PropertyDataType::String;
	case PropType::CustomString: return PropertyDataType::String;
	case PropType::DropDownList: return PropertyDataType::Integer;
	case PropType::FloatRangeSlider: return PropertyDataType::FloatRange;
	case PropType::FloatSlider: return PropertyDataType::Float;
	case PropType::IntegerRangeSlider: return PropertyDataType::IntegerRange;
	case PropType::IntegerSlider: return PropertyDataType::Integer;
	case PropType::Ramp: return PropertyDataType::FloatKeyFrame;
	case PropType::Text: return PropertyDataType::String;
	case PropType::Vector3: return PropertyDataType::Vector3;
	default:
		SEOUL_FAIL("Out-of-sync enum.");
		return PropertyDataType::String;
	};
}

// Keep in sync with PropertyType in PlatformProcess.cs, FxStudio code base.
enum class SerializedPropertyType
{
	Integer,
	IntegerRange,
	IntegerArray,
	ColorKeyFrame,
	Float,
	FloatRange,
	FloatArray,
	FloatKeyFrame,
	String,
	StringArray,
	Vector3,
	Vector3Range,
	Vector3Array,
	FixedFunction,
	Vector4,
	Unknown = -1
};

static SerializedPropertyType GetSerializedPropertyType(PropType eType)
{
	switch (eType)
	{
	case PropType::Boolean: return SerializedPropertyType::Integer;
	case PropType::ColorRamp: return SerializedPropertyType::ColorKeyFrame;
	case PropType::CustomImage: return SerializedPropertyType::String;
	case PropType::CustomString: return SerializedPropertyType::String;
	case PropType::DropDownList: return SerializedPropertyType::Integer;
	case PropType::FloatRangeSlider: return SerializedPropertyType::FloatRange;
	case PropType::FloatSlider: return SerializedPropertyType::Float;
	case PropType::IntegerRangeSlider: return SerializedPropertyType::IntegerRange;
	case PropType::IntegerSlider: return SerializedPropertyType::Integer;
	case PropType::Ramp: return SerializedPropertyType::FloatKeyFrame;
	case PropType::Text: return SerializedPropertyType::String;
	case PropType::Vector3: return SerializedPropertyType::Vector3;
	default:
		SEOUL_FAIL("Out-of-sync enum.");
		return SerializedPropertyType::Unknown;
	};
}

static inline Float32 SanitizeFloat(Float f)
{
	if (IsNaN(f))
	{
		union {
			UInt32 uIn;
			Float32 fOut;
		};
		uIn = 0xFFFFFFFE; // Canonical 32-bit NaN.
		return fOut;
	}
	else if (0.0f == f)
	{
		// Handle -0.0f vs. 0.0f;
		return 0.0f;
	}
	else
	{
		return f;
	}
}

static void WriteKeys(StreamBuffer& writer, PropertyDataType::Enum eType, const RampChannel::Keyframes& keys)
{
	// Write out type if FloatKeyFrame type - ColorKeyFrame type is
	// always one type.
	if (PropertyDataType::FloatKeyFrame == eType)
	{
		// Type 0 = linear, always linear at runtime in SeoulEngine.
		writer.Write((UInt32)0);
	}
	writer.Write((UInt32)keys.GetSize());
	for (auto const& keyframe : keys)
	{
		writer.Write(SanitizeFloat((Float)keyframe.m_fTime));
		if (PropertyDataType::FloatKeyFrame == eType)
		{
			writer.Write(SanitizeFloat((Float)keyframe.m_fValueOrAlpha));
		}
		else
		{
			ColorARGBu8 rgba = keyframe.m_RGB;
			rgba.m_A = (UInt8)((Int32)keyframe.m_fValueOrAlpha);
			writer.Write((Int32)rgba.m_Value);
		}
	}
}

namespace Resample
{
	// Rdundant with SeoulMath variant, used here to exactly Match FxStudio C# code.
	inline Bool FxStudioAboutEqual(Float a, Float b, Float fEpsilon)
	{
		return (Abs(a - b) < fEpsilon);
	}

	// Redundant with SeoulMath variant, used here to exactly Match FxStudio C# code.
	static inline Float FxStudioLerp(Float fStart, Float fEnd, Float fUnitTime)
	{
		return fStart + fUnitTime * (fEnd - fStart);
	}

	static inline Vector2D FxStudioLerp(const Vector2D& a, const Vector2D& b, Float fUnitTime)
	{
		return Vector2D(FxStudioLerp(a.X, b.X, fUnitTime), FxStudioLerp(a.Y, b.Y, fUnitTime));
	}

	static Vector2D EvaluateBezier(Float fTime, const Vector2D& a, const Vector2D& b, const Vector2D& c, const Vector2D& d)
	{
		// Evaluate using DeCasteljau Algorithm.

		// a and d are the terminal points.
		// b and c are the control points.

		Vector2D ab = FxStudioLerp(a, b, fTime);
		Vector2D bc = FxStudioLerp(b, c, fTime);
		Vector2D cd = FxStudioLerp(c, d, fTime);

		Vector2D abbc = FxStudioLerp(ab, bc, fTime);
		Vector2D bccd = FxStudioLerp(bc, cd, fTime);

		return FxStudioLerp(abbc, bccd, fTime);
	}

	/**
	 * Performs a binary search to find the Bezier curve sample that most
	 * closely matches the desired time value on the curve.
	 */
	static Vector2D FindBestSample(
		Float fDesiredTime,
		const Vector2D& p0,
		const Vector2D& c0,
		const Vector2D& c1,
		const Vector2D& p1,
		Float& fLowerT,
		Float fUpperT)
	{
		// 12 is dependent on the 32 max samples used in GetLinearPackedValueFromSpline().
		static const Int iMaxIterations = 12;

		// Reasonable time threshold.
		static const Float fDesiredTimeTolerance = 1e-4f;

		// Compute our initial midpoint and evaluate the Bezier at it.
		Float fT = (fUpperT + fLowerT) / 2.0f;
		auto  ret = EvaluateBezier(fT, p0, c0, c1, p1);

		// Now iterate until we hit our desired time, or we hit the max iterations (to avoid
		// looping forever).
		Int i = 0;
		while (i < iMaxIterations && !FxStudioAboutEqual(ret.X, fDesiredTime, fDesiredTimeTolerance))
		{
			// Adjust endpoints based on where we fell relative to the midpoint.
			if (fDesiredTime > ret.X)
			{
				fLowerT = fT;
			}
			else
			{
				fUpperT = fT;
			}

			// Recompute values based on the new midpoint.
			fT = (fLowerT + fUpperT) / 2.0f;
			ret = EvaluateBezier(fT, p0, c0, c1, p1);

			// Advance to the next iteration.
			++i;
		}

		return ret;
	}

	/**
	 * This determines the point at which the error is greatest (and greater than the max-allowable error).
	 * It does not check the startIndex, and will return the endIndex if a point was not found.
	 */
	static Int DetermineBestIndex(
		const RampChannel::Keyframes& sampledValues,
		Int startIndex,
		Int endIndex,
		Float maxError)
	{
		Int result = endIndex;

		auto startSample = sampledValues[startIndex];
		auto endSample = sampledValues[endIndex];

		Float duration = endSample.m_fTime - startSample.m_fTime;

		if (duration > 0.0f)
		{
			Float largestError = 0;

			// Find the point of biggest error which is greater than maxError.
			for (Int i = startIndex + 1; i < endIndex; ++i)
			{
				Float time = sampledValues[i].m_fTime - startSample.m_fTime;
				Float unitTime = time / duration;
				Float lineValue = (Float)FxStudioLerp(startSample.m_fValueOrAlpha, endSample.m_fValueOrAlpha, unitTime);

				Float currentError = (Float)Abs(lineValue - sampledValues[i].m_fValueOrAlpha);

				if (currentError > largestError && currentError > maxError)
				{
					largestError = currentError;
					result = i;
				}
			}
		}

		return result;
	}

	static Keyframe CreateSample(
		PropertyDataType::Enum eType,
		Float fTime,
		Float fValue,
		const Keyframe& keyframe,
		const Keyframe& nextKeyframe)
	{
		Keyframe ret;
		ret.m_fTime = fTime;
		ret.m_fValueOrAlpha = fValue;
		ret.m_RGB = ColorARGBu8::TransparentBlack();
		if (PropertyDataType::FloatKeyFrame != eType)
		{
			ret.m_fValueOrAlpha = Clamp(ret.m_fValueOrAlpha, 0.0f, 255.0f);
			auto const fDuration = (nextKeyframe.m_fTime - keyframe.m_fTime);
			if (fDuration > 0.0f)
			{
				auto const fUnitTime = (fTime - keyframe.m_fTime) / fDuration;
				auto const a = keyframe.m_RGB;
				auto const b = nextKeyframe.m_RGB;
				auto const iR = Clamp((Int)a.m_R + (Int)(((Float)b.m_R - (Float)a.m_R) * fUnitTime), 0, 255);
				auto const iG = Clamp((Int)a.m_G + (Int)(((Float)b.m_G - (Float)a.m_G) * fUnitTime), 0, 255);
				auto const iB = Clamp((Int)a.m_B + (Int)(((Float)b.m_B - (Float)a.m_B) * fUnitTime), 0, 255);

				ret.m_RGB.m_R = (UInt8)iR;
				ret.m_RGB.m_G = (UInt8)iG;
				ret.m_RGB.m_B = (UInt8)iB;
			}
			else
			{
				ret.m_RGB = keyframe.m_RGB;
			}
		}

		return ret;
	}

	/**
	 * Gets a linear interpolation of a spline ramp.
	 */
	static void ResampleAsPwla(
		PropertyDataType::Enum eType,
		const RampChannel::Keyframes& keyframes,
		const RampChannel::CpPairs& cps,
		RampChannel::Keyframes& outKeys)
	{
		// Number of fixed samples used by the (SeoulEngine) runtime.
		static const Int kiTotalSamples = 32;

		// Construct an array of samples. We want samples at the exact fixed time
		// steps to line up with what will be used at runtime.
		Float const fMaxTime = keyframes[keyframes.GetSize() - 1].m_fTime;
		Float const fStep = fMaxTime / (Float)(kiTotalSamples - 1);

		RampChannel::Keyframes samples(kiTotalSamples);

		// Cache properties at the current Bezier curve endpoints.
		Int iKeyframe = 1;

		auto prev = keyframes[iKeyframe - 1];
		auto prevStartCp = cps[iKeyframe - 1].m_Start;
		auto cur = keyframes[iKeyframe];
		auto curEndCp = cps[iKeyframe].m_End;

		auto p0 = Vector2D(prev.m_fTime, prev.m_fValueOrAlpha);
		auto p1 = Vector2D(cur.m_fTime, cur.m_fValueOrAlpha);
		auto c0 = prevStartCp.ComputeStartTimeAndValue(p0, p1);
		auto c1 = curEndCp.ComputeEndTimeAndValue(p0, p1);

		Float fMinCurveTime = Max(p0.X, c0.X, c1.X, p1.X);
		Float fMaxCurveTime = Max(p0.X, c0.X, c1.X, p1.X);

		Float fLowerT = 0.0f;
		Float fUpperT = 1.0f;

		// Make sure we include the exact endpoints. Point 0.
		{
			// At the given time, find the best sample on the current Bezier.
			auto const sample = EvaluateBezier(0.0f, p0, c0, c1, p1);

			// Add the sample.
			samples[0] = CreateSample(eType, sample.X, sample.Y, prev, cur);
		}

		// Iterate for the desired number of samples - may need to advance
		// forward by keyframe.
		for (Int i = 1; i < kiTotalSamples - 1; ++i)
		{
			// This is the X time value we want to evaluate.
			Float fDesiredTime = Min((Float)i * fStep, fMaxTime);

			// Check if we want to move onto the next curve.
			while (fMaxCurveTime < fDesiredTime)
			{
				// Recompute values at the next curve.
				++iKeyframe;

				prev = keyframes[iKeyframe - 1];
				prevStartCp = cps[iKeyframe - 1].m_Start;
				cur = keyframes[iKeyframe];
				curEndCp = cps[iKeyframe].m_End;

				p0 = Vector2D(prev.m_fTime, prev.m_fValueOrAlpha);
				p1 = Vector2D(cur.m_fTime, cur.m_fValueOrAlpha);
				c0 = prevStartCp.ComputeStartTimeAndValue(p0, p1);
				c1 = curEndCp.ComputeEndTimeAndValue(p0, p1);

				fMinCurveTime = Max(p0.X, c0.X, c1.X, p1.X);
				fMaxCurveTime = Max(p0.X, c0.X, c1.X, p1.X);

				// Reset lower and upper - fLowerT persists until we change curves,
				// as we enforce that the relationship between [0, 1] t evaluator
				// and the X time value is monotonic (our Bezier curves never cross
				// themselves on X).
				fLowerT = 0.0f;
				fUpperT = 1.0f;
			}

			// At the given time, find the best sample on the current Bezier.
			auto const sample = FindBestSample(fDesiredTime, p0, c0, c1, p1, fLowerT, fUpperT);

			// Add the sample.
			samples[i] = CreateSample(eType, sample.X, sample.Y, prev, cur);
		}

		// Make sure we include the exact endpoints. Last point.
		{
			// At the given time, find the best sample on the current Bezier.
			auto const sample = EvaluateBezier(1.0f, p0, c0, c1, p1);

			// Add the sample.
			samples[kiTotalSamples - 1] = CreateSample(eType, sample.X, sample.Y, prev, cur);
		}

		outKeys.Swap(samples);
	}
}

static void WriteAnyValue(StreamBuffer& writer, PropertyDataType::Enum eType, Reflection::Any data)
{
	if (PropertyDataType::ColorKeyFrame == eType || PropertyDataType::FloatKeyFrame == eType)
	{
		RampChannel::Keyframes vResample;
		auto const& channel = *data.Cast<RampChannel const*>();

		// Splines need to be resampled into piecewise linear approximations.
		if (RampType::Linear != channel.m_eType)
		{
			auto const& keys = channel.m_vKeyframes;
			auto const& cps = channel.m_vCpPairs;
			Resample::ResampleAsPwla(eType, keys, cps, vResample);
			WriteKeys(writer, eType, vResample);
		}
		else
		{
			auto const& keys = channel.m_vKeyframes;
			WriteKeys(writer, eType, keys);
		}
	}
	else
	{
		// Special handling.
		switch (eType)
		{
		case PropertyDataType::Float:
		{
			// Sanitize float values so they are bit identical.
			data = SanitizeFloat(data.Cast<Float32>());
		}
		break;
		case PropertyDataType::FloatRange:
		{
			// Sanitize float values so they are bit identical.
			auto v = data.Cast<Vector2D>();
			v.X = SanitizeFloat(v.X);
			v.Y = SanitizeFloat(v.Y);
			data = v;
		}
		break;
		case PropertyDataType::Vector3:
		{
			// Sanitize float values so they are bit identical.
			auto v = data.Cast<Vector3D>();
			v.X = SanitizeFloat(v.X);
			v.Y = SanitizeFloat(v.Y);
			v.Z = SanitizeFloat(v.Z);
			data = v;
		}
		break;
		case PropertyDataType::Vector4:
		{
			// Sanitize float values so they are bit identical.
			auto v = data.Cast<Vector4D>();
			v.X = SanitizeFloat(v.X);
			v.Y = SanitizeFloat(v.Y);
			v.Z = SanitizeFloat(v.Z);
			v.W = SanitizeFloat(v.W);
			data = v;
		}
		break;

			// Entirely special handling - string body.
		case PropertyDataType::String:
		{
			auto const& s = data.Cast<String>();
			writer.Write(s.CStr(), s.GetSize() + 1u);
		}
		return;

		default:
			break;
		};

		// Default handling - body of the value.
		Byte const* p = (Byte const*)data.GetConstVoidStarPointerToObject();
		UInt32 const u = (UInt32)data.GetTypeInfo().GetSizeInBytes();
		writer.Write(p, u);
	}
}

struct IndirectOffset SEOUL_SEALED
{
	UInt32 m_uOffset : 24u;
	UInt32 m_uType : 8u;

	static IndirectOffset Create(PropertyDataType::Enum eType, UInt32 uOffset)
	{
		IndirectOffset ret;
		ret.m_uType = (UInt32)eType;
		ret.m_uOffset = uOffset;
		return ret;
	}
};
SEOUL_STATIC_ASSERT(sizeof(IndirectOffset) == sizeof(UInt32));

class IndirectDataBank SEOUL_SEALED
{
public:
	UInt32 Position = 0;

	IndirectDataBank() = default;

	IndirectOffset AddAnyValue(PropertyDataType::Enum eType, const Reflection::Any& data)
	{
		// String handling.
		if (PropertyDataType::String == eType)
		{
			return AddString(data.Cast<String>());
		}

		auto const uStart = m_Buffer.GetOffset();
		WriteAnyValue(m_Buffer, eType, data);
		auto const uEnd = m_Buffer.GetOffset();

		// Sanity, can never be true.
		SEOUL_ASSERT(uEnd > uStart);

		// Duplicate, rewind and done.
		UInt32 uExisting = 0u;
		if (GetOffset(m_Buffer.GetBuffer() + uStart, (uEnd - uStart), uExisting))
		{
			m_Buffer.TruncateTo(uStart);
			return IndirectOffset::Create(eType, uExisting - Position);
		}

		Entry entry;
		entry.m_uOffset = uStart;
		entry.m_uSize = (uEnd - uStart);
		m_vEntries.PushBack(entry);

		return IndirectOffset::Create(eType, uStart);
	}

	IndirectOffset AddChannelTable(IndirectOffset const* p, UInt32 uSizeInElements)
	{
		UInt32 const uSizeInBytes = sizeof(IndirectOffset) * uSizeInElements;

		UInt32 uExisting = 0u;
		if (GetOffset((Byte const*)p, uSizeInBytes, uExisting))
		{
			return IndirectOffset::Create(PropertyDataType::ChannelTable, uExisting - Position);
		}

		Entry entry;
		entry.m_uOffset = m_Buffer.GetOffset();
		entry.m_uSize = uSizeInBytes;
		m_vEntries.PushBack(entry);
		m_Buffer.Write(p, uSizeInBytes);
		return IndirectOffset::Create(PropertyDataType::ChannelTable, entry.m_uOffset);
	}

	IndirectOffset AddString(Byte const* s, UInt32 u)
	{
		UInt32 uExisting = 0u;
		if (GetOffset(s, u, uExisting))
		{
			return IndirectOffset::Create(PropertyDataType::String, uExisting - Position);
		}

		Entry entry;
		entry.m_uOffset = m_Buffer.GetOffset();
		entry.m_uSize = u;
		m_vEntries.PushBack(entry);
		m_Buffer.Write(s, u);
		m_Buffer.Write('\0');
		return IndirectOffset::Create(PropertyDataType::String, entry.m_uOffset);
	}
	IndirectOffset AddString(const String& s) { return AddString(s.CStr(), s.GetSize()); }
	IndirectOffset AddString(HString s) { return AddString(s.CStr(), s.GetSizeInBytes()); }

	void Commit(StreamBuffer& rBuffer)
	{
		rBuffer.Write(m_Buffer.GetBuffer(), m_Buffer.GetTotalDataSizeInBytes());
	}

	typedef FixedArray<IndirectDataBank, PropertyDataType::INDIRECT_COUNT> IndirectDataBanks;
	void FixupChannelTables(const IndirectDataBanks& banks)
	{
		auto pIndirect = (IndirectOffset*)m_Buffer.GetBuffer();
		auto const pEnd = (IndirectOffset*)(m_Buffer.GetBuffer() + m_Buffer.GetOffset());
		for (; pIndirect < pEnd; ++pIndirect)
		{
			IndirectOffset const offset = *pIndirect;
			*((UInt32*)pIndirect) = banks[offset.m_uType].Position + offset.m_uOffset;
		}
	}

	Bool GetOffset(Byte const* s, UInt32 uSize, UInt32& ruOffset) const
	{
		for (auto const& e : m_vEntries)
		{
			if (uSize != e.m_uSize)
			{
				continue;
			}

			if (0 == memcmp(s, m_Buffer.GetBuffer() + e.m_uOffset, uSize))
			{
				ruOffset = Position + e.m_uOffset;
				return true;
			}
		}

		return false;
	}

	Bool GetOffset(HString s, UInt32& ruOffset) const { return GetOffset(s.CStr(), s.GetSizeInBytes(), ruOffset); }
	Bool GetOffset(const String& s, UInt32& ruOffset) const { return GetOffset(s.CStr(), s.GetSize(), ruOffset); }

	Bool WriteOffset(FilePath filePath, StreamBuffer& writer, Byte const* s, UInt32 u) const
	{
		UInt32 uOffset = 0u;
		if (!GetOffset(s, u, uOffset))
		{
			SEOUL_LOG_COOKING("%s: '%.*s' not found in string table.", (Int)u, s);
			return false;
		}

		writer.Write(uOffset);
		return true;
	}
	Bool WriteOffset(FilePath filePath, StreamBuffer& writer, const String& s) const { return WriteOffset(filePath, writer, s.CStr(), s.GetSize()); }
	Bool WriteOffset(FilePath filePath, StreamBuffer& writer, HString s) const { return WriteOffset(filePath, writer, s.CStr(), s.GetSizeInBytes()); }

private:
	StreamBuffer m_Buffer;

	struct Entry
	{
		UInt32 m_uOffset{};
		UInt32 m_uSize{};
	};
	typedef Vector<Entry, MemoryBudgets::Cooking> Entries;
	Entries m_vEntries;

	SEOUL_DISABLE_COPY(IndirectDataBank);
};
typedef FixedArray<IndirectDataBank, PropertyDataType::INDIRECT_COUNT> IndirectDataBanks;

static inline UInt32 ToFileOffset(const IndirectDataBanks& banks, IndirectOffset offset)
{
	return banks[offset.m_uType].Position + offset.m_uOffset;
}

class ChannelDataBank SEOUL_SEALED
{
public:
	UInt32 Position = 0;

	ChannelDataBank() = default;

	void Commit(StreamBuffer& rBuffer)
	{
		// TODO:
	}

private:
	SEOUL_DISABLE_COPY(ChannelDataBank);
};

static void FixupOffset(StreamBuffer& writer, UInt32 uToFixLocation)
{
	auto const uCurrent = writer.GetOffset();
	writer.SeekToOffset(uToFixLocation);
	writer.Write((UInt32)uCurrent);
	writer.SeekToOffset(uCurrent);
}

static const String& GetPhaseName(const ComponentDefinition& def, const Phase& phase)
{
	for (auto const& phaseDef : def.m_vPhases)
	{
		if (phaseDef.m_Id == phase.m_DefinitionId)
		{
			return phaseDef.m_sName;
		}
	}

	return ksEmpty;
}

typedef HashTable<void const*, IndirectOffset, MemoryBudgets::Cooking> IndirectLookup;

static void FillPropIndirectData(
	const ComponentDefinition& def,
	const XfxData& effect,
	IndirectLookup& rtIndirect,
	IndirectDataBanks& rBanks,
	PropertyDataType::Enum ePropertyDataType,
	void const* pProp,
	const Reflection::Any& value)
{
	// Nothing to do if an inline property.
	if (ePropertyDataType >= PropertyDataType::INDIRECT_COUNT)
	{
		return;
	}

	// Special handling - handle each channel separately, accumulating a
	// lookup table, which then becomes the lookup, unless there is only 1 channel.
	IndirectOffset offset{};
	if (PropertyDataType::ColorKeyFrame == ePropertyDataType ||
		PropertyDataType::FloatKeyFrame == ePropertyDataType)
	{
		auto const& rampData = value.Cast<RampChannelData>();
		auto const uSize = rampData.m_vRampChannels.GetSize();
		if (uSize == 1u)
		{
			offset = rBanks[ePropertyDataType].AddAnyValue(
				ePropertyDataType,
				rampData.m_vRampChannels.Get(0));
		}
		else
		{
			StackOrHeapArray<IndirectOffset, 4u, MemoryBudgets::Cooking> aChannels(uSize);
			for (UInt32 i = 0u; i < uSize; ++i)
			{
				aChannels[i] = rBanks[ePropertyDataType].AddAnyValue(
					ePropertyDataType,
					rampData.m_vRampChannels.Get(i));
			}
			offset = rBanks[PropertyDataType::ChannelTable].AddChannelTable(
				aChannels.Data(),
				aChannels.GetSize());
		}
	}
	else
	{
		offset = rBanks[ePropertyDataType].AddAnyValue(ePropertyDataType, value);
	}

	// Add indirection mapping.
	rtIndirect.Insert(pProp, offset);
}

static void FillDataBanks(
	const ComponentDefinition& def,
	const XfxData& effect,
	IndirectLookup& rtIndirect,
	IndirectDataBanks& rBanks)
{
	auto& stringTable = rBanks[PropertyDataType::String];

	// Empty values in various data banks, expected. Mostly to exactly match the
	// behavior of the C# processor in the FxStudio source code.
	stringTable.AddString(String());
	rBanks[PropertyDataType::Vector3].AddAnyValue(PropertyDataType::Vector3, Vector3D::Zero());
	rBanks[PropertyDataType::Vector4].AddAnyValue(PropertyDataType::Vector4, Vector4D::Zero());

	// Add effect bank name
	stringTable.AddString(effect.m_sBankName);

	// Add component names
	for (auto const& comp : def.m_vComponents)
	{
		// Add in component factory class name
		stringTable.AddString(comp.m_Class);

		// Add component definition property names and values
		for (auto const& prop : comp.m_Props.m_vProps)
		{
			stringTable.AddString(prop.m_sFullName);
			auto const ePropertyDataType = GetPropertyDataType(prop.m_eType);
			FillPropIndirectData(def, effect, rtIndirect, rBanks, ePropertyDataType, &prop, prop.m_Data);
		}
	}

	// Add effect types from the component defintion
	for (auto const& phase : def.m_vPhases)
	{
		stringTable.AddString(phase.m_sName);
	}

	// Add effect name
	stringTable.AddString(effect.m_sEffectName);
	for (auto const& pProp : effect.m_vPackedNotDefaultProps)
	{
		auto const& prop = *pProp;
		auto const ePropertyDataType = GetPropertyDataType(prop.m_pDef->m_eType);
		FillPropIndirectData(def, effect, rtIndirect, rBanks, ePropertyDataType, &prop, prop.m_Data);
	}
}

static void WritePropValue(
	const IndirectDataBanks& banks,
	const IndirectLookup& lookup,
	StreamBuffer& writer,
	PropertyDataType::Enum eType,
	void const* pProp,
	const Reflection::Any& value)
{
	// Indirect, write offset.
	if (eType < PropertyDataType::INDIRECT_COUNT)
	{
		IndirectOffset offset{};
		SEOUL_VERIFY(lookup.GetValue(pProp, offset));
		writer.Write((UInt32)ToFileOffset(banks, offset));
	}
	// Otherwise, right value directly.
	else
	{
		WriteAnyValue(writer, eType, value);
	}
}

} // namespace FxBankCookDetail

CheckedPtr<FxBankCookDetail::ComponentDefinition const> FxBankLoadComponentDefinition(Platform ePlatform)
{
	CheckedPtr<FxBankCookDetail::ComponentDefinition> pComp(SEOUL_NEW(MemoryBudgets::Cooking) FxBankCookDetail::ComponentDefinition);
	if (!FxBankCookDetail::LoadComponentDefinition(ePlatform, *pComp))
	{
		SafeDelete(pComp);
		return nullptr;
	}

	return pComp;
}

void FxBankDestroyComponentDefinition(CheckedPtr<FxBankCookDetail::ComponentDefinition const>& rp)
{
	SafeDelete(rp);
}

Bool FxBankCook(const FxBankCookDetail::ComponentDefinition& def, Platform ePlatform, FilePath filePath, StreamBuffer& r)
{
	using namespace FxBankCookDetail;

	XfxData effect;
	if (!LoadXfxData(def, filePath, effect))
	{
		return false;
	}

	IndirectLookup indirectLookup;
	IndirectDataBanks dataBanks;

	// Add dynamic data
	FillDataBanks(def, effect, indirectLookup, dataBanks);

	// Allocate and Serialize header
	StreamBuffer writer;

	// Placeholder header - fill in offsets as we go, replace later.
	Header headerData = DefaultHeader(ePlatform);

	// Write header.
	writer.Write(headerData);

	// Serialize dynamic data banks.
	SEOUL_STATIC_ASSERT(PropertyDataType::ChannelTable == PropertyDataType::INDIRECT_COUNT - 1);
	for (Int i = 0; i < PropertyDataType::INDIRECT_COUNT - 1; ++i)
	{
		// Commit.
		dataBanks[i].Position = writer.GetOffset();
		dataBanks[i].Commit(writer);
		// Align.
		writer.PadTo((UInt32)RoundUpToAlignment(writer.GetOffset(), 4));
	}
	// Channel table last - first, fixup offsets now that other banks have been populated.
	{
		// Commit.
		dataBanks[PropertyDataType::ChannelTable].FixupChannelTables(dataBanks);
		dataBanks[PropertyDataType::ChannelTable].Position = writer.GetOffset();
		dataBanks[PropertyDataType::ChannelTable].Commit(writer);
		// Align.
		writer.PadTo((UInt32)RoundUpToAlignment(writer.GetOffset(), 4));
	}

	// String table.
	auto const& stringTable = dataBanks[PropertyDataType::String];

	// LOD bank anchor (unused).
	UInt32 const uLodDataBankPosition = writer.GetOffset();

	// Store the start of the component definitions
	UInt32 const uStreamComponentDefinitionOffset = writer.GetOffset();

	// Serialize component defintions
	HashTable<HString, UInt32, MemoryBudgets::Cooking> tComponentOffsets;
	HashTable<String, UInt32, MemoryBudgets::Cooking> tPropOffsets;
	for (auto const& comp : def.m_vComponents)
	{
		// Write component data

		// Add offset data
		tComponentOffsets.Insert(comp.m_Class, writer.GetOffset());

		// Write component name
		if (!stringTable.WriteOffset(filePath, writer, comp.m_Class))
		{
			return false;
		}

		// Write number of properties.
		writer.Write((UInt32)comp.m_Props.m_vProps.GetSize());

		// Write each property
		for (auto const& prop : comp.m_Props.m_vProps)
		{
			auto const ePropertyDataType = GetPropertyDataType(prop.m_eType);
			auto const eSerializedPropertyType = GetSerializedPropertyType(prop.m_eType);

			tPropOffsets.Insert(String(comp.m_Class) + prop.m_Id.ToString(), writer.GetOffset());

			// Write property name
			if (!stringTable.WriteOffset(filePath, writer, prop.m_sFullName))
			{
				return false;
			}

			// Write property type
			writer.Write((UInt32)eSerializedPropertyType);

			// Flags.
			UInt32 uFlags = 0u;

			// Write channel count if a ramp property - fits in lower 6 bits
			if (PropertyDataType::ColorKeyFrame == ePropertyDataType ||
				PropertyDataType::FloatKeyFrame == ePropertyDataType)
			{
				SEOUL_ASSERT(!prop.m_vRampChannelDefs.IsEmpty());
				uFlags |= Min(prop.m_vRampChannelDefs.GetSize() - 1u, 63u);
			}

			// Flag commit.
			writer.Write(uFlags);

			// Write the property's value
			WritePropValue(dataBanks, indirectLookup, writer, ePropertyDataType, &prop, prop.m_Data);
		}
	}

	// Write inputs.
	UInt32 const uStreamInputOffset = writer.GetOffset();

	// Serialize the LOD categories.
	UInt32 const uLodCategoryBankPosition = writer.GetOffset();

	// Write effect name TOC - this is just a single entry (FxStudio
	// originally stored multiple FX per bank, but SeoulEngine
	// uses it as one bank per effect).
	UInt32 const uStreamEffectBankNameTOCOffset = writer.GetOffset();
	// Effect offset will be the current offset + 16.
	UInt32 const uEffectOffset = writer.GetOffset() + 16u;
	if (!stringTable.WriteOffset(filePath, writer, effect.m_sEffectName))
	{
		return false;
	}
	// Offset.
	writer.Write(uEffectOffset);

	// Write effect id TOC - this is always 0 since SeoulEngine
	// uses 1 .xfx per fx bank.
	const UInt32 kuFixedEffectId = 0u;
	UInt32 const uStreamEffectBankIDTOCOffset = writer.GetOffset();
	writer.Write(kuFixedEffectId);
	// Offset.
	writer.Write(uEffectOffset);

	// Store the start of the effects
	UInt32 const uStreamEffectsOffset = writer.GetOffset();

	// Write effect name
	if (!stringTable.WriteOffset(filePath, writer, effect.m_sEffectName))
	{
		return false;
	}

	// Write effect id
	writer.Write(kuFixedEffectId);

	// Write the effect duration
	writer.Write(effect.ComputeDuration());

	// Sentinel that indicates "no LOD category".
	writer.Write(UIntMax);

	// Write number of inputs
	writer.Write((UInt32)0u);

	// Write start of input bank, this will be re-written below.
	UInt32 const uStreamEffectInputOffset = writer.GetOffset();
	writer.Write((UInt32)0u);

	// Write input data size.
	writer.Write((UInt32)0u);

	// Write number of phases
	writer.Write((UInt32)effect.m_vPhases.GetSize());

	// Write phase start offset (fixup in phase 2)
	UInt32 const uStreamPhasePos = writer.GetOffset();
	writer.Write((UInt32)0u);

	// Write number of components
	writer.Write((UInt32)effect.m_vPackedComponents.GetSize());

	// Write component start offset (fixup)
	UInt32 const uStreamComponentPos = writer.GetOffset();
	writer.Write((UInt32)0u);

	// Write inuts and fixup input start offset.
	FixupOffset(writer, uStreamEffectInputOffset);

	// TODO: Would right inputs here, if supported.

	// Write phases and fixup phase start offset
	FixupOffset(writer, uStreamPhasePos);

	for (auto const& phase : effect.m_vPhases)
	{
		// Write the phase name
		if (!stringTable.WriteOffset(filePath, writer, GetPhaseName(def, phase)))
		{
			return false;
		}

		// Write the phase duration
		writer.Write(phase.m_fDuration);

		// Write play count
		writer.Write((UInt32)phase.m_iPlayCount);
	}

	// Write components
	{
		// Fixup component start offset
		FixupOffset(writer, uStreamComponentPos);

		auto iNonDefault = effect.m_vPackedNotDefaultProps.Begin();
		for (auto const& packed : effect.m_vPackedComponents)
		{
			// Component.
			auto const& component = *packed.m_pComponent;

			// Write component defintion offset
			{
				UInt32 uOffset = 0u;
				if (!tComponentOffsets.GetValue(component.m_Class, uOffset))
				{
					SEOUL_LOG_COOKING("%s: invalid component class: %s", filePath.CStr(), component.m_Class.CStr());
					return false;
				}
				else
				{
					writer.Write((UInt32)uOffset);
				}
			}

			// Write start time
			writer.Write(component.m_fStart);

			// Write end time
			writer.Write(component.m_fEnd);

			// Write track group index
			writer.Write((UInt32)packed.m_uTrackGroupIndex);

			// Write number of non default fixed size properties
			writer.Write((UInt32)packed.m_uNonDefaultProps);

			// Write non default fixed size properties offset (fixup)
			UInt32 const uStreamNonDefaultFixedSizePos = writer.GetOffset();
			writer.Write((UInt32)0);

			// Write number of inputs
			writer.Write((UInt32)0u);

			// Write input offset, will be filled in below.
			UInt32 const uStreamComponentInputProperties = writer.GetOffset();
			writer.Write((UInt32)0);

			// Write number of dynamics (always 0 - not supported)
			writer.Write((UInt32)0u);

			// Write dynamic offset (always 0 - not supported)
			writer.Write((UInt32)0u);

			// Write non default fixed size properties
			if (packed.m_uNonDefaultProps > 0u)
			{
				// Fixup offset of non default fixed size properties
				FixupOffset(writer, uStreamNonDefaultFixedSizePos);

				auto const iEnd = iNonDefault + packed.m_uNonDefaultProps;
				for (; iNonDefault < iEnd; ++iNonDefault)
				{
					auto const& prop = **iNonDefault;

					UInt32 uPropOffset = 0u;
					if (!tPropOffsets.GetValue(String(component.m_Class) + prop.m_Id.ToString(), uPropOffset))
					{
						SEOUL_LOG_COOKING("%s::%s not found", component.m_Class.CStr(), prop.m_Id.ToString().CStr());
						return false;
					}

					// Write property offset
					writer.Write((UInt32)uPropOffset);

					auto const ePropertyDataType = GetPropertyDataType(prop.m_pDef->m_eType);

					// Pack the non-lod value.
					WritePropValue(dataBanks, indirectLookup, writer, ePropertyDataType, &prop, prop.m_Data);
				}
			}

			// Fixup offset to input wired properties.
			FixupOffset(writer, uStreamComponentInputProperties);
		}
	}

	// Store the current stream position
	UInt32 const uStreamEndPos = writer.GetOffset();

	// Fixup bank size
	UInt32 const uBankSize = uStreamEndPos;

	// Update header.
	headerData.m_uBankSize = uBankSize;
	if (!stringTable.GetOffset(effect.m_sBankName, headerData.m_uBankNameID))
	{
		SEOUL_LOG_COOKING("%s: '%s' not in string table.", filePath.CStr(), effect.m_sBankName.CStr());
		return false;
	}
	headerData.m_uStringTableOffset = (UInt32)dataBanks[PropertyDataType::String].Position;
	headerData.m_uVector3TableOffset = (UInt32)dataBanks[PropertyDataType::Vector3].Position;
	headerData.m_uVector4TableOffset = (UInt32)dataBanks[PropertyDataType::Vector4].Position;
	headerData.m_uFloatRangeTableOffset = (UInt32)dataBanks[PropertyDataType::FloatRange].Position;
	headerData.m_uIntegerRangeTableOffset = (UInt32)dataBanks[PropertyDataType::IntegerRange].Position;
	headerData.m_uFixedFunctionTableOffset = (UInt32)dataBanks[PropertyDataType::FixedFunction].Position;
	headerData.m_uColorARGBChannelDataOffset = (UInt32)dataBanks[PropertyDataType::ColorKeyFrame].Position;
	headerData.m_uFloatChannelDataOffset = (UInt32)dataBanks[PropertyDataType::FloatKeyFrame].Position;
	headerData.m_uChannelTableOffset = (UInt32)dataBanks[PropertyDataType::ChannelTable].Position;
	headerData.m_uLODTableOffset = (UInt32)uLodDataBankPosition;
	headerData.m_uComponentDefinitions = (UInt32)def.m_vComponents.GetSize();
	headerData.m_uComponentDefinitionsOffset = (UInt32)uStreamComponentDefinitionOffset;
	headerData.m_uLODCategoryOffset = (UInt32)uLodCategoryBankPosition;
	headerData.m_uInputDefinitions = (UInt32)0u;
	headerData.m_uInputDefinitionsOffset = (UInt32)uStreamInputOffset;
	headerData.m_uNameTOCOffset = (UInt32)uStreamEffectBankNameTOCOffset;
	headerData.m_uIdTOCOffset = (UInt32)uStreamEffectBankIDTOCOffset;
	headerData.m_uEffects = (UInt32)1u;
	headerData.m_uEffectsOffset = (UInt32)uStreamEffectsOffset;

	// Write back into stream at appropriate position.
	writer.SeekToOffset(0);
	writer.Write(headerData);
	writer.SeekToOffset(uStreamEndPos);

	r.Swap(writer);
	return true;
}

} // namespace Cooking

SEOUL_TYPE(Cooking::FxBankCookDetail::RampChannel);
SEOUL_TYPE(Cooking::FxBankCookDetail::RampChannelData);

} // namespace Seoul
