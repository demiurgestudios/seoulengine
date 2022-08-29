/**
 * \file GLSLFXLite.h
 * \brief Implement of a shader Effect system, includes:
 * - render state management.
 * - shader and shader parameter management using GLSL shaders.
 * - offline generation of compiled Effects for GLSL platforms.
 *
 * Copyright (c) Demiurge Studios, Inc.
 * 
 * This source code is licensed under the MIT license.
 * Full license details can be found in the LICENSE file
 * in the root directory of this source tree.
 */

#pragma once
#ifndef GLSLFX_LITE_H
#define GLSLFX_LITE_H

#include "FilePath.h"
#include "Prereqs.h"
#include "ScopedPtr.h"
#include "UnsafeHandle.h"
#include "Vector.h"
namespace Seoul { class BaseTexture; }

namespace Seoul
{

typedef UInt16 GLSLFXLiteHandle;

struct GLSLFXLiteEffectDescription
{
	UInt32 m_uShaders;
	UInt32 m_uParameters;
	UInt32 m_uPasses;
	UInt32 m_uTechniques;
};

enum GLSLFXparameterclass
{
	GLSLFX_PARAMETERCLASS_UNKNOWN,
	GLSLFX_PARAMETERCLASS_ARRAY,
	GLSLFX_PARAMETERCLASS_MATRIX,
	GLSLFX_PARAMETERCLASS_OBJECT,
	GLSLFX_PARAMETERCLASS_SAMPLER,
	GLSLFX_PARAMETERCLASS_SCALAR,
	GLSLFX_PARAMETERCLASS_STRUCT,
	GLSLFX_PARAMETERCLASS_VECTOR,
	GLSLFX_PARAMETERCLASS_DUMMY = 0xFFFFFFFF
};

enum GLSLFXtype
{
	GLSLFX_UNKNOWN_TYPE      = 0,
	GLSLFX_ARRAY             = 2,
	GLSLFX_STRING            = 1135,
	GLSLFX_STRUCT            = 1,
	GLSLFX_TYPELESS_STRUCT   = 3,
	GLSLFX_TEXTURE           = 1137,
	GLSLFX_BUFFER            = 1319,
	GLSLFX_UNIFORMBUFFER     = 1320,
	GLSLFX_ADDRESS           = 1321,
	GLSLFX_PIXELSHADER_TYPE  = 1142,
	GLSLFX_PROGRAM_TYPE      = 1136,
	GLSLFX_VERTEXSHADER_TYPE = 1141,
	GLSLFX_SAMPLER           = 1143,
	GLSLFX_SAMPLER1D         = 1065,
	GLSLFX_SAMPLER1DARRAY    = 1138,
	GLSLFX_SAMPLER1DSHADOW   = 1313,
	GLSLFX_SAMPLER2D         = 1066,
	GLSLFX_SAMPLER2DARRAY    = 1139,
	GLSLFX_SAMPLER2DMS       = 1317, /* ARB_texture_multisample */
	GLSLFX_SAMPLER2DMSARRAY  = 1318, /* ARB_texture_multisample */
	GLSLFX_SAMPLER2DSHADOW   = 1314,
	GLSLFX_SAMPLER3D         = 1067,
	GLSLFX_SAMPLERBUF        = 1144,
	GLSLFX_SAMPLERCUBE       = 1069,
	GLSLFX_SAMPLERCUBEARRAY  = 1140,
	GLSLFX_SAMPLERRBUF       = 1316, /* NV_explicit_multisample */
	GLSLFX_SAMPLERRECT       = 1068,
	GLSLFX_SAMPLERRECTSHADOW = 1315,
	GLSLFX_TYPE_START_ENUM   = 1024,
	GLSLFX_BOOL              = 1114,
	GLSLFX_BOOL1             = 1115,
	GLSLFX_BOOL2             = 1116,
	GLSLFX_BOOL3             = 1117,
	GLSLFX_BOOL4             = 1118,
	GLSLFX_BOOL1x1           = 1119,
	GLSLFX_BOOL1x2           = 1120,
	GLSLFX_BOOL1x3           = 1121,
	GLSLFX_BOOL1x4           = 1122,
	GLSLFX_BOOL2x1           = 1123,
	GLSLFX_BOOL2x2           = 1124,
	GLSLFX_BOOL2x3           = 1125,
	GLSLFX_BOOL2x4           = 1126,
	GLSLFX_BOOL3x1           = 1127,
	GLSLFX_BOOL3x2           = 1128,
	GLSLFX_BOOL3x3           = 1129,
	GLSLFX_BOOL3x4           = 1130,
	GLSLFX_BOOL4x1           = 1131,
	GLSLFX_BOOL4x2           = 1132,
	GLSLFX_BOOL4x3           = 1133,
	GLSLFX_BOOL4x4           = 1134,
	GLSLFX_CHAR              = 1166,
	GLSLFX_CHAR1             = 1167,
	GLSLFX_CHAR2             = 1168,
	GLSLFX_CHAR3             = 1169,
	GLSLFX_CHAR4             = 1170,
	GLSLFX_CHAR1x1           = 1171,
	GLSLFX_CHAR1x2           = 1172,
	GLSLFX_CHAR1x3           = 1173,
	GLSLFX_CHAR1x4           = 1174,
	GLSLFX_CHAR2x1           = 1175,
	GLSLFX_CHAR2x2           = 1176,
	GLSLFX_CHAR2x3           = 1177,
	GLSLFX_CHAR2x4           = 1178,
	GLSLFX_CHAR3x1           = 1179,
	GLSLFX_CHAR3x2           = 1180,
	GLSLFX_CHAR3x3           = 1181,
	GLSLFX_CHAR3x4           = 1182,
	GLSLFX_CHAR4x1           = 1183,
	GLSLFX_CHAR4x2           = 1184,
	GLSLFX_CHAR4x3           = 1185,
	GLSLFX_CHAR4x4           = 1186,
	GLSLFX_DOUBLE            = 1145,
	GLSLFX_DOUBLE1           = 1146,
	GLSLFX_DOUBLE2           = 1147,
	GLSLFX_DOUBLE3           = 1148,
	GLSLFX_DOUBLE4           = 1149,
	GLSLFX_DOUBLE1x1         = 1150,
	GLSLFX_DOUBLE1x2         = 1151,
	GLSLFX_DOUBLE1x3         = 1152,
	GLSLFX_DOUBLE1x4         = 1153,
	GLSLFX_DOUBLE2x1         = 1154,
	GLSLFX_DOUBLE2x2         = 1155,
	GLSLFX_DOUBLE2x3         = 1156,
	GLSLFX_DOUBLE2x4         = 1157,
	GLSLFX_DOUBLE3x1         = 1158,
	GLSLFX_DOUBLE3x2         = 1159,
	GLSLFX_DOUBLE3x3         = 1160,
	GLSLFX_DOUBLE3x4         = 1161,
	GLSLFX_DOUBLE4x1         = 1162,
	GLSLFX_DOUBLE4x2         = 1163,
	GLSLFX_DOUBLE4x3         = 1164,
	GLSLFX_DOUBLE4x4         = 1165,
	GLSLFX_FIXED             = 1070,
	GLSLFX_FIXED1            = 1092,
	GLSLFX_FIXED2            = 1071,
	GLSLFX_FIXED3            = 1072,
	GLSLFX_FIXED4            = 1073,
	GLSLFX_FIXED1x1          = 1074,
	GLSLFX_FIXED1x2          = 1075,
	GLSLFX_FIXED1x3          = 1076,
	GLSLFX_FIXED1x4          = 1077,
	GLSLFX_FIXED2x1          = 1078,
	GLSLFX_FIXED2x2          = 1079,
	GLSLFX_FIXED2x3          = 1080,
	GLSLFX_FIXED2x4          = 1081,
	GLSLFX_FIXED3x1          = 1082,
	GLSLFX_FIXED3x2          = 1083,
	GLSLFX_FIXED3x3          = 1084,
	GLSLFX_FIXED3x4          = 1085,
	GLSLFX_FIXED4x1          = 1086,
	GLSLFX_FIXED4x2          = 1087,
	GLSLFX_FIXED4x3          = 1088,
	GLSLFX_FIXED4x4          = 1089,
	GLSLFX_FLOAT             = 1045,
	GLSLFX_FLOAT1            = 1091,
	GLSLFX_FLOAT2            = 1046,
	GLSLFX_FLOAT3            = 1047,
	GLSLFX_FLOAT4            = 1048,
	GLSLFX_FLOAT1x1          = 1049,
	GLSLFX_FLOAT1x2          = 1050,
	GLSLFX_FLOAT1x3          = 1051,
	GLSLFX_FLOAT1x4          = 1052,
	GLSLFX_FLOAT2x1          = 1053,
	GLSLFX_FLOAT2x2          = 1054,
	GLSLFX_FLOAT2x3          = 1055,
	GLSLFX_FLOAT2x4          = 1056,
	GLSLFX_FLOAT3x1          = 1057,
	GLSLFX_FLOAT3x2          = 1058,
	GLSLFX_FLOAT3x3          = 1059,
	GLSLFX_FLOAT3x4          = 1060,
	GLSLFX_FLOAT4x1          = 1061,
	GLSLFX_FLOAT4x2          = 1062,
	GLSLFX_FLOAT4x3          = 1063,
	GLSLFX_FLOAT4x4          = 1064,
	GLSLFX_HALF              = 1025,
	GLSLFX_HALF1             = 1090,
	GLSLFX_HALF2             = 1026,
	GLSLFX_HALF3             = 1027,
	GLSLFX_HALF4             = 1028,
	GLSLFX_HALF1x1           = 1029,
	GLSLFX_HALF1x2           = 1030,
	GLSLFX_HALF1x3           = 1031,
	GLSLFX_HALF1x4           = 1032,
	GLSLFX_HALF2x1           = 1033,
	GLSLFX_HALF2x2           = 1034,
	GLSLFX_HALF2x3           = 1035,
	GLSLFX_HALF2x4           = 1036,
	GLSLFX_HALF3x1           = 1037,
	GLSLFX_HALF3x2           = 1038,
	GLSLFX_HALF3x3           = 1039,
	GLSLFX_HALF3x4           = 1040,
	GLSLFX_HALF4x1           = 1041,
	GLSLFX_HALF4x2           = 1042,
	GLSLFX_HALF4x3           = 1043,
	GLSLFX_HALF4x4           = 1044,
	GLSLFX_INT               = 1093,
	GLSLFX_INT1              = 1094,
	GLSLFX_INT2              = 1095,
	GLSLFX_INT3              = 1096,
	GLSLFX_INT4              = 1097,
	GLSLFX_INT1x1            = 1098,
	GLSLFX_INT1x2            = 1099,
	GLSLFX_INT1x3            = 1100,
	GLSLFX_INT1x4            = 1101,
	GLSLFX_INT2x1            = 1102,
	GLSLFX_INT2x2            = 1103,
	GLSLFX_INT2x3            = 1104,
	GLSLFX_INT2x4            = 1105,
	GLSLFX_INT3x1            = 1106,
	GLSLFX_INT3x2            = 1107,
	GLSLFX_INT3x3            = 1108,
	GLSLFX_INT3x4            = 1109,
	GLSLFX_INT4x1            = 1110,
	GLSLFX_INT4x2            = 1111,
	GLSLFX_INT4x3            = 1112,
	GLSLFX_INT4x4            = 1113,
	GLSLFX_LONG              = 1271,
	GLSLFX_LONG1             = 1272,
	GLSLFX_LONG2             = 1273,
	GLSLFX_LONG3             = 1274,
	GLSLFX_LONG4             = 1275,
	GLSLFX_LONG1x1           = 1276,
	GLSLFX_LONG1x2           = 1277,
	GLSLFX_LONG1x3           = 1278,
	GLSLFX_LONG1x4           = 1279,
	GLSLFX_LONG2x1           = 1280,
	GLSLFX_LONG2x2           = 1281,
	GLSLFX_LONG2x3           = 1282,
	GLSLFX_LONG2x4           = 1283,
	GLSLFX_LONG3x1           = 1284,
	GLSLFX_LONG3x2           = 1285,
	GLSLFX_LONG3x3           = 1286,
	GLSLFX_LONG3x4           = 1287,
	GLSLFX_LONG4x1           = 1288,
	GLSLFX_LONG4x2           = 1289,
	GLSLFX_LONG4x3           = 1290,
	GLSLFX_LONG4x4           = 1291,
	GLSLFX_SHORT             = 1208,
	GLSLFX_SHORT1            = 1209,
	GLSLFX_SHORT2            = 1210,
	GLSLFX_SHORT3            = 1211,
	GLSLFX_SHORT4            = 1212,
	GLSLFX_SHORT1x1          = 1213,
	GLSLFX_SHORT1x2          = 1214,
	GLSLFX_SHORT1x3          = 1215,
	GLSLFX_SHORT1x4          = 1216,
	GLSLFX_SHORT2x1          = 1217,
	GLSLFX_SHORT2x2          = 1218,
	GLSLFX_SHORT2x3          = 1219,
	GLSLFX_SHORT2x4          = 1220,
	GLSLFX_SHORT3x1          = 1221,
	GLSLFX_SHORT3x2          = 1222,
	GLSLFX_SHORT3x3          = 1223,
	GLSLFX_SHORT3x4          = 1224,
	GLSLFX_SHORT4x1          = 1225,
	GLSLFX_SHORT4x2          = 1226,
	GLSLFX_SHORT4x3          = 1227,
	GLSLFX_SHORT4x4          = 1228,
	GLSLFX_UCHAR             = 1187,
	GLSLFX_UCHAR1            = 1188,
	GLSLFX_UCHAR2            = 1189,
	GLSLFX_UCHAR3            = 1190,
	GLSLFX_UCHAR4            = 1191,
	GLSLFX_UCHAR1x1          = 1192,
	GLSLFX_UCHAR1x2          = 1193,
	GLSLFX_UCHAR1x3          = 1194,
	GLSLFX_UCHAR1x4          = 1195,
	GLSLFX_UCHAR2x1          = 1196,
	GLSLFX_UCHAR2x2          = 1197,
	GLSLFX_UCHAR2x3          = 1198,
	GLSLFX_UCHAR2x4          = 1199,
	GLSLFX_UCHAR3x1          = 1200,
	GLSLFX_UCHAR3x2          = 1201,
	GLSLFX_UCHAR3x3          = 1202,
	GLSLFX_UCHAR3x4          = 1203,
	GLSLFX_UCHAR4x1          = 1204,
	GLSLFX_UCHAR4x2          = 1205,
	GLSLFX_UCHAR4x3          = 1206,
	GLSLFX_UCHAR4x4          = 1207,
	GLSLFX_UINT              = 1250,
	GLSLFX_UINT1             = 1251,
	GLSLFX_UINT2             = 1252,
	GLSLFX_UINT3             = 1253,
	GLSLFX_UINT4             = 1254,
	GLSLFX_UINT1x1           = 1255,
	GLSLFX_UINT1x2           = 1256,
	GLSLFX_UINT1x3           = 1257,
	GLSLFX_UINT1x4           = 1258,
	GLSLFX_UINT2x1           = 1259,
	GLSLFX_UINT2x2           = 1260,
	GLSLFX_UINT2x3           = 1261,
	GLSLFX_UINT2x4           = 1262,
	GLSLFX_UINT3x1           = 1263,
	GLSLFX_UINT3x2           = 1264,
	GLSLFX_UINT3x3           = 1265,
	GLSLFX_UINT3x4           = 1266,
	GLSLFX_UINT4x1           = 1267,
	GLSLFX_UINT4x2           = 1268,
	GLSLFX_UINT4x3           = 1269,
	GLSLFX_UINT4x4           = 1270,
	GLSLFX_ULONG             = 1292,
	GLSLFX_ULONG1            = 1293,
	GLSLFX_ULONG2            = 1294,
	GLSLFX_ULONG3            = 1295,
	GLSLFX_ULONG4            = 1296,
	GLSLFX_ULONG1x1          = 1297,
	GLSLFX_ULONG1x2          = 1298,
	GLSLFX_ULONG1x3          = 1299,
	GLSLFX_ULONG1x4          = 1300,
	GLSLFX_ULONG2x1          = 1301,
	GLSLFX_ULONG2x2          = 1302,
	GLSLFX_ULONG2x3          = 1303,
	GLSLFX_ULONG2x4          = 1304,
	GLSLFX_ULONG3x1          = 1305,
	GLSLFX_ULONG3x2          = 1306,
	GLSLFX_ULONG3x3          = 1307,
	GLSLFX_ULONG3x4          = 1308,
	GLSLFX_ULONG4x1          = 1309,
	GLSLFX_ULONG4x2          = 1310,
	GLSLFX_ULONG4x3          = 1311,
	GLSLFX_ULONG4x4          = 1312,
	GLSLFX_USHORT            = 1229,
	GLSLFX_USHORT1           = 1230,
	GLSLFX_USHORT2           = 1231,
	GLSLFX_USHORT3           = 1232,
	GLSLFX_USHORT4           = 1233,
	GLSLFX_USHORT1x1         = 1234,
	GLSLFX_USHORT1x2         = 1235,
	GLSLFX_USHORT1x3         = 1236,
	GLSLFX_USHORT1x4         = 1237,
	GLSLFX_USHORT2x1         = 1238,
	GLSLFX_USHORT2x2         = 1239,
	GLSLFX_USHORT2x3         = 1240,
	GLSLFX_USHORT2x4         = 1241,
	GLSLFX_USHORT3x1         = 1242,
	GLSLFX_USHORT3x2         = 1243,
	GLSLFX_USHORT3x3         = 1244,
	GLSLFX_USHORT3x4         = 1245,
	GLSLFX_USHORT4x1         = 1246,
	GLSLFX_USHORT4x2         = 1247,
	GLSLFX_USHORT4x3         = 1248,
	GLSLFX_USHORT4x4         = 1249
};

struct GLSLFXLiteParameterDescription
{
	GLSLFXparameterclass m_Class;
	UInt32 m_uColumns;
	UInt32 m_uElements;
	UInt32 m_uRows;
	UInt32 m_uSize;
	GLSLFXtype m_Type;
	GLSLFXLiteHandle m_hName;
	UInt16 m_uUnusedPadding;
};

struct GLSLFXLiteTechniqueDescription
{
	UInt32 m_uPasses;
	GLSLFXLiteHandle m_hName;
	UInt16 m_uUnusedPadding;
};

struct GLSLFXLitePassDescription
{
	GLSLFXLiteHandle m_hName;
	UInt16 m_uUnusedPadding;
};

struct GLSLFXLiteParameterData
{
	union
	{
		Int32 m_iFixed;
		Float32 m_fFloat;
		UInt32 m_Texture;
	};
};

SEOUL_STATIC_ASSERT(sizeof(GLSLFXLiteParameterData) == sizeof(Float32));

struct GLSLFXLiteGlobalParameterEntry
{
	UInt16 m_uIndex;
	UInt16 m_uCount;
	UInt32 m_uDirtyStamp;
};

struct GLSLFXLiteTechniqueEntry
{
	GLSLFXLiteHandle m_hFirstPass;
	GLSLFXLiteHandle m_hLastPass;
};

struct GLSLFXLiteRenderState
{
	UInt32 m_uState;
	UInt32 m_uValue;
};

struct GLSLFXLitePassEntry
{
	GLSLFXLiteHandle m_hFirstRenderState;
	GLSLFXLiteHandle m_hLastRenderState;

