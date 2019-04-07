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

static const float2 g_SpecPowerRange = { 0.1, 250.0 };
static const float2 arrBasePos[4] = { float2(-1.0, 1.0), float2(1.0, 1.0), float2(-1.0, -1.0), float2(1.0, -1.0) };

struct InstanceData
{
    float4x4 World; 
    float4x4 TexTransform;
    uint MaterialIndex;
    uint InstPad0;
    uint InstPad1;
    float4 UIPos;
    float4 UIUVPos;
};

struct MaterialConstants
{
    float4 DiffuseAlbedo;
    float3 FresnelR0;
    float Roughness;
    float4x4 MatTransform;
    uint DiffuseMapIndex;
    uint MatPad0;
    uint MatPad1;
    uint MatPad2;
};


StructuredBuffer<InstanceData> gInstanceData : register(t0, space1);
StructuredBuffer<MaterialConstants> gMaterialData : register(t1, space1);

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
Texture2D gDiffuseMap[5] : register(t0);
Texture2D gShadowMap : register(t5);
Texture2D gBufferResource[4] : register(t6);

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);
SamplerComparisonState gsamShadow : register(s6);

struct VertexIn
{
    float4 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float4 ShadowPosH : POSITION0;
    float3 PosW : POSITION1;
    float3 NormalW : NORMAL;
    float2 TexC : TEXCOORD;

    nointerpolation uint MatIndex : MATINDEX;
};

struct ShadowVertexIn
{
    float3 PosL : POSITION;
    float2 TexC : TEXCOORD;
};

struct ShadowVertexOut
{
    float4 PosH : SV_POSITION;
    float2 TexC : TEXCOORD;

    nointerpolation uint MatIndex : MATINDEX;
};

struct DeferredVSOut
{
    float4 Pos : SV_POSITION;
    float2 UV : TEXCOORD;
};

struct PS_GBUFFER_OUT
{
    float4 ColorSpecInt : SV_TARGET0;
    float4 Normal : SV_TARGET1;
    float4 specPow : SV_TARGET2;
};

struct SURFACE_DATA
{
    float LinearDepth;
    float3 Color;
    float3 Normal;
    float SpecInt;
    float SpecPow;
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

    float depth = gBufferResource[0].Load(location3).x;
    Out.LinearDepth = ConvertDepthToLinear(depth);

    float4 baseColorSpecInt = gBufferResource[1].Load(location3);
    Out.Color = baseColorSpecInt.xyz;
    Out.SpecInt = baseColorSpecInt.w;

    Out.Normal = gBufferResource[2].Load(location3);
    Out.Normal = normalize(Out.Normal * 2.0 - 1.0);

    float SpecPowerNorm = gBufferResource[3].Load(location3).x;
    Out.SpecPow = SpecPowerNorm.x + SpecPowerNorm * g_SpecPowerRange.y;

    return Out;
}

float CalcShadowFactor(float4 shadowPosH)
{
    shadowPosH.xyz /= shadowPosH.w;

    float depth = shadowPosH.z;

    uint width, height, numMips;
    gShadowMap.GetDimensions(0, width, height, numMips);

    float dx = 1.0f / (float) width;

    float percentLit = 0.0f;
    const float2 offsets[9] =
    {
        float2(-dx, -dx), float2(0.0f, -dx), float2(dx, -dx),
        float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
        float2(-dx, +dx), float2(0.0f, +dx), float2(dx, +dx)
    };

    [unroll]
    for (int i = 0; i < 9; ++i)
    {
        percentLit += gShadowMap.SampleCmpLevelZero(gsamShadow,
            shadowPosH.xy + offsets[i], depth).r;
    }
    
    return percentLit / 9.0f;
}

float3 CalcWorldPos(float2 csPos, float linearDepth)
{
    float4 position;

    position.xy = csPos.xy * float2(1/gProj[0][0], 1/gProj[1][1]) * linearDepth;
    position.z = linearDepth;
    position.w = 1.0f;

    return mul(position, gInvView).xyz;
}

