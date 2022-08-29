//
// E:\projects\seoul\SeoulTools\Code\Cooking\TextureCookISPCKernel_ispc.h
// (Header automatically generated by the ispc compiler.)
// DO NOT EDIT THIS FILE.
//

#ifndef ISPC_D__PROJECTS_SEOUL_SEOULTOOLS_CODE_COOKING_TEXTURECOOKISPCKERNEL_ISPC_H
#define ISPC_D__PROJECTS_SEOUL_SEOULTOOLS_CODE_COOKING_TEXTURECOOKISPCKERNEL_ISPC_H

#include <stdint.h>



#ifdef __cplusplus
namespace ispc { /* namespace */
#endif // __cplusplus


#ifndef __ISPC_ALIGN__
#if defined(__clang__) || !defined(_MSC_VER)
// Clang, GCC, ICC
#define __ISPC_ALIGN__(s) __attribute__((aligned(s)))
#define __ISPC_ALIGNED_STRUCT__(s) struct __ISPC_ALIGN__(s)
#else
// Visual Studio
#define __ISPC_ALIGN__(s) __declspec(align(s))
#define __ISPC_ALIGNED_STRUCT__(s) __ISPC_ALIGN__(s) struct
#endif
#endif

#ifndef __ISPC_STRUCT_rgba_surface__
#define __ISPC_STRUCT_rgba_surface__
struct Image {
	uint8_t* m_pData;
	int32_t m_iWidth;
	int32_t m_iHeight;
	int32_t m_iPitchInBytes;
};
#endif

///////////////////////////////////////////////////////////////////////////
// Functions exported from ispc code
///////////////////////////////////////////////////////////////////////////
#if defined(__cplusplus) && (! defined(__ISPC_NO_EXTERN_C) || !__ISPC_NO_EXTERN_C )
extern "C" {
#endif // __cplusplus
	extern void DXT1_Compress(struct Image * src, uint8_t * dst);
	extern void DXT5_Compress(struct Image * src, uint8_t * dst);
	extern void ETC1_Compress(struct Image * src, uint8_t * dst, int32_t iQuality);
#if defined(__cplusplus) && (! defined(__ISPC_NO_EXTERN_C) || !__ISPC_NO_EXTERN_C )
} /* end extern C */
#endif // __cplusplus


#ifndef __ISPC_ALIGN__
#if defined(__clang__) || !defined(_MSC_VER)
// Clang, GCC, ICC
#define __ISPC_ALIGN__(s) __attribute__((aligned(s)))
#define __ISPC_ALIGNED_STRUCT__(s) struct __ISPC_ALIGN__(s)
#else
// Visual Studio
#define __ISPC_ALIGN__(s) __declspec(align(s))
#define __ISPC_ALIGNED_STRUCT__(s) __ISPC_ALIGN__(s) struct
#endif
#endif



#ifdef __cplusplus
} /* namespace */
#endif // __cplusplus

#endif // ISPC_D__PROJECTS_SEOUL_SEOULTOOLS_CODE_COOKING_TEXTURECOOKISPCKERNEL_ISPC_H
