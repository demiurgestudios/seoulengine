/**
 * \file SceneAssetCook.cpp
 * \brief Conversion of source asset file types (e.g. .fbx) into
 * runtime SeoulEngine .ssa file format.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Compress.h"
#include "FileManager.h"
#include "GamePaths.h"
#include "HashSet.h"
#include "HashTable.h"
#include "Logger.h"
#include "Material.h"
#include "MaterialLibrary.h"
#include "Matrix3x4.h"
#include "Matrix4D.h"
#include "Path.h"
#include "Prereqs.h"
#include "PrimitiveType.h"
#include "Quaternion.h"
#include "SeoulFile.h"
#include "SeoulFileReaders.h"
#include "SeoulFileWriters.h"
#include "SeoulHString.h"
#include "SeoulString.h"
#include "Vector.h"
#include "VertexElement.h"

#include <fbxsdk.h>
#include <tootlelib.h>

namespace Seoul
{

namespace Cooking
{

// TODO: Big endian support.
SEOUL_STATIC_ASSERT(SEOUL_LITTLE_ENDIAN == 1);

// Convenience utility.
static const char* ksEmptyString = "";

template <typename T>
class ScopedFbxPointer SEOUL_SEALED
{
public:
	ScopedFbxPointer(T* p = nullptr)
		: m_p(p)
	{
	}

	~ScopedFbxPointer()
	{
		Reset();
	}

	T& operator*()
	{
		SEOUL_ASSERT(nullptr != m_p);
		return *m_p;
	}

	const T& operator*() const
	{
		SEOUL_ASSERT(nullptr != m_p);
		return *m_p;
	}

	T* operator->() const
	{
		return m_p;
	}

	operator T const*() const
	{
		return GetPtr();
	}

	operator T*()
	{
		return GetPtr();
	}

	T const* GetPtr() const
	{
		return m_p;
	}

	T* GetPtr()
	{
		return m_p;
	}

	Bool IsValid() const
	{
		return (nullptr != m_p);
	}

	void Reset(T* p = nullptr)
	{
		if (nullptr != m_p)
		{
			m_p->Destroy();
			m_p = nullptr;
		}

		m_p = p;
	}

private:
	SEOUL_DISABLE_COPY(ScopedFbxPointer);

	T* m_p;
}; // class ScopedFbxPointer

struct BoneEntry SEOUL_SEALED
{
	BoneEntry()
		: m_Id()
		, m_ParentId()
		, m_qRotation(Quaternion::Identity())
		, m_vPosition(Vector3D::Zero())
		, m_vScale(Vector3D::One())
	{
	}

	HString m_Id;
	HString m_ParentId;
	Quaternion m_qRotation;
	Vector3D m_vPosition;
	Vector3D m_vScale;
}; // struct BoneEntry
typedef Vector<BoneEntry, MemoryBudgets::Cooking> Bones;

// Non-virtual by design (these are simple structs used
// in great quantities, and cache usage is a critical
// consideration). Don't use BaseKeyFrame directly, always
// use the subclasses (treat BaseKeyFrame as a mixin).
struct BaseKeyFrame
{
	BaseKeyFrame(Float fT = 0.0f)
		: m_fTime(fT)
	{
	}

	Float32 m_fTime;
}; // struct BaseKeyFrame
SEOUL_STATIC_ASSERT(sizeof(BaseKeyFrame) == 4);

struct KeyFrame3D SEOUL_SEALED : public BaseKeyFrame
{
	KeyFrame3D(Float fT = 0.0f, const Vector3D& v = Vector3D::Zero())
		: BaseKeyFrame(fT)
		, m_Value(v)
	{
	}

	Vector3D m_Value;
}; // struct KeyFrame3D
SEOUL_STATIC_ASSERT(sizeof(KeyFrame3D) == 16);

struct KeyFrameRotation SEOUL_SEALED : public BaseKeyFrame
{
	KeyFrameRotation(Float fT = 0.0f, const Quaternion& q = Quaternion::Identity())
		: BaseKeyFrame(fT)
		, m_Value(q)
	{
	}

	Quaternion m_Value;
}; // struct KeyFrameRotation
SEOUL_STATIC_ASSERT(sizeof(KeyFrameRotation) == 20);

} // namespace Cooking

template <> struct CanMemCpy<Cooking::BaseKeyFrame> { static const Bool Value = true; };
template <> struct CanMemCpy<Cooking::KeyFrame3D> { static const Bool Value = true; };
template <> struct CanMemCpy<Cooking::KeyFrameRotation> { static const Bool Value = true; };
template <> struct CanZeroInit<Cooking::BaseKeyFrame> { static const Bool Value = true; };
template <> struct CanZeroInit<Cooking::KeyFrame3D> { static const Bool Value = true; };

namespace Cooking
{

typedef Vector<KeyFrame3D, MemoryBudgets::Animation3D> KeyFrames3D;
typedef Vector<KeyFrameRotation, MemoryBudgets::Animation3D> KeyFramesRotation;

static inline Vector3D Interpolate(const Vector3D& a, const Vector3D& b, Float fT)
{
	return Vector3D::Lerp(a, b, fT);
}
static inline Quaternion Interpolate(const Quaternion& a, const Quaternion& b, Float fT)
{
	return Quaternion::Slerp(a, b, fT);
}

template <typename T>
static inline T InterpolateKeys(const T& a, const T& b, Float fTime)
{
	auto const fA = (fTime - a.m_fTime) / (b.m_fTime - a.m_fTime);
	return T(fTime, Interpolate(a.m_Value, b.m_Value, fA));
}

template <typename T>
static inline void Simplify(T& rv, Float32 fTolerance)
{
	// Early out if we don't have at least 2 samples.
	if (rv.GetSize() < 2u)
	{
		return;
	}

	T vNew;
	vNew.Reserve(rv.GetSize());

	// Add the first keyframe.
	vNew.PushBack(rv.Front());

	Int32 const iSize = (Int32)rv.GetSize();
	Int32 iPrev = 0;
	for (Int32 i = 2; i < iSize; ++i)
	{
		auto const derived = InterpolateKeys(rv[i], rv[iPrev], rv[i-1].m_fTime);

		// i-1 is significant, so it is added and becomes the
		// new start frame.
		if (!Equals(derived.m_Value, rv[i - 1].m_Value, fTolerance))
		{
			vNew.PushBack(rv[i - 1]);
			iPrev = i - 1;
		}
	}

	// Add the last keyframe.
	vNew.PushBack(rv.Back());

	// Replace old with new.
	rv.Swap(vNew);
}

static inline void SimplifyRotation(KeyFramesRotation& rv)
{
	static const Float32 kfRotationTolerance = 1e-4f;

	Simplify(rv, kfRotationTolerance);

	// Points elimited, swap with empty.
	if (rv.GetSize() == 2u &&
		Equals(rv.Front().m_Value, rv.Back().m_Value, kfRotationTolerance) &&
		Equals(rv.Front().m_Value, Quaternion::Identity(), kfRotationTolerance))
	{
		KeyFramesRotation empty;
		rv.Swap(empty);
	}
}

static inline void SimplifyScale(KeyFrames3D& rv)
{
	static const Float32 kfScaleTolerance = 1e-4f;

	Simplify(rv, kfScaleTolerance);

	// Points elimited, swap with empty.
	if (rv.GetSize() == 2u &&
		Equals(rv.Front().m_Value, rv.Back().m_Value, kfScaleTolerance) &&
		Equals(rv.Front().m_Value, Vector3D::One(), kfScaleTolerance))
	{
		KeyFrames3D empty;
		rv.Swap(empty);
	}
}

static inline void SimplifyTranslation(KeyFrames3D& rv)
{
	static const Float32 kfTranslationTolerance = 1e-3f;

	Simplify(rv, kfTranslationTolerance);

	// Points elimited, swap with empty.
	if (rv.GetSize() == 2u &&
		Equals(rv.Front().m_Value, rv.Back().m_Value, kfTranslationTolerance) &&
		Equals(rv.Front().m_Value, Vector3D::Zero(), kfTranslationTolerance))
	{
		KeyFrames3D empty;
		rv.Swap(empty);
	}
}

struct BoneKeyFrames SEOUL_SEALED
{
	BoneKeyFrames()
		: m_vRotation()
		, m_vScale()
		, m_vTranslation()
	{
	}

	KeyFramesRotation m_vRotation;
	KeyFrames3D m_vScale;
	KeyFrames3D m_vTranslation;
}; // struct BoneKeyFrames

struct UByte4 SEOUL_SEALED
{
	UByte4(UInt8 uX = 0u, UInt8 uY = 0u, UInt8 uZ = 0u, UInt8 uW = 0u)
		: m_uX(uX)
		, m_uY(uY)
		, m_uZ(uZ)
		, m_uW(uW)
	{
	}

	Bool operator==(UByte4 b) const
	{
		return
			(m_uX == b.m_uX) &&
			(m_uY == b.m_uY) &&
			(m_uZ == b.m_uZ) &&
			(m_uW == b.m_uW);
	}

	Bool operator!=(UByte b) const
	{
		return !(*this == b);
	}

	UInt8 m_uX;
	UInt8 m_uY;
	UInt8 m_uZ;
	UInt8 m_uW;
}; // struct UByte4

struct Int4 SEOUL_SEALED
{
	Int4(Int32 iX = 0, Int32 iY = 0, Int32 iZ = 0, UInt32 iW = 0)
		: m_iX(iX)
		, m_iY(iY)
		, m_iZ(iZ)
		, m_iW(iW)
	{
	}

	Bool operator==(Int4 b) const
	{
		return
			(m_iX == b.m_iX) &&
			(m_iY == b.m_iY) &&
			(m_iZ == b.m_iZ) &&
			(m_iW == b.m_iW);
	}

	Bool operator!=(Int4 b) const
	{
		return !(*this == b);
	}

	Int32 m_iX;
	Int32 m_iY;
	Int32 m_iZ;
	Int32 m_iW;
}; // struct Int4

struct SkinnedVertex
{
	Vector3D m_vPosition;
	Vector3D m_vNormal;
	Vector2D m_vTextureCoords;
	Vector4D m_vBlendWeights;
	UByte4 m_vBlendIndices;
};
SEOUL_STATIC_ASSERT(sizeof(SkinnedVertex) ==  52);

typedef Vector<UInt32, MemoryBudgets::Cooking> Indices;
typedef Vector<SkinnedVertex, MemoryBudgets::Cooking> Vertices;

class ScopedTootle SEOUL_SEALED
{
public:
	ScopedTootle()
	{
		if (1 == ++s_Init)
		{
			SEOUL_VERIFY(TOOTLE_OK == TootleInit());
		}
	}

	~ScopedTootle()
	{
		if (0 == --s_Init)
		{
			TootleCleanup();
		}
	}

	Bool OptimizeInPlace(
		Indices& rvIndices,
		Vertices& rvVertices) const
	{
		if (!rvIndices.IsEmpty() && !rvVertices.IsEmpty())
		{
			if (TOOTLE_OK == TootleFastOptimize(
				rvVertices.Data(),
				rvIndices.Data(),
				rvVertices.GetSize(),
				rvIndices.GetSize() / 3,
				sizeof(SkinnedVertex),
				TOOTLE_DEFAULT_VCACHE_SIZE,
				TOOTLE_CCW,
				rvIndices.Data(),
				nullptr))
			{
				Bool const bReturn = (TOOTLE_OK == TootleOptimizeVertexMemory(
					rvVertices.Data(),
					rvIndices.Data(),
					rvVertices.GetSize(),
					rvIndices.GetSize() / 3,
					sizeof(SkinnedVertex),
					rvVertices.Data(),
					rvIndices.Data(),
					nullptr));
				return bReturn;
			}
		}

		return true;
	}

private:
	static Atomic32 s_Init;

	SEOUL_DISABLE_COPY(ScopedTootle);
}; // class ScopedTootle
Atomic32 ScopedTootle::s_Init(0);

enum class ValueType
{
	// NOTE: Order here matters - see App\Source\Authored\Effects\World\MeshVariations.fxh
	Diffuse,
	Emissive,
	AlphaMask,
	COUNT,
};

static inline constexpr Bool IsDiscardValue(ValueType eType, const Vector3D& v)
{
	switch (eType)
	{
	case ValueType::Diffuse:
	case ValueType::Emissive:
	case ValueType::AlphaMask:
		return (Vector3D::Zero() == v);
	default:
		return false;
	};
}

static inline constexpr Byte const* ValueTypeToCStr(ValueType eType)
{
	switch (eType)
	{
	case ValueType::Diffuse: return "Diffuse";
	case ValueType::Emissive: return "Emissive";
	case ValueType::AlphaMask: return "AlphaMask";
	default:
		return "";
	};
}

static inline constexpr Byte const* ValueTypeToFbxMaterialPropertyName(ValueType eType)
{
	switch (eType)
	{
	case ValueType::Diffuse: return FbxSurfaceMaterial::sDiffuse;
	case ValueType::Emissive: return FbxSurfaceMaterial::sEmissive;
	case ValueType::AlphaMask: return "3dsMax|Parameters|cutout_map"; // TODO:
	default:
		return "";
	};
}

class ValueEntry SEOUL_SEALED
{
private:
	FilePath m_Texture;
	Vector3D m_vColor;
	Bool m_bSet = false;

public:
	Bool operator==(const ValueEntry& b) const
	{
		return (m_Texture == b.m_Texture && m_vColor == b.m_vColor && m_bSet == b.m_bSet);
	}

	Bool operator!=(const ValueEntry& b) const
	{
		return !(*this == b);
	}

	UInt32 ComputeHash() const
	{
		UInt32 u = 0u;
		IncrementalHash(u, Seoul::GetHash(m_Texture));
		IncrementalHash(u, Seoul::GetHash(m_vColor.X));
		IncrementalHash(u, Seoul::GetHash(m_vColor.Y));
		IncrementalHash(u, Seoul::GetHash(m_vColor.Z));
		IncrementalHash(u, Seoul::GetHash(m_bSet ? 1 : 0));
		return u;
	}

	MaterialParameterType ComputeMaterialParameterType() const
	{
		if (m_Texture.IsValid())
		{
			return MaterialParameterType::kTexture;
		}
		else
		{
			return MaterialParameterType::kVector4D;
		}
	}

	String ComputeParameterName(ValueType eType) const
	{
		return String::Printf("seoul_%s%s",
			ValueTypeToCStr(eType),
			(IsTexture() ? "Texture" : "Color"));
	}

	Bool IsSet() const
	{
		return m_bSet;
	}

	Bool IsTexture() const
	{
		return m_Texture.IsValid();
	}

	void Set(const FilePath& filePath)
	{
		m_Texture = filePath;
		m_vColor = Vector3D::Zero();
		m_bSet = true;
	}

	void Set(const Vector3D& v)
	{
		m_Texture = FilePath();
		m_vColor = v;
		m_bSet = true;
	}

	Bool WriteValue(SyncFile& file) const
	{
		if (!m_bSet)
		{
			SEOUL_LOG_COOKING("Programmer error, writing an unset parameter.");
			return false;
		}

		if (m_Texture.IsValid())
		{
			return WriteString(file, m_Texture.ToSerializedUrl());
		}
		else
		{
			return WriteVector4D(file, Vector4D(m_vColor, 1.0f));
		}
	}
}; // struct ValueEntry

struct MaterialEntry SEOUL_SEALED
{
	FixedArray<ValueEntry, (UInt32)ValueType::COUNT> m_aValues;
	UInt32 m_uHashValue = 0u;

	Bool operator==(const MaterialEntry& b) const
	{
		for (UInt32 i = 0u, iEnd = m_aValues.GetSize(); i < iEnd; ++i)
		{
			if (m_aValues[i] != b.m_aValues[i])
			{
				return false;
			}
		}

		return true;
	}

	Bool operator!=(const MaterialEntry& b) const
	{
		return !(*this == b);
	}

	void RecomputeHash()
	{
		m_uHashValue = 0u;
		for (auto const& value : m_aValues)
		{
			IncrementalHash(m_uHashValue, value.ComputeHash());
		}
	}

	UInt32 ComputeSetValueCount() const
	{
		UInt32 u = 0u;
		for (auto const& value : m_aValues)
		{
			u += value.IsSet() ? 1u : 0u;
		}
		return u;
	}

	String ComputeTechniqueName(Bool bSkinned) const
	{
		String sReturn("seoul_Render");
		for (auto const& value : m_aValues)
		{
			if (!value.IsSet())
			{
				sReturn += "_FM_0";
			}
			else if (value.IsTexture())
			{
				sReturn += "_FM_T";
			}
			else
			{
				sReturn += "_FM_C";
			}
		}
		if (bSkinned)
		{
			sReturn += "_Skinned";
		}

		return sReturn;
	}

	const ValueEntry& Get(ValueType eType) const
	{
		return m_aValues[(UInt32)eType];
	}

	ValueEntry& Get(ValueType eType)
	{
		return m_aValues[(UInt32)eType];
	}

	Bool Write(Bool bSkinned, SyncFile& file) const
	{
		// Material delimiter.
		if (!WriteInt32(file, DataTypeMaterial))
		{
			SEOUL_LOG_COOKING("%s: asset cook failed writing material library material delimiter.", file.GetAbsoluteFilename().CStr());
			return false;
		}

		// Technique.
		if (!WriteString(file, ComputeTechniqueName(bSkinned)))
		{
			SEOUL_LOG_COOKING("%s: asset cook failed writing material technique.", file.GetAbsoluteFilename().CStr());
			return false;
		}

		// Parameter count.
		if (!WriteUInt32(file, ComputeSetValueCount()))
		{
			SEOUL_LOG_COOKING("%s: asset cook failed writing material parameter count.", file.GetAbsoluteFilename().CStr());
			return false;
		}

		// Parameters.
		for (UInt32 i = 0u, iEnd = m_aValues.GetSize(); i < iEnd; ++i)
		{
			// Cache.
			auto const& value = m_aValues[i];
			if (!value.IsSet())
			{
				continue;
			}

			// Signature.
			if (!WriteInt32(file, (Int32)DataTypeMaterialParameter))
			{
				SEOUL_LOG_COOKING("%s: asset cook failed writing material parameter delimiter.", file.GetAbsoluteFilename().CStr());
				return false;
			}

			// Name.
			if (!WriteString(file, value.ComputeParameterName((ValueType)i)))
			{
				SEOUL_LOG_COOKING("%s: asset cook failed writing material parameter semantic name.", file.GetAbsoluteFilename().CStr());
				return false;
			}

			// Type.
			if (!WriteUInt32(file, (UInt32)value.ComputeMaterialParameterType()))
			{
				SEOUL_LOG_COOKING("%s: asset cook failed writing material parameter type texture.", file.GetAbsoluteFilename().CStr());
				return false;
			}

			// Value.
			if (!value.WriteValue(file))
			{
				SEOUL_LOG_COOKING("%s: asset cook failed writing material parameter texture file path.", file.GetAbsoluteFilename().CStr());
				return false;
			}
		}

		return true;
	}
}; // struct MaterialEntry

static inline UInt32 GetHash(const MaterialEntry& e)
{
	return e.m_uHashValue;
}

} // namespace Cooking

template <>
struct DefaultHashTableKeyTraits<Cooking::MaterialEntry>
{
	inline static Float GetLoadFactor()
	{
		return 0.75f;
	}

	inline static Cooking::MaterialEntry GetNullKey()
	{
		return Cooking::MaterialEntry();
	}

	static const Bool kCheckHashBeforeEquals = true;
};

namespace Cooking
{

struct VertexEntry SEOUL_SEALED
{
	SkinnedVertex m_Vertex;
	UInt32 m_uHashValue = 0u;
	Int4 m_QuantizedPosition;

	Bool operator==(const VertexEntry& e) const
	{
		return (
			m_Vertex.m_vNormal == e.m_Vertex.m_vNormal &&
			m_QuantizedPosition == e.m_QuantizedPosition &&
			m_Vertex.m_vTextureCoords == e.m_Vertex.m_vTextureCoords &&
			m_Vertex.m_vBlendIndices == e.m_Vertex.m_vBlendIndices &&
			m_Vertex.m_vBlendWeights == e.m_Vertex.m_vBlendWeights);
	}

	Bool operator!=(const VertexEntry& e) const
	{
		return !(*this == e);
	}

	Int4 QuantizePosition() const
	{
		// A millimeter difference is a difference.
		// If runtime scale is changed, this must be
		// changed as well.
		static const Float32 kfQuantizeFactor = 1000.0f;

		return Int4(
			(Int32)(m_Vertex.m_vPosition.X * kfQuantizeFactor),
			(Int32)(m_Vertex.m_vPosition.Y * kfQuantizeFactor),
			(Int32)(m_Vertex.m_vPosition.Z * kfQuantizeFactor),
			0);
	}

	void RecomputeEquality()
	{
		// Quantize a few values to all approximate comparisons.
		m_QuantizedPosition = QuantizePosition();

		m_uHashValue = 0u;
		IncrementalHash(m_uHashValue, Seoul::GetHash(m_Vertex.m_vNormal.X));
		IncrementalHash(m_uHashValue, Seoul::GetHash(m_Vertex.m_vNormal.Y));
		IncrementalHash(m_uHashValue, Seoul::GetHash(m_Vertex.m_vNormal.Z));
		IncrementalHash(m_uHashValue, Seoul::GetHash(m_QuantizedPosition.m_iX));
		IncrementalHash(m_uHashValue, Seoul::GetHash(m_QuantizedPosition.m_iY));
		IncrementalHash(m_uHashValue, Seoul::GetHash(m_QuantizedPosition.m_iZ));
		IncrementalHash(m_uHashValue, Seoul::GetHash(m_Vertex.m_vTextureCoords.X));
		IncrementalHash(m_uHashValue, Seoul::GetHash(m_Vertex.m_vTextureCoords.Y));
		IncrementalHash(m_uHashValue, Seoul::GetHash(m_Vertex.m_vBlendIndices.m_uX));
		IncrementalHash(m_uHashValue, Seoul::GetHash(m_Vertex.m_vBlendIndices.m_uY));
		IncrementalHash(m_uHashValue, Seoul::GetHash(m_Vertex.m_vBlendIndices.m_uZ));
		IncrementalHash(m_uHashValue, Seoul::GetHash(m_Vertex.m_vBlendIndices.m_uW));
		IncrementalHash(m_uHashValue, Seoul::GetHash(m_Vertex.m_vBlendWeights.X));
		IncrementalHash(m_uHashValue, Seoul::GetHash(m_Vertex.m_vBlendWeights.Y));
		IncrementalHash(m_uHashValue, Seoul::GetHash(m_Vertex.m_vBlendWeights.Z));
		IncrementalHash(m_uHashValue, Seoul::GetHash(m_Vertex.m_vBlendWeights.W));
	}
}; // struct VertexEntry

static inline UInt32 GetHash(const VertexEntry& e)
{
	return e.m_uHashValue;
}

} // namespace Cooking

template <>
struct DefaultHashTableKeyTraits<Cooking::VertexEntry>
{
	inline static Float GetLoadFactor()
	{
		return 0.75f;
	}

	inline static Cooking::VertexEntry GetNullKey()
	{
		return Cooking::VertexEntry();
	}

	static const Bool kCheckHashBeforeEquals = true;
};

namespace Cooking
{

struct SkinningData SEOUL_SEALED
{
	static const UInt32 kuMaxInfluences = 4u;

	SkinningData(UInt32 uSize = 0u)
		: m_vData(uSize)
	{
	}

	void AddWeight(UInt32 uVertexIndex, UInt8 uIndex, Float32 fWeight)
	{
		m_vData[uVertexIndex].AddWeight(uIndex, fWeight);
	}

	void Cleanup()
	{
		for (Data::Iterator i = m_vData.Begin(); m_vData.End() != i; ++i)
		{
			i->Cleanup();
		}
	}

	UInt32 GetInfluenceCount(UInt32 uVertexIndex) const
	{
		return m_vData[uVertexIndex].GetCount();
	}

	UByte4 GetIndices(UInt32 uVertexIndex) const
	{
		return m_vData[uVertexIndex].GetIndices();
	}

	Vector4D GetWeights(UInt32 uVertexIndex) const
	{
		return m_vData[uVertexIndex].GetWeights();
	}

	Bool IsValid() const
	{
		return (!m_vData.IsEmpty());
	}

	void Resize(UInt32 uSize)
	{
		m_vData.Resize(uSize);
	}

private:
	struct SkinWeight SEOUL_SEALED
	{
		SkinWeight()
			: m_auIndices()
			, m_afWeights()
			, m_uCount(0u)
		{
		}

		/**
		 * Adds an influence to the influences of this vertex.
		 * If the maximum number of influences have already been reached,
		 * then either the influence is ignored or it replaces an influence
		 * with the lowest weight < the weight of this influence.
		 */
		void AddWeight(UInt8 uIndex, Float32 fWeight)
		{
			if (kuMaxInfluences == m_uCount)
			{
				Int32 iInsertIndex = -1;
				Float32 fLowestWeight = FloatMax;

				for (UInt32 i = 0u; i < m_uCount; ++i)
				{
					if (m_afWeights[i] < fWeight && m_afWeights[i] < fLowestWeight)
					{
						iInsertIndex = (Int32)i;
						fLowestWeight = m_afWeights[i];
					}
				}

				if (iInsertIndex >= 0)
				{
					m_auIndices[iInsertIndex] = uIndex;
					m_afWeights[iInsertIndex] = fWeight;
				}
			}
			else
			{
				m_auIndices[m_uCount] = uIndex;
				m_afWeights[m_uCount++] = fWeight;
			}
		}

		/**
		 * Resorts influences from greatest to lowest weight and normalizes
		 * them to sum to 1.0.
		 */
		void Cleanup()
		{
			// Bubble sort, but only a max of 4, so it's fine.
			Bool bDone = false;
			do
			{
				bDone = true;
				for (UInt32 i = 1u; i < m_uCount; ++i)
				{
					if (m_afWeights[i] > m_afWeights[i-1])
					{
						bDone = false;
						Swap(m_auIndices[i], m_auIndices[i-1]);
						Swap(m_afWeights[i], m_afWeights[i-1]);
					}
				}
			} while (!bDone);

			for (UInt32 i = m_uCount; i < kuMaxInfluences; ++i)
			{
				m_auIndices[i] = 0u;
				m_afWeights[i] = 0.0f;
			}

			Float32 fSum = 0.0f;
			for (UInt32 i = 0u; i < kuMaxInfluences; ++i)
			{
				fSum += m_afWeights[i];
			}

			// Normalize weights.
			if (fSum >= 1e-5f)
			{
				for (UInt32 i = 0u; i < kuMaxInfluences; ++i)
				{
					m_afWeights[i] /= fSum;
				}
			}
			// Otherwise, zero out weights.
			else
			{
				m_afWeights.Fill(0.0f);
			}
		}

		UInt32 GetCount() const
		{
			return m_uCount;
		}

		UByte4 GetIndices() const
		{
			UByte4 const ret(
				(m_uCount > 0u) ? m_auIndices[0] : 0u,
				(m_uCount > 1u) ? m_auIndices[1] : 0u,
				(m_uCount > 2u) ? m_auIndices[2] : 0u,
				(m_uCount > 3u) ? m_auIndices[3] : 0u);

			return ret;
		}

		Vector4D GetWeights() const
		{
			Vector4D const vReturn(
				(m_uCount > 0u) ? m_afWeights[0] : 0u,
				(m_uCount > 1u) ? m_afWeights[1] : 0u,
				(m_uCount > 2u) ? m_afWeights[2] : 0u,
				(m_uCount > 3u) ? m_afWeights[3] : 0u);

			return vReturn;
		}

	private:
		FixedArray<UInt8, kuMaxInfluences> m_auIndices;
		FixedArray<Float32, kuMaxInfluences> m_afWeights;
		UInt32 m_uCount;
	}; // struct SkinWeight

	typedef Vector<SkinWeight, MemoryBudgets::Cooking> Data;
	Data m_vData;
}; // struct SkinningData