VertexOut VS(VertexIn vin, uint instanceID : SV_InstanceID)
{
    VertexOut vout = (VertexOut) 0.0f;
	
    InstanceData instData = gInstanceData[instanceID];
    float4x4 world = instData.World;
    float4x4 texTransform = instData.TexTransform;
    uint matIndex = instData.MaterialIndex;
    MaterialConstants matData = gMaterialData[0];
	
    vout.MatIndex = matIndex;

    float4 posW = mul(float4(vin.PosL.xyz, 1.0f), world);

    vout.PosW = posW.xyz;

    vout.NormalW = mul(vin.NormalL, (float3x3) world);

    vout.PosH = mul(posW, gViewProj);
	
    float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), texTransform);
    vout.TexC = mul(texC, matData.MatTransform).xy;
	
    vout.ShadowPosH = mul(posW, gShadowTransform);

    return vout;
};

float4 PS(VertexOut pin) : SV_Target
{
    MaterialConstants matData = gMaterialData[0];
    float4 diffuseAlbedo = matData.DiffuseAlbedo;
    float3 fresnelR0 = matData.FresnelR0;
    float roughness = matData.Roughness;
    uint diffuseTexIndex = matData.DiffuseMapIndex;
	
    diffuseAlbedo *= gDiffuseMap[pin.MatIndex].Sample(gsamAnisotropicWrap, pin.TexC);

    pin.NormalW = normalize(pin.NormalW);

    float3 toEyeW = normalize(gEyePosW - pin.PosW);

    float4 ambient = gAmbientLight * diffuseAlbedo;

    float3 shadowFactor = float3(1.0f, 1.0f, 1.0f);
    shadowFactor[0] = CalcShadowFactor(pin.ShadowPosH);

    const float shininess = 1.0f - roughness;
    Material mat = { diffuseAlbedo, fresnelR0, shininess };
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
        pin.NormalW, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;

    litColor.a = diffuseAlbedo.a;

    return litColor;
};

DeferredVSOut DVS(VertexIn vin, uint instanceID : SV_InstanceID, uint vertexID : SV_VertexID)
{
    DeferredVSOut vout = (DeferredVSOut) 0.0f;
    vout.Pos = float4(arrBasePos[vertexID].xy, 0.0f, 1.0f);
    vout.UV = vout.Pos.xy;
    return vout;
};

float4 DPS(DeferredVSOut pin) : SV_Target
{
    SURFACE_DATA data = UnpackGBuffer(pin.UV);

    float3 normal = data.Normal;
    float3 position = CalcWorldPos(pin.UV, data.LinearDepth);

    float4 ambient = gAmbientLight * float4(data.Color, 1.0f);
    float3 shadowFactor = float3(1.0f, 1.0f, 1.0f);
    
    const float shininess = data.SpecPow;
    float3 fresnelR0 = data.SpecInt;
    
    float3 toEyeW = normalize(gEyePosW - position);

    Material mat = { float4(data.Color,1.0f), fresnelR0, shininess};
    
    float4 directLight = ComputeLighting(gLights, mat, position,
       normal, toEyeW, shadowFactor);

    float4 color = ambient + directLight;

    return color;

}

PS_GBUFFER_OUT DrawPS(VertexOut pin)
{
    MaterialConstants matData = gMaterialData[0];
    float4 diffuseAlbedo = matData.DiffuseAlbedo;
    float3 fresnelR0 = matData.FresnelR0;
    float roughness = matData.Roughness;
    uint diffuseTexIndex = matData.DiffuseMapIndex;
    diffuseAlbedo *= gDiffuseMap[pin.MatIndex].Sample(gsamAnisotropicWrap, pin.TexC);
    pin.NormalW = normalize(pin.NormalW);
    const float shininess = 1.0f - roughness;

    return PackGBuffer(diffuseAlbedo.xyz, pin.NormalW, fresnelR0.x, shininess);
};

