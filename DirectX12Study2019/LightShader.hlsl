// サンプラ(0番)
SamplerState smp : register(s0);
// 定数バッファ(0番)
cbuffer camera : register(b0)
{
	matrix world;
	matrix viewProj;
	matrix wvp;			// 合成済み
	matrix lightVP;		// ﾗｲﾄﾋﾞｭｰﾌﾟﾛｼﾞｪｸｼｮﾝ
	float4 eye;			// 視点
};
// ボーン行列(1番)
cbuffer bones : register(b1)
{
	matrix boneMats[512];
}
// 影
Texture2D<float> shadow : register(t0);


struct Output {
	float4 svpos	: SV_POSITION;		// VP乗算済み
	float4 pos		: POSITION;
	float4 normal	: NORMAL;
	float2 uv		: TEXCOORD;
	float2 boneno	: BONENO;
	float2 weight	: WEIGHT;
};

// 頂点ｼｪｰﾀﾞ
Output vs(float4 pos : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD, min16uint2 boneNo : BONENO, min16uint2 weight : WEIGHT)
{
	float w = weight / 100.f;
	matrix m = boneMats[boneNo.x] * w + boneMats[boneNo.y] * (1.00 - w);

	Output output;
	pos = mul(m, pos);		// 骨の曲がりをかける
	output.pos = mul(world, pos);
	output.svpos = mul(mul(lightVP, world), pos);
	output.uv = uv;
	output.boneno = boneNo / 64.0;
	output.weight = float2(w, 1 - w);	// 百分率のやつ
	return output;
}

float4 ps(Output output) : SV_TARGET
{
	float depth = shadow.Sample(smp, output.uv);
	depth = pow(depth, 50);

	//return float4(depth, depth, depth, 1.0f);
	return float4(0.0f, 0.0f, 1.0f, 1.0f);
}