struct CookedPrimitiveGroup SEOUL_SEALED
{
	typedef HashTable<VertexEntry, UInt32, MemoryBudgets::Cooking> Lookup;

	CookedPrimitiveGroup()
		: m_vIndices()
		, m_vVertices()
		, m_tLookup()
		, m_bSkinned(false)
	{
	}

	Indices m_vIndices;
	Vertices m_vVertices;
	Lookup m_tLookup;
	Bool m_bSkinned;
}; // struct CookedPrimitiveGroup

class CookedMesh SEOUL_SEALED
{
public:
	typedef HashTable<Int32, MaterialEntry, MemoryBudgets::Cooking> Materials;
	typedef Vector<CookedPrimitiveGroup*, MemoryBudgets::Cooking> GroupVector;
	typedef HashTable<MaterialEntry, GroupVector, MemoryBudgets::Cooking> GroupTable;
	typedef Vector<Matrix3x4, MemoryBudgets::Cooking> InverseBindPoses;

	CookedMesh()
		: m_tMaterials()
		, m_tMaterialToGroups()
		, m_vInverseBindPoses()
	{
	}

	~CookedMesh()
	{
		for (auto pair : m_tMaterialToGroups)
		{
			SafeDeleteVector(pair.Second);
		}
		m_tMaterialToGroups.Clear();
	}

