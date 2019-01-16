#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 3
#endif

#ifndef NUM_POINT_LIGHTS
    #define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
    #define NUM_SPOT_LIGHTS 0
#endif

#include "LightingUtil.hlsl"

Texture2D gDiffuseMap : register(t0);

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

// ���̴� �������� = ��Ʈ�ñ״��� cbv���̺�

// �ν��Ͻ� -> b0, b2 -> ���۷� ��� ���̴��� �ø���. t0,t1 .... DiffuseMap�� ��ġ���ʰ� ������ �ٸ���
cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
    float4x4 gTexTransform;
    uint MaterialIndex;
    uint InstPad0;
    uint InstPad1;
    uint InstPad2;
};
// const buffer
cbuffer cbPass : register(b1)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float3 gEyePosW;
    float cbPerObjectPad1;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
    float4 gAmbientLight;

    Light gLights[MaxLights];
};

cbuffer cbMaterial : register(b2)
{
    float4 gDiffuseAlbedo;
    float3 gFresnelR0;
    float gRoughness;
    float4x4 gMatTransform;
};

struct VertexIn
{
	float4 PosL     : POSITION;
    float3 NormalL   : NORMAL;
    float2 TexC      : TEXCOORD;
};

struct VertexOut
{
	float4 PosH     : SV_POSITION;
    float3 PosW     : POSITION;
    float3 NormalW   : NORMAL;
    float2 TexC      : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut) 0.0f;
	
    float4 posW = mul(float4(vin.PosL.xyz, 1.0f), (gWorld));
    vout.PosW = posW.xyz;

    vout.NormalW = mul(vin.NormalL, (float3x3) gWorld);
    
    vout.PosH = mul(posW, gViewProj);

    float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
    vout.TexC = mul(texC, gMatTransform).xy;

    return vout;
};

float4 PS(VertexOut pin) : SV_Target
{
    float4 diffuseAlbedo = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC) * gDiffuseAlbedo;
	
    pin.NormalW = normalize(pin.NormalW);

    float3 toEyeW = pin.PosW - gEyePosW;
    float distToEye = length(toEyeW);
    toEyeW /= distToEye;

    float4 ambient = gAmbientLight * diffuseAlbedo;

    const float shininess = 1.0f - gRoughness;
    Material mat = { diffuseAlbedo, gFresnelR0, shininess };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
        pin.NormalW, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;

    float4 fogColor = float4(0.7f, 0.7f, 0.7f, 1.0f);

    // ��������� �Ϸ�
    // ex) 1,0,0,0 * 0.5 = 0.5 0 0 0;
    litColor = lerp(litColor, fogColor, saturate((distToEye - 5.0f) / 150.0f));
    //if(pin.PosH.z <= 1)
    //    litColor = 1;
    //else
    //    litColor = 0;
    // 0�� ������ 1�� ���
    // ��ο� �� �� �ȼ����̴����� �Ȱ�ȿ���� �� �� ������� �Ȱ�ȿ���� ���� ������ �Ȱ�ȿ���� �ȴ�������.
    // ������ �Ȱ����� �����ϰ� �� �� �Ȱ�ȿ���� ���� �� ����, ��ī�̹ڽ��� ����ϸ� �ذ�� ������ ����
    litColor.a = diffuseAlbedo.a;
    
    return litColor;
};

