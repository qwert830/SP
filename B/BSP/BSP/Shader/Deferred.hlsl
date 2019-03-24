
#include "LightingUtil.hlsl"

cbuffer cbPass : register(b0)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float4x4 gShadowTransform;
    float3 gEyePosW;
    float superheat;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
    float4 gAmbientLight;

    Light gLights[MaxLights];
};

Texture2D GBufferResource[4] : register(t0);

static const float2 g_SpecPowerRange = { 0.1, 250.0 };

struct PS_GBUFFER_OUT
{
    float4 ColorSpecInt : SV_TARGET0;
    float4 Normal : SV_TARGET1;
    float4 specPow : SV_TARGET2;
};

struct SURFACE_DATA
{
    float  LinearDepth;
    float3 Color;
    float3 Normal;
    float  SpecInt;
    float  SpecPow;
};

float ConvertDepthToLinear(float depth)
{
    float linearDepth = gProj[3][2] / (depth - gProj[2][2]);
    return linearDepth;
}

PS_GBUFFER_OUT PackGBuffer(float3 BaseColor, float3 Normal, float SpecIntensity, float SpecPower)
{
    PS_GBUFFER_OUT Out;

    float SpecPowerNorm = (SpecPower - g_SpecPowerRange.x) / g_SpecPowerRange.y;

    Out.ColorSpecInt = float4(BaseColor.rgb, SpecIntensity);
    Out.Normal = float4(Normal.xyz * 0.5 + 0.5, 0.0);
    Out.specPow = float4(SpecPowerNorm, 0.0, 0.0, 0.0);

    return Out;
}

SURFACE_DATA UnpackGBuffer(int2 location)
{
    SURFACE_DATA Out;

    int3 location3 = int3(location, 0);

    float depth = GBufferResource[0].Load(location3).x;
    Out.LinearDepth = ConvertDepthToLinear(depth);

    float4 baseColorSpecInt = GBufferResource[1].Load(location3);
    Out.Color = baseColorSpecInt.xyz;
    Out.SpecInt = baseColorSpecInt.w;

    Out.Normal = GBufferResource[2].Load(location3);
    Out.Normal = normalize(Out.Normal * 2.0 - 1.0);

    float SpecPowerNorm = GBufferResource[3].Load(location3).x;
    Out.SpecPow = SpecPowerNorm.x + SpecPowerNorm * g_SpecPowerRange.y;

    return Out;
}