	GLSLFXLiteHandle m_hPixelShader;
	GLSLFXLiteHandle m_hVertexShader;

	GLSLFXLiteHandle m_hParameterFirst;
	GLSLFXLiteHandle m_hParameterLast;

	UInt32 m_Program;
};

struct GLSLFXLiteShaderEntry
{
	UInt32 m_hShaderCodeFirst;
	UInt32 m_hShaderCodeLast;
	GLSLFXLiteHandle m_hDeprecatedName;
	UInt16 m_bIsVertexShader;
};

struct GLSLFXLiteProgramParameter
{
	UInt32 m_uDirtyStamp;
	UInt16 m_uGlobalParameterIndex;
	UInt16 m_uParameterIndex;
	UInt16 m_uParameterCount;
	UInt16 m_uParameterClass;
	Int32 m_iHardwareIndex;
	GLSLFXLiteHandle m_hParameterLookupName;
	UInt16 m_ReservedUnusedPadding;
};

struct GLSLFXLiteDataRuntime
{
	GLSLFXLiteEffectDescription m_Description;
	Byte* m_pStrings;
	GLSLFXLiteParameterDescription* m_pParameters;
	GLSLFXLiteTechniqueDescription* m_pTechniques;
	GLSLFXLitePassDescription* m_pPasses;

