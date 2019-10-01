// ø���(0��)
Texture2D<float4> tex	: register(t0);
// �����(0)
SamplerState smp : register(s0);
// �萔�o�b�t�@
cbuffer mat : register(b0)
{
	matrix world;		// ܰ���
	matrix viewProj;	// �ޭ���ۼު����
	matrix wvp;			// �����ς�
	matrix lightVP;		// ײ��ޭ���ۼު����
};

struct Output
{
	float4 pos			: POSITION;
	float4 svpos		: SV_POSITION;
	float2 uv			: TEXCOORD;
};

// ���_�V�F�[�_
Output vs(float4 pos : POSITION, float2 uv : TEXCOORD)
{
	Output output;
	//output.pos = pos;
	output.pos = mul(world, pos);
	//output.svpos = pos;
	output.svpos = mul(mul(viewProj, world), pos);	// 2������
	output.uv = uv;

	return output;
}


// �s�N�Z���V�F�[�_
float4 ps(Output output) : SV_TARGET
{
	//return float4(1.0f, float2(output.uv), 1.0f);
	return float4(tex.Sample(smp, output.uv).rgb, 1.0f);
}