	Materials m_tMaterials;
	GroupTable m_tMaterialToGroups;
	InverseBindPoses m_vInverseBindPoses;

	Bool IsSkinned() const
	{
		for (auto pairs : m_tMaterialToGroups)
		{
			for (const auto& pGroup : pairs.Second)
			{
				if (pGroup->m_bSkinned)
				{
					return true;
				}
			}
		}

		return false;
	}

	AABB GetTotalAABB() const
	{
		AABB ret = AABB::InverseMaxAABB();
		for (auto const pair : m_tMaterialToGroups)
		{
			for (auto const& pGroup : pair.Second)
			{
				for (auto const& vertex : pGroup->m_vVertices)
				{
					ret.AbsorbPoint(vertex.m_vPosition);
				}
			}
		}

		return ret;
	}

	UInt32 GetTotalMaterials() const
	{
		return m_tMaterialToGroups.GetSize();
	}

	UInt32 GetTotalPrimitiveGroups() const
	{
		UInt32 uReturn = 0u;
		for (auto const pair : m_tMaterialToGroups)
		{
			uReturn += pair.Second.GetSize();
		}
		return uReturn;
	}

	UInt32 GetTotalVertices() const
	{
		UInt32 uReturn = 0u;
		for (auto const pair : m_tMaterialToGroups)
		{
			for (auto const& pGroup : pair.Second)
			{
				uReturn += pGroup->m_vVertices.GetSize();
			}
		}

		return uReturn;
	}

	Bool Finalize()
	{
		// TODO: This makes the (potentially incorrect)
		// assumption that matrix 0 is the unmodified root
		// bone and will always be the identity matrix.
		if (IsSkinned())
		{
			// Make sure any primitive groups of a skinned mesh
			// that don't contain skinning are normalized to have
			// reasonable weight values.
			for (auto const pair : m_tMaterialToGroups)
			{
				for (auto& pGroup : pair.Second)
				{
					if (!pGroup->m_bSkinned)
					{
						for (auto& rv : pGroup->m_vVertices)
						{
							if (rv.m_vBlendWeights.IsZero())
							{
								rv.m_vBlendWeights.X = 1.0f;
							}
						}
						pGroup->m_bSkinned = true;
					}
				}
			}
		}

		return Optimize();
	}

	Bool WriteInverseBindPoses(SyncFile& file) const
	{
		return WriteBuffer(file, m_vInverseBindPoses);
	}

	Bool WriteMaterials(SyncFile& file) const
	{
		Bool const bSkinned = IsSkinned();

		if (!WriteInt32(file, DataTypeMaterialLibrary))
		{
			SEOUL_LOG_COOKING("%s: asset cook failed writing material library delimiter.", file.GetAbsoluteFilename().CStr());
			return false;
		}
		if (!WriteUInt32(file, GetTotalMaterials()))
		{
			SEOUL_LOG_COOKING("%s: asset cook failed writing material library material count.", file.GetAbsoluteFilename().CStr());
			return false;
		}
		for (auto const pair : m_tMaterialToGroups)
		{
			const MaterialEntry& m = pair.First;
			if (!m.Write(bSkinned, file))
			{
				return false;
			}
		}

		return true;
	}

