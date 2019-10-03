// ﾃｸｽﾁｬ(0番)
Texture2D<float4> tex	: register(t0);
// ｻﾝﾌﾟﾗ(0)
SamplerState smp : register(s0);
// 定数バッファ
cbuffer mat : register(b0)
{
	matrix world;		// ﾜｰﾙﾄﾞ
	matrix viewProj;	// ﾋﾞｭｰﾌﾟﾛｼﾞｪｸｼｮﾝ
	matrix wvp;			// 合成済み
	matrix lightVP;		// ﾗｲﾄﾋﾞｭｰﾌﾟﾛｼﾞｪｸｼｮﾝ
};

struct Output
{
	float4 pos			: POSITION;
	float4 svpos		: SV_POSITION;
	float4 normal		: NORMAL;
	float2 uv			: TEXCOORD;
};

// 頂点シェーダ
Output vs(float4 pos : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD)
{
	Output output;
	//output.pos = pos;
	output.pos = mul(world, pos);
	//output.svpos = pos;
	output.svpos = mul(mul(viewProj, world), pos);	// 2次元上
	output.uv = uv;
	output.normal = normal;

	return output;
}


// ピクセルシェーダ
float4 ps(Output output) : SV_TARGET
{
	//return float4(1.0f, float2(output.uv), 1.0f);
	//return float4(tex.Sample(smp, output.uv).rgb, 1.0f);
	//return float4(1.0f, 1.0f, 1.0f, 1.0f);

	return float4(float3(output.normal.rgb), 1.0f);

	/*float3 light = float3(1, 1, 1);
	light = normalize(light);
	float brightness = dot(output.normal.rgb, light);
	return float4(brightness, brightness, brightness, 1.0);*/
}