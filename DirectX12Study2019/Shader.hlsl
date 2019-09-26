// ﾃｸｽﾁｬ(0番)
Texture2D<float4> tex	: register(t0);
// ｻﾝﾌﾟﾗ(0)
SamplerState smp : register(s0);

struct Output
{
	float4 pos			: POSITION;
	float4 svpos		: SV_POSITION;
	float2 uv			: TEXCOORD;
};

// 頂点シェーダ
Output vs(float4 pos : POSITION, float2 uv : TEXCOORD)
{
	Output output;
	output.pos = pos;
	output.svpos = pos;
	output.uv = uv;

	return output;
}


// ピクセルシェーダ
float4 ps(Output output) : SV_TARGET
{
	//return float4(0, (output.pos.xy + float2(1, 1)) / 2, 1);
	return float4(1.0f, float2(output.uv), 1.0f);
	//return float4(tex.Sample(smp, output.uv).rgb, 1.0f)
}