	Bool WritePrimitiveGroups(SyncFile& file) const
	{
		// Write primitive groups.
		if (!WriteUInt32(file, GetTotalPrimitiveGroups()))
		{
			SEOUL_LOG_COOKING("%s: asset cook failed writing primitive group count.", file.GetAbsoluteFilename().CStr());
			return false;
		}

		UInt32 uVertexOffset = 0u;
		Int32 iMaterialIndex = 0;
		for (auto const pair : m_tMaterialToGroups)
		{
			for (auto const& pGroup : pair.Second)
			{
				// Signature.
				if (!WriteInt32(file, (Int32)DataTypePrimitiveGroup))
				{
					SEOUL_LOG_COOKING("%s: asset cook failed writing primitive group delimiter.", file.GetAbsoluteFilename().CStr());
					return false;
				}

				// Material index.
				if (!WriteInt32(file, iMaterialIndex))
				{
					SEOUL_LOG_COOKING("%s: asset cook failed writing primitive group material index.", file.GetAbsoluteFilename().CStr());
					return false;
				}

				// Primitive type, always triangle list.
				if (!WriteUInt32(file, (UInt32)PrimitiveType::kTriangleList))
				{
					SEOUL_LOG_COOKING("%s: asset cook failed writing primitive group primitive type.", file.GetAbsoluteFilename().CStr());
					return false;
				}

				// Total number of indices.
				if (!WriteUInt32(file, pGroup->m_vIndices.GetSize()))
				{
					SEOUL_LOG_COOKING("%s: asset cook failed writing primitive group index count.", file.GetAbsoluteFilename().CStr());
					return false;
				}

				// Start vertex offset.
				if (!WriteUInt32(file, uVertexOffset))
				{
					SEOUL_LOG_COOKING("%s: asset cook failed writing vertex offset.", file.GetAbsoluteFilename().CStr());
					return false;
				}

				// Vertex count.
				if (!WriteUInt32(file, pGroup->m_vVertices.GetSize()))
				{
					SEOUL_LOG_COOKING("%s: asset cook failed writing vertex count.", file.GetAbsoluteFilename().CStr());
					return false;
				}

				// Write the indices.
				if (!pGroup->m_vIndices.IsEmpty())
				{
					for (auto const uIndex : pGroup->m_vIndices)
					{
						if (!WriteUInt16(file, (UInt16)uIndex))
						{
							SEOUL_LOG_COOKING("%s: asset cook failed writing primitive group index entry.", file.GetAbsoluteFilename().CStr());
							return false;
						}
					}
				}

				// Advance offsets.
				uVertexOffset += pGroup->m_vVertices.GetSize();
			}

			++iMaterialIndex;
		}

		return true;
	}

	Bool WriteVertices(SyncFile& file) const
	{
		// Remove skinning terms if no skinning.
		if (!IsSkinned())
		{
			struct StandardVertex
			{
				Vector3D m_vPosition;
				Vector3D m_vNormal;
				Vector2D m_vTextureCoords;
			};
			SEOUL_STATIC_ASSERT(sizeof(StandardVertex) ==  32);

			// Write vertices.
			UInt32 const uTotalVertices = GetTotalVertices();
			if (!WriteUInt32(file, uTotalVertices * sizeof(StandardVertex)))
			{
				SEOUL_LOG_COOKING("%s: asset cook failed writing vertex buffer data size in bytes.", file.GetAbsoluteFilename().CStr());
				return false;
			}

			for (auto const pair : m_tMaterialToGroups)
			{
				for (auto const& pGroup : pair.Second)
				{
					const Vertices& vVertices = pGroup->m_vVertices;
					for (auto const& vertex : vVertices)
					{
						StandardVertex v;
						v.m_vNormal = vertex.m_vNormal;
						v.m_vPosition = vertex.m_vPosition;
						v.m_vTextureCoords = vertex.m_vTextureCoords;
						if (sizeof(v) != file.WriteRawData(&v, sizeof(v)))
						{
							SEOUL_LOG_COOKING("%s: asset cook failed writing vertex buffer data entry.", file.GetAbsoluteFilename().CStr());
							return false;
						}
					}
				}
			}
		}
		// Otherwise, write verbatim.
		else
		{
			// Write vertices.
			UInt32 const uTotalVertices = GetTotalVertices();
			if (!WriteUInt32(file, uTotalVertices * sizeof(SkinnedVertex)))
			{
				SEOUL_LOG_COOKING("%s: asset cook failed writing vertex buffer data size, flat operation.", file.GetAbsoluteFilename().CStr());
				return false;
			}

			for (auto const pair : m_tMaterialToGroups)
			{
				for (auto const& pGroup : pair.Second)
				{
					const Vertices& vVertices = pGroup->m_vVertices;
					if (!vVertices.IsEmpty())
					{
						if (vVertices.GetSizeInBytes() != file.WriteRawData(vVertices.Data(), vVertices.GetSizeInBytes()))
						{
							SEOUL_LOG_COOKING("%s: asset cook failed writing vertex buffer data, flat operation.", file.GetAbsoluteFilename().CStr());
							return false;
						}
					}
				}
			}
		}

		return true;
	}

private:
	SEOUL_DISABLE_COPY(CookedMesh);

	Bool Optimize()
	{
		ScopedTootle tootle;
		for (auto const pair : m_tMaterialToGroups)
		{
			for (auto& pGroup : pair.Second)
			{
				if (!tootle.OptimizeInPlace(
					pGroup->m_vIndices,
					pGroup->m_vVertices))
				{
					SEOUL_LOG_COOKING("Asset cook Tootle optimization failed.");
					return false;
				}
			}
		}

		return true;
	}
};

static String ResolveFileRelative(
	const String& sFromFile,
	const String& sRelativeFile)
{
	String sReturn;
	(void)Path::CombineAndSimplify(
		String(),
		Path::Combine(Path::GetDirectoryName(sFromFile), sRelativeFile),
		sReturn);
	return sReturn;
}

static Bool WriteVertexFormat(SyncFile& file, Bool bSkinningVertex)
{
	// Signature.
	if (!WriteInt32(file, DataTypeVertexDecl)) { return false; }

	// Semantic count, 3 (position, normal, texcoords) + 2 (blend weights and blend indices)
	// if a skinning vertex.
	if (!WriteUInt32(file, 3 + (bSkinningVertex ? 2 : 0))) { return false; }

	// Write entry for position.
	if (!WriteInt32(file, DataTypeVertexElement)) { return false; }
	if (!WriteUInt16(file, 0)) { return false; } // Stream.
	if (!WriteUInt16(file, 0)) { return false; } // Offset
	if (!WriteUInt8(file, VertexElement::TypeFloat3)) { return false; } // Type, always a vec3.
	if (!WriteUInt8(file, VertexElement::MethodDefault)) { return false; } // Method, always the default.
	if (!WriteUInt8(file, VertexElement::UsagePosition)) { return false; } // Usage, Position component.
	if (!WriteUInt8(file, 0)) { return false; } // Usage index 0

	// Write entry for normal.
	if (!WriteInt32(file, DataTypeVertexElement)) { return false; }
	if (!WriteUInt16(file, 0)) { return false; } // Stream.
	if (!WriteUInt16(file, 12)) { return false; } // Ofset
	if (!WriteUInt8(file, VertexElement::TypeFloat3)) { return false; } // Type, Always a vec3.
	if (!WriteUInt8(file, VertexElement::MethodDefault)) { return false; } // Method, Always the default.
	if (!WriteUInt8(file, VertexElement::UsageNormal)) { return false; } // Usage, Normal component.
	if (!WriteUInt8(file, 0)) { return false; } // Usage index 0

	// Write entry for texture coordinates.
	if (!WriteInt32(file, DataTypeVertexElement)) { return false; }
	if (!WriteUInt16(file, 0)) { return false; } // Stream.
	if (!WriteUInt16(file, 24)) { return false; } // Ofset
	if (!WriteUInt8(file, VertexElement::TypeFloat2)) { return false; } // Type, Always a vec2.
	if (!WriteUInt8(file, VertexElement::MethodDefault)) { return false; } // Method, Always the default.
	if (!WriteUInt8(file, VertexElement::UsageTexcoord)) { return false; } // Usage, Texcoords component.
	if (!WriteUInt8(file, 0)) { return false; } // Usage index 0

	if (bSkinningVertex)
	{
		// Write entry for blend weights.
		if (!WriteInt32(file, DataTypeVertexElement)) { return false; }
		if (!WriteUInt16(file, 0)) { return false; } // Stream.
		if (!WriteUInt16(file, 32)) { return false; } // Ofset
		if (!WriteUInt8(file, VertexElement::TypeFloat4)) { return false; } // Type, Always a vec3.
		if (!WriteUInt8(file, VertexElement::MethodDefault)) { return false; } // Method, Always the default.
		if (!WriteUInt8(file, VertexElement::UsageBlendWeight)) { return false; } // Usage, Normal component.
		if (!WriteUInt8(file, 0)) { return false; } // Usage index 0

		// Write entry for blend indices.
		if (!WriteInt32(file, DataTypeVertexElement)) { return false; }
		if (!WriteUInt16(file, 0)) { return false; } // Stream.
		if (!WriteUInt16(file, 48)) { return false; } // Ofset
		if (!WriteUInt8(file, VertexElement::TypeUByte4)) { return false; } // Type, Always a vec2.
		if (!WriteUInt8(file, VertexElement::MethodDefault)) { return false; } // Method, Always the default.
		if (!WriteUInt8(file, VertexElement::UsageBlendIndices)) { return false; } // Usage, Texcoords component.
		if (!WriteUInt8(file, 0)) { return false; } // Usage index 0
	}

	return true;
}

static inline FbxVector4 ToFbxVector4(const Vector3D& v, Float fW)
{
	FbxVector4 ret;
	ret.Set((Double)v.X, (Double)v.Y, (Double)v.Z, (Double)fW);
	return ret;
}

template <typename T>
static inline Matrix3x4 ToMatrix3x4(const T& m)
{
	// FbxMatrix's are vectors as rows, so we transpose in-place
	// during assignment.
	Matrix3x4 ret;
	ret.M00 = (Float32)m[0][0]; ret.M01 = (Float32)m[1][0]; ret.M02 = (Float32)m[2][0]; ret.M03 = (Float32)m[3][0];
	ret.M10 = (Float32)m[0][1]; ret.M11 = (Float32)m[1][1]; ret.M12 = (Float32)m[2][1]; ret.M13 = (Float32)m[3][1];
	ret.M20 = (Float32)m[0][2]; ret.M21 = (Float32)m[1][2]; ret.M22 = (Float32)m[2][2]; ret.M23 = (Float32)m[3][2];
	return ret;
}

