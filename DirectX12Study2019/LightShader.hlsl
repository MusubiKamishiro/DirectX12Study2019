// �T���v��(0��)
SamplerState smp : register(s0);
// �萔�o�b�t�@(0��)
cbuffer camera : register(b0)
{
	matrix world;
	matrix viewProj;
	matrix wvp;			// �����ς�
	matrix lightVP;		// ײ��ޭ���ۼު����
	float4 eye;			// ���_
};
// �{�[���s��(1��)
cbuffer bones : register(b1)
{
	matrix boneMats[512];
}
// �e
Texture2D<float> shadow : register(t0);


struct Output {
	float4 svpos	: SV_POSITION;		// VP��Z�ς�
	float4 pos		: POSITION;
	float4 normal	: NORMAL;
	float2 uv		: TEXCOORD;
	float2 boneno	: BONENO;
	float2 weight	: WEIGHT;
};

// ���_�����
Output vs(float4 pos : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD, min16uint2 boneNo : BONENO, min16uint2 weight : WEIGHT)
{
	float w = weight / 100.f;
	matrix m = boneMats[boneNo.x] * w + boneMats[boneNo.y] * (1.00 - w);

	Output output;
	pos = mul(m, pos);		// ���̋Ȃ����������
	output.pos = mul(world, pos);
	output.svpos = mul(mul(lightVP, world), pos);
	output.uv = uv;
	output.boneno = boneNo / 64.0;
	output.weight = float2(w, 1 - w);	// �S�����̂��
	return output;
}

float4 ps(Output output) : SV_TARGET
{
	float depth = shadow.Sample(smp, output.uv);
	depth = pow(depth, 50);

	//return float4(depth, depth, depth, 1.0f);
	return float4(0.0f, 0.0f, 1.0f, 1.0f);
}