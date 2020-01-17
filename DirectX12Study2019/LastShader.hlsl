// �e�N�X�`��(0��)
Texture2D<float4> tex : register(t0);
// �e�N�X�`��(1��)
Texture2D<float4> normalTex : register(t1);
// �e�N�X�`��(2��)
Texture2D<float4> highLim : register(t2);
// �T���v��(0��)
SamplerState smp : register(s0);

struct Output
{
	float4 svpos	: SV_POSITION;	// �V�X�e���p���_���W
	float2 uv		: TEXCOORD;		// UV�l
		
	float4 truePos	: TRUEPOS;
};

// ���_�V�F�[�_
Output vs(float4 pos : POSITION, float2 uv : TEXCOORD)
{
	Output output;
	output.svpos = pos;
	output.uv = uv;
	output.truePos = pos;
	
	return output;
}

float SDFCircle2D(float2 xy, float2 center, float r)
{
	return (length(center - xy) - r);
}
float SDFCircle3D(float3 xyz, float3 center, float r)
{
	return (length(center - xyz) - r);
}
float SDFLatticeCircle2D(float2 xy, float divider)
{
	return (length(fmod(xy, divider) - divider / 2) - divider / 2);
}
float SDFLatticeCircle3D(float3 xyz, float divider, float r)
{
	return (length(fmod(xyz, divider) - divider / 2) - r);
}

// �s�N�Z���V�F�[�_
float4 ps(Output output) : SV_TARGET
{
	float3 ret = tex.Sample(smp, output.uv).rgb;

	//�@���o��
	if (output.uv.x < 0.2 && output.uv.y < 0.2)
	{
		return normalTex.Sample(smp, (output.uv) * 5);
	}
	else if (output.uv.x < 0.2 && output.uv.y < 0.4)
	{
		return highLim.Sample(smp, (output.uv - float2(0, 0.2)) * 5);
	}
	
	if (normalTex.Sample(smp, (output.uv)).a == 0)
	{
		return float4(ret, 1.0f);
	}
	
	// ���C�}�[�`���O���K
	// 2D
	//if (0 > SDFCircle2D(output.truePos.xy, float2(0, 0), 0.5))
	//{
	//	return float4(1, 0, 0, 1);
	//}
	//if (0 > SDFLatticeCircle2D(output.uv * float2((1280.f / 720.f), 1.0f), 0.1f))
	//{
	//	return float4(1, 0, 0, 1);
	//}
	// 3D
	float3 eye = float3(0, 0, -2.5);
	float3 tpos = float3(output.truePos.xy * float2((1280.f / 720.f), 1.0f), 0);
	float3 ray = normalize(tpos - eye);
	float r = 1.0f; // ���̂̔��a
	
	for (int i = 0; i < 64; ++i)
	{
		//float len = SDFCircle3D(eye, float3(0, 0, 5), r);
		float len = SDFLatticeCircle3D(abs(eye), r * 4, r / 2);
		eye += ray * len;
		if (len < 0.001f)
		{
			//return float4((float)(64- i) / 64.0f, (float)(64-i) / 64.0f, (float)(64-i) / 64.0f, 1);
			return float4(output.truePos.x / r, output.truePos.y / r, output.truePos.z / r, 1);
		}
	}
	
	
	if ((output.uv.x + output.uv.y) < 1.0f)
	{
		// ���]
		//ret = 1 - ret;
		// �|�X�^���[�[�V����
		//ret = ret - fmod(ret, 0.25f);
		// ���m�N���F
		//ret = dot(float3(0.2126f, 0.7152f, 0.0722f), ret);
		// �֊s�����o
		/*ret *= 4;
		ret -= tex.Sample(smp, output.uv, int2(0, -2));
		ret -= tex.Sample(smp, output.uv, int2(0, 2));
		ret -= tex.Sample(smp, output.uv, int2(2, 0));
		ret -= tex.Sample(smp, output.uv, int2(-2, 0));*/
	}

	return float4(ret, 1.0f);
}