VertexOut UI_VS(VertexIn vin, uint instanceID : SV_InstanceID, uint vertexID : SV_VertexID)
{
    VertexOut vout = (VertexOut) 0.0f;
	
    InstanceData instData = gInstanceData[instanceID];
    float4x4 world = instData.World; // 기본행렬 * 회전행렬
    float4x4 texTransform = instData.TexTransform; // 기본행렬
    float4 uiPos = instData.UIPos;
    float4 uiUVPos = instData.UIUVPos;
    uint matIndex = instData.MaterialIndex;
    MaterialConstants matData = gMaterialData[0];
	
    vout.MatIndex = matIndex;

    if (vertexID == 0)
    {
        vout.PosH = float4(uiPos.x, uiPos.y, 0.0f, 1.0f);
        vout.TexC = float2(uiUVPos.x, uiUVPos.y);
    }
    else if (vertexID == 1)
    {
        vout.PosH = float4(uiPos.z, uiPos.y, 0.0f, 1.0f);
        vout.TexC = float2(uiUVPos.x+uiUVPos.z, uiUVPos.y);
    }
    else if (vertexID == 2)
    {
        vout.PosH = float4(uiPos.x, uiPos.w, 0.0f, 1.0f);
        vout.TexC = float2(uiUVPos.x, uiUVPos.y+uiUVPos.w);
    }
    else if (vertexID == 3)
    {
        vout.PosH = float4(uiPos.z, uiPos.w, 0.0f, 1.0f);
        vout.TexC = float2(uiUVPos.x + uiUVPos.z, uiUVPos.y + uiUVPos.w);
    }

    float4 texC = mul(float4(vout.TexC, 0.0f, 1.0f), texTransform);
    vout.TexC = mul(texC, matData.MatTransform).xy;
	
    return vout;
};

float4 UI_PS(VertexOut pin) : SV_Target
{
    MaterialConstants matData = gMaterialData[0];
    float4 diffuseAlbedo = matData.DiffuseAlbedo;
    float3 fresnelR0 = matData.FresnelR0;
    float roughness = matData.Roughness;
    uint diffuseTexIndex = matData.DiffuseMapIndex;
	
    diffuseAlbedo *= gDiffuseMap[pin.MatIndex].Sample(gsamLinearWrap, pin.TexC);

    if (diffuseAlbedo.a < 0.00001)
        discard;
    if (pin.MatIndex == 2)
    if (superheat / 100.0f > pin.TexC.x)
        return float4(1.0f, 0.0f, 0.0f, 0.0f);

    return diffuseAlbedo;
};

ShadowVertexOut SHADOW_VS(ShadowVertexIn vin, uint instanceID : SV_InstanceID)
{
    ShadowVertexOut vout = (ShadowVertexOut) 0.0f;

    InstanceData instData = gInstanceData[instanceID];
    float4x4 world = instData.World;
    float4x4 texTransform = instData.TexTransform;
    uint matIndex = instData.MaterialIndex;
    MaterialConstants matData = gMaterialData[0];

    vout.MatIndex = matIndex;

    float4 posW = mul(float4(vin.PosL, 1.0f), world);

    vout.PosH = mul(posW, gViewProj);
	
    float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), texTransform);
    vout.TexC = mul(texC, matData.MatTransform).xy;
	
    return vout;
};

void SHADOW_PS(ShadowVertexOut pin)
{
    MaterialConstants matData = gMaterialData[0];
    float4 diffuseAlbedo = matData.DiffuseAlbedo;
    float3 fresnelR0 = matData.FresnelR0;
    float roughness = matData.Roughness;
    uint diffuseTexIndex = matData.DiffuseMapIndex;

    diffuseAlbedo *= gDiffuseMap[diffuseTexIndex].Sample(gsamAnisotropicWrap, pin.TexC);

#ifdef ALPHA_TEST
    clip(diffuseAlbedo.a - 0.1f);
#endif
};

ShadowVertexOut SDEBUG_VS(ShadowVertexIn vin)
{
    ShadowVertexOut vout = (ShadowVertexOut) 0.0f;

    // Already in homogeneous clip space.
    vout.PosH = float4(vin.PosL, 1.0f);
	
    vout.TexC = vin.TexC;
	
    return vout;
}

float4 SDEBUG_PS(ShadowVertexOut pin) : SV_Target
{
    return float4(gShadowMap.Sample(gsamLinearWrap, pin.TexC).rrr, 1.0f);
}

