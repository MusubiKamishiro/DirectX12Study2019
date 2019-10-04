// テクスチャ(0番)
Texture2D<float4> tex	: register(t0);
// サンプラ(0番)
SamplerState smp : register(s0);
// 定数バッファ(0番)
cbuffer mat : register(b0)
{
	matrix world;		// ワールド
	matrix viewProj;	// ビュープロジェクション
	matrix wvp;			// 合成済み
	matrix lightVP;		// ライトビュープロジェクション
};

// 定数ﾊﾞｯﾌｧ(1番)
cbuffer material : register(b1)
{
	float3 diffuse;		// ディフューズカラー(減光色)
	float3 specular;	// スペキュラカラー(光沢色)
	float3 mirror;		// アンビエントカラー(環境色)
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

	//return float4(float3(output.normal.rgb), 1.0f);

	float3 light = float3(-1, 1, -1);
	light = normalize(light);
	float brightness = dot(output.normal.rgb, light);
	return float4(float3(diffuse) * brightness, 1.0);
}