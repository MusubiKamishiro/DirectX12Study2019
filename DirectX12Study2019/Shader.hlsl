// �e�N�X�`��(0��)	// �ʏ�
Texture2D<float4> tex	: register(t0);
// �e�N�X�`��(1��)	// ���Z
Texture2D<float4> spa	: register(t1);
// �e�N�X�`��(2��)	// ��Z
Texture2D<float4> sph	: register(t2);
// �e�N�X�`��(3��)	// �g�D�[��
Texture2D<float4> toon	: register(t3);
// �T���v��(0��)
SamplerState smp : register(s0);
// �萔�o�b�t�@(0��)
cbuffer mat : register(b0)
{
	matrix world;		// ���[���h
	matrix viewProj;	// �r���[�v���W�F�N�V����
	matrix wvp;			// �����ς�
	matrix lightVP;		// ���C�g�r���[�v���W�F�N�V����
};
// �萔�o�b�t�@(1��)
cbuffer material : register(b1)
{
	float3 diffuse;		// �f�B�t���[�Y�J���[(�����F)
	float3 specular;	// �X�y�L�����J���[(����F)
	float3 mirror;		// �A���r�G���g�J���[(���F)
};
// �{�[���s��(2��)
cbuffer bones : register(b2)
{
	matrix boneMats[512];
}

struct Output
{
	float4 pos		: POSITION;		// �V�X�e���p���_���W
	float4 svpos	: SV_POSITION;	// �V�X�e���p���_���W
	float4 normal	: NORMAL;		// �@���x�N�g��
	float2 uv		: TEXCOORD;		// UV�l
	float2 boneno	: BONENO;		// ���ԍ�
	float2 weight	: WEIGHT;		// �e���x
};

// ���_�V�F�[�_
Output vs(float4 pos : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD, min16uint2 boneno : BONENO, min16uint2 weight : WEIGHT)
{
	float w = weight / 100.f;
	matrix m = boneMats[boneno.x] * w + boneMats[boneno.y] * (1.00 - w);
	pos = mul(m, pos);

	Output output;
	output.pos = mul(world, pos);
	output.svpos = mul(mul(viewProj, world), pos);	// 2������
	output.uv = uv;
	output.normal = normal;
	output.boneno = boneno /*/ 64.0*/;

	return output;
}


struct PixelOutput
{
	float4 color	: SV_TARGET0;	// �J���[�l���o��
	float4 normal	: SV_TARGET1;	// �@�����o��
	float4 highLum	: SV_TARGET2;	// ���P�x(High Luminance)
};

PixelOutput ps(Output output)
{
	// ���W�n�����킹�Ă���
	float2 spuv = ((float2(1, -1) + output.normal.xy) * float2(0.5f, -0.5f));

	float3 light = float3(-1, 1, -1);	// ���s�����x�N�g��
	light = normalize(light);
	//���̔��˃x�N�g��
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