template <typename T>
static inline Matrix4D ToMatrix4D(const T& m)
{
	// FbxMatrix's are vectors as rows, so we transpose in-place
	// during assignment.
	Matrix4D ret;
	ret.M00 = (Float32)m[0][0]; ret.M01 = (Float32)m[1][0]; ret.M02 = (Float32)m[2][0]; ret.M03 = (Float32)m[3][0];
	ret.M10 = (Float32)m[0][1]; ret.M11 = (Float32)m[1][1]; ret.M12 = (Float32)m[2][1]; ret.M13 = (Float32)m[3][1];
	ret.M20 = (Float32)m[0][2]; ret.M21 = (Float32)m[1][2]; ret.M22 = (Float32)m[2][2]; ret.M23 = (Float32)m[3][2];
	ret.M30 = (Float32)m[0][3]; ret.M31 = (Float32)m[1][3]; ret.M32 = (Float32)m[2][3]; ret.M33 = (Float32)m[3][3];
	return ret;
}

template <typename T>
static inline Vector2D ToVector2D(const T& v)
{
	return Vector2D((Float32)v[0], (Float32)v[1]);
}

template <typename T>
static inline Vector3D ToVector3D(const T& v)
{
	return Vector3D((Float32)v[0], (Float32)v[1], (Float32)v[2]);
}

/**
 * Checks that access to array data of various types
 * uses a type we support. This can include access to arrays
 * of normals, vertex colors, etc.
 */
static inline Bool IsValidAccess(
	FbxLayerElement::EMappingMode eMappingMode,
	FbxLayerElement::EReferenceMode eReferenceMode)
{
	Bool bReturn = (
		eMappingMode == FbxLayerElement::eByControlPoint ||
		eMappingMode == FbxLayerElement::eByPolygonVertex ||
		eMappingMode == FbxLayerElement::eByPolygon) && (

		eReferenceMode == FbxLayerElement::eDirect ||
		eReferenceMode == FbxLayerElement::eIndexToDirect);

	return bReturn;
}

/**
 * General purpose helper, grabs a particular
 * element from a particular FBX array, taking into
 * account the access mode of the array.
 */
template <typename T, typename U>
static inline U GetElement(
	FbxMesh* pMesh,
	T const* pArray,
	Int32 iElement)
{
	FbxLayerElement::EMappingMode const eMappingMode = pArray->GetMappingMode();
	FbxLayerElement::EReferenceMode const eReferenceMode = pArray->GetReferenceMode();

	SEOUL_ASSERT(IsValidAccess(eMappingMode, eReferenceMode));

	Int32 iIndex = 0;
	switch (eMappingMode)
	{
	case FbxLayerElement::eByControlPoint:
		iIndex = pMesh->GetPolygonVertex(iElement / 3, iElement % 3);
		break;
	case FbxLayerElement::eByPolygonVertex:
		iIndex = iElement;
		break;
	case FbxLayerElement::eByPolygon:
		iIndex = (iElement / 3);
		break;
	default:
		// Should have been checked earlier, should not get here.
		SEOUL_FAIL("Invalid/unexpected access mode.");
		iIndex = iElement;
		break;
	};

	if (eReferenceMode == FbxLayerElement::eDirect)
	{
		SEOUL_ASSERT(pArray->GetDirectArray().GetCount() > iIndex);
		return pArray->GetDirectArray().GetAt(iIndex);
	}
	else
	{
		SEOUL_ASSERT(pArray->GetIndexArray().GetCount() > iIndex);

		Int32 const iIndirectIndex = pArray->GetIndexArray().GetAt(iIndex);

		SEOUL_ASSERT(pArray->GetDirectArray().GetCount() > iIndirectIndex);
		return pArray->GetDirectArray().GetAt(iIndirectIndex);
	}
}

static UByte4 GetBlendIndices(FbxMesh* pMesh, Int i, const SkinningData& skinningData)
{
	const int iV = pMesh->GetPolygonVertex(i / 3, i % 3);
	return skinningData.GetIndices(iV);
}

static Vector4D GetBlendWeights(FbxMesh* pMesh, Int i, const SkinningData& skinningData)
{
	const int iV = pMesh->GetPolygonVertex(i / 3, i % 3);
	return skinningData.GetWeights(iV);
}

static Vector3D GetNormal(FbxMesh* pMesh, Int32 i, const Matrix4D& mTransform)
{
	Vector3D const vNormal = ToVector3D(GetElement<FbxLayerElementNormal, FbxVector4>(
		pMesh,
		pMesh->GetLayer(0)->GetNormals(),
		i));

	return Vector3D::Normalize(Matrix4D::TransformDirection(mTransform, vNormal));
}

static Vector3D GetPosition(FbxMesh* pMesh, Int32 i, const Matrix4D& mTransform)
{
	FbxVector4 const* pControlPoints = pMesh->GetControlPoints();
	Int32 const iV = pMesh->GetPolygonVertex(i / 3, i % 3);

	return Matrix4D::TransformPosition(mTransform, ToVector3D(pControlPoints[iV]));
}

static inline Float32 FlipV(Float32 f)
{
	return (1.0f - f);
}

static Vector2D GetTexcoords(
	FbxMesh* pMesh,
	Int32 i,
	FbxLayerElement::EType eType)
{
	Vector2D vTexcoords = ToVector2D(GetElement<FbxLayerElementUV, FbxVector2>(
		pMesh,
		pMesh->GetLayer(0)->GetUVs(eType),
		i));
	vTexcoords.Y = FlipV(vTexcoords.Y);
	return vTexcoords;
}

static Bool SetupMaterialGroups(
	FbxNode* pNode,
	FbxMesh* pMesh,
	Vector<Int32>& rvMaterialGroups)
{
	Int32 const iCount = pMesh->GetPolygonCount();
	FbxLayerElementMaterial const* pMaterialGroups = pMesh->GetLayer(0)->GetMaterials();

	// In this case, return the default mat
	// for all entries.
	if (nullptr == pMaterialGroups)
	{
		rvMaterialGroups.Resize((UInt32)iCount, (Int32)-1);
		return true;
	}

	FbxLayerElement::EMappingMode const eMappingMode = pMaterialGroups->GetMappingMode();
	FbxLayerElement::EReferenceMode const eReferenceMode = pMaterialGroups->GetReferenceMode();
	if ((eMappingMode != FbxLayerElement::eAllSame &&
		eMappingMode != FbxLayerElement::eByPolygon) ||
		eReferenceMode != FbxLayerElement::eIndexToDirect)
	{
		SEOUL_LOG_COOKING("%s: material groups have invalid mapping or reference mode.", pNode->GetName());
		return false;
	}

	// Get ids for the materials used by this mesh.
	// Also initialize the id -> index mapping to a default
	// state, which is to point to material index -1 (invalid material).
	if (eMappingMode == FbxLayerElement::eAllSame)
	{
		Int32 const iId = pMaterialGroups->GetIndexArray().GetAt(0);
		for (Int32 i = 0; i < iCount; ++i)
		{
			rvMaterialGroups.PushBack(iId);
		}
	}
	else
	{
		for (Int32 i = 0; i < iCount; ++i)
		{
			Int32 const iId = pMaterialGroups->GetIndexArray().GetAt(i);
			rvMaterialGroups.PushBack(iId);
		}
	}

	return true;
}

static inline Bool SafeToVector3D(const FbxProperty& prop, Vector3D& r)
{
	FbxDataType const eDataType = prop.GetPropertyDataType();
	if (eDataType == FbxFloatDT || eDataType == FbxDoubleDT)
	{
		r = Vector3D((Float32)prop.Get<FbxDouble>());
		return true;
	}
	else if (eDataType == FbxDouble2DT)
	{
		FbxDouble2 const v = prop.Get<FbxDouble2>();
		r = Vector3D((Float32)v[0], (Float32)v[1], 0.0f);
		return true;
	}
	else if (eDataType == FbxColor3DT || eDataType == FbxDouble3DT)
	{
		FbxDouble3 const v = prop.Get<FbxDouble3>();
		r = Vector3D((Float32)v[0], (Float32)v[1], (Float32)v[2]);
		return true;
	}
	else if (eDataType == FbxColor4DT || eDataType == FbxDouble4DT)
	{
		FbxDouble4 const v = prop.Get<FbxDouble4>();
		r = Vector3D((Float32)v[0], (Float32)v[1], (Float32)v[2]);
		return true;
	}

	return false;
}

/**
 * Helper function, populates a skinning buffer helper
 * structure that is used later to build the flat vertex buffer.
 */
static Bool PopulateSkinningBuffer(
	const Bones& vBones,
	FbxNode* pNode,
	FbxMesh* pMesh,
	CookedMesh& rCookedMesh,
	SkinningData& rSkinningData)
{
	Int32 const iSkinCount = pMesh->GetDeformerCount(FbxDeformer::eSkin);
	Int32 const iBufferSize = pMesh->GetControlPointsCount();

	// Invalid.
	if (iSkinCount <= 0 || iBufferSize <= 0)
	{
		SEOUL_LOG_COOKING("%s: invalid skin count or buffer size.", pNode->GetName());
		return false;
	}

	rSkinningData.Resize((UInt32)iBufferSize);

	// Build a joint map.
	HashTable<HString, UInt32, MemoryBudgets::Cooking> tJointMap;
	UInt32 const uBones = vBones.GetSize();
	for (UInt32 i = 0u; i < uBones; ++i)
	{
		if (!tJointMap.Insert(vBones[i].m_Id, i).Second)
		{
			SEOUL_LOG_COOKING("%s: duplicate joint \"%s\".", pNode->GetName(), vBones[i].m_Id.CStr());
			return false;
		}
	}

	// Prepare the inverse bind poses matrices count.
	rCookedMesh.m_vInverseBindPoses.Resize(uBones, Matrix3x4::Identity());

	String sTooManyInfluences;
	Bool bHadTooManyInfluences = false;
	for (Int32 i = 0; i < iSkinCount; ++i)
	{
		FbxSkin* pSkin = FbxCast<FbxSkin>(pMesh->GetDeformer(i, FbxDeformer::eSkin));
		if (nullptr == pSkin)
		{
			SEOUL_LOG_COOKING("\"%s\" is a skinned mesh node with invalid skin data.\n", pNode->GetName());
			return false;
		}

		Int32 const iClusterCount = pSkin->GetClusterCount();
		for (Int32 j = 0; j < iClusterCount; ++j)
		{
			FbxCluster* pCluster = FbxCast<FbxCluster>(pSkin->GetCluster(j));
			if (nullptr == pCluster)
			{
				SEOUL_LOG_COOKING("\"%s\" is a skinned mesh node with invalid skin cluster data.\n", pNode->GetName());
				return false;
			}

			// These used to return warnings, but this is a normal case, nodes can have no influences.
			// And because morpheme is handling our animation data, we don't care about maintaining
			// the entire skeleton, since we just have to plugin final transforms.
			Int const* piIndices = pCluster->GetControlPointIndices();
			if (nullptr == piIndices)
			{
				continue;
			}

			Double const* pfWeights = pCluster->GetControlPointWeights();
			if (nullptr == pfWeights)
			{
				continue;
			}

			UInt32 uBoneIndex = 0u;
			HString const sBoneName(pCluster->GetLink()->GetName());
			if (!tJointMap.GetValue(sBoneName, uBoneIndex))
			{
				SEOUL_LOG_COOKING("%s: invalid joint \"%s\".", pNode->GetName(), sBoneName.CStr());
				return false;
			}

			// Update inverse bind poses.
			{
				FbxAMatrix mTransformLink;
				pCluster->GetTransformLinkMatrix(mTransformLink);

				FbxAMatrix const mFbxInverseBindPose = (
					mTransformLink.Inverse());

				// Convert the input matrix into our matrices.
				auto const mInverseBindPose(ToMatrix3x4(mFbxInverseBindPose));
				if (rCookedMesh.m_vInverseBindPoses.GetSize() <= uBoneIndex)
				{
					rCookedMesh.m_vInverseBindPoses.Resize(uBoneIndex + 1u, Matrix3x4::Identity());
				}

				rCookedMesh.m_vInverseBindPoses[uBoneIndex] = mInverseBindPose;
			}

			Int32 const iIndexCount = pCluster->GetControlPointIndicesCount();
			for (Int32 k = 0; k < iIndexCount; ++k)
			{
				if (rSkinningData.GetInfluenceCount(piIndices[k]) >= SkinningData::kuMaxInfluences)
				{
					bHadTooManyInfluences = true;
					sTooManyInfluences += String::Printf("%d ", piIndices[k]);
				}

				// Despite the warning, we still call add weight, to allow AddWeight
				// to decide whether to replace an existing weight with this weight or not.
				rSkinningData.AddWeight(piIndices[k], (UInt8)uBoneIndex, (Float32)pfWeights[k]);
			}
		}
	}

	if (bHadTooManyInfluences)
	{
		SEOUL_LOG_COOKING("%s: has too many influences: %s\n", pNode->GetName(), sTooManyInfluences.CStr());
		return false;
	}

	// Normalize and fixup weights and indices.
	rSkinningData.Cleanup();
	return true;
}

