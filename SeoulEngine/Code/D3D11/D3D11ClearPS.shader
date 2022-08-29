cbuffer Data : register(b0)
{
	float4 Color;
};

half4 D3D11ClearPS(float4 pos : SV_POSITION) : SV_Target
{
	return half4(Color);
}