	GLSLFXLiteParameterData* m_pParameterData;
	GLSLFXLiteGlobalParameterEntry* m_pParameterEntries;
	GLSLFXLiteTechniqueEntry* m_pTechniqueEntries;
	GLSLFXLitePassEntry* m_pPassEntries;
	GLSLFXLiteRenderState* m_pRenderStates;
	GLSLFXLiteShaderEntry* m_pShaderEntries;
	Byte* m_pShaderCode;
	GLSLFXLiteProgramParameter* m_pProgramParameters;
};

struct GLSLFXLiteDataSerialized
{
	GLSLFXLiteEffectDescription m_Description;
	UInt32 m_uStrings;
	UInt32 m_uParameters;
	UInt32 m_uTechniques;
	UInt32 m_uPasses;

	UInt32 m_uParameterData;
	UInt32 m_uParameterEntries;
	UInt32 m_uTechniqueEntries;
	UInt32 m_uPassEntries;
	UInt32 m_uRenderStates;
	UInt32 m_uShaderEntries;
	UInt32 m_uShaderCode;
	UInt32 m_uProgramParameters;
};

SEOUL_STATIC_ASSERT(sizeof(GLSLFXLiteDataSerialized) == 64);

struct GLSLFXLiteRuntimeShaderData
{
	GLSLFXLiteRuntimeShaderData(
		Bool bVertexShader,
		const char* sShaderSource,
		Int zSourceLength);
	~GLSLFXLiteRuntimeShaderData();

