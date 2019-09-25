struct Output
{
	float4 pos			: POSITION;
	float4 svpos		: SV_POSITION;
};

// 頂点シェーダ
Output vs(float4 pos : POSITION)
{
	Output output;
	output.pos = pos;
	output.svpos = pos;

	return output;
}


// ピクセルシェーダ
float4 ps(Output output) : SV_TARGET
{
	//return float4(1.0f, 0.0f, 0.0f, 1.0f);
	return float4(0, (output.pos.xy + float2(1, 1)) / 2, 1);
}