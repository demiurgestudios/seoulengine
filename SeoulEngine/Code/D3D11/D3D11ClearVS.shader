cbuffer Data : register(b0)
{
	float4 Depth;
};

float4 D3D11ClearVS(uint uId : SV_VERTEXID) : SV_POSITION
{
	float2 uv = float2(uId & 1, uId >> 1);
	return float4((uv.x - 0.5f) * 2.0f, (0.5f - uv.y) * 2.0f, Depth.x, 1.0f);
}
