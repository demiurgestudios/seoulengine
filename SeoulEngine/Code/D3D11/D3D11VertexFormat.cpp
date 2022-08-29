/**
 * \file D3D11VertexFormat.cpp
 *
 * \brief D3D11VertexFormat is a specialization of VertexFormat for the
 * D3D11 API. VertexFormat describes the vertex attributes that will
 * be in use for the draw call(s) that are issued while the vertex format
 * is active. The actual vertex buffer and index buffer data is stored in
 * D3D11VertexBuffer and D3D11IndexBuffer respectively.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "D3D11Device.h"
#include "D3D11Util.h"
#include "D3D11VertexFormat.h"
#include "VertexElement.h"

namespace Seoul
{

/**
 * @return A Vector<> of VertexElements defined by the
 * terminated array pElements.
 */
static inline VertexFormat::VertexElements GetElements(VertexElement const* pElements)
{
	VertexFormat::VertexElements vElements;

	UInt32 nElementCount = 0u;
	while (VertexElementEnd != pElements[nElementCount])
	{
		++nElementCount;
	}

	vElements.Resize(nElementCount);
	memcpy(vElements.Get(0u), pElements, sizeof(VertexElement) * nElementCount);

	return vElements;
}

D3D11VertexFormat::D3D11VertexFormat(
	VertexElement const* pElements)
	: VertexFormat(GetElements(pElements))
	, m_pInputLayout(nullptr)
{
}

D3D11VertexFormat::~D3D11VertexFormat()
{
	SafeRelease(m_pInputLayout);
}

/**
 * Helper method, converts a vertex element type to
 * an DXGI_FORMAT enum.
 */
static DXGI_FORMAT ToDXGIFormat(VertexElement::EType eType)
{
	switch (eType)
	{
	case VertexElement::TypeFloat1: return DXGI_FORMAT_R32_FLOAT;
	case VertexElement::TypeFloat2: return DXGI_FORMAT_R32G32_FLOAT;
	case VertexElement::TypeFloat3: return DXGI_FORMAT_R32G32B32_FLOAT;
	case VertexElement::TypeFloat4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
	case VertexElement::TypeColor: return DXGI_FORMAT_R8G8B8A8_UNORM;
	case VertexElement::TypeUByte4: return DXGI_FORMAT_R8G8B8A8_UINT;
	case VertexElement::TypeShort2: return DXGI_FORMAT_R16G16_SINT;
	case VertexElement::TypeShort4: return DXGI_FORMAT_R16G16B16A16_SINT;
	case VertexElement::TypeUByte4N: return DXGI_FORMAT_R8G8B8A8_UNORM;
	case VertexElement::TypeShort2N: return DXGI_FORMAT_R16G16_SNORM;
	case VertexElement::TypeShort4N: return DXGI_FORMAT_R16G16B16A16_SNORM;
	case VertexElement::TypeUShort2N: return DXGI_FORMAT_R16G16_UNORM;
	case VertexElement::TypeUShort4N: return DXGI_FORMAT_R16G16B16A16_UNORM;
	case VertexElement::TypeFloat16_2: return DXGI_FORMAT_R16G16_FLOAT;
	case VertexElement::TypeFloat16_4: return DXGI_FORMAT_R16G16B16A16_FLOAT;
	case VertexElement::TypeUnused: return DXGI_FORMAT_UNKNOWN;
	default:
		SEOUL_FAIL("Out of date switch-case statement.");
		return DXGI_FORMAT_UNKNOWN;
	}
}

/**
 * Helper method, converts a vertex element usage to
 * a D3D11 semantic binding name.
 */
static Byte const* ToD3D11SemanticName(VertexElement::EUsage eUsage)
{
	switch (eUsage)
	{
	case VertexElement::UsagePosition: return "POSITION";
	case VertexElement::UsageBlendWeight: return "BLENDWEIGHT";
	case VertexElement::UsageBlendIndices: return "BLENDINDICES";
	case VertexElement::UsageNormal: return "NORMAL";
	case VertexElement::UsagePSize: return "PSIZE";
	case VertexElement::UsageTexcoord: return "TEXCOORD";
	case VertexElement::UsageTangent: return "TANGENT";
	case VertexElement::UsageBinormal: return "BINORMAL";
	case VertexElement::UsageTessfactor: return "TESSFACTOR";
	case VertexElement::UsagePositionT: return "POSITIONT";
	case VertexElement::UsageColor: return "COLOR";
	case VertexElement::UsageFog: return "FOG";
	case VertexElement::UsageDepth: return "DEPTH";
	case VertexElement::UsageSample: return "SAMPLE";
	default:
		SEOUL_FAIL("Out of date switch-case statement.");
		return "";
	}
}

/**
 * Helper method, converts a vertex element to
 * a D3D11 input layout descriptor.
 */
