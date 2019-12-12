// �e�N�X�`��(0��)
Texture2D<float4> tex : register(t0);
// �e�N�X�`��(1��)
Texture2D<float4> normalTex : register(t1);
// �T���v��(0��)
SamplerState smp : register(s0);

struct Output
{
	float4 svpos	: SV_POSITION;	// �V�X�e���p���_���W
	float2 uv		: TEXCOORD;		// UV�l
};

// ���_�V�F�[�_
Output vs(float4 pos : POSITION, float2 uv : TEXCOORD)
{
	Output output;
	output.svpos = pos;
	output.uv = uv;

	return output;
}


// �s�N�Z���V�F�[�_
float4 ps(Output output) : SV_TARGET
{
	float3 ret = tex.Sample(smp, output.uv).rgb;

	//�@���o��
	if (output.uv.x < 0.2 && output.uv.y < 0.2)
	{
		return normalTex.Sample(smp, (output.uv /*- float2(0, 0.4)*/) * 5);
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