// �T���v��(0��)
SamplerState smp : register(s0);
cbuffer mat : register(b0)
{
	matrix world;
	matrix viewProj;
	matrix wvp;		// �����ς�
	matrix lightVP;	// ���C�g�r���[�v���W�F�N�V����
	float4 eye;		// ���_
};

// �e�N�X�`��(0��)	// �e
Texture2D<float> shadow	: register(t0);
// �e�N�X�`��(1��)	// �摜
Texture2D<float4> tex	: register(t1);


struct Output {
	float4 svpos	: SV_POSITION;		// VP��Z�ς�
	float4 pos		: POSITION;
	float4 normal	: NORMAL;
	float2 uv		: TEXCOORD;
	float4 shadowPos	: SHADOW_POS;
};

// ���_�����
Output vs(float4 pos : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD)
{
	Output output;
	output.svpos = mul(mul(viewProj, world), pos);
	output.shadowPos = mul(mul(lightVP, world), pos);
	output.pos = mul(world, pos);
	output.normal = normal;
	output.uv = uv;
	return output;
}

// �߸�ټ����
float4 ps(Output output) : SV_TARGET
{
	float ld = output.shadowPos.z;	// ���C�g�r���[�ϊ����z���W
	float2 uv = ((float2(1, -1) + output.shadowPos.xy) * float2(0.5, -0.5));

	float width, height;
	tex.GetDimensions(width, height);	// texture�̃T�C�Y���Ƃ��Ă���

	float4 ret = tex.Sample(smp, output.uv);
	//ret *= 4;

	//ret -= tex.Sample(smp, output.uv, int2(0, -2));
	//ret -= tex.Sample(smp, output.uv, int2(0, 2));
	//ret -= tex.Sample(smp, output.uv, int2(2, 0));
	//ret -= tex.Sample(smp, output.uv, int2(-2, 0));

	//float3 reverse = 1 - ret.rgb;

	if (ld > shadow.Sample(smp, uv))
	{
		return float4(1.0f, 0.0f, 0.0f, 1.0f);
	}

	return float4(0, 1, 0, 1);
	//return float4(reverse, 1.0f);
	return float4(ret);
	return float4(tex.Sample(smp, output.uv).rgb, 1.0f);
}