	UInt32 m_Object;
}; // struct GLSLFXLiteRuntimeShaderData

namespace GLSLFXLiteUtil
{

template <typename T>
inline void FixupPointer(GLSLFXLiteDataSerialized const* pBase, UInt32 uOffset, T*& rp)
{
	if (0u != uOffset)
	{
		rp = (T*)((size_t)pBase + (size_t)uOffset);

		// Sanity check to verify that data was serialized
		// correctly and has the correct alignment after fixup.
		SEOUL_ASSERT(0u == (((size_t)rp) % SEOUL_ALIGN_OF(T)));
	}
	else
	{
		rp = nullptr;
	}
}

static inline void SetupSerializedData(
	GLSLFXLiteDataSerialized* p,
	GLSLFXLiteDataRuntime& rData)
{
	// Zero initialize the runtime data.
	memset(&rData, 0, sizeof(rData));

	// Copy over the shared effect description block.
	memcpy(&(rData.m_Description), &(p->m_Description), sizeof(rData.m_Description));

	// Fixup pointers
	FixupPointer(p, p->m_uStrings, rData.m_pStrings);
	FixupPointer(p, p->m_uParameters, rData.m_pParameters);
	FixupPointer(p, p->m_uTechniques, rData.m_pTechniques);
	FixupPointer(p, p->m_uPasses, rData.m_pPasses);

	FixupPointer(p, p->m_uParameterData, rData.m_pParameterData);
	FixupPointer(p, p->m_uParameterEntries, rData.m_pParameterEntries);
	FixupPointer(p, p->m_uTechniqueEntries, rData.m_pTechniqueEntries);
	FixupPointer(p, p->m_uPassEntries, rData.m_pPassEntries);
	FixupPointer(p, p->m_uRenderStates, rData.m_pRenderStates);
	FixupPointer(p, p->m_uShaderEntries, rData.m_pShaderEntries);
	FixupPointer(p, p->m_uShaderCode, rData.m_pShaderCode);
	FixupPointer(p, p->m_uProgramParameters, rData.m_pProgramParameters);
}

static inline UInt32 HandleToOffset(GLSLFXLiteHandle h)
{
	return (h - 1u);
}

static inline Bool IsValid(GLSLFXLiteHandle h)
{
	return (0u != h);
}

static inline Byte const* GetString(Byte* pStrings, GLSLFXLiteHandle hString)
{
	if (IsValid(hString))
	{
		return (pStrings + HandleToOffset(hString));
	}

	return nullptr;
}

static inline HString GetHString(Byte* pStrings, GLSLFXLiteHandle hString)
{
	auto s = GetString(pStrings, hString);
	if (nullptr == s)
	{
		return HString();
	}

	return HString(s);
}

} // namespace GLSLFxLiteUtil

class GLSLFXLite SEOUL_SEALED
{
public:
	GLSLFXLite(
		FilePath filePath,
		void* pRawEffectFileData,
		UInt32 zFileSizeInBytes);
	~GLSLFXLite();