/** Utility to resolve (sometimes crazy) texture file paths into a FilePath. */
static FilePath ResolveTextureFilePath(
	const String& sInputFileName,
	const String& sTextureFileName)
{
	auto sFileName(Path::GetExactPathName(Path::Combine(
		Path::GetDirectoryName(sInputFileName),
		sTextureFileName)));

	// Check if the resolve path exists - if not, fall back
	// to use the base name and resolving to the path next to the
	// input file.
	if (!FileManager::Get()->Exists(sFileName))
	{
		sFileName = Path::GetExactPathName(Path::Combine(
			Path::GetDirectoryName(sInputFileName),
			Path::GetFileName(sTextureFileName)));
	}

	return FilePath::CreateContentFilePath(sFileName);
}

static Matrix4D ComputeGeometricWorldTransform(FbxNode* pNode)
{
	FbxAMatrix const GeometricTransform(
		pNode->GetGeometricTranslation(FbxNode::eSourcePivot),
		pNode->GetGeometricRotation(FbxNode::eSourcePivot),
		pNode->GetGeometricScaling(FbxNode::eSourcePivot));
	FbxAMatrix const& GlobalTransform = pNode->EvaluateGlobalTransform();

	return ToMatrix4D(GlobalTransform * GeometricTransform);
}

static Bool SetupIndexAndVertexBuffers(
	const String& sInputFileName,
	const Bones& vBones,
	FbxNode* pNode,
	FbxMesh* pMesh,
	CookedMesh& rCookedMesh)
{
	// Sanity.
	if (nullptr == pMesh->GetLayer(0))
	{
		SEOUL_LOG_COOKING("%s: mesh \"%s\" has no default layer.", pNode->GetName(), pMesh->GetName());
		return false;
	}

	// Material groups.
	Vector<Int32> vMaterialGroups;
	if (!SetupMaterialGroups(pNode, pMesh, vMaterialGroups))
	{
		SEOUL_LOG_COOKING("%s: mesh \"%s\" failed processing material groups.", pNode->GetName(), pMesh->GetName());
		return false;
	}

	Bool const bHasTexcoords = (nullptr != pMesh->GetLayer(0)->GetUVs(FbxLayerElement::eTextureDiffuse));

	// Check if we have skinning.
	Bool bHasSkinning = false;
	SkinningData skinningData;
	if (pMesh->GetDeformerCount(FbxDeformer::eSkin) > 0)
	{
		bHasSkinning = true;

		// Populate a helper buffer now, to be used later.
		if (!PopulateSkinningBuffer(vBones, pNode, pMesh, rCookedMesh, skinningData))
		{
			SEOUL_LOG_COOKING("%s: is a skinned mesh, but creation of skinning helpers failed.", pNode->GetName());
			return false;
		}
	}

	Matrix4D const mTransform = ComputeGeometricWorldTransform(pNode);
	Matrix4D const mNormalTransform = Matrix4D::CreateNormalTransform(mTransform);

	Int32 const iIndexCount = (pMesh->GetPolygonCount() * 3);
	for (Int32 i = 0; i < iIndexCount; ++i)
	{
		Int32 const iPolygon = (i / 3);
		Int32 const iMaterialId = vMaterialGroups[iPolygon];

		VertexEntry v;
		v.m_Vertex.m_vPosition = ToVector3D(ToFbxVector4(GetPosition(pMesh, i, mTransform), 1.0f));
		v.m_Vertex.m_vNormal = ToVector3D(ToFbxVector4(GetNormal(pMesh, i, mNormalTransform), 0.0f));
		if (bHasTexcoords)
		{
			v.m_Vertex.m_vTextureCoords = GetTexcoords(pMesh, i, FbxLayerElement::eTextureDiffuse);
		}
		if (bHasSkinning)
		{
			v.m_Vertex.m_vBlendIndices = GetBlendIndices(pMesh, i, skinningData);
			v.m_Vertex.m_vBlendWeights = GetBlendWeights(pMesh, i, skinningData);
		}
		v.RecomputeEquality();

		MaterialEntry m;
		if (iMaterialId >= 0)
		{
			if (!rCookedMesh.m_tMaterials.GetValue(iMaterialId, m))
			{
				FbxSurfaceMaterial* pMaterial = pNode->GetMaterial(iMaterialId);
				if (nullptr == pMaterial)
				{
					SEOUL_LOG_COOKING("%s: mesh \"%s\" failed getting material %d.", pNode->GetName(), pMesh->GetName(), iMaterialId);
					return false;
				}

				for (UInt32 i = 0u; i < (UInt32)ValueType::COUNT; ++i)
				{
					auto const eType = (ValueType)i;
					auto const prop = pMaterial->FindPropertyHierarchical(ValueTypeToFbxMaterialPropertyName(eType));
					if (!prop.IsValid())
					{
						continue;
					}

					auto pTexture = FbxCast<FbxFileTexture>(prop.GetSrcObject<FbxTexture>());
					if (pTexture)
					{
						m.Get(eType).Set(ResolveTextureFilePath(
							sInputFileName,
							String(pTexture->GetFileName())));
					}
					else
					{
						Vector3D v;
						if (SafeToVector3D(prop, v) && !IsDiscardValue(eType, v))
						{
							m.Get(eType).Set(v);
						}
					}
				}

				m.RecomputeHash();
				SEOUL_VERIFY(rCookedMesh.m_tMaterials.Insert(iMaterialId, m).Second);
			}
		}

		auto pGroupVector = rCookedMesh.m_tMaterialToGroups.Find(m);
		if (!pGroupVector)
		{
			auto const e = rCookedMesh.m_tMaterialToGroups.Insert(m, CookedMesh::GroupVector());
			SEOUL_ASSERT(e.Second);
			pGroupVector = &e.First->Second;
			pGroupVector->PushBack(SEOUL_NEW(MemoryBudgets::Cooking) CookedPrimitiveGroup);
		}
		auto pPrimitiveGroup = pGroupVector->Back();

		UInt32 uExistingIndex = 0;
		if (pPrimitiveGroup->m_tLookup.GetValue(v, uExistingIndex))
		{
			pPrimitiveGroup->m_vIndices.PushBack(uExistingIndex);
		}
		else
		{
			// Out of slots for 16-bit indices, create a new primitive group.
			if (pPrimitiveGroup->m_vVertices.GetSize() >= (UInt32)UInt16Max)
			{
				// If this is not the first index of a new triangle, we need
				// to evict the already added indices and then rewind and reprocess.
				auto const iRewind = (i % 3);
				auto const bNew = (iRewind == 0);

				// TODO: This is not ideal since we're fully breaking the group - as such, any vertices
				// that could've been shared with the prior group will not be.
				pGroupVector->PushBack(SEOUL_NEW(MemoryBudgets::Cooking) CookedPrimitiveGroup);
				// If not new, evict the already emitted before updating the group.
				if (!bNew)
				{
					SEOUL_ASSERT(pPrimitiveGroup->m_vIndices.GetSize() >= (UInt32)iRewind);
					// TODO: Always want to evict vertices if they were freshly added and are now
					// no longer used.
					pPrimitiveGroup->m_vIndices.Resize(pPrimitiveGroup->m_vIndices.GetSize() - (UInt32)iRewind);
				}
				pPrimitiveGroup = pGroupVector->Back();
				// If not new, rewind and continue instead of falling through and processing this index.
				if (!bNew)
				{
					i -= (iRewind + 1);
					continue;
				}
			}

			UInt32 const uIndex = pPrimitiveGroup->m_vVertices.GetSize();
			pPrimitiveGroup->m_vIndices.PushBack(uIndex);
			SEOUL_VERIFY(pPrimitiveGroup->m_tLookup.Insert(v, uIndex).Second);
			pPrimitiveGroup->m_vVertices.PushBack(v.m_Vertex);
			pPrimitiveGroup->m_bSkinned =
				(pPrimitiveGroup->m_bSkinned || v.m_Vertex.m_vBlendWeights != Vector4D::Zero());
		}
	}

	return true;
}

static Bool CookMesh(
	FbxMesh* pMesh,
	const String& sInputFileName,
	const Bones& vBones,
	FbxNode* pNode,
	CookedMesh& rCookedMesh)
{
	// Fbx cleaning routines.
	if (pMesh->RemoveBadPolygons() < 0)
	{
		SEOUL_LOG_COOKING("%s: node \"%s\" failed bad polygon removal.", sInputFileName.CStr(), pNode->GetName());
		return false;
	}

	// Triangulation
	if (!pMesh->IsTriangleMesh())
	{
		FbxGeometryConverter converter(pNode->GetFbxManager());
		FbxMesh* const pOrig = pMesh;

		// TODO: The new triangulation algorithm in the FBXSDK
		// apparently has a bug - it erroneously stripped a polygon from
		// several test .fbx used when developing this code.
		//
		// We use the legacy algorithm unless triangulation fails.
		pMesh = FbxCast<FbxMesh>(converter.Triangulate(pOrig, true, true));
		if (nullptr == pMesh)
		{
			// Attempt again with non-legacy method.
			pMesh = FbxCast<FbxMesh>(converter.Triangulate(pOrig, true, false));
			if (nullptr == pMesh)
			{
				SEOUL_LOG_COOKING("%s: node \"%s\" failed triangulation.", sInputFileName.CStr(), pNode->GetName());
				return false;
			}
		}
	}

	// Make sure the mesh has normals.
	if (!pMesh->GenerateNormals())
	{
		SEOUL_LOG_COOKING("%s: node \"%s\" failed normal generation.", sInputFileName.CStr(), pNode->GetName());
		return false;
	}

	// Generate indices and vertices.
	if (!SetupIndexAndVertexBuffers(
		sInputFileName,
		vBones,
		pNode,
		pMesh,
		rCookedMesh))
	{
		SEOUL_LOG_COOKING("%s: node \"%s\" failed index/vertex buffer generation.", sInputFileName.CStr(), pNode->GetName());
		return false;
	}

	return true;
}

