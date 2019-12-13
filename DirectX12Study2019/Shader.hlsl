// テクスチャ(0番)	// 通常
Texture2D<float4> tex	: register(t0);
// テクスチャ(1番)	// 加算
Texture2D<float4> spa	: register(t1);
// テクスチャ(2番)	// 乗算
Texture2D<float4> sph	: register(t2);
// テクスチャ(3番)	// トゥーン
Texture2D<float4> toon	: register(t3);
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
// 定数バッファ(1番)
cbuffer material : register(b1)
{
	float3 diffuse;		// ディフューズカラー(減光色)
	float3 specular;	// スペキュラカラー(光沢色)
	float3 mirror;		// アンビエントカラー(環境色)
};
// ボーン行列(2番)
cbuffer bones : register(b2)
{
	matrix boneMats[512];
}

struct Output
{
	float4 pos		: POSITION;		// システム用頂点座標
	float4 svpos	: SV_POSITION;	// システム用頂点座標
	float4 normal	: NORMAL;		// 法線ベクトル
	float2 uv		: TEXCOORD;		// UV値
	float2 boneno	: BONENO;		// 骨番号
	float2 weight	: WEIGHT;		// 影響度
};

// 頂点シェーダ
Output vs(float4 pos : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD, min16uint2 boneno : BONENO, min16uint2 weight : WEIGHT)
{
	float w = weight / 100.f;
	matrix m = boneMats[boneno.x] * w + boneMats[boneno.y] * (1.00 - w);
	pos = mul(m, pos);

	Output output;
	output.pos = mul(world, pos);
	output.svpos = mul(mul(viewProj, world), pos);	// 2次元上
	output.uv = uv;
	output.normal = normal;
	output.boneno = boneno /*/ 64.0*/;

	return output;
}


struct PixelOutput
{
	float4 color	: SV_TARGET0;	// カラー値を出力
	float4 normal	: SV_TARGET1;	// 法線を出力
	float4 highLum	: SV_TARGET2;	// 高輝度(High Luminance)
};

PixelOutput ps(Output output)
{
	// 座標系をあわせている
	float2 spuv = ((float2(1, -1) + output.normal.xy) * float2(0.5f, -0.5f));

	float3 light = float3(-1, 1, -1);	// 平行光線ベクトル
	light = normalize(light);
	//光の反射ベクトル
	float3 refLight = normalize(reflect(light, output.normal.rgb));
	//float specularB = pow(saturate(dot(refLight, -input.ray)), specular.a);

	float brightness = dot(light, output.normal);
	float4 toonDif = toon.Sample(smp, float2(0, 1.0f - brightness));

	float3 texColor = tex.Sample(smp, output.uv) * sph.Sample(smp, spuv).rgb + spa.Sample(smp, spuv).rgb;;
	float3 matColor = toonDif.rgb * diffuse + specular + (mirror * 0.5f);

	float4 ret = float4(float3(brightness, brightness, brightness) * toonDif.rgb * texColor * diffuse
		* tex.Sample(smp, output.uv).rgb * sph.Sample(smp, spuv).rgb + spa.Sample(smp, spuv).rgb + float3(texColor * mirror), 1.0f);
	
	PixelOutput po;
	po.color = ret;

	po.normal.rgb = float3((output.normal.xyz + 1.0f) / 2.0f);
	po.normal.a = 1;
	po.highLum = (ret > 1.0f);
	return po;
}