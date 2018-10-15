//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

// 셰이더 레지스터 = 루트시그니쳐 cbv테이블
cbuffer cbPerObject : register(b0)
{
	float4x4 gWorldViewProj;
    float4 gPulseColor;
    float gTime;
};

Texture2D gDiffuseMap : register(t0);

struct VertexIn
{
	float4 PosL  : POSITION;
    float3 Normal : NORMAL;
    float2 Tex : TEXCOORD;
};

struct VertexOut
{
	float4 PosH  : SV_POSITION;
    float3 Normal : NORMAL;
    float2 Tex : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
    //vin.PosL.xy += 0.5f * sin(vin.PosL.x) * sin(3.0f * gTime);
    //vin.PosL.z *= 0.6f + 0.4f * sin(2.0f * gTime);
	// Transform to homogeneous clip space.
	vout.PosH = mul(vin.PosL, gWorldViewProj);
	
	// Just pass vertex color into the pixel shader.
    vout.Tex = vin.Tex;
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    float4 c = float4(1.0f, 0.5f, 0.5f, 0.5f);
    return c;
}