static Bool IsVisible(FbxNode* pNode)
{
	// Whether this mesh is visible or not.
	if (!pNode->Show.Get())
	{
		return false;
	}

	// As of FBX exporter 2011.2, we need to also explicitly
	// check the display layer of the node, as the display layer
	// setting no longer propagates to its members.
	int iDisplayLayers = pNode->GetDstObjectCount<FbxDisplayLayer>();
	for (int i = 0; i < iDisplayLayers; ++i)
	{
		auto pDisplayLayer = pNode->GetDstObject<FbxDisplayLayer>(i);
		if (!pDisplayLayer->Show.Get())
		{
			return false;
		}
	}

	return true;
}

static Bool CookNode(
	const String& sInputFileName,
	const Bones& vBones,
	FbxNode* pNode,
	CookedMesh& rCookedMesh)
{
	// Skip hidden nodes.
	if (!IsVisible(pNode))
	{
		return true;
	}

	if (auto pMesh = pNode->GetMesh())
	{
		if (!CookMesh(pMesh, sInputFileName, vBones, pNode, rCookedMesh))
		{
			return false;
		}
	}

	Int32 const iChildCount = pNode->GetChildCount();
	for (Int32 i = 0; i < iChildCount; ++i)
	{
		if (!CookNode(sInputFileName, vBones, pNode->GetChild(i), rCookedMesh))
		{
			return false;
		}
	}

	return true;
}

enum class SkeletonResult
{
	kSuccess,
	kNoSkeleton,
	kError,
};

static SkeletonResult ExtractSkeletonInner(
	FbxNode* pNode,
	HString parentId,
	Bones& rvBones)
{
	// Node will be added to the skeleton
	// if it has a skeletal parent, or if it
	// itself is tagged as a skeleton node.
	auto const bAdd = (
		!parentId.IsEmpty() ||
		(nullptr != pNode->GetNodeAttribute() && pNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton));

	/// Add this particular node if specified.
	HString currentId;
	if (bAdd)
	{
		currentId = HString(pNode->GetName());

		BoneEntry entry;
		entry.m_Id = currentId;
		entry.m_ParentId = parentId;

		auto const mFbxTransform(parentId.IsEmpty() ? pNode->EvaluateGlobalTransform() : pNode->EvaluateLocalTransform());
		auto const mTransform(ToMatrix4D(mFbxTransform));

		Matrix3D mPreRotation;
		Matrix3D mRotation;
		Vector3D vTranslation;
		if (!Matrix4D::Decompose(mTransform, mPreRotation, mRotation, vTranslation))
		{
			SEOUL_LOG_COOKING("Node \"%s\" is defined by an invalid transform.", entry.m_Id.CStr());
			return SkeletonResult::kError;
		}

		entry.m_qRotation = Quaternion::Normalize(Quaternion::CreateFromRotationMatrix(mRotation));
		entry.m_vPosition = vTranslation;
		entry.m_vScale = mPreRotation.GetDiagonal();

		rvBones.PushBack(entry);
	}

	Bool bHasSkeleton = bAdd;

	// Process children.
	Int32 const iChildren = pNode->GetChildCount();
	for (Int32 i = 0; i < iChildren; ++i)
	{
		auto pChild = pNode->GetChild(i);
		auto const eChildResult = ExtractSkeletonInner(pChild, currentId, rvBones);

		if (eChildResult == SkeletonResult::kError)
		{
			return eChildResult;
		}
		else if (eChildResult == SkeletonResult::kSuccess)
		{
			bHasSkeleton = true;
		}
	}

	return (bHasSkeleton ? SkeletonResult::kSuccess : SkeletonResult::kNoSkeleton);
}

static SkeletonResult ExtractSkeleton(FbxScene* pScene, Bones& rvBones)
{
	return ExtractSkeletonInner(pScene->GetRootNode(), HString(), rvBones);
}

static Bool CookAnimationClipInner(
	HString id,
	const Bones& vBones,
	FbxScene* pScene,
	FbxAnimStack* pAnimStack,
	Seoul::Platform ePlatform,
	const String& sInputFileName,
	MemorySyncFile& file)
{
	// TODO: Configure.
	static const Float64 kfSamplingInterval = (1.0 / 60.0);

	// Use the anim stack name as the id if none was provided.
	id = (id.IsEmpty() ? HString(pAnimStack->GetName()) : id);

	// Setup for evaluator of the particular stack.
	pScene->SetCurrentAnimationStack(pAnimStack);

	// Get the overall time of the animation clip.
	FbxTimeSpan timeSpan;
	auto pTakeInfo = pScene->GetTakeInfo(pAnimStack->GetName());
	if (nullptr != pTakeInfo)
	{
		timeSpan = pTakeInfo->mLocalTimeSpan;
	}
	else
	{
		pScene->GetGlobalSettings().GetTimelineDefaultTimeSpan(timeSpan);
	}

	// Extract parameters.
	auto const fStart = timeSpan.GetStart().GetSecondDouble();
	auto const fEnd = timeSpan.GetStop().GetSecondDouble();
	auto const fDuration = (fEnd > fStart ? (fEnd - fStart) : 1.0);

	// Now build a table of bones to track data.
	HashTable<Int16, BoneKeyFrames, MemoryBudgets::Cooking> tTracks;

	// Enumerate bones and build track data for each.
	auto pEvaluator = pScene->GetAnimationEvaluator();
	Int16 const iBones = (Int16)vBones.GetSize();
	for (Int16 i = 0; i < iBones; ++i)
	{
		auto const& bone = vBones[i];
		auto const id = bone.m_Id;
		auto const parentId = bone.m_ParentId;
		auto pNode = pScene->FindNodeByName(id.CStr());

		auto const qInverse(bone.m_qRotation.Inverse());

		// Skip, no animation data for the bone.
		if (nullptr == pNode)
		{
			continue;
		}

		Bool bErase = false;
		{
			// Now sample translation, scale, and rotation.
			auto& rFrames = tTracks.Insert(i, BoneKeyFrames()).First->Second;

			// Loop from start to end, evaluating at the given sampling interval.
			Bool bDone = false;
			for (auto f = fStart; !bDone; f += kfSamplingInterval)
			{
				if (f >= fEnd)
				{
					f = fEnd;
					bDone = true;
				}

				// Sample and decompose the transform.
				auto const mFbxTransform(parentId.IsEmpty()
					? pEvaluator->GetNodeGlobalTransform(pNode, FbxTimeSeconds(f))
					: pEvaluator->GetNodeLocalTransform(pNode, FbxTimeSeconds(f)));
				auto const mTransform(ToMatrix4D(mFbxTransform));

				Matrix3D mPreRotation;
				Matrix3D mRotation;
				Vector3D vTranslation;
				if (!Matrix4D::Decompose(mTransform, mPreRotation, mRotation, vTranslation))
				{
					SEOUL_LOG_COOKING("Animation \"%s\" has invalid transform at time %f for bone \"%s\"\n", pAnimStack->GetName(), f, id.CStr());
					return false;
				}

				// Derive.
				auto const qR = Quaternion::Normalize(Quaternion::CreateFromRotationMatrix(mRotation) * qInverse);
				auto const vS = Vector3D::ComponentwiseDivide(mPreRotation.GetDiagonal(), bone.m_vScale);
				auto const vT = (vTranslation - bone.m_vPosition);

				// Add the raw samples. These will be simplified later.
				rFrames.m_vRotation.PushBack(KeyFrameRotation((Float32)f, qR));
				rFrames.m_vScale.PushBack(KeyFrame3D((Float32)f, vS));
				rFrames.m_vTranslation.PushBack(KeyFrame3D((Float32)f, vT));
			}

			// Eliminate redundant keyframes.
			SimplifyRotation(rFrames.m_vRotation);
			SimplifyScale(rFrames.m_vScale);
			SimplifyTranslation(rFrames.m_vTranslation);

			// Track whether we've cleared everything.
			if (rFrames.m_vRotation.IsEmpty() &&
				rFrames.m_vScale.IsEmpty() &&
				rFrames.m_vTranslation.IsEmpty())
			{
				bErase = true;
			}
		}

		// If all frames were removed, erase the keyframes entirely.
		if (bErase)
		{
			(void)tTracks.Erase(i);
		}
	}

	// Now write out results.
	MemorySyncFile innerFile;
	if (!WriteInt32(innerFile, (Int32)DataTypeAnimationClip)) { return false; }
	if (!WriteHString(innerFile, id)) { return false; }
	if (!WriteUInt32(innerFile, tTracks.GetSize())) { return false; }
	for (Int16 i = 0; i < iBones; ++i)
	{
		auto const pBones = tTracks.Find(i);
		if (nullptr == pBones)
		{
			continue;
		}

		if (!WriteInt16(innerFile, i)) { return false; }
		if (!WriteBuffer(innerFile, pBones->m_vRotation)) { return false; }
		if (!WriteBuffer(innerFile, pBones->m_vScale)) { return false; }
		if (!WriteBuffer(innerFile, pBones->m_vTranslation)) { return false; }
	}

	UInt32 const uSizeInBytes = (UInt32)innerFile.GetSize();
	if (!WriteInt32(file, (Int32)DataTypeAnimationClip)) { return false; }
	if (!WriteUInt32(file, uSizeInBytes)) { return false; }
	if (uSizeInBytes != file.WriteRawData(innerFile.GetBuffer().GetBuffer(), uSizeInBytes)) { return false; }

	return true;
}

