#if 0
//
// Generated by Microsoft (R) D3D Shader Disassembler
//
//
//   fxc /Tps_5_0 /ED3D11ClearPS /O3 /WX /Ges /Gis /Qstrip_reflect
//    /Qstrip_debug /FhD3D11ClearPS.h D3D11ClearPS.shader
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue Format   Used
// -------------------- ----- ------ -------- -------- ------ ------
// SV_POSITION              0   xyzw        0      POS  float       
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue Format   Used
// -------------------- ----- ------ -------- -------- ------ ------
// SV_Target                0   xyzw        0   TARGET  float   xyzw
//
ps_5_0
dcl_globalFlags refactoringAllowed
dcl_constantbuffer cb0[1], immediateIndexed
dcl_output o0.xyzw
mov [precise] o0.xyzw, cb0[0].xyzw
ret 
// Approximately 0 instruction slots used
#endif

const BYTE g_D3D11ClearPS[] =
{
	 68,  88,  66,  67,   6, 164, 
	139, 166, 243, 123, 236,  67, 
	170, 224,  81,  59,  63, 120, 
	197, 108,   1,   0,   0,   0, 
	224,   0,   0,   0,   3,   0, 
	  0,   0,  44,   0,   0,   0, 
	 96,   0,   0,   0, 148,   0, 
	  0,   0,  73,  83,  71,  78, 
	 44,   0,   0,   0,   1,   0, 
	  0,   0,   8,   0,   0,   0, 
	 32,   0,   0,   0,   0,   0, 
	  0,   0,   1,   0,   0,   0, 
	  3,   0,   0,   0,   0,   0, 
	  0,   0,  15,   0,   0,   0, 
	 83,  86,  95,  80,  79,  83, 
	 73,  84,  73,  79,  78,   0, 
	 79,  83,  71,  78,  44,   0, 
	  0,   0,   1,   0,   0,   0, 
	  8,   0,   0,   0,  32,   0, 
	  0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   3,   0, 
	  0,   0,   0,   0,   0,   0, 
	 15,   0,   0,   0,  83,  86, 
	 95,  84,  97, 114, 103, 101, 
	116,   0, 171, 171,  83,  72, 
	 69,  88,  68,   0,   0,   0, 
	 80,   0,   0,   0,  17,   0, 
	  0,   0, 106,   8,   0,   1, 
	 89,   0,   0,   4,  70, 142, 
	 32,   0,   0,   0,   0,   0, 
	  1,   0,   0,   0, 101,   0, 
	  0,   3, 242,  32,  16,   0, 
	  0,   0,   0,   0,  54,   0, 
	120,   6, 242,  32,  16,   0, 
	  0,   0,   0,   0,  70, 142, 
	 32,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,  62,   0, 
	  0,   1
};