static D3D11_INPUT_ELEMENT_DESC ToD3D11InputElementDescription(const VertexElement& element, Bool bFirst)
{
	D3D11_INPUT_ELEMENT_DESC ret;
	memset(&ret, 0, sizeof(ret));

	ret.AlignedByteOffset = (bFirst ? 0u : D3D11_APPEND_ALIGNED_ELEMENT);
	ret.Format = ToDXGIFormat(element.m_Type);
	ret.InputSlot = element.m_Stream;
	ret.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	ret.InstanceDataStepRate = 0;
	ret.SemanticIndex = element.m_UsageIndex;
	ret.SemanticName = ToD3D11SemanticName(element.m_Usage);

	return ret;
}

static inline String ToShaderSemantic(const VertexElement& v)
{
	String sReturn(ToD3D11SemanticName(v.m_Usage));

	// Color and texture coordinates need a usage index appended.
	if (v.m_Usage == VertexElement::UsageColor ||
		v.m_Usage == VertexElement::UsageTexcoord)
	{
		sReturn.Append(String::Printf("%u", v.m_UsageIndex));
	}

	return sReturn;
}

static inline Byte const* ToShaderType(const VertexElement& v)
{
	switch (v.m_Type)
	{
	case VertexElement::TypeFloat1: return "float";
	case VertexElement::TypeFloat2: return "float2";
	case VertexElement::TypeFloat3: return "float3";
	case VertexElement::TypeFloat4: return "float4";
	case VertexElement::TypeColor: return "float4";
	case VertexElement::TypeUByte4: return "uint4";
	case VertexElement::TypeShort2: return "int2";
	case VertexElement::TypeShort4: return "int4";
	case VertexElement::TypeUByte4N: return "uint4";
	case VertexElement::TypeShort2N: return "int2";
	case VertexElement::TypeShort4N: return "int4";
	case VertexElement::TypeUShort2N: return "uint2";
	case VertexElement::TypeUShort4N: return "uint4";
	case VertexElement::TypeUDec3: return "uint3";
	case VertexElement::TypeDec3N: return "int3";
	case VertexElement::TypeFloat16_2: return "float2";
	case VertexElement::TypeFloat16_4: return "float4";
	case VertexElement::TypeUnused: // fall-through
	default:
		return "float4";
	};
}

static inline Bool GenerateFakeShader(
	const VertexFormat::VertexElements& vElements,
	ID3DBlob*& rpBlob)
{
	static Byte const* ksPrologue =
		"struct vsSig\n"
		"{\n";
	static Byte const* ksEpilogue =
		"};\n"
		"vsSig main(vsSig input) { return input; }\n";

	String sCode;
	sCode.Append(ksPrologue);
	UInt32 u = 0u;
	for (auto vElem : vElements)
	{
		sCode.Append(String::Printf("\t%s v%u : %s;\n",
			ToShaderType(vElem),
			u++,
			ToShaderSemantic(vElem).CStr()));
	}
	sCode.Append(ksEpilogue);

	return SUCCEEDED(D3DCompile(
		sCode.CStr(),
		sCode.GetSize(),
		"VertexFormatFakeShader",
		nullptr,
		nullptr,
		"main",
		"vs_4_0", // TODO:
		D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_IEEE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3 | D3DCOMPILE_WARNINGS_ARE_ERRORS,
		0u,
		&rpBlob,
		nullptr));
}

Bool D3D11VertexFormat::OnCreate()
{
	const VertexElements& vVertexElements = GetVertexElements();
	if (vVertexElements.IsEmpty())
	{
		SEOUL_VERIFY(VertexFormat::OnCreate());
		return true;
	}

	// Convert each element to a D3D11 InputElement.
	FixedArray<Bool, kMaxStreams> vbFirst(true);

	Vector<D3D11_INPUT_ELEMENT_DESC> vInputElementDescriptions(vVertexElements.GetSize());
	for (UInt32 i = 0u; i < vVertexElements.GetSize(); ++i)
	{
		Bool const bFirst = vbFirst[vVertexElements[i].m_Stream];
		vbFirst[vVertexElements[i].m_Stream] = false;

		vInputElementDescriptions[i] = ToD3D11InputElementDescription(vVertexElements[i], bFirst);
	}

	// TODO: Need to update our pipeline API
	// to not require the generation of this fake
	// shader. Well, "need" might be a strong word -
	// as long as we're exactly matching our stages
	// (format, buffer layout, and shader spec.),
	// this is a perfectly fine way of doing things,
	// and sort of illustrates how weird a choice
	// it was for the D3D11 API to force this requirement.
	ID3DBlob* pBlob = nullptr;
	if (!GenerateFakeShader(vVertexElements, pBlob))
	{
		return false;
	}

	// Create the D3D11 InputLayout with the elements and
	// a (generated and fake) shader bytecode.
	ID3D11InputLayout* pInputLayout = nullptr;
	if (SUCCEEDED(GetD3D11Device().GetD3DDevice()->CreateInputLayout(
		vInputElementDescriptions.Get(0u),
		vInputElementDescriptions.GetSize(),
		pBlob->GetBufferPointer(),
		pBlob->GetBufferSize(),
		&pInputLayout)) || nullptr == pInputLayout)
	{
		SafeRelease(pBlob);
		m_pInputLayout.Reset(pInputLayout);
		SEOUL_VERIFY(VertexFormat::OnCreate());

		return true;
	}

	SafeRelease(pBlob);
	return false;
}

} // namespace Seoul