static Bool LoadFbxFile(
	FbxManager* pManager,
	FbxScene* pScene,
	const String& sInputFileName)
{
	ScopedFbxPointer<FbxIOSettings> pIoSettings = FbxIOSettings::Create(pManager, IOSROOT);
	pIoSettings->SetBoolProp(IMP_FBX_ANIMATION, true);
	pIoSettings->SetBoolProp(IMP_FBX_CHARACTER, true);
	pIoSettings->SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);
	pIoSettings->SetBoolProp(IMP_FBX_GOBO, true);
	pIoSettings->SetBoolProp(IMP_FBX_LINK, true);
	pIoSettings->SetBoolProp(IMP_FBX_MATERIAL, true);
	pIoSettings->SetBoolProp(IMP_FBX_MODEL, true);
	pIoSettings->SetBoolProp(IMP_FBX_SHAPE, true);
	pIoSettings->SetBoolProp(IMP_FBX_TEXTURE, true);
	pIoSettings->SetBoolProp(IMP_SKINS, true);
	pIoSettings->SetBoolProp(IMP_TAKE, true);

	{
		ScopedFbxPointer<FbxImporter> pImporter(FbxImporter::Create(pManager, ksEmptyString));
		if (!pImporter->Initialize(sInputFileName.CStr(), -1, pManager->GetIOSettings()))
		{
			SEOUL_LOG_COOKING("Failed to initialize the FBX importer.\n");
			SEOUL_LOG_COOKING("%s\n", pImporter->GetStatus().GetErrorString());

			if (pImporter->GetStatus().GetCode() == FbxStatus::eInvalidFileVersion)
			{
				Int iSDKMajor = -1;
				Int iSDKMinor = -1;
				Int iSDKRevision = -1;
				FbxManager::GetFileFormatVersion(iSDKMajor, iSDKMinor, iSDKRevision);

				Int iFileMajor = -1;
				Int iFileMinor = -1;
				Int iFileRevision = -1;
				pImporter->GetFileVersion(iFileMajor, iFileMinor, iFileRevision);

				SEOUL_LOG_COOKING("FBX version for the import system is: %d.%d.%d\n", iSDKMajor, iSDKMinor, iSDKRevision);
				SEOUL_LOG_COOKING("FBX version for the input file is:    %d.%d.%d\n", iFileMajor, iFileMinor, iFileRevision);
			}

			return false;
		}

		if (!pImporter->IsFBX())
		{
			SEOUL_LOG_COOKING("Input file is not an FBX file.\n");
			return false;
		}

		if (!pImporter->Import(pScene))
		{
			SEOUL_LOG_COOKING("Failed importing FBX scene.\n");
			SEOUL_LOG_COOKING("%s\n", pImporter->GetStatus().GetErrorString());
			return false;
		}

		// Make sure the scene is a right-handed coordinate system
		// with +X right, +Y up, and -Z forward (right handed coordinate system).
		FbxAxisSystem::MayaYUp.ConvertScene(pScene);
		// Convert to 1 unit = 1 meter.
		FbxSystemUnit::m.ConvertScene(pScene);
		// Reset.
		pScene->GetAnimationEvaluator()->Reset();
	}

	return true;
}

static Bool CookAnimationClip(
	HString id,
	FbxManager* pManager,
	const Bones& vBones,
	Seoul::Platform ePlatform,
	const String& sInputFileName,
	MemorySyncFile& file)
{
	ScopedFbxPointer<FbxScene> pScene(FbxScene::Create(pManager, ksEmptyString));
	if (!LoadFbxFile(pManager, pScene.GetPtr(), sInputFileName))
	{
		return false;
	}

	auto const iStackCount = pScene->GetSrcObjectCount<FbxAnimStack>();
	if (0 == iStackCount)
	{
		// Not an error if no animations.
		return true;
	}

	// If an id was provided but there are multiple clips,
	// flag this as an error.
	if (iStackCount > 1 && !id.IsEmpty())
	{
		// Currently, it's an error for a file to have multiple animations.
		SEOUL_LOG_COOKING("%s: contains %d animations, but id '%s' was provided, this only supports 1 clip for the given file.", sInputFileName.CStr(), iStackCount, id.CStr());
		return false;
	}

	// Get stack 0 and then process it.
	auto pAnimationStack = pScene->GetSrcObject<FbxAnimStack>(0);
	return CookAnimationClipInner(
		id,
		vBones,
		pScene.GetPtr(),
		pAnimationStack,
		ePlatform,
		sInputFileName,
		file);
}

static Bool CookAnimationClips(
	FbxManager* pManager,
	const Bones& vBones,
	Seoul::Platform ePlatform,
	const String& sInputFileName,
	MemorySyncFile& file)
{
	// Cook the file itself as a source of animations
	// This processing has no id, the name must
	// be derived from the anim stack.
	if (!CookAnimationClip(HString(), pManager, vBones, ePlatform, sInputFileName, file))
	{
		return false;
	}

	// List all siblings.
	String const sDirectory(Path::GetDirectoryName(sInputFileName));
	String const sExtension(Path::GetExtension(sInputFileName));

	// Should succeed but return 0 results, so return a failure if this fails.
	Vector<String> vsClips;
	if (!FileManager::Get()->GetDirectoryListing(
		sDirectory,
		vsClips,
		false,
		false,
		sExtension))
	{
		SEOUL_LOG_COOKING("Failed enumerating directory to find animation clips for \"%s\"\n", sInputFileName.CStr());
		return false;
	}

	// Now process any siblings - these are files that start with the input file plus an underscore.
	auto const sCompare(Path::Combine(sDirectory, Path::GetFileNameWithoutExtension(sInputFileName) + "_"));
	for (auto const& s : vsClips)
	{
		// Not a sibling, skip.
		if (!s.StartsWithASCIICaseInsensitive(sCompare))
		{
			continue;
		}

		// Extract the id as the portion after the underscore, converted to lowercase
		// for consistency.
		HString const id(Path::GetFileNameWithoutExtension(s.Substring(sCompare.GetSize())).ToLowerASCII());
		if (!CookAnimationClip(id, pManager, vBones, ePlatform, s, file))
		{
			return false;
		}
	}

	return true;
}

static Bool CookAnimation(
	FbxManager* pManager,
	FbxScene* pScene,
	Seoul::Platform ePlatform,
	const String& sInputFileName,
	MemorySyncFile& file,
	Bones& rvBones)
{
	auto const eSkeletonResult = ExtractSkeleton(pScene, rvBones);
	if (SkeletonResult::kSuccess != eSkeletonResult)
	{
		// We're done processing if no skeleton, but success or failure
		// depends on whether there was an error getting the skeleton,
		// or whether there just is no skeleton.
		return (SkeletonResult::kError != eSkeletonResult);
	}

	// Write skeleton.
	{
		MemorySyncFile innerFile;
		if (!WriteInt32(innerFile, (Int32)DataTypeAnimationSkeleton)) { return false; }
		if (!WriteUInt32(innerFile, rvBones.GetSize())) { return false; }
		for (auto const& e : rvBones)
		{
			if (!WriteHString(innerFile, e.m_Id)) { return false; }
			if (!WriteHString(innerFile, e.m_ParentId)) { return false; }
			if (!WriteQuaternion(innerFile, e.m_qRotation)) { return false; }
			if (!WriteVector3D(innerFile, e.m_vPosition)) { return false; }
			if (!WriteVector3D(innerFile, e.m_vScale)) { return false; }
		}

		UInt32 const uSizeInBytes = (UInt32)innerFile.GetSize();
		if (!WriteInt32(file, (Int32)DataTypeAnimationSkeleton)) { return false; }
		if (!WriteUInt32(file, uSizeInBytes)) { return false; }
		if (uSizeInBytes != file.WriteRawData(innerFile.GetBuffer().GetBuffer(), uSizeInBytes)) { return false; }
	}

	// Now process animation clips.
	return CookAnimationClips(pManager, rvBones, ePlatform, sInputFileName, file);
}

static Bool CookMesh(
	FbxManager* pManager,
	FbxScene* pScene,
	Seoul::Platform ePlatform,
	const String& sInputFileName,
	MemorySyncFile& outerFile,
	const Bones& vBones)
{
	// Get the root node and traverse.
	FbxNode* pRootNode = pScene->GetRootNode();
	CookedMesh cookedMesh;
	if (!CookNode(sInputFileName, vBones, pRootNode, cookedMesh))
	{
		return false;
	}

	// Optimize the mesh - if this fails for some reason,
	// fail the entire operation, since it indicates
	// bad data.
	if (!cookedMesh.Finalize())
	{
		return false;
	}

	AABB const sceneAABB = cookedMesh.GetTotalAABB();

	// Write data.

	// Material library - must be first, as it is associated
	// with the Mesh at runtime load.

	// Don't write empty materials.
	if (!cookedMesh.m_tMaterialToGroups.IsEmpty())
	{
		MemorySyncFile file;
		if (!cookedMesh.WriteMaterials(file)) { return false; }

		UInt32 const uSizeInBytes = (UInt32)file.GetSize();
		if (!WriteInt32(outerFile, (Int32)DataTypeMaterialLibrary)) { return false; }
		if (!WriteUInt32(outerFile, uSizeInBytes)) { return false; }
		if (uSizeInBytes != outerFile.WriteRawData(file.GetBuffer().GetBuffer(), uSizeInBytes)) { return false; }
	}

	// Mesh data.

	// Don't write an empty mesh.
	if (cookedMesh.GetTotalVertices() > 0u)
	{
		Bool const bSkinned = cookedMesh.IsSkinned();

		MemorySyncFile file;
		if (!WriteInt32(file, (Int32)DataTypeMesh)) { return false; }
		if (!WriteAABB(file, sceneAABB)) { return false; }
		if (!WriteVertexFormat(file, bSkinned))
		{
			SEOUL_LOG_COOKING("%s: asset cook failed writing vertex format.", sInputFileName.CStr());
			return false;
		}
		if (!cookedMesh.WriteVertices(file)) { return false; }
		if (!cookedMesh.WritePrimitiveGroups(file)) { return false; }
		if (!cookedMesh.WriteInverseBindPoses(file)) { return false; }

		UInt32 const uSizeInBytes = (UInt32)file.GetSize();
		if (!WriteInt32(outerFile, (Int32)DataTypeMesh)) { return false; }
		if (!WriteUInt32(outerFile, uSizeInBytes)) { return false; }
		if (uSizeInBytes != outerFile.WriteRawData(file.GetBuffer().GetBuffer(), uSizeInBytes)) { return false; }
	}

	// Done, success.
	return true;
}

Bool CookSceneAsset(
	Seoul::Platform ePlatform,
	Byte const* sInputFileName,
	void** ppOutputData,
	UInt32* pzOutputDataSizeInBytes)
{
	ScopedFbxPointer<FbxManager> pManager(FbxManager::Create());
	{
		ScopedFbxPointer<FbxScene> pScene(FbxScene::Create(pManager.GetPtr(), ksEmptyString));
		if (!LoadFbxFile(pManager.GetPtr(), pScene.GetPtr(), sInputFileName))
		{
			return false;
		}

		// Animation cook first.
		MemorySyncFile file;

		Bones vBones;
		if (!CookAnimation(pManager.GetPtr(), pScene.GetPtr(), ePlatform, sInputFileName, file, vBones))
		{
			return false;
		}

		// Mesh cook second.
		if (!CookMesh(pManager.GetPtr(), pScene.GetPtr(), ePlatform, sInputFileName, file, vBones))
		{
			return false;
		}

		// Done with success, populate the output files.
		file.GetBuffer().RelinquishBuffer(*ppOutputData, *pzOutputDataSizeInBytes);
		return true;
	}
}

} // namespace Cooking

} // namespace Seoul