	void BeginPassFromIndex(UInt32 nPassIndex);
	void BeginTechnique(UnsafeHandle hTechnique);
	void Commit();
	void EndPass();
	void EndTechnique();
	void GetEffectDescription(
		GLSLFXLiteEffectDescription* pDescription) const;
	void GetParameterDescription(
		UnsafeHandle hParameter,
		GLSLFXLiteParameterDescription* pDescription) const;
	UnsafeHandle GetParameterHandleFromIndex(
		UInt nParameterIndex) const;
	Byte const* GetString(GLSLFXLiteHandle hString) const;
	void GetTechniqueDescription(
		UnsafeHandle hTechnique,
		GLSLFXLiteTechniqueDescription* pDescription) const;
	UnsafeHandle GetTechniqueHandleFromIndex(
		UInt nTechniqueIndex) const;
	void OnLostDevice();
	void OnResetDevice();

	void SetBool(UnsafeHandle hParameter, Bool bValue);
	void SetFloat(UnsafeHandle hParameter, Float fValue);
	void SetInt(UnsafeHandle hParameter, Int iValue);
	void SetMatrixF4x4(UnsafeHandle hParameter, Float const* pMatrix4D);
	void SetSampler(UnsafeHandle hParameter, BaseTexture* pTexture);
	void SetScalarArrayF(UnsafeHandle hParameter, Float const* pValues, UInt nNumberOfFloats);

private:
	void InternalSetupHardwareIndices(const GLSLFXLitePassEntry& passEntry);

	void InternalStaticInlineSetValue(
		UnsafeHandle hParameter,
		UInt32 const nInputCount,
		void const* pData);

	void InternalCommitProgramParameters(const GLSLFXLitePassEntry& pass);

	UnsafeHandle m_hActivePass;
	UnsafeHandle m_hActiveTechnique;
	GLSLFXLiteHandle m_hPreviousPixelShader;
	GLSLFXLiteHandle m_hPreviousVertexShader;

	GLSLFXLiteDataRuntime m_Data;
	GLSLFXLiteDataSerialized* m_pDataSerialized;
	GLSLFXLiteRuntimeShaderData* m_pShaderData;
	BaseTexture** m_pTextureReferences;
	Int32* m_pSecondaryTextureData;
}; // class GLSLFXLite

} // namespace Seoul

#endif // include guard
