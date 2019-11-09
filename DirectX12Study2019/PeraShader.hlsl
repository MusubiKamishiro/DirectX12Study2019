// テクスチャ(0番)
Texture2D<float4> tex : register(t0);
// サンプラ(0番)
SamplerState smp : register(s0);

struct Output
{
	//float4 pos		: POSITION;		// システム用頂点座標
	float4 svpos	: SV_POSITION;	// システム用頂点座標
	float2 uv		: TEXCOORD;		// UV値
};

// 頂点シェーダ
Output vs(float4 pos : POSITION, float2 uv : TEXCOORD)
{
	Output output;
	output.svpos = pos;
	output.uv = uv;

	return output;
}


// ピクセルシェーダ
float4 ps(Output output) : SV_TARGET
{
	float3 ret = tex.Sample(smp, output.uv).rgb;

	// 反転
	//ret = 1 - ret;
	// ポスタリゼーション
	//ret = ret - fmod(ret, 0.25f);
	// モノクロ色
	//ret = dot(float3(0.2126f, 0.7152f, 0.0722f), ret);
	// 輪郭線抽出
	ret *= 4;
	ret -= tex.Sample(smp, output.uv, int2(0, -2));
	ret -= tex.Sample(smp, output.uv, int2(0, 2));
	ret -= tex.Sample(smp, output.uv, int2(2, 0));
	ret -= tex.Sample(smp, output.uv, int2(-2, 0));
	
	return float4(ret, 1.